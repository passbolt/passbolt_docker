# frozen_string_literal: true

require 'serverspec'
require 'docker'

ROOT_DOCKERFILES = File.expand_path('..', __dir__)
FIXTURES_PATH = File.expand_path('fixtures', File.dirname(__FILE__))
LOCAL_SUBSCRIPTION_KEY_PATH = File.expand_path('../subscription_key.txt', __dir__)
SUBSCRIPTION_KEY_PATH = '/etc/passbolt/subscription_key.txt'

$cron_binary = '/usr/sbin/cron'
$dockerfile = 'debian/Dockerfile'
$http_port = '80'
$https_port = '443'
$root_user = 'root'
$config_group = 'www-data'
$binds = []

$buildargs = {
  PASSBOLT_FLAVOUR: ENV['PASSBOLT_FLAVOUR'].to_s,
  PASSBOLT_COMPONENT: ENV['PASSBOLT_COMPONENT'].to_s
}

$binds = ["#{LOCAL_SUBSCRIPTION_KEY_PATH}:#{SUBSCRIPTION_KEY_PATH}"] if ENV['PASSBOLT_FLAVOUR'] == 'pro'

set :backend, :docker
Docker.options[:read_timeout]  = 3600
Docker.options[:write_timeout] = 3600

if ENV['GITLAB_CI']
  Docker.authenticate!(
    'username' => ENV['REGISTRY_USERNAME'].to_s,
    'password' => ENV['REGISTRY_PASSWORD'].to_s,
    'email' => ENV['REGISTRY_EMAIL'].to_s,
    'serveraddress' => 'https://registry.gitlab.com/'
  )
end

if ENV['ROOTLESS'] == true
  $cron_binary = '/usr/local/bin/supercronic'
  $dockerfile = 'debian/Dockerfile.rootless'
  $http_port = '8080'
  $https_port = '4433'
  # Where www-data has to be the owner instead of root
  $root_user = 'www-data'
  $config_group = '0'
end
