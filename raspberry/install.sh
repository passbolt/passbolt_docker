#!/bin/bash
#
#  This script will install passbolt Docker on the RaspberryPI
#
#  You must use Raspbian or a derivative distro.
#  Tested on pipaos: http://pipaos.mitako.eu/
#

function prerequisites() {
    # Install docker if needed
    if [ `which docker` != 0 ]; then
        curl -sSL https://get.docker.com | sh
        if  [ $? != 0 ]; then
            echo "Could not install Docker"
            exit 1
        fi
    fi
    
    # Passbolt docker needs enthropy to generate ssh keys
    if [ `which rng-tools` != 0 ]; then
        sudo apt-get install -y rng-tools
    fi
}

function download_images() {

    docker pull armhf/alpine
    docker pull blabla/mysql-armhf

}


if [ `uid` != 0 ]; then
    echo "You must run this script as root"
    exit 1
fi

required_software
download_images

# Generate Docker and mysql containers
PLATFORM="armhf/" docker .. -t passbolt:1.4.0-alpine

# bring up the mysql docker instance
# TODO: start in the background
run.sh

# Start passbolt container
# TODO: ??? find the mysql container ip address (docker inspect)
docker run -e db_host=??? passbolt:1.4.0-alpine
