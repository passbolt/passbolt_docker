FROM debian:jessie

# mysql before installation configuration
RUN export DEBIAN_FRONTEND="noninteractive" \
    && echo "mysql-server mysql-server/root_password password root" | debconf-set-selections \
    && echo "mysql-server mysql-server/root_password_again password root" | debconf-set-selections

# debian packages installation
RUN apt-get clean && apt-get update && apt-get install -y \
    # persistent &runtime deps. \
    ca-certificates curl libpcre3 librecode0 libsqlite3-0 libxml2 --no-install-recommends \
    # unix tools \
    nano wget openssh-client \
    # versioning & package manager \
    git npm \
    # phpize dependencies \
    autoconf file g++ gcc libc-dev make pkg-config re2c \
    # persistance
    redis-server mysql-server-5.5 \
    # php \
    php5-json php5-cli php5-common \
    php5-curl php5-dev php5-gd php5-mcrypt \
    php5-mysql php5-xdebug php5-xsl php5-intl \
    # memchached \
    memcached php5-memcached \
    # apache \
    apache2 apache2-utils libapache2-mod-php5 \
    # gnupg dependency \
    libgpgme11-dev \
    # pear \
    php-pear \
    # Commented until the following bug is fixed : https://github.com/docker/hub-feedback/issues/556 \
    #&& apt-get clean \
     && rm -rf /var/lib/apt/lists/*

# Configure the user www-data environment
RUN mkdir /home/www-data/ \
    && chown www-data:www-data /home/www-data/ \
    && usermod -d /home/www-data www-data

# Configure node and install grunt
# On debian they choose to rename node in nodejs, some tools try to access nodejs by using the commande noe.
RUN ln -s /usr/bin/nodejs /usr/bin/node \
    # install grunt
    && npm install -g grunt-cli

# Apache2 SSL
RUN mkdir /etc/apache2/ssl \
	&& openssl req -x509 -nodes -days 365 -new -newkey rsa:2048 -subj "/C=US/ST=Denial/L=Goa/O=Dis/CN=www.passbolt.com" -keyout /etc/apache2/ssl/apache.key -out /etc/apache2/ssl/apache.crt \
	&& chmod 600 /etc/apache2/ssl/* \
	&& a2enmod ssl

# Install and configure gnupg
RUN pecl install gnupg \
    && echo "extension=gnupg.so;" > /etc/php5/mods-available/gnupg.ini \
    && ln -s /etc/php5/mods-available/gnupg.ini /etc/php5/apache2/conf.d/20-gnupg.ini \
    && ln -s /etc/php5/mods-available/gnupg.ini /etc/php5/cli/conf.d/20-gnupg.ini \
    # configure the user www-data env to work with gnupg \
    && mkdir /home/www-data/.gnupg \
    && chown www-data:www-data /home/www-data/.gnupg \
    && chmod 0777 /home/www-data/.gnupg

# Configure apache
ADD /server-conf/apache/passbolt.conf /etc/apache2/sites-available/passbolt.conf
ADD /server-conf/apache/000-default.conf /etc/apache2/sites-available/000-default.conf

RUN rm -f /etc/apache2/sites-enabled/* \
    && rm -fr /var/www/html \
    && a2enmod proxy \
    && a2enmod proxy_http \
    && a2enmod rewrite \
    && a2ensite passbolt \
    && a2ensite 000-default.conf

# Configure php
RUN echo "memory_limit=256M" > /etc/php5/apache2/conf.d/20-memory-limit.ini \
    && echo "memory_limit=256M" > /etc/php5/cli/conf.d/20-memory-limit.ini

# Install composer
RUN curl -sS https://getcomposer.org/installer | php \
    && mv composer.phar /usr/local/bin/composer

# Generate the gpg server key
ADD /conf/gpg_server_key_public.key /home/www-data/gpg_server_key_public.key
ADD /conf/gpg_server_key_private.key /home/www-data/gpg_server_key_private.key

# Special hack for macosx, to let www-data able to write on mounted volumes.
# See docker bug: https://github.com/boot2docker/boot2docker/issues/581.
RUN usermod -u 1000 www-data \
    && usermod -a -G staff www-data \
    && chown -Rf www-data:www-data /var/www/

ADD /entry-point.sh /entry-point.sh
RUN chmod 0755 /entry-point.sh

CMD ["bash", "/entry-point.sh"]
