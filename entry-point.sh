#!/bin/bash
IS_MYSQL_LOCAL=1
if [[ $MYSQL_HOST != "localhost" ]];
then
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

# Restart the apache2 service
service apache2 restart

# Start the memcached service
service memcached restart

# Default configuration files
cp -a /var/www/passbolt/app/Config/app.php.default /var/www/passbolt/app/Config/app.php
cp -a /var/www/passbolt/app/Config/core.php.default /var/www/passbolt/app/Config/core.php
cp -a /var/www/passbolt/app/webroot/js/app/config/config.json.default /var/www/passbolt/app/webroot/js/app/config/config.json

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
if [[ $IS_PASSBOLT_INSTALLED == "0"]]; then
    echo "Installing"
    su -s /bin/bash -c "/var/www/passbolt/app/Console/cake install" www-data
    echo "We are all set. Have fun with Passbolt !"
    echo "Reminder : THIS IS A DEMO CONTAINER. DO NOT USE IT IN PRODUCTION!!!!"
fi

/bin/bash