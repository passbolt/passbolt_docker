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

`$ docker run -e db_host=<mysql_container_ip> passbolt:local`

Once the process is done, just navigate to the following url in your browser: https://passbolt_container_ip

### Note on starting passbolt container on MacOS systems

Due to the [limitations](https://docs.docker.com/docker-for-mac/networking/#known-limitations-use-cases-and-workarounds)
of docker networking under MacOS users should start the container exposing a port on the host:

`$ docker run -p host_port:443 -e db_host=<mysql_container_ip> passbolt:local`

And access it using https://localhost:host_port

# Configure passbolt

## Environment variables

Passbolt docker image provides several environment variables to configure different aspects:

### GnuPG key creation related variables

* key_length: gpg desired key length
* subkey_length: gpg desired subkey length
* key_name: key owner name
* key_email: key owner email address
* key_expiration: key expiration date

### App file variables

* fingerprint: GnuPG fingerprint
* registration: defines if users can register
* ssl

### Core file variables

* salt
* cipherseed
* url: url of the passbolt installation

### Database variables

* db_host: database hostname This param has to be specified either using env var or in database.php
* db_user: database username (defaults to passbolt)
* db_pass: database password (defaults to P4ssb0lt)
* db_name: database name     (defaults to passbolt)

### Email variables

* email_transport: transport protocol             ( defaults to Smtp)
* email_from:     from email address             ( defaults to contact@mydomain.local)
* email_host:     server hostname                ( defaults to localhost)
* email_port:     server port                    ( defaults to 587)
* email_timeout:  timeout                        ( defaults to 30s)
* email_username: username for email server auth ( defaults to email_user)
* email_password: password for email server auth ( defaults to email_password)
* email_tls:      set tls, boolean               ( defaults to false)

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

## Automated setup

In the following example passbolt is launched with the defaults enabled usind mysql official docker container to store passbolt data:

```bash
$ docker run -e MYSQL_ROOT_PASSWORD=c0mplexp4ss \
             -e MYSQL_DATABASE=passbolt \
             -e MYSQL_USER=passbolt \
             -e MYSQL_PASSWORD=P4ssb0lt \
             mysql
```

Once mysql container is running we should extract its ip address. Let's assume 172.17.0.2 for this example

`$ docker run -e db_host=172.17.0.2 passbolt:local`

Point your browser to the passbolt container ip or localhost:exposed_port

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
