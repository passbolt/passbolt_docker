#!/bin/bash
#
# Script to start the containers
#
# WARNING:The session used to run this script must stay alive all the time.
#

set -e

source pivars

function start_mysql() {

    echo "Starting $image_mysql..."
    docker run -d -e MYSQL_ROOT_PASSWORD="" \
       -e MYSQL_ALLOW_EMPTY_PASSWORD=1 \
       -e MYSQL_RANDOM_ROOT_PASSWORD=1 \
       -e MYSQL_DATABASE=passbolt \
       -e MYSQL_USER=passbolt \
       -e MYSQL_PASSWORD=P4ssb0lt \
       $image_mysql

    sleep 5
}

function start_passbolt() {

    id_passbolt=`docker ps -aq --filter ancestor=$container_passbolt --filter status=running`
    if [ "$id_passbolt" != "" ]; then
	echo "Passbolt container is already running $id_passbolt"
    else
	id_mysql=`docker ps -aq --filter ancestor=$image_mysql --filter status=running`
	ipaddress=`docker inspect --format='{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' $id_mysql`
	echo "Starting $container_passbolt container binding IP=$ipaddress"
	docker run -d -e db_host=$ipaddress $container_passbolt
    fi
}


# Start mysql and passbolt containers
start_mysql
start_passbolt
