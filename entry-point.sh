#!/bin/bash

########################################################
## Configure Mysql
########################################################

IS_MYSQL_LOCAL=1
if [ $MYSQL_HOST != "localhost" ]; then
    IS_MYSQL_LOCAL=0
fi

# If Mysql is local (no persistence), we reset everything and create the database.
if [ $IS_MYSQL_LOCAL == 1 ]; then
    echo "using local mysql"
    echo "Resetting root password, and create user ${MYSQL_USERNAME}"
    # Start mysql
    service mysql start
    # Change password of database
mysql --host=localhost --user=root --password=root << EOSQL
    SET @@SESSION.SQL_LOG_BIN=0;
    SET PASSWORD FOR 'root'@'localhost' = PASSWORD('${MYSQL_ROOT_PASSWORD}');
    GRANT ALL ON *.* TO 'root'@'%' WITH GRANT OPTION ;
    DROP DATABASE IF EXISTS test ;
    FLUSH PRIVILEGES ;
EOSQL

    # Create the passbolt database
    echo "Create database ${MYSQL_DATABASE}"
    mysql -u "root" --password="${MYSQL_ROOT_PASSWORD}" -e "create database ${MYSQL_DATABASE}"
    echo "Create user ${MYSQL_USERNAME} and give access to ${MYSQL_DATABASE}"
    mysql -u "root" --password="${MYSQL_ROOT_PASSWORD}" -e "GRANT ALL PRIVILEGES ON ${MYSQL_DATABASE}.* To '${MYSQL_USERNAME}'@'localhost' IDENTIFIED BY '${MYSQL_PASSWORD}'"
    echo "flush privileges"
    mysql -u "root" --password="${MYSQL_ROOT_PASSWORD}" -e "FLUSH PRIVILEGES"

# If Mysql is on a different host, check if the database exists.
else
    echo "using remote mysql"
    echo "Checking database ${MYSQL_DATABASE}"
    RESULT=`mysql -h $MYSQL_HOST  -u $MYSQL_USERNAME -p$MYSQL_PASSWORD --skip-column-names -e "SHOW DATABASES LIKE '$MYSQL_DATABASE'"`
    if [ "$RESULT" != "$MYSQL_DATABASE" ]; then
        echo "The database $MYSQL_DATABASE does not exist in the mysql instance provided."
    fi
    echo "ok"
fi

########################################################
## Restart services
########################################################

# Restart the apache2 service
service apache2 restart

# Start the memcached service
service memcached restart

########################################################
## Prepare the source code
########################################################

# Default configuration files
cp -a /var/www/passbolt/app/Config/app.php.default /var/www/passbolt/app/Config/app.php
cp -a /var/www/passbolt/app/Config/core.php.default /var/www/passbolt/app/Config/core.php
cp -a /var/www/passbolt/app/webroot/js/app/config/config.json.default /var/www/passbolt/app/webroot/js/app/config/config.json

# gpg
#Init gpg directory
gpg --fingerprint
GPG_SERVER_KEY_FINGERPRINT=`gpg -n --with-fingerprint /home/www-data/gpg_server_key_public.key | awk -v FS="=" '/Key fingerprint =/{print $2}' | sed 's/[ ]*//g'`
/var/www/passbolt/app/Console/cake passbolt app_config write GPG.serverKey.fingerprint $GPG_SERVER_KEY_FINGERPRINT
/var/www/passbolt/app/Console/cake passbolt app_config write GPG.serverKey.public /home/www-data/gpg_server_key_public.key
/var/www/passbolt/app/Console/cake passbolt app_config write GPG.serverKey.private /home/www-data/gpg_server_key_private.key
chown www-data:www-data /home/www-data/gpg_server_key_public.key
chown www-data:www-data /home/www-data/gpg_server_key_private.key

# overwrite the core configuration
/var/www/passbolt/app/Console/cake passbolt core_config gen-cipher-seed
/var/www/passbolt/app/Console/cake passbolt core_config gen-security-salt
/var/www/passbolt/app/Console/cake passbolt core_config write App.fullBaseUrl ${APP_URL}

# overwrite the database configuration
# @TODO based on the cake task DbConfigTask implement a task to manipulate the dabase configuration
#/var/www/passbolt/app/Console/cake passbolt db_config ${MYSQL_HOST} ${MYSQL_USERNAME} ${MYSQL_PASSWORD} ${MYSQL_DATABASE}

DATABASE_CONF=/var/www/passbolt/app/Config/database.php
# Set configuration in file
cat > $DATABASE_CONF << EOL
    <?php
    class DATABASE_CONFIG {
        public \$default = array(
            'datasource' => 'Database/Mysql',
            'persistent' => false,
            'host' => '${MYSQL_HOST}',
            'login' => '${MYSQL_USERNAME}',
            'password' => '${MYSQL_PASSWORD}',
            'database' => '${MYSQL_DATABASE}',
            'prefix' => '',
            'encoding' => 'utf8',
        );
    };
EOL

#######################################################
## Install passbolt
########################################################

# Check if passbolt is already installed.
IS_PASSBOLT_INSTALLED=0
OUTPUT=$(mysql -N -s -u ${MYSQL_USERNAME} -p${MYSQL_PASSWORD} -e "select count(*) from information_schema.tables where table_schema='${MYSQL_DATABASE}' and table_name='users';")
echo "OUTPUT=${OUTPUT}"
if [ $OUTPUT  == "1" ]; then
    echo "passbolt is already installed in this database"
    IS_PASSBOLT_INSTALLED=1
else
    echo "passbolt is not installed in this database. Proceeding.."
fi

# Install passbolt
if [ $IS_PASSBOLT_INSTALLED == "0" ]; then
    echo "Installing"
    su -s /bin/bash -c "/var/www/passbolt/app/Console/cake install --admin-username ${ADMIN_USERNAME} --admin-first-name=${ADMIN_FIRST_NAME} --admin-last-name=${ADMIN_LAST_NAME}" www-data
    echo "We are all set. Have fun with Passbolt !"
    echo "Reminder : THIS IS A DEMO CONTAINER. DO NOT USE IT IN PRODUCTION!!!!"
fi

########################################################
## After install
########################################################

# start fcron
fcron &

# add passbolt email sender job to fcron
touch /var/log/passbolt.log
chown www-data:www-data /var/log/passbolt.log
env > /etc/cron.d/passbolt
echo "* * * * * /var/www/passbolt/app/Console/cake EmailQueue.sender > /var/log/passbolt.log" >> /etc/cron.d/passbolt
fcrontab -u www-data /etc/cron.d/passbolt

/bin/bash
