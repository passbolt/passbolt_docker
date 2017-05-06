#!/bin/bash
#
#  This script will install Docker and the Passbolt containers on the RaspberryPI.
#
#  On termination you should be able to access Passbolt from your computer
#  browser, which will be running on the networked Raspberry.
#

set -e

source ./pivars

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

function build_passbolt() {

    image_passbolt=`docker images -qa "$container_passbolt"`
    if [ "$image_passbolt" != "b5bf3d67c085" ]; then
	echo "Passbolt image already built: $image_passbolt"
    else
	# Generate Docker and mysql containers
	# FIXME: pass platform name correctly
	PLATFORM="armhf/" docker build .. -t $container_passbolt
    fi
}


# sanity check
if [ `id -u` != 0 ]; then
    echo "You must run this script as root"
    exit 1
fi

# Prepare the environment
required_software
required_images
echo "Required software ready"

build_passbolt
echo "Passbolt container built"
exit 0
