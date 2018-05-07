require 'spec_helper'

describe 'passbolt_api service' do

  before(:all) do
    @mysql = Docker::Container.create(
      'Env' => [
        'MYSQL_ROOT_PASSWORD=test',
        'MYSQL_DATABASE=passbolt',
        'MYSQL_USER=passbolt',
        'MYSQL_PASSWORD=P4ssb0lt'
      ],
      "Healthcheck" => {
        "Test": [
          "CMD-SHELL",
          "mysqladmin ping --silent"
        ]
      },
      'Image' => 'mariadb')
    @mysql.start

    while @mysql.json['State']['Health']['Status'] != 'healthy'
      sleep 1
    end

    @image     = Docker::Image.build_from_dir(ROOT_DOCKERFILES)
    @container = Docker::Container.create(
      'Env' => [
        "DATASOURCES_DEFAULT_HOST=#{@mysql.json['NetworkSettings']['IPAddress']}",
        'DATASOURCES_DEFAULT_PASSWORD=P4ssb0lt',
        'DATASOURCES_DEFAULT_USERNAME=passbolt',
        'DATASOURCES_DEFAULT_DATABASE=passbolt',
        'PASSBOLT_SSL_FORCE=true'
      ],
      'Image' => @image.id)
    @container.start
    @container.logs(stdout: true)

    set :docker_container, @container.id
    sleep 17
  end

  after(:all) do
    @mysql.kill
    @container.kill
  end

  let(:healthcheck)       { 'curl -sk -o /dev/null -w "%{http_code}" -H "Host: passbolt.local" https://localhost/healthcheck/status.json' }
  let(:serverkey)         { 'curl -sk -o /dev/null -w "%{http_code}" -H "Host: passbolt.local" https://localhost/config/gpg/serverkey.asc' }
  let(:serverkey_private) { 'curl -sk -o /dev/null -w "%{http_code}" -H "Host: passbolt.local" https://localhost/config/gpg/serverkey_private.asc' }
  let(:tmp)               { 'curl -sk -o /dev/null -w "%{http_code}" -H "Host: passbolt.local" https://localhost/tmp/cache/database/empty' }
  let(:logs)              { 'curl -sk -o /dev/null -w "%{http_code}" -H "Host: passbolt.local" https://localhost/logs/error.log' }
  let(:conf_app)          { 'curl -sk -o /dev/null -w "%{http_code}" -H "Host: passbolt.local" https://localhost/conf/app.php' }

  describe 'php service' do
    it 'is running supervised' do
      expect(service('php-fpm')).to be_running.under('supervisor')
    end

    it 'has its port open' do
      expect(@container.json['Config']['ExposedPorts']).to have_key('9000/tcp')
    end
  end

  describe 'email cron' do
    it 'is running supervised' do
      expect(service('cron')).to be_running.under('supervisor')
    end
  end

  describe 'web service' do
    it 'is running supervised' do
      expect(service('nginx')).to be_running.under('supervisor')
    end

    it 'is listening on port 80' do
      expect(@container.json['Config']['ExposedPorts']).to have_key('80/tcp')
    end

    it 'is listening on port 443' do
      expect(@container.json['Config']['ExposedPorts']).to have_key('443/tcp')
    end
  end

  describe 'passbolt status' do
    it 'returns 200' do
      expect(command(healthcheck).stdout).to eq '200'
    end
  end

  describe 'passbolt serverkey unaccessible' do
    it 'returns 404' do
      expect(command(serverkey).stdout).to eq '404'
    end
  end

  describe 'passbolt serverkey private unaccessible' do
    it 'returns 404' do
      expect(command(serverkey_private).stdout).to eq '404'
    end
  end

  describe 'passbolt tmp folder is unaccessible' do
    it 'returns 404' do
      expect(command(tmp).stdout).to eq '404'
    end
  end

  describe 'passbolt conf files can not be retrieved' do
    it 'returns 404' do
      expect(command(conf_app).stdout).to eq '404'
    end
  end

  describe 'passbolt error log folder is unaccessible' do
    it 'returns 404' do
      expect(command(logs).stdout).to eq '404'
    end
  end

end
