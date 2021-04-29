FROM php:7.3.24-fpm

LABEL maintainer="Passbolt SA <contact@passbolt.com>"

ARG PASSBOLT_VERSION="3.1.0"
ARG PASSBOLT_URL="https://github.com/passbolt/passbolt_api/archive/v${PASSBOLT_VERSION}.tar.gz"
ARG PASSBOLT_CURL_HEADERS=""
ARG PASSBOLT_FLAVOUR="ce"

ARG PHP_EXTENSIONS="gd \
      intl \
      pdo_mysql \
      opcache \
      xsl \
      ldap"

ARG PECL_PASSBOLT_EXTENSIONS="gnupg \
      redis \
      mcrypt"

ARG PASSBOLT_DEV_PACKAGES="libgpgme11-dev \
      libpng-dev \
      libjpeg62-turbo-dev \
      libicu-dev \
      libxslt1-dev \
      libmcrypt-dev \
      unzip \
      libldap2-dev"

ARG PASSBOLT_BASE_PACKAGES="nginx \
         gnupg \
         libgpgme11 \
         libmcrypt4 \
         mariadb-client \
         supervisor \
         cron"

ENV PECL_BASE_URL="https://pecl.php.net/get"
ENV PHP_EXT_DIR="/usr/src/php/ext"
ENV PASSBOLT_FLAVOUR=$PASSBOLT_FLAVOUR

WORKDIR /var/www/passbolt
RUN apt-get update \
    && apt-get -y install --no-install-recommends \
      $PASSBOLT_DEV_PACKAGES \
      $PASSBOLT_BASE_PACKAGES \
    && mkdir /home/www-data \
    && chown -R www-data:www-data /home/www-data \
    && usermod -d /home/www-data www-data \
    && docker-php-source extract \
    && for i in $PECL_PASSBOLT_EXTENSIONS; do \
         mkdir $PHP_EXT_DIR/$i; \
         curl -sSL $PECL_BASE_URL/$i | tar zxf - -C $PHP_EXT_DIR/$i --strip-components 1; \
       done \
    && docker-php-ext-configure gd --with-jpeg-dir=/usr/include/ \
    && docker-php-ext-install -j4 $PHP_EXTENSIONS $PECL_PASSBOLT_EXTENSIONS \
    && docker-php-ext-enable $PHP_EXTENSIONS $PECL_PASSBOLT_EXTENSIONS \
    && docker-php-source delete \
    && EXPECTED_SIGNATURE=$(curl -s https://composer.github.io/installer.sig) \
    && curl -o composer-setup.php https://getcomposer.org/installer \
    && ACTUAL_SIGNATURE=$(php -r "echo hash_file('SHA384', 'composer-setup.php');") \
    && if [ "$EXPECTED_SIGNATURE" != "$ACTUAL_SIGNATURE" ]; then \
         >&2 echo 'ERROR: Invalid installer signature'; \
         rm composer-setup.php; \
         exit 1; \
       fi \
    && php composer-setup.php \
    && mv composer.phar /usr/local/bin/composer \
    && rm composer-setup.php \
    && curl -sSL -H "$PASSBOLT_CURL_HEADERS" "$PASSBOLT_URL" | tar zxf - -C . --strip-components 1 \
    && composer install -n --no-dev --optimize-autoloader \
    && chown -R www-data:www-data . \
    && chmod 775 $(find /var/www/passbolt/tmp -type d) \
    && chmod 664 $(find /var/www/passbolt/tmp -type f) \
    && chmod 775 $(find /var/www/passbolt/webroot/img/public -type d) \
    && chmod 664 $(find /var/www/passbolt/webroot/img/public -type f) \
    && rm /etc/nginx/sites-enabled/default \
    && apt-get purge -y --auto-remove $PASSBOLT_DEV_PACKAGES \
    && rm -rf /var/lib/apt/lists/* \
    && rm /usr/local/bin/composer \
    && echo 'php_flag[expose_php] = off' > /usr/local/etc/php-fpm.d/expose.conf \
    && sed -i 's/# server_tokens/server_tokens/' /etc/nginx/nginx.conf \
    && mv "$PHP_INI_DIR/php.ini-production" "$PHP_INI_DIR/php.ini" \
    && echo "* * * * * su -c \"source /etc/environment ; /var/www/passbolt/bin/cake EmailQueue.sender\" -s /bin/bash www-data >> /var/log/cron.log 2>&1" >> /etc/cron.d/passbolt_email \
    && crontab /etc/cron.d/passbolt_email \
    && ln -s $(which php-fpm) $(which php-fpm)7.3

COPY conf/passbolt.conf /etc/nginx/conf.d/default.conf
COPY conf/supervisor/*.conf /etc/supervisor/conf.d/
COPY dev/bin/docker-entrypoint.sh /docker-entrypoint.sh
COPY scripts/wait-for.sh /usr/bin/wait-for.sh

EXPOSE 80 443

CMD ["/docker-entrypoint.sh"]
