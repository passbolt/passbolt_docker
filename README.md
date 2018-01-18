# Passbolt docker official image

# Warning

This is a work in progress branch use at your own risk.

# What is passbolt?

Passbolt is a free and open source password manager that allows team members to
store and share credentials securely.

# Usage

Users are encouraged to use [official docker image from the docker hub](https://hub.docker.com/r/passbolt/passbolt/).

## Start passbolt instance

Passbolt requires mysql to be running. The following example use mysql official
docker image with the default passbolt credentials.

```bash
$ docker run -e MYSQL_ROOT_PASSWORD=<root_password> \
             -e MYSQL_DATABASE=<mysql_database> \
             -e MYSQL_USER=<mysql_user> \
             -e MYSQL_PASSWORD=<mysql_password> \
             mysql
```

Then you can start passbolt just by providing the database container ip in the
`db_host` environment variable.

```bash
$ docker run --name passbolt \
             -e DATASOURCES_DEFAULT_HOST=<mysql_container_host> \
             -e DATASOURCES_DEFAULT_PASSWORD=<mysql_password> \
             -e DATASOURCES_DEFAULT_USERNAME=<mysql_user> \
             -e DATASOURCES_DEFAULT_DATABASE=<mysql_database> \
             -e APP_FULL_BASE_URL=https://mydomain.com \
             passbolt/passbolt:2.0.0-rc1
```

Once the container is running create your first admin user:

```bash
$ docker exec passbolt su -m -c "/var/www/passbolt/bin/cake passbolt register_user -u your@email.com -f yourname -l surname -r admin" -s /bin/sh www-data
```

This registration command will return a single use url required to continue the
web browser setup and finish the registration. Your passbolt instance should be
available browsing `https://yourdomain.com`

# Configure passbolt

## Environment variables reference

Passbolt docker image provides several environment variables to configure different aspects:

```
* APP_FULL_BASE_URL:                   Passbolt base url                     (Example https://yourdomain.com)
* DATASOURCES_DEFAULT_HOST:            database hostname                     (default: localhost)
* DATASOURCES_DEFAULT_PORT:            database port                         (default: 3306)
* DATASOURCES_DEFAULT_USERNAME:        database username                     (default: my_app)
* DATASOURCES_DEFAULT_PASSWORD:        database password                     (default: secret)
* DATASOURCES_DEFAULT_DATABASE:        database name                         (default: my_app)
* EMAIL_DEFAULT_FROM:                  from email address                    (default: contact@mydomain.local)
* EMAIL_DEFAULT_TRANSPORT:             sets transport method                 (default: default)
* EMAIL_TRANSPORT_DEFAULT_HOST:        server hostname                       (default: localhost)
* EMAIL_TRANSPORT_DEFAULT_PORT:        server port                           (default: 25)
* EMAIL_TRANSPORT_DEFAULT_TIMEOUT:     timeout                               (default: 30)
* EMAIL_TRANSPORT_DEFAULT_USERNAME:    username for email server auth        (default: null)
* EMAIL_TRANSPORT_DEFAULT_PASSWORD:    password for email server auth        (default: null)
* EMAIL_TRANSPORT_DEFAULT_CLIENT:      client                                (default: null)
* EMAIL_TRANSPORT_DEFAULT_TLS:         set tls                               (default: null)
* EMAIL_TRANSPORT_DEFAULT_URL:         set url                               (default: null)
* GNUPGHOME:                           Path to gnupghome directory           (default: web_user_home_directory/.gnupg )
* PASSBOLT_KEY_LENGTH:                 gpg desired key length                (default: 2048)
* PASSBOLT_SUBKEY_LENGTH:              gpg desired subkey length             (default: 2048)
* PASSBOLT_KEY_NAME:                   key owner name                        (default: Passbolt default user)
* PASSBOLT_KEY_EMAIL:                  key owner email address               (default: passbolt@yourdomain.com)
* PASSBOLT_KEY_EXPIRATION:             key expiration date                   (default: 0, never expires)
* PASSBOLT_GPG_SERVER_KEY_FINGERPRINT: GnuPG fingerprint
* PASSBOLT_GPG_SERVER_KEY_PUBLIC:      Path to GnuPG public server key       (defaults to /var/www/passbolt/config/gpg/serverkey.asc)
* PASSBOLT_GPG_SERVER_KEY_PRIVATE:     Path to GnuPG private server key      (defaults to /var/www/passbolt/config/gpg/serverkey_private.asc)
* PASSBOLT_REGISTRATION_PUBLIC:        Defines if users can register         (defaults to false)
* PASSBOLT_SSL_FORCE:                  Redirects http to https from passbolt (defaults to true)
* PASSBOLT_SECURITY_SET_HEADERS:       Send CSP Headers from passbolt        (defaults to true)
* SECURITY_SALT:                       A random number user in security hashing methods.
```

### Configuration files

What if you already have a set of gpg keys and custom configuration files for passbolt?
It it possible to mount the desired configuration files as volumes.

* /var/www/passbolt/config/app.php
* /var/www/passbolt/config/passbolt.php
* /var/www/passbolt/config/gpg/serverkey.asc
* /var/www/passbolt/config/gpg/serverkey_private.asc
* /var/www/passbolt/app/webroot/img/public/images

### SSL certificate files

It is also possible to mount a ssl certificate on the following paths:

* /etc/ssl/certs/certificate.crt
* /etc/ssl/certs/certificate.key

### docker-compose

Usage:

```
$ docker-compose up
```

# Requirements:

* rng-tools are required on host machine to speed up entropy generation on containers.
This way gpg key creation on passbolt container will be faster.
* mysql >= 5.6
