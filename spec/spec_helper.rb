require 'serverspec'
require 'docker'

ROOT_DOCKERFILES = File.expand_path('../../', __FILE__)
FIXTURES_PATH    = File::expand_path("fixtures", File::dirname(__FILE__))
LOCAL_SUBSCRIPTION_KEY_PATH    = File.expand_path('../../subscription_key.txt', __FILE__)
SUBSCRIPTION_KEY_PATH = '/etc/passbolt/subscription_key.txt'

$cron_binary = '/usr/sbin/cron'
$dockerfile = 'debian/Dockerfile'
$http_port = '80'
$https_port = '443'
$root_user = 'root'
$config_group = 'www-data'
$binds = []

$buildargs = {
  :PASSBOLT_FLAVOUR=>"#{ENV['PASSBOLT_FLAVOUR']}",
  :PASSBOLT_COMPONENT=>"#{ENV['PASSBOLT_COMPONENT']}",
}

if ENV['PASSBOLT_FLAVOUR'] == "pro"
  $binds = ["#{LOCAL_SUBSCRIPTION_KEY_PATH}:#{SUBSCRIPTION_KEY_PATH}"]
end

set :backend, :docker
Docker.options[:read_timeout]  = 3600
Docker.options[:write_timeout] = 3600
Docker.authenticate!('username' => "#{ENV['REGISTRY_USERNAME']}", 'password' => "#{ENV['REGISTRY_PASSWORD']}", 'email' => "#{ENV['REGISTRY_EMAIL']}", 'serveraddress' => 'https://registry.gitlab.com/')

if ENV['ROOTLESS'] == "true"
  $cron_binary = '/usr/local/bin/supercronic'
  $dockerfile = 'debian/Dockerfile.rootless'
  $http_port = '8080'
  $https_port = '4433'
  # Where www-data has to be the owner instead of root
  $root_user = 'www-data'
  $config_group = '0'
end
