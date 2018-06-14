require 'spec_helper'

describe 'passbolt_api service' do

  before(:all) do
    @mysql_image = Docker::Image.create('fromImage' => 'mariadb:latest')
    @mysql = Docker::Container.create(
      'Env' => [
        'MYSQL_ROOT_PASSWORD=test',
        'MYSQL_DATABASE=passbolt',
        'MYSQL_USER=passbolt',
        'MYSQL_PASSWORD=±!@#$%^&*()_+=-}{|:;<>?'
      ],
      "Healthcheck" => {
        "Test": [
          "CMD-SHELL",
          "mysqladmin ping --silent"
        ]
      },
      'Image' => @mysql_image.id)
    @mysql.start

    while @mysql.json['State']['Health']['Status'] != 'healthy'
      sleep 1
    end

    @image     = Docker::Image.build_from_dir(ROOT_DOCKERFILES)
    @container = Docker::Container.create(
      'Env' => [
        "DATASOURCES_DEFAULT_HOST=#{@mysql.json['NetworkSettings']['IPAddress']}",
        'DATASOURCES_DEFAULT_PASSWORD=±!@#$%^&*()_+=-}{|:;<>?',
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

  let(:passbolt_host)     { @container.json['NetworkSettings']['IPAddress'] }
  let(:uri)               { "/healthcheck/status.json" }
  let(:curl)              { "curl -sk -o /dev/null -w '%{http_code}' -H 'Host: passbolt.local' https://#{passbolt_host}/#{uri}" }
  let(:conf_app)          { "curl -sk -o /dev/null -w '%{http_code}' -H 'Host: passbolt.local' https://#{passbolt_host}/conf/app.php" }

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
      expect(command(curl).stdout).to eq '200'
    end
  end

  describe 'passbolt serverkey unaccessible' do
    let(:uri) { '/config/gpg/serverkey.asc' }
    it "returns 404" do
      expect(command(curl).stdout).to eq '404'
    end
  end

  describe 'passbolt serverkey private unaccessible' do
    let(:uri) { '/config/gpg/serverkey_private.asc' }
    it 'returns 404' do
      expect(command(curl).stdout).to eq '404'
    end
  end

  describe 'passbolt conf unaccessible' do
    let(:uri) { '/config/app.php' }
    it 'returns 404' do
      expect(command(curl).stdout).to eq '404'
    end
  end
  describe 'passbolt tmp folder is unaccessible' do
    let(:uri) { '/tmp/cache/database/empty' }
    it 'returns 404' do
      expect(command(curl).stdout).to eq '404'
    end
  end

end
