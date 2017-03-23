#!/bin/sh

SRC=$(cd $(dirname "$0"); pwd)
source "${SRC}/conf/conf.sh"

docker run -p 8081:8081 -p 80:80 -p 443:443 -d -it --hostname=passbolt.docker --name passbolt \
    -v $PASSBOLT_DIR:/var/www/passbolt \
    -e APP_URL=https://passbolt.docker \
    -e MYSQL_HOST=$MYSQL_HOST \
    -e MYSQL_ROOT_PASSWORD=$MYSQL_ROOT_PASSWORD \
    -e MYSQL_USERNAME=$MYSQL_USERNAME \
    -e MYSQL_PASSWORD=$MYSQL_PASSWORD \
    -e MYSQL_DATABASE=$MYSQL_DATABASE \
    -e ADMIN_USERNAME=$ADMIN_USERNAME \
    -e ADMIN_FIRST_NAME=$ADMIN_FIRST_NAME \
    -e ADMIN_LAST_NAME=$ADMIN_LAST_NAME \
    passbolt_debian
