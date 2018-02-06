FROM php:7-fpm-alpine3.7

LABEL maintainer="diego@passbolt.com"

ENV PASSBOLT_VERSION 2.0.0-rc1
ENV PASSBOLT_URL https://github.com/passbolt/passbolt_api/archive/v${PASSBOLT_VERSION}.tar.gz

ARG PHP_EXTENSIONS="gd \
      intl \
      pdo_mysql \
      xsl"

ARG PHP_GNUPG_BUILD_DEPS="php7-dev \
      make \
      gcc \
      g++ \
      libc-dev \
      pkgconfig \
      re2c \
      gpgme-dev \
      autoconf \
      zlib-dev \
      file"

ARG PECL_PASSBOLT_EXTENSIONS="gnupg \
      redis"

RUN apk add --no-cache $PHP_GNUPG_BUILD_DEPS \
      nginx \
      gpgme \
      gnupg1 \
      mysql-client \
      libpng-dev \
      icu-dev \
      libxslt-dev \
      libmcrypt-dev \
      supervisor \
      git \
    && pecl install $PECL_PASSBOLT_EXTENSIONS mcrypt-snapshot \
    && docker-php-ext-install -j4 $PHP_EXTENSIONS \
    && docker-php-ext-enable $PHP_EXTENSIONS $PECL_PASSBOLT_EXTENSIONS mcrypt \
    && apk del $PHP_GNUPG_BUILD_DEPS \
    && curl -sS https://getcomposer.org/installer | php \
    && mv composer.phar /usr/local/bin/composer

WORKDIR /var/www/passbolt
RUN curl -sSL $PASSBOLT_URL | tar zxf - -C . --strip-components 1 \
    && composer install --no-dev --optimize-autoloader \
    && chown -R www-data:www-data . \
    && chmod 775 $(find /var/www/passbolt/tmp -type d) \
    && chmod 664 $(find /var/www/passbolt/tmp -type f) \
    && chmod 775 $(find /var/www/passbolt/webroot/img/public -type d) \
    && chmod 664 $(find /var/www/passbolt/webroot/img/public -type f)

COPY conf/passbolt.conf /etc/nginx/conf.d/default.conf
COPY conf/supervisord.conf /etc/supervisord.conf
COPY bin/docker-entrypoint.sh /docker-entrypoint.sh

EXPOSE 80 443

CMD ["/docker-entrypoint.sh"]
