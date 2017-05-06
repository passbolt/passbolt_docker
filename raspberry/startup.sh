#!/bin/bash
#
# Script to start the containers
#

set -e

source rpi_variables

function start_mysql() {

    docker run -e MYSQL_ROOT_PASSWORD="" \
       -e MYSQL_ALLOW_EMPTY_PASSWORD=1 \
       -e MYSQL_RANDOM_ROOT_PASSWORD=1 \
       -e MYSQL_DATABASE=passbolt \
       -e MYSQL_USER=passbolt \
       -e MYSQL_PASSWORD=P4ssb0lt \
       $image_mysql
}

function start_passbolt() {

    image_passbolt=`docker images $container_passbolt"`
    if [ "$image_passbolt" != "" ]; then
	echo "Passbolt container already built: $id_passbolt"
    else
	id_mysql=`docker ps -aq --filter ancestor=$image_mysql --filter status=running`
	ip=`docker inspect --format='{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' $id_mysql`
	echo "id=$id_mysql ip=$ip"
	#docker run -e db_host=??? $container_passbolt
    fi
}


# Start the passbolt container
# TODO: move below steps to an external script
start_mysql
start_passbolt
