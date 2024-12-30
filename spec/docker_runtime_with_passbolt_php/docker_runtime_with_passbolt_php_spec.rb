require 'spec_helper'

describe 'passbolt_api service' do
  before(:all) do
    @mysql_image =
      Docker::Image.create(
        'fromImage' => ENV['CI_DEPENDENCY_PROXY_DIRECT_GROUP_IMAGE_PREFIX'] ? "#{ENV['CI_DEPENDENCY_PROXY_DIRECT_GROUP_IMAGE_PREFIX']}/mariadb:10.11" : 'mariadb:10.11'
      )

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

    @image = if ENV['GITLAB_CI']
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
               Docker::Image.build_from_dir(
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
        'PASSBOLT_GPG_SERVER_KEY_FINGERPRINT_FORCE=true',
        'PASSBOLT_HEALTHCHECK_ERROR=true'
      ],
      'Image' => @image.id,
      'HostConfig' => {
        'Binds' => $binds.append(
        "#{FIXTURES_PATH + '/passbolt-no-fingerprint.php'}:#{PASSBOLT_CONFIG_PATH + '/passbolt.php'}",
        "#{FIXTURES_PATH + '/public-test.key'}:#{PASSBOLT_CONFIG_PATH + 'gpg/unsecure.key'}",
        "#{FIXTURES_PATH + '/private-test.key'}:#{PASSBOLT_CONFIG_PATH + 'gpg/unsecure_private.key'}"
        )
      },
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

  let(:passbolt_host)     { @container.json['NetworkSettings']['IPAddress'] }
  let(:curl)              { "curl -sk -o /dev/null -w '%{http_code}' -H 'Host: passbolt.local' https://#{passbolt_host}:#{$https_port}/#{uri}" }

  describe 'force fingerprint calculation' do
    it 'is contains fingerprint environment variable' do
      expect(file('/etc/environment').content).to match(/PASSBOLT_GPG_SERVER_KEY_FINGERPRINT/)
    end
  end

  describe 'throws exception in logs' do
    let(:uri) { 'healthcheck/error' }
    it 'returns 500' do
      expect(command(curl).stdout).to eq '500'
    end

    it 'shows exception in logs' do
      expect(@container.logs(stderr: true)).to match(/^.*\[Cake\\Http\\Exception\\InternalErrorException\] Internal Server Error.*/)
    end
  end

  describe 'can not access outside webroot' do
    let(:uri) { 'vendor/autoload.php' }
    let(:curl) { "curl -sk -o /dev/null -w '%{http_code}' -H 'Host: passbolt.local' https://#{passbolt_host}:#{$https_port}/#{uri}" }
    it 'returns 404' do
      expect(command(curl).stdout).to eq '404'
    end
  end

end
