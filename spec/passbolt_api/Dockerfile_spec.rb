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
      'Image' => 'mysql')
    @mysql.start

    image = Docker::Image.build_from_dir(ROOT_DOCKERFILES)

    set :docker_image, image.id
    set :env, { 'DB_HOST' => @mysql.json['NetworkSettings']['IPAddress'] }
  end

  after(:all) do
    @mysql.kill
  end

  let(:nginx_conf)      { '/etc/nginx/nginx.conf' }
  let(:site_conf)       { '/etc/nginx/conf.d/default.conf' }
  let(:passbolt_home)   { '/var/www/passbolt' }
  let(:passbolt_tmp)    { '/var/www/passbolt/tmp' }
  let(:passbolt_image)  { '/var/www/passbolt/webroot/img/public' }
  let(:passbolt_owner)  { 'nginx' }

  describe "passbolt required php extensions" do

    php_extensions = [
      'curl', 'gd', 'intl', 'json', 'mcrypt', 'mysqlnd', 'xsl', 'phar',
      'posix', 'xml', 'xsl', 'zlib', 'ctype', 'pdo', 'gnupg'
    ]

    php_extensions.each do |ext|
      it "#{ext} must be installed" do
        expect(command("php --ri #{ext}").exit_status).to eq 0
      end
    end
  end

  describe 'supervisor' do
    xit 'is installed' do
      expect(package('supervisor')).to be_installed
    end
  end

  describe 'passbolt home dirs' do
    it 'must exist and be directories' do
      expect(file(passbolt_home)).to be_a_directory
      expect(file(passbolt_tmp)).to be_a_directory
      expect(file(passbolt_image)).to be_a_directory
    end

    it 'must be owned by correct user' do
      expect(file(passbolt_home)).to be_owned_by(passbolt_owner)
    end
  end

  describe 'nginx configuration' do
    it 'is installed correctly' do
      expect(file(nginx_conf)).to exist
    end

    it 'has the correct permissions' do
      expect(file(nginx_conf)).to be_owned_by 'root'
    end
  end

  describe 'site configuration' do
    it 'is installed correctly' do
      expect(file(site_conf)).to exist
    end

    it 'has the correct permissions' do
      expect(file(site_conf)).to be_owned_by 'root'
    end
  end

  describe 'php service' do
    xit 'is running supervised' do
      expect(service('php-fpm')).to be_running.under('supervisor')
    end
  end

  describe port(9000) do
    xit { is_expected.to be_listening.with('tcp') }
  end

  describe 'email cron' do
    xit 'is running supervised' do
      expect(service('crond')).to be_running.under('supervisor')
    end
  end

  describe 'web service' do
    xit 'is running supervised' do
      expect(service('nginx')).to be_running.under('supervisor')
    end

    xit 'is listening on port 80' do
      expect(port(80)).to be_listening.with('tcp')
    end
  end

end
