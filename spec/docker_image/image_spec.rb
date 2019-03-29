require 'spec_helper'

describe 'Dockerfile' do

  before(:all) do
    set :env, {
      'DATASOURCES_DEFAULT_HOST'     => '172.17.0.2',
      'DATASOURCES_DEFAULT_PASSWORD' => 'P4ssb0lt',
      'DATASOURCES_DEFAULT_USERNAME' => 'passbolt',
      'DATASOURCES_DEFAULT_DATABASE' => 'passbolt',
      'PASSBOLT_GPG_KEYRING'         => '/var/lib/nginx/.gnupg'
    }

    @image = Docker::Image.build_from_dir(ROOT_DOCKERFILES)
    set :docker_image, @image.id
    set :docker_container_create_options, { 'Cmd' => '/bin/sh' }
  end

  let(:nginx_conf)      { '/etc/nginx/nginx.conf' }
  let(:php_conf)        { '/usr/local/etc/php-fpm.d/expose.conf' }
  let(:site_conf)       { '/etc/nginx/conf.d/default.conf' }
  let(:supervisor_conf) do
    [ '/etc/supervisor/conf.d/nginx.conf',
    '/etc/supervisor/conf.d/php.conf',
    '/etc/supervisor/conf.d/cron.conf' ]
  end
  let(:passbolt_home)   { '/var/www/passbolt' }
  let(:passbolt_tmp)    { '/var/www/passbolt/tmp' }
  let(:passbolt_image)  { '/var/www/passbolt/webroot/img/public' }
  let(:passbolt_owner)  { 'www-data' }
  let(:exposed_ports)   { [ '80', '443' ] }
  let(:composer)        { '/usr/local/bin/composer'}
  let(:php_extensions)  { [
    'curl', 'gd', 'intl', 'json', 'mcrypt', 'mysqlnd', 'xsl', 'phar',
    'posix', 'xml', 'zlib', 'ctype', 'pdo', 'gnupg', 'pdo_mysql'
    ] }
  let(:wait_for) { '/usr/bin/wait-for.sh' }

  describe 'passbolt required php extensions' do
    it 'has php extensions installed' do
      php_extensions.each do |ext|
        expect(command("php --ri #{ext}").exit_status).to eq 0
      end
    end
  end

  describe 'php composer' do
    it 'is not installed' do
      expect(file(composer)).to_not exist
    end
  end

  describe 'supervisor' do
    it 'is installed' do
      expect(package('supervisor')).to be_installed
    end

    it 'has config files' do
      supervisor_conf.each do |config|
        expect(file(config)).to exist
      end
    end
  end

  describe 'wait-for' do
    it 'is installed' do
      expect(file(wait_for)).to exist and be_executable
    end
  end

  describe 'passbolt directory structure' do
    it 'must exist and be directories' do
      expect(file(passbolt_home)).to be_a_directory
      expect(file(passbolt_tmp)).to be_a_directory
      expect(file(passbolt_image)).to be_a_directory
    end

    it 'must be owned by correct user' do
      expect(file(passbolt_home)).to be_owned_by(passbolt_owner)
      expect(file(passbolt_tmp)).to be_owned_by(passbolt_owner)
      expect(file(passbolt_image)).to be_owned_by(passbolt_owner)
    end

    it 'must have the correct permissions on tmp' do
      expect(file(passbolt_tmp)).to be_mode('775')
    end

    it 'must have the correct permissions on img' do
      expect(file(passbolt_image)).to be_mode('775')
    end
  end

  describe 'php config' do
    it 'exists' do
      expect(file(php_conf)).to exist
    end

    it 'does not expose php version' do
      expect(file(php_conf).content).to match(/^php_flag\[expose_php\]\s+=\s+off$/)
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

  describe 'nginx site configuration' do
    it 'is installed correctly' do
      expect(file(site_conf)).to exist
    end

    it 'has the correct permissions' do
      expect(file(site_conf)).to be_owned_by 'root'
    end

    it 'points to the correct root folder' do
      expect(file(site_conf).content).to match 'root /var/www/passbolt/webroot'
    end

    it 'has server tokens off' do
      expect(file(nginx_conf).content).to match(/^\s+server_tokens off;/)
    end
  end

  describe 'ports exposed' do
    it 'exposes port' do
      exposed_ports.each do |port|
        expect(@image.json['ContainerConfig']['ExposedPorts']).to include("#{port}/tcp")
      end
    end
  end
end
