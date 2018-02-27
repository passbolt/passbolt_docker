FROM php:7-fpm

LABEL maintainer="diego@passbolt.com"

ARG PASSBOLT_VERSION="2.0.0-rc2"
ARG PASSBOLT_URL="https://github.com/passbolt/passbolt_api/archive/v${PASSBOLT_VERSION}.tar.gz"

ARG PHP_EXTENSIONS="gd \
      intl \
      pdo_mysql \
      xsl"

ARG PECL_PASSBOLT_EXTENSIONS="gnupg \
      redis \
      mcrypt"

ENV PECL_BASE_URL="https://pecl.php.net/get"
ENV PHP_EXT_DIR="/usr/src/php/ext"

WORKDIR /var/www/passbolt
RUN apt-get update && apt-get -y install \
      --no-install-recommends \
      nginx \
      libgpgme11-dev \
      gnupg1 \
      mysql-client \
      libpng-dev \
      libicu-dev \
      libxslt1-dev \
      libmcrypt-dev \
      supervisor \
      git \
      netcat \
      procps \
      cron \
    && mv /usr/bin/gpg /usr/bin/gpg2 \
    && update-alternatives --verbose --install /usr/bin/gpg gnupg /usr/bin/gpg1 50 \
    && mkdir /home/www-data \
    && chown -R www-data:www-data /home/www-data \
    && usermod -d /home/www-data www-data \
    && docker-php-source extract \
    && for i in $PECL_PASSBOLT_EXTENSIONS; do \
         mkdir $PHP_EXT_DIR/$i; \
         curl -sSL $PECL_BASE_URL/$i | tar zxf - -C $PHP_EXT_DIR/$i --strip-components 1; \
       done \
    && docker-php-ext-install -j4 $PHP_EXTENSIONS $PECL_PASSBOLT_EXTENSIONS \
    && docker-php-ext-enable $PHP_EXTENSIONS $PECL_PASSBOLT_EXTENSIONS \
    && curl -sS https://getcomposer.org/installer | php \
    && mv composer.phar /usr/local/bin/composer \
    && curl -sSL $PASSBOLT_URL | tar zxf - -C . --strip-components 1 \
    && composer install --no-dev --optimize-autoloader \
    && chown -R www-data:www-data . \
    && chmod 775 $(find /var/www/passbolt/tmp -type d) \
    && chmod 664 $(find /var/www/passbolt/tmp -type f) \
    && chmod 775 $(find /var/www/passbolt/webroot/img/public -type d) \
    && chmod 664 $(find /var/www/passbolt/webroot/img/public -type f) \
    && rm /etc/nginx/sites-enabled/default

COPY conf/passbolt.conf /etc/nginx/conf.d/default.conf
COPY conf/supervisord.conf /etc/supervisord.conf
COPY bin/docker-entrypoint.sh /docker-entrypoint.sh

EXPOSE 80 443

CMD ["/docker-entrypoint.sh"]
