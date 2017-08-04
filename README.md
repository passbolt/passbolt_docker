# Passbolt docker official image

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
$ docker run -e MYSQL_ROOT_PASSWORD=<your_root_password> \
             -e MYSQL_DATABASE=passbolt \
             -e MYSQL_USER=passbolt \
             -e MYSQL_PASSWORD=P4ssb0lt \
             mysql
```

Then you can start passbolt just by providing the database container ip in the `db_host` environment variable.

`$ docker run -e DB_HOST=<mysql_container_ip> passbolt:local`

Once the process is done, just navigate to the following url in your browser: https://passbolt_container_ip

### Note on starting passbolt container on MacOS systems

Due to the [limitations](https://docs.docker.com/docker-for-mac/networking/#known-limitations-use-cases-and-workarounds)
of docker networking under MacOS users should start the container exposing a port on the host:

`$ docker run -p host_port:443 -e DB_HOST=<mysql_container_ip> passbolt:local`

And access it using https://localhost:host_port

# Configure passbolt

## Environment variables

Passbolt docker image provides several environment variables to configure different aspects:

### GnuPG key creation related variables

* KEY_LENGTH:     gpg desired key length
* SUBKEY_LENGTH:  gpg desired subkey length
* KEY_NAME:       key owner name
* KEY_EMAIL:      key owner email address
* KEY_EXPIRATION: key expiration date

### App file variables

* FINGERPRINT:  GnuPG fingerprint
* REGISTRATION: Defines if users can register (defaults to false)
* SSL:          Forces passbolt to redirect to SSL any non-SSL request

### Core file variables

* SALT:       a random string used by cakephp in security hashing methods
* CIPHERSEED: a random string used by cakephp to encrypt/decrypt strings
* URL:        URL of the passbolt installation (defaults to passbolt.local)

### Database variables

* DB_HOST: database hostname This param has to be specified either using env var or in database.php (defaults to passbolt.local)
* DB_USER: database username (defaults to passbolt)
* DB_PASS: database password (defaults to P4ssb0lt)
* DB_NAME: database name     (defaults to passbolt)

### Email variables

* EMAIL_TRANSPORT: transport protocol             ( defaults to Smtp)
* EMAIL_FROM:      from email address             ( defaults to contact@mydomain.local)
* EMAIL_HOST:      server hostname                ( defaults to localhost)
* EMAIL_PORT:      server port                    ( defaults to 587)
* EMAIL_TIMEOUT:   timeout                        ( defaults to 30s)
* EMAIL_USERNAME:  username for email server auth ( defaults to email_user)
* EMAIL_PASSWORD:  password for email server auth ( defaults to email_password)
* EMAIL_TLS:       set tls, boolean               ( defaults to false)

## Advanced configuration

What if you already have a set of gpg keys and custom configuration files for passbolt?
It it possible to mount the desired configuration files as volumes.

### Configuration files subject to be persisted:

* /var/www/passbolt/app/Config/app.php
* /var/www/passbolt/app/Config/core.php
* /var/www/passbolt/app/Config/database.php
* /var/www/passbolt/app/Config/email.php
* /var/www/passbolt/app/Config/gpg/serverkey.asc
* /var/www/passbolt/app/Config/gpg/serverkey.private.asc
* /var/www/passbolt/app/webroot/img/public/images

### SSL certificate files

It is also possible to mount a ssl certificate on the following paths:

* /etc/ssl/certs/certificate.crt
* /etc/ssl/certs/certificate.key

# Examples

For the following examples it is assumed that passbolt container image has been built from this repo following the instructions
described on the [Build](#Build-the-image) section.

In the following example passbolt is launched with the defaults enabled usind mysql official docker container to store passbolt data:

```bash
$ docker run -e MYSQL_ROOT_PASSWORD=c0mplexp4ss \
             -e MYSQL_DATABASE=passbolt \
             -e MYSQL_USER=passbolt \
             -e MYSQL_PASSWORD=P4ssb0lt \
             mysql
```

Once mysql container is running we should extract its ip address. Let's assume 172.17.0.2 for this example

`$ docker run -e DB_HOST=172.17.0.2 passbolt:local`

Point your browser to the passbolt container ip or localhost:exposed_port.

## Advanced configuration

In the following example passbolt is launched with a customized setup mounting and persisting configuration files. We also make use of
mysql official docker container to store passbolt data.

```bash
$ docker run -e MYSQL_ROOT_PASSWORD=c0mplexp4ss \
             -e MYSQL_DATABASE=passbolt \
             -e MYSQL_USER=passbolt \
             -e MYSQL_PASSWORD=P4ssb0lt \
             mysql
```

Using docker inspect or any other method you can get the ip address of the mysql container. This example uses 172.17.0.2.

Once this container is running and you have the mysql ip address we run passbolt container mounting all configuration files stored
under a example conf directory in $PWD

```bash
$ docker run -v $PWD/conf/app.php:/var/www/passbolt/app/Config/app.php \
             -v $PWD/conf/core.php:/var/www/passbolt/app/Config/core.php \
             -v $PWD/conf/database.php:/var/www/passbolt/app/Config/database.php \
             -v $PWD/conf/email.php:/var/www/passbolt/app/Config/email.php \
             -v $PWD/conf/private.asc:/var/www/passbolt/app/Config/gpg/serverkey.private.asc \
             -v $PWD/conf/public.asc:/var/www/passbolt/app/Config/gpg/serverkey.asc \
             passbolt:local
```

Navigate with the browser to the passbolt container ip or localhost:exposed_port

# Requirements:

* rng-tools are required on host machine to speed up entropy generation on containers. This way gpg key creation on passbolt container will be faster.
* mysql >= 5.6
