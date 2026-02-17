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
           # Check if we have a timestamped image from this pipeline's build job
           if ENV['USE_TIMESTAMPED_IMAGE'] == 'true' && ENV['IMAGE_TAG']
             # Use fresh image from build artifact
             Docker::Image.create(
               'fromImage' => "#{ENV['CI_REGISTRY_IMAGE']}:#{ENV['IMAGE_TAG']}"
             )
           elsif ENV['ROOTLESS'] == 'true'
             # Fallback to rootless latest
             Docker::Image.create(
               'fromImage' => "#{ENV['CI_REGISTRY_IMAGE']}:#{ENV['PASSBOLT_FLAVOUR']}-rootless-latest"
             )
           else
             # Fallback to root latest
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

    container_base_env = [
      "DATASOURCES_DEFAULT_HOST=#{@mysql.json['NetworkSettings']['IPAddress']}",
      'DATASOURCES_DEFAULT_PASSWORD=±!@#$%^&*()_+=-}{|:;<>?',
      'DATASOURCES_DEFAULT_USERNAME=passbolt',
      'DATASOURCES_DEFAULT_DATABASE=passbolt',
      'PASSBOLT_SSL_FORCE=true',
      'PASSBOLT_PLUGINS_JWT_AUTHENTICATION_ENABLED=true'
    ]
    container_env = container_base_env

    if ENV['PASSBOLT_FLAVOUR'] == 'pro' && !ENV['SUBSCRIPTION_KEY'].to_s.empty?
      container_env << "SUBSCRIPTION_KEY=#{ENV['SUBSCRIPTION_KEY']}"
    end

    @container = Docker::Container.create(
      'Env' => container_env,
      'Image' => @image.id,
      'HostConfig' => {
        'Binds' => $binds
      }
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
  let(:curl)              { "curl -sLk -o /dev/null -w '%{http_code}' -H 'Host: passbolt.local' https://#{passbolt_host}:#{$https_port}#{uri}" }
  let(:jwt_conf)          { "#{PASSBOLT_CONFIG_PATH + '/jwt'}" }
  let(:jwt_key_pair)      { ["#{jwt_conf}/jwt.key", "#{jwt_conf}/jwt.pem"] }

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
    let(:curl) { "curl -skL -D - -H 'Host: passbolt.local' https://#{passbolt_host}:#{$https_port}#{uri} -o /dev/null" }
    it 'hides php version' do
      expect(command("#{curl} | grep 'X-Powered-By: PHP'").stdout).to be_empty
    end

    it 'returns 200' do
      expect(command(curl).stdout).to contain 'HTTP/2 200'
    end

    it 'hides nginx version' do
      expect(command("#{curl} | grep 'server:'").stdout.strip).to match(/^server:\s+nginx.*$/)
    end
  end

  describe 'gpg key generation' do
    let(:gpg_dir) { '/etc/passbolt/gpg' }
    let(:gpg_private_key) { "#{gpg_dir}/serverkey_private.asc" }
    let(:gpg_public_key) { "#{gpg_dir}/serverkey.asc" }
    let(:gnupghome) { '/var/lib/passbolt/.gnupg' }

    let(:list_keys_cmd) do
      if ENV['ROOTLESS'] == 'true'
        ['gpg', '--homedir', gnupghome, '--list-keys', '--with-colons']
      else
        ['su', '-s', '/bin/bash', '-c', "gpg --homedir #{gnupghome} --list-keys --with-colons", 'www-data']
      end
    end

    let(:healthcheck_cmd) do
      if ENV['ROOTLESS'] == 'true'
        ['bash', '-c', 'source /etc/environment && /usr/share/php/passbolt/bin/cake passbolt healthcheck --gpg']
      else
        ['su', '-s', '/bin/bash', '-c',
         'source /etc/environment && /usr/share/php/passbolt/bin/cake passbolt healthcheck --gpg', 'www-data']
      end
    end

    describe 'generated keys' do
      it 'should have created private key file' do
        expect(file(gpg_private_key)).to exist
        expect(file(gpg_private_key)).to be_file
        expect(file(gpg_private_key)).to be_readable
        expect(file(gpg_private_key)).to be_owned_by('www-data')
        expect(file(gpg_private_key)).to be_grouped_into('www-data')
      end

      it 'should have created public key file' do
        expect(file(gpg_public_key)).to exist
        expect(file(gpg_public_key)).to be_file
        expect(file(gpg_public_key)).to be_readable
        expect(file(gpg_public_key)).to be_owned_by('www-data')
        expect(file(gpg_public_key)).to be_grouped_into('www-data')
      end

      it 'should have correct key usage for primary key' do
        output = @container.exec(list_keys_cmd)[0].join
        pub_line = output.lines.find { |line| line.start_with?('pub:') }
        expect(pub_line).not_to be_nil

        fields = pub_line.split(':')
        usage_flags = fields[11]

        expect(usage_flags).to include('s')
        expect(usage_flags).to include('c')
        expect(usage_flags).not_to include('e')
      end

      it 'should have correct key usage for subkey' do
        output = @container.exec(list_keys_cmd)[0].join
        sub_line = output.lines.find { |line| line.start_with?('sub:') }
        expect(sub_line).not_to be_nil

        fields = sub_line.split(':')
        usage_flags = fields[11]

        expect(usage_flags).to include('e')
        expect(usage_flags).not_to include('s')
        expect(usage_flags).not_to include('c')
      end
    end

    it 'should pass all GPG checks' do
      output = @container.exec(healthcheck_cmd)[0].join

      expect(output).to include('[PASS] PHP GPG Module is installed and loaded')
      expect(output).to include('[PASS] The environment variable GNUPGHOME is set')
      expect(output).to include('[PASS] The server OpenPGP key is not the default one')
      expect(output).to include('[PASS] The public key file is defined')
      expect(output).to include('[PASS] The private key file is defined')

      pass_count = output.scan(/\[PASS\]/).count
      fail_count = output.scan(/\[FAIL\]/).count

      expect(pass_count).to be >= 10
      expect(fail_count).to eq(0)

      expect(output).to include('[PASS] No error found')
    end
  end


 describe 'subscription import' do
   before { skip('Only relevant for PRO images') unless ENV['PASSBOLT_FLAVOUR'] == 'pro' }

   context 'subscription key behavior in CI' do
     it 'imports subscription successfully when SUBSCRIPTION_KEY is provided' do
       skip('SUBSCRIPTION_KEY not provided in CI') if ENV['SUBSCRIPTION_KEY'].to_s.empty?
       skip('Not running in CI environment') unless ENV['GITLAB_CI']

       logs = @container.logs(stdout: true, stderr: true)

       expect(logs).to match(/Using SUBSCRIPTION_KEY environment variable/)
       expect(logs).not_to match(/Subscription key could not be found/)
     end

     it 'shows expected behavior when subscription key is not provided' do
       skip('SUBSCRIPTION_KEY is provided, testing without key not possible') unless ENV['SUBSCRIPTION_KEY'].to_s.empty?
       skip('Not running in CI environment') unless ENV['GITLAB_CI']

       logs = @container.logs(stdout: true, stderr: true)

       expect(logs).to match(/Subscription key could not be found/)
       expect(@container.json['State']['Running']).to be true
     end
   end

   context 'invalid subscription key handling' do
     before(:all) do
       skip('Only test invalid key behavior in CI') unless ENV['GITLAB_CI']

       container_env = container_baseenv
       container_env << 'SUBSCRIPTION_KEY=invalid-not-base64-!@#$'

       @invalid_key_container = Docker::Container.create(
         'Env' => container_env,
         'Image' => @image.id
       )
       @invalid_key_container.start
       sleep 15
     end

     after(:all) do
       @invalid_key_container.kill if @invalid_key_container
     end

     it 'fails gracefully with invalid SUBSCRIPTION_KEY' do
       logs = @invalid_key_container.logs(stdout: true, stderr: true)

       expect(logs).to match(/Using SUBSCRIPTION_KEY environment variable/)
       expect(@invalid_key_container.json['State']['Running']).to be true
     end
   end
 end

  describe 'jwt configuration' do
    it 'should have the correct permissions' do
      expect(file(jwt_conf)).to be_a_directory
      expect(file(jwt_conf)).to be_mode 550
      expect(file(jwt_conf)).to be_owned_by($root_user)
      expect(file(jwt_conf)).to be_grouped_into($config_group)
    end

    describe 'JWT key file' do
      it 'should exist' do
        expect(file("#{jwt_conf}/jwt.key")).to exist
        expect(file("#{jwt_conf}/jwt.key")).to be_mode 440
      end
    end

    describe 'JWT pem file' do
      it 'should exist' do
        expect(file("#{jwt_conf}/jwt.pem")).to exist
        expect(file("#{jwt_conf}/jwt.pem")).to be_mode 440
      end
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
