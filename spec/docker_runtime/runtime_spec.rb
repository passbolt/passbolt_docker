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
        'PASSBOLT_SSL_FORCE=true'
      ],
      'Image' => @image.id,
      'Binds' => $binds
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
  let(:uri)               { '/healthcheck/status.json' }
  let(:curl)              { "curl -sk -o /dev/null -w '%{http_code}' -H 'Host: passbolt.local' https://#{passbolt_host}:#{$https_port}/#{uri}" }

  let(:rootless_env_setup) do
    # The sed command needs to create a temporary file on the same directory as the destination file (/etc/cron.d).
    # So when running this tests on the rootless image we have to move the crontab file to tmp, execute the sed on it
    # and copy it back to /etc/cron.d.
    @container.exec(['cp', "/etc/cron.d/passbolt-#{ENV['PASSBOLT_FLAVOUR']}-server", '/tmp/passbolt-cron'])
    @container.exec(['cp', "/etc/cron.d/passbolt-#{ENV['PASSBOLT_FLAVOUR']}-server", '/tmp/passbolt-cron-temporary'])
    @container.exec(
      [
        'sed',
        '-i',
        "s\,$PASSBOLT_BASE_DIR/bin/cron.*\,/bin/bash -c \"\\.\\ /etc/environment\\ \\&\\&\\ env > /tmp/cron-test\"\,",
        '/tmp/passbolt-cron-temporary'
      ]
    )
    @container.exec(['cp', '/tmp/passbolt-cron-temporary', "/etc/cron.d/passbolt-#{ENV['PASSBOLT_FLAVOUR']}-server"])
    # force reload supercronic cron file
    @container.exec(%w[supervisorctl restart cron])

    # wait for cron
    sleep 61
  end

  let(:cron_env_teardown) do
    @container.exec(['mv', '/tmp/passbolt-cron', "/etc/cron.d/passbolt-#{ENV['PASSBOLT_FLAVOUR']}-server"])
    @container.exec(['rm', '/tmp/passbolt-cron-temporary'])
  end

  let(:root_env_setup) do
    @container.exec(['cp', "/etc/cron.d/passbolt-#{ENV['PASSBOLT_FLAVOUR']}-server", '/tmp/passbolt-cron'])
    @container.exec(
      [
        'sed',
        '-i',
        "s\,\\.\\ /etc/environment\\ \\&\\&\\ $PASSBOLT_BASE_DIR/bin/cron\,\\.\\ /etc/environment\\ \\&\\&\\ env > /tmp/cron-test\,",
        "/etc/cron.d/passbolt-#{ENV['PASSBOLT_FLAVOUR']}-server"
      ]
    )
    @container.exec(
      [
        'cp',
        '/tmp/passbolt-cron-temporary', "/etc/cron.d/passbolt-#{ENV['PASSBOLT_FLAVOUR']}-server"
      ]
    )

    # wait for cron
    sleep 61
  end

  describe 'php service' do
    it 'is running supervised' do
      expect(service('php-fpm')).to be_running.under('supervisor')
    end
  end

  describe 'web service' do
    it 'is running supervised' do
      expect(service('nginx')).to be_running.under('supervisor')
    end

    it "is listening on port #{$http_port}" do
      expect(@container.json['Config']['ExposedPorts']).to have_key("#{$http_port}/tcp")
    end

    it "is listening on port #{$https_port}" do
      expect(@container.json['Config']['ExposedPorts']).to have_key("#{$https_port}/tcp")
    end
  end

  describe 'passbolt status' do
    it 'returns 200' do
      expect(command(curl).stdout).to eq '200'
    end
  end

  describe 'can not access outside webroot' do
    let(:uri) { '/vendor/autoload.php' }
    it 'returns 404' do
      expect(command(curl).stdout).to eq '404'
    end
  end

  describe 'hide information' do
    let(:curl) { "curl -Isk -H 'Host: passbolt.local' https://#{passbolt_host}:#{$https_port}/" }
    it 'hides php version' do
      expect(command("#{curl} | grep 'X-Powered-By: PHP'").stdout).to be_empty
    end

    it 'hides nginx version' do
      expect(command("#{curl} | grep 'server:'").stdout.strip).to match(/^server:\s+nginx.*$/)
    end
  end

  describe 'cron service' do
    context 'cron process' do
      it 'is running supervised' do
        expect(service('cron')).to be_running.under('supervisor')
      end
    end

    # In order to be able to run this test on the rootess image
    # you will have to add the following to the Dockerfile (debian/Dockerfile.rootless)
    # && chown root:www-data /etc/cron.d/$PASSBOLT_PKG \
    # && chmod 664 /etc/cron.d/$PASSBOLT_PKG
    # And change the xit to it keyword on the test

    context 'cron rootless environment' do
      before { skip('Needs chown and chmod lines on the debian/Dockerfile.rootless to be able to run.') }
      before(:each) { rootless_env_setup }
      after(:each) { cron_env_teardown }

      it 'is contains the correct env' do
        expect(file('/tmp/cron-test').content).to match(/PASSBOLT_GPG_SERVER_KEY_FINGERPRINT/)
      end
    end

    context 'cron root environment' do
      before { skip('Rootless environment does not need this test') if ENV['ROOTLESS'] == 'true' }
      before(:each) { root_env_setup }
      after(:each) { cron_env_teardown }

      it 'is contains the correct env' do
        expect(file('/tmp/cron-test').content).to match(/PASSBOLT_GPG_SERVER_KEY_FINGERPRINT/)
      end
    end
  end
end
