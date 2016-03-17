#!/bin/sh

# Passbolt directory.
PASSBOLT_DIR=~/projects/passbolt_for_docker_debian

# MySQL configuration.
MYSQL_HOST=localhost
# Only necessary if we use the local database.
MYSQL_ROOT_PASSWORD=rootpassword
MYSQL_USERNAME=passbolt
MYSQL_PASSWORD=password123
MYSQL_DATABASE=passbolt

# Admin settings.
ADMIN_USERNAME=admin@passbolt.com
ADMIN_FIRST_NAME=Admin
ADMIN_LAST_NAME=Admin
