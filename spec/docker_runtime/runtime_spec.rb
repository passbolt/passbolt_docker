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
      'Image' => 'mysql')
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

  let(:http_path) { "/healthcheck/status.json" }
  let(:healthcheck) { 'curl -s -o /dev/null -w "%{http_code}" http://localhost/healthcheck/status.json' }

  describe 'php service' do
    it 'is running supervised' do
      expect(process('php-fpm')).to be_running.under('supervisor')
    end

    it 'has its port open' do
      expect(port(9000)).to be_listening.with('tcp')
    end
  end

  describe 'email cron' do
    it 'is running supervised' do
      expect(service('crond')).to be_running.under('supervisor')
    end
  end

  describe 'web service' do
    it 'is running supervised' do
      expect(service('nginx')).to be_running.under('supervisor')
    end

    it 'is listening on port 80' do
      expect(port(80)).to be_listening.with('tcp')
    end

    it 'is listening on port 443' do
      expect(port(443)).to be_listening.with('tcp')
    end
  end

  describe 'passbolt healthcheck' do
    it 'returns 200' do
      expect(command(healthcheck).stdout).to eq '200'
    end
  end
end
