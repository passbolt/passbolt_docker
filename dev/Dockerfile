FROM composer:2.4 AS composer

FROM php:8.1-fpm

ARG API_REPOSITORY="https://github.com/passbolt/passbolt_api"
ARG IMAGE_DESCRIPTION="Passbolt CE Backend, a JSON API written with CakePHP"
ARG BUILD_DATE=""

LABEL org.opencontainers.image.created="${BUILD_DATE}"
LABEL org.opencontainers.image.description="${IMAGE_DESCRIPTION}"
LABEL org.opencontainers.image.documentation=https://help.passbolt.com/
LABEL org.opencontainers.image.authors="Passbolt SA <contact@passbolt.com>"
LABEL org.opencontainers.image.licenses=AGPL-3.0-only
LABEL org.opencontainers.image.source="${API_REPOSITORY}"
LABEL org.opencontainers.image.title=passbolt/passbolt
LABEL org.opencontainers.image.url=https://passbolt.com

ARG PASSBOLT_VERSION="3.8.3"
ARG PASSBOLT_URL="https://github.com/passbolt/passbolt_api/archive/v${PASSBOLT_VERSION}.tar.gz"
ARG PASSBOLT_CURL_HEADERS=""
ARG PASSBOLT_FLAVOUR="ce"

ARG PHP_EXTENSIONS="gd \
      zip \
      intl \
      pdo_mysql \
      pdo_pgsql \
      pgsql \
      opcache \
      xsl \
      ldap\
      xdebug"

ARG PECL_PASSBOLT_EXTENSIONS="gnupg \
      redis \
      xdebug"

ARG PASSBOLT_DEV_PACKAGES="libgpgme11-dev \
      libpng-dev \
      libpq-dev \
      libjpeg62-turbo-dev \
      libicu-dev \
      libxslt1-dev \
      libmcrypt-dev \
      libldap2-dev \
      libzip-dev \
      libldap2-dev \
      unzip \
      zip"

ARG PASSBOLT_BASE_PACKAGES="nginx \
      git \
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

COPY --from=composer /usr/bin/composer /usr/bin/composer

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
    && docker-php-ext-configure gd --with-jpeg=/usr/include/ \
    && docker-php-ext-install -j4 $PHP_EXTENSIONS $PECL_PASSBOLT_EXTENSIONS \
    && docker-php-ext-enable $PHP_EXTENSIONS $PECL_PASSBOLT_EXTENSIONS \
    && docker-php-source delete \
    && curl -sSL -H "$PASSBOLT_CURL_HEADERS" "$PASSBOLT_URL" | tar zxf - -C . --strip-components 1 \
    && composer install --no-ansi --no-interaction \
    && chown -R www-data:www-data . \
    && chmod 775 $(find /var/www/passbolt/tmp -type d) \
    && chmod 664 $(find /var/www/passbolt/tmp -type f) \
    && chmod 775 $(find /var/www/passbolt/webroot/img/public -type d) \
    && chmod 664 $(find /var/www/passbolt/webroot/img/public -type f) \
    && rm /etc/nginx/sites-enabled/default \
    && rm -rf /var/lib/apt/lists/* \
    && echo 'php_flag[expose_php] = off' > /usr/local/etc/php-fpm.d/expose.conf \
    && sed -i 's/# server_tokens/server_tokens/' /etc/nginx/nginx.conf \
    && mv "$PHP_INI_DIR/php.ini-development" "$PHP_INI_DIR/php.ini" \
    && echo "* * * * * su -c \"source /etc/environment ; /var/www/passbolt/bin/cron\" -s /bin/bash www-data >> /var/log/cron.log 2>&1" >> /etc/cron.d/passbolt_email \
    && crontab /etc/cron.d/passbolt_email \
    && ln -s $(which php-fpm) $(which php-fpm)7.3 \
    && touch /var/log/xdebug.log \
    && chown www-data:www-data /var/log/xdebug.log \
    && chmod 664 /var/log/xdebug.log

COPY conf/passbolt.conf /etc/nginx/conf.d/default.conf
COPY conf/supervisor/*.conf /etc/supervisor/conf.d/
COPY conf/supervisor/php-dev.conf /etc/supervisor/conf.d/php.conf
COPY conf/xdebug.ini /usr/local/etc/php/conf.d/docker-php-ext-xdebug.ini
COPY dev/bin/docker-entrypoint.sh /docker-entrypoint.sh
COPY scripts/wait-for.sh /usr/bin/wait-for.sh

EXPOSE 80 443

CMD ["/docker-entrypoint.sh"]
