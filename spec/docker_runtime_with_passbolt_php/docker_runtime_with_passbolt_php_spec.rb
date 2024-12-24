require 'spec_helper'

describe 'passbolt_api service' do
  before(:all) do
    @mysql_image =
      if ENV['GITLAB_CI']
        Docker::Image.create(
          'fromImage' => "#{ENV['CI_DEPENDENCY_PROXY_DIRECT_GROUP_IMAGE_PREFIX']}/mariadb:10.11"
        )
      else
        Docker::Image.create('fromImage' => 'mariadb:latest')
      end

    @mysql = Docker::Container.create(
      'Env' => [
        'MARIADB_ROOT_PASSWORD=test',
        'MARIADB_DATABASE=passbolt',
        'MARIADB_USER=passbolt',
        'MARIADB_PASSWORD=±!@#$%^&*()_+=-}{|:;<>?'
      ],
      'Healthcheck' => {
        "Test": [
          'CMD-SHELL',
          'mariadb-admin ping --silent'
        ]
      },
      'Image' => @mysql_image.id
    )

    @mysql.start

    sleep 1 while @mysql.json['State']['Health']['Status'] != 'healthy'

    if ENV['GITLAB_CI']
      Docker.authenticate!(
        'username' => ENV['CI_REGISTRY_USER'].to_s,
        'password' => ENV['CI_REGISTRY_PASSWORD'].to_s,
        'serveraddress' => 'https://registry.gitlab.com/'
      )
      @image =
        if ENV['ROOTLESS'] == 'true'
          Docker::Image.create(
            'fromImage' => "#{ENV['CI_REGISTRY_IMAGE']}:#{ENV['PASSBOLT_FLAVOUR']}-rootless-latest"
          )
        else
          Docker::Image.create(
            'fromImage' => "#{ENV['CI_REGISTRY_IMAGE']}:#{ENV['PASSBOLT_FLAVOUR']}-root-latest"
          )
        end
    else
      @image = Docker::Image.build_from_dir(
        ROOT_DOCKERFILES,
        {
          'dockerfile' => $dockerfile,
          'buildargs' => JSON.generate($buildargs)
        }
      )
    end

    @container = Docker::Container.create(
      'Env' => [
        "DATASOURCES_DEFAULT_HOST=#{@mysql.json['NetworkSettings']['IPAddress']}",
        'DATASOURCES_DEFAULT_PASSWORD=±!@#$%^&*()_+=-}{|:;<>?',
        'DATASOURCES_DEFAULT_USERNAME=passbolt',
        'DATASOURCES_DEFAULT_DATABASE=passbolt',
        'PASSBOLT_SSL_FORCE=true',
        'PASSBOLT_GPG_SERVER_KEY_FINGERPRINT_FORCE=true'
      ],
      'Image' => @image.id,
      'Binds' => $binds.append(
        "#{FIXTURES_PATH + '/passbolt-no-fingerprint.php'}:#{PASSBOLT_CONFIG_PATH + '/passbolt.php'}",
        "#{FIXTURES_PATH + '/public-test.key'}:#{PASSBOLT_CONFIG_PATH + 'gpg/unsecure.key'}",
        "#{FIXTURES_PATH + '/private-test.key'}:#{PASSBOLT_CONFIG_PATH + 'gpg/unsecure_private.key'}"
      )
    )

    @container.start
    @container.logs(stdout: true)

    set :docker_container, @container.id
    sleep 17
  end

  after(:all) do
    @mysql.kill
    @container.kill
  end

  describe 'force fingerprint calculation' do
    it 'is contains fingerprint environment variable' do
      expect(file('/etc/environment').content).to match(/PASSBOLT_GPG_SERVER_KEY_FINGERPRINT/)
    end
  end
end
