#!/bin/sh

SRC=$(cd $(dirname "$0"); pwd)
source "${SRC}/conf/conf.sh"

docker run -p 8081:8081 -p 80:80 -d -it --hostname=passbolt.docker --name passbolt \
    -v $PASSBOLT_DIR:/var/www/passbolt \
    -e MYSQL_HOST=$MYSQL_HOST \
    -e MYSQL_ROOT_PASSWORD=$MYSQL_ROOT_PASSWORD \
    -e MYSQL_USERNAME=$MYSQL_USERNAME \
    -e MYSQL_PASSWORD=$MYSQL_PASSWORD \
    -e MYSQL_DATABASE=$MYSQL_DATABASE \
    passbolt_debian
