require 'serverspec'
require 'docker'

ROOT_DOCKERFILES = File.expand_path('../../', __FILE__)
FIXTURES_PATH    = File::expand_path("fixtures", File::dirname(__FILE__))

$cron_binary = '/usr/sbin/cron'
$dockerfile = 'debian/Dockerfile'
$http_port = '80'
$https_port = '443'
$root_user = 'root'

set :backend, :docker
Docker.options[:read_timeout]  = 3600
Docker.options[:write_timeout] = 3600

if ENV['ROOTLESS'] == "true"
  $cron_binary = '/usr/local/bin/supercronic'
  $dockerfile = 'debian/Dockerfile.rootless'
  $http_port = '8080'
  $https_port = '4433'
  # Where www-data has to be the owner instead of root
  $root_user = 'www-data'
end

puts($root_user)
