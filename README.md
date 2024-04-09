```
       ____                  __          ____          .-.
      / __ \____  _____ ____/ /_  ____  / / /_    .--./ /      _.---.,
     / /_/ / __ `/ ___/ ___/ __ \/ __ \/ / __/     '-,  (__..-`       \
    / ____/ /_/ (__  |__  ) /_/ / /_/ / / /_          \                |
   /_/    \__,_/____/____/_,___/\____/_/\__/           `,.__.   ^___.-/
                                                         `-./ .'...--`
  The open source password manager for teams                `'
  (c) 2023 Passbolt SA
  https://www.passbolt.com
```
[![Docker Pulls](https://img.shields.io/docker/pulls/passbolt/passbolt.svg?style=flat-square)](https://hub.docker.com/r/passbolt/passbolt/tags/)
[![GitHub release](https://img.shields.io/github/release/passbolt/passbolt_docker.svg?style=flat-square)](https://github.com/passbolt/passbolt_docker/releases)
[![license](https://img.shields.io/github/license/passbolt/passbolt_docker.svg?style=flat-square)](https://github.com/passbolt/passbolt_docker/LICENSE)
[![Twitter Follow](https://img.shields.io/twitter/follow/passbolt.svg?style=social&label=Follow)](https://twitter.com/passbolt)

# What is passbolt?

Passbolt is a free and open source password manager that allows team members to
store and share credentials securely.

# Requirements

* rng-tools or haveged might be required on host machine to speed up entropy generation on containers.
This way gpg key creation on passbolt container will be faster.
* mariadb/mysql >= 5.0

# Usage

### docker-compose

Usage:

```
$ docker-compose -f docker-compose/docker-compose-ce.yaml up
```

Users are encouraged to use [official docker image from the docker hub](https://hub.docker.com/r/passbolt/passbolt/).

## Start passbolt instance

Passbolt requires mysql to be running. The following example use mysql official
docker image with the default passbolt credentials.

```bash
$ docker run -e MYSQL_ROOT_PASSWORD=<root_password> \
             -e MYSQL_DATABASE=<mariadb_database> \
             -e MYSQL_USER=<mariadb_user> \
             -e MYSQL_PASSWORD=<mariadb_password> \
             mariadb
```

Then you can start passbolt just by providing the database container's IP address in the
`DATASOURCES_DEFAULT_HOST` environment variable.

```bash
$ docker run --name passbolt \
             -p 80:80 \
             -p 443:443 \
             -e DATASOURCES_DEFAULT_HOST=<mariadb_container_host> \
             -e DATASOURCES_DEFAULT_PASSWORD=<mariadb_password> \
             -e DATASOURCES_DEFAULT_USERNAME=<mariadb_user> \
             -e DATASOURCES_DEFAULT_DATABASE=<mariadb_database> \
             -e APP_FULL_BASE_URL=https://example.com \
             passbolt/passbolt:develop-debian
```

Once the container is running create your first admin user:

```bash
$ docker exec passbolt su -m -c "bin/cake passbolt register_user -u your@email.com -f yourname -l surname -r admin" -s /bin/sh www-data
```

This registration command will return a single use url required to continue the
web browser setup and finish the registration. Your passbolt instance should be
available browsing `https://example.com`

If you encounter a `DNS_PROBE_FINISHED_NXDOMAIN` error when deploying locally, you may need to manually edit the 
hosts file on your machine so that the `passbolt.local` domain is resolved to your localhost ip address. On Linux, 
append the line `127.0.0.1   passbolt.local` to your `/etc/hosts` file.

# Configure passbolt

## Environment variables reference

Passbolt docker image provides several environment variables to configure different aspects:

| Variable name                       | Description                                                               | Default value
| ----------------------------------- | --------------------------------                                          | -------------------
| APP_BASE                            | In case you want to run Passbolt in a subdirectory (e.g. `https://example.com/passbolt`), set this to the path to the subdirectory (e.g. `/passbolt`). Make sure this does **not** end in a trailing slash! | null
| APP_FULL_BASE_URL                   | The hostname where your server is reachable, including `https://` (or `http://`). Make sure this does **not** end in a trailing slash! And in case you are running Passbolt from a subdirectory (e.g. `https://example.com/passbolt`), please include the subdirectory in this variable, too. | false
| DATASOURCES_DEFAULT_HOST            | Database hostname                                                         | localhost
| DATASOURCES_DEFAULT_PORT            | Database port                                                             | 3306
| DATASOURCES_DEFAULT_USERNAME        | Database username                                                         | ''
| DATASOURCES_DEFAULT_PASSWORD        | Database password                                                         | ''
| DATASOURCES_DEFAULT_DATABASE        | Database name                                                             | ''
| DATASOURCES_DEFAULT_SSL_KEY         | Database SSL Key                                                          | ''
| DATASOURCES_DEFAULT_SSL_CERT        | Database SSL Cert                                                         | ''
| DATASOURCES_DEFAULT_SSL_CA          | Database SSL CA                                                           | ''
| EMAIL_TRANSPORT_DEFAULT_CLASS_NAME  | Email classname                                                           | Smtp
| EMAIL_DEFAULT_FROM                  | From email address                                                        | you@localhost
| EMAIL_DEFAULT_TRANSPORT             | Sets transport method                                                     | default
| EMAIL_TRANSPORT_DEFAULT_HOST        | Server hostname                                                           | localhost
| EMAIL_TRANSPORT_DEFAULT_PORT        | Server port                                                               | 25
| EMAIL_TRANSPORT_DEFAULT_TIMEOUT     | Timeout                                                                   | 30
| EMAIL_TRANSPORT_DEFAULT_USERNAME    | Username for email server auth                                            | null
| EMAIL_TRANSPORT_DEFAULT_PASSWORD    | Password for email server auth                                            | null
| EMAIL_TRANSPORT_DEFAULT_CLIENT      | Client                                                                    | null
| EMAIL_TRANSPORT_DEFAULT_TLS         | Set tls                                                                   | null
| EMAIL_TRANSPORT_DEFAULT_URL         | Set url                                                                   | null
| GNUPGHOME                           | path to gnupghome directory                                               | /var/lib/passbolt/.gnupg
| PASSBOLT_KEY_LENGTH                 | Gpg desired key length                                                    | 3072
| PASSBOLT_SUBKEY_LENGTH              | Gpg desired subkey length                                                 | 3072
| PASSBOLT_KEY_NAME                   | Key owner name                                                            | Passbolt default user
| PASSBOLT_KEY_EMAIL                  | Key owner email address                                                   | passbolt@yourdomain.com
| PASSBOLT_KEY_EXPIRATION             | Key expiration date                                                       | 0, never expires
| PASSBOLT_GPG_SERVER_KEY_FINGERPRINT | GnuPG fingerprint                                                         | null
| PASSBOLT_GPG_SERVER_KEY_FINGERPRINT_FORCE | Force calculation of GnuPG fingerprint for server key               | null
| PASSBOLT_GPG_SERVER_KEY_PUBLIC      | Path to GnuPG public server key                                           | /etc/passbolt/gpg/serverkey.asc
| PASSBOLT_GPG_SERVER_KEY_PRIVATE     | Path to GnuPG private server key                                          | /etc/passbolt/gpg/serverkey_private.asc
| PASSBOLT_PLUGINS_EXPORT_ENABLED     | Enable export plugin                                                      | true
| PASSBOLT_PLUGINS_IMPORT_ENABLED     | Enable import plugin                                                      | true
| PASSBOLT_REGISTRATION_PUBLIC        | Defines if users can register                                             | false
| PASSBOLT_SSL_FORCE                  | Redirects http to https                                                   | true
| PASSBOLT_SECURITY_SET_HEADERS       | Send CSP Headers                                                          | true
| SECURITY_SALT                       | CakePHP security salt                                                     | __SALT__

For more env variables supported please check [default.php](https://github.com/passbolt/passbolt_api/blob/master/config/default.php)
and [app.default.php](https://github.com/passbolt/passbolt_api/blob/master/config/app.default.php)

### Configuration files

What if you already have a set of gpg keys and custom configuration files for passbolt?
It it possible to mount the desired configuration files as volumes.

* /etc/passbolt/app.php
* /etc/passbolt/passbolt.php
* /etc/passbolt/gpg/serverkey.asc
* /etc/passbolt/gpg/serverkey_private.asc
* /usr/share/php/passbolt/webroot/img/public/images

### SSL certificate files

It is also possible to mount a ssl certificate on the following paths:

For **image: passbolt/passbolt:latest-ce-non-root**
* /etc/passbolt/certs/certificate.crt
* /etc/passbolt/certs/certificate.key

For **image: passbolt/passbolt:latest-ce**
* /etc/ssl/certs/certificate.crt
* /etc/ssl/certs/certificate.key

### Database SSL certificate files

If Database SSL certs provided, you must mount mysql/mariadb specific conf on the following paths:
* /etc/mysql/conf.d # if using mysql
* /etc/mysql/mariadb.conf.d/ #if using mariadb

Example:
```
[client]
ssl-ca=/etc/mysql/ssl/ca-cert.pem
ssl-cert=/etc/mysql/ssl/server-cert.pem
ssl-key=/etc/mysql/ssl/server-key.pem
```


### CLI healthcheck

In order to run the healthcheck from the CLI on the container:

On a root docker image:

```
$ su -s /bin/bash www-data
$ export PASSBOLT_GPG_SERVER_KEY_FINGERPRINT="$(su -c "gpg --homedir $GNUPGHOME --list-keys --with-colons ${PASSBOLT_KEY_EMAIL:-passbolt@yourdomain.com} |grep fpr |head -1| cut -f10 -d:" -ls /bin/bash www-data)"
$ bin/cake passbolt healthcheck
```

Non root image:

```
$ export PASSBOLT_GPG_SERVER_KEY_FINGERPRINT="$(su -c "gpg --homedir $GNUPGHOME --list-keys --with-colons ${PASSBOLT_KEY_EMAIL:-passbolt@yourdomain.com} |grep fpr |head -1| cut -f10 -d:" -ls /bin/bash www-data)"
$ bin/cake passbolt healthcheck
```

## Docker secrets support

As an alternative to passing sensitive information via environment variables, _FILE may be appended to the previously listed environment variables, causing the initialization script to load the values for those variables from files present in the container. In particular, this can be used to load passwords from Docker secrets stored in /run/secrets/<secret_name> files. For example:

```
$ docker run --name passsbolt -e DATASOURCES_DEFAULT_PASSWORD_FILE=/run/secrets/db-password -d passbolt/passbolt
```

Currently, this is only supported for DATASOURCES_DEFAULT_PASSWORD, DATASOURCES_DEFAULT_HOST, DATASOURCES_DEFAULT_USERNAME, DATASOURCES_DEFAULT_DATABASE

Following the behaviour we use to mount docker secrets as environment variables, it is also posible to mount docker secrets as a file inside the passbolt container. So, for some secret files the user can store them using docker secrets and then inject them into the container with a env variable and the entrypoint script will create a symlink to the proper path.

```
$ docker run --name passsbolt -e PASSBOLT_SSL_SERVER_CERT_FILE=/run/secrets/ssl-cert -d passbolt/passbolt
```

This feature is only supported for:

- PASSBOLT_SSL_SERVER_CERT_FILE that points to /etc/ssl/certs/certificate.crt
- PASSBOLT_SSL_SERVER_KEY_FILE that points to /etc/ssl/certs/certificate.key
- PASSBOLT_GPG_SERVER_KEY_PRIVATE_FILE that points to /etc/passbolt/gpg/serverkey_private.asc
- PASSBOLT_GPG_SERVER_KEY_PUBLIC_FILE that points to /etc/passbolt/gpg/serverkey.asc

## Develop on Passbolt

This repository also provides a way to quickly setup Passbolt for development purposes. This way should never be used in production, as this would be unsafe.
You can use the docker-compose files under [docker-compose/](./docker-compose/) to spin up Passbolt for production using docker compose.
If you would like to setup Passbolt for development purposes, please follow the steps described [here](./dev/README.md).

## Run passbolt docker tests

```bash
PASSBOLT_FLAVOUR=ce PASSBOLT_COMPONENT=stable ROOTLESS=false bundle exec rake spec
```

