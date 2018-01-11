require 'spec_helper'
require 'rspec/wait'

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
        #'DATASOURCES_DEFAULT_PORT=3306',
        #'PASSBOLT_GPG_KEYRING=/var/lib/nginx/.gnupg',
        #'PASSBOLT_GPG_SERVER_KEY_PUBLIC=/var/www/passbolt/config/gpg/serverkey.asc',
        #'PASSBOLT_GPG_SERVER_KEY_PRIVATE=/var/www/passbolt/config/gpg/serverkey_private.asc'
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

end
