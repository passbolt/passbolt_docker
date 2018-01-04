require 'spec_helper'

describe 'passbolt_api service' do

  before(:all) do
    image = Docker::Image.build_from_dir(ROOT_DOCKERFILES)

    set :docker_image, image.id
    set :env, { 'DB_HOST' => '172.17.0.2' }
end

  let(:global_conf)     { '/etc/nginx/nginx.conf' }
  let(:site_conf)       { '/etc/nginx/conf.d/default.conf' }
  let(:passbolt_home)   { '/var/www/passbolt' }
  let(:passbolt_tmp)    { '/var/www/passbolt/app/tmp' }
  let(:passbolt_image)  { '/var/www/passbolt/app/webroot/img/public' }
  let(:passbolt_owner)  { 'www-data' }

  describe "passbolt required php extension" do

    php_extensions = [
      'curl', 'gd', 'intl', 'json', 'mcrypt', 'mysqlnd', 'xsl', 'phar',
      'posix', 'xml', 'xsl', 'zlib', 'ctype', 'pdo', 'pdo_mysql', 'gnupg'
    ]

    php_extensions.each do |ext|
      it "#{ext} must be installed" do
        expect(command("php --ri #{ext}").exit_status).to eq 0
      end
    end
  end

  describe 'supervisor' do
    it 'is installed' do
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

  describe 'global configuration' do
    it 'is installed correctly' do
      expect(file(global_conf)).to exist
    end

    it 'has the correct permissions' do
      expect(file(global_conf)).to be_owned_by 'root'
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
    it 'is running supervised' do
      expect(service('php-fpm')).to be_running.under('supervisor')
    end
  end

  describe port(9000) do
    it { is_expected.to be_listening.with('tcp') }
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
  end

end
