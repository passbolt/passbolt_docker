#!/bin/bash
#
#  This script will install Docker and the Passbolt containers on the RaspberryPI.
#
#  On termination you should be able to access Passbolt from your computer
#  browser, which will be running on the networked Raspberry.
#

set -e

# From https://hub.docker.com/
image_alpine="armhf/alpine"
image_mysql="hypriot/rpi-mysql"

container_passbolt="passbolt:1.4.0-alpine"

function required_software() {

    # Install docker if needed
    which docker || echo "curl -sSL https://get.docker.com | sh"

    # Passbolt docker needs entropy to generate ssh keys
    which rngd || sudo apt-get install -y rng-tools
}

function required_images() {
    docker image inspect $image_alpine | grep "Id" || docker pull $image_alpine
    docker image inspect $image_mysql | grep "Id"  || docker pull $image_mysql
}

function start_mysql() {

    docker run -e MYSQL_ROOT_PASSWORD="" \
       -e MYSQL_ALLOW_EMPTY_PASSWORD=1 \
       -e MYSQL_RANDOM_ROOT_PASSWORD=1 \
       -e MYSQL_DATABASE=passbolt \
       -e MYSQL_USER=passbolt \
       -e MYSQL_PASSWORD=P4ssb0lt \
       $image_mysql
}

function start_passbolt() {

    # TODO: ??? find the mysql container ip address (docker inspect)
    mysql_instance=`docker inspect --format="{{.Id}}" $image_mysql`
    ip=`docker inspect --format='{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' $mysql_instance`
    echo "instance=$mysql_instance ip=$ip"

    #docker run -e db_host=??? $container_passbolt
}


# sanity check
if [ `id -u` != 0 ]; then
    echo "You must run this script as root"
    exit 1
fi

# Prepare the environment
required_software
required_images

exit 0

# Generate Docker and mysql containers
PLATFORM="armhf/" docker .. -t $container_passbolt
start_mysql

# Start the passbolt container
start_passbolt
