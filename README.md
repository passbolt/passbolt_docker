# Passbolt docker official image

# Warning

This is a work in progress branch use at your own risk.

# What is passbolt?

Passbolt is a free and open source password manager that allows team members to
store and share credentials securely.

# Scope of this repository

This repository will allow passbolt power users to customize their passbolt image to fit their needs on
specific environments. It is also a community meeting point to exchange feedback, request for new features
track issues and pull requests.

Users that do not require any special modifications are encouraged to `docker pull` the
[official docker image from the docker hub](https://hub.docker.com/r/passbolt/passbolt/).

# Build the image

Inside the repo directory:

`$ docker build . -t passbolt:local`

# How to use the local image?

## Start passbolt instance

Passbolt requires mysql to be running. The following example use mysql official docker image
with the default passbolt credentials.

```bash
$ docker run -e MYSQL_ROOT_PASSWORD=<root_password> \
             -e MYSQL_DATABASE=<mysql_database> \
             -e MYSQL_USER=<mysql_user> \
             -e MYSQL_PASSWORD=<mysql_password> \
             mysql
```

Then you can start passbolt just by providing the database container ip in the `db_host` environment variable.

```bash
$ docker run -e DATASOURCES_DEFAULT_HOST=<mysql_container_host> \
             -e DATASOURCES_DEFAULT_PASSWORD=<mysql_password> \
             -e DATASOURCES_DEFAULT_USERNAME=<mysql_user> \
             -e DATASOURCES_DEFAULT_DATABASE=<mysql_database> \
             passbolt:local
```

Once the process is done, just navigate to the following url in your browser: https://passbolt_container_ip

### Note on starting passbolt container on MacOS systems

Due to the [limitations](https://docs.docker.com/docker-for-mac/networking/#known-limitations-use-cases-and-workarounds)
of docker networking under MacOS users should start the container exposing a port on the host:

`$ docker run -p host_port:443 -e DB_HOST=<mysql_container_ip> passbolt:local`

And access it using https://localhost:host_port

# Configure passbolt

## Environment variables reference

Passbolt docker image provides several environment variables to configure different aspects:

* APP_FULL_BASE_URL: Defines Passbolt base url (Example https://yourdomain.com)
* DATASOURCES_DEFAULT_HOST:            database hostname              (defaults to localhost)
* DATASOURCES_DEFAULT_PORT:            database port                  (defaults to 3306)
* DATASOURCES_DEFAULT_USERNAME:        database username              (defaults to my_app)
* DATASOURCES_DEFAULT_PASSWORD:        database password              (defaults to secret)
* DATASOURCES_DEFAULT_DATABASE:        database name                  (defaults to my_app)
* EMAIL_DEFAULT_FROM:                  from email address             (defaults to contact@mydomain.local)
* EMAIL_DEFAULT_TRANSPORT:             sets transport method          (defaults to default)
* EMAIL_TRANSPORT_DEFAULT_HOST:        server hostname                (defaults to localhost)
* EMAIL_TRANSPORT_DEFAULT_PORT:        server port                    (defaults to 25)
* EMAIL_TRANSPORT_DEFAULT_TIMEOUT:     timeout                        (defaults to 30)
* EMAIL_TRANSPORT_DEFAULT_USERNAME:    username for email server auth (defaults to null)
* EMAIL_TRANSPORT_DEFAULT_PASSWORD:    password for email server auth (defaults to null)
* EMAIL_TRANSPORT_DEFAULT_CLIENT:      client                         (defaults to null)
* EMAIL_TRANSPORT_DEFAULT_TLS:         set tls                        (defaults to null)
* EMAIL_TRANSPORT_DEFAULT_URL:         set url                        (defaults to null)
* GNUPGHOME:                           Path to gnupghome directory    (defaults to web_user_home_directory/.gnupg )
* PASSBOLT_KEY_LENGTH:                 gpg desired key length
* PASSBOLT_SUBKEY_LENGTH:              gpg desired subkey length
* PASSBOLT_KEY_NAME:                   key owner name
* PASSBOLT_KEY_EMAIL:                  key owner email address
* PASSBOLT_KEY_EXPIRATION:             key expiration date
* PASSBOLT_GPG_SERVER_KEY_FINGERPRINT: GnuPG fingerprint
* PASSBOLT_GPG_SERVER_KEY_PUBLIC:      Path to GnuPG public server key
* PASSBOLT_GPG_SERVER_KEY_PRIVATE:     Path to GnuPG private server key
* PASSBOLT_REGISTRATION_PUBLIC:        Defines if users can register (defaults to false)
* PASSBOLT_SSL_FORCE:                  Forces passbolt to redirect to SSL any non-SSL request
* PASSBOLT_SECURITY_SET_HEADERS:       Forces passbolt to send CSP Headers (defaults to true)
* SECURITY_SALT:                       A random number user in security hashing methods.

## Advanced configuration

What if you already have a set of gpg keys and custom configuration files for passbolt?
It it possible to mount the desired configuration files as volumes.

### Configuration files subject to be persisted:

* /var/www/passbolt/config/app.php
* /var/www/passbolt/config/gpg/serverkey.asc
* /var/www/passbolt/config/gpg/serverkey_private.asc
* /var/www/passbolt/app/webroot/img/public/images

### SSL certificate files

It is also possible to mount a ssl certificate on the following paths:

* /etc/ssl/certs/certificate.crt
* /etc/ssl/certs/certificate.key

# Requirements:

* rng-tools are required on host machine to speed up entropy generation on containers. This way gpg key creation on passbolt container will be faster.
* mysql >= 5.6
