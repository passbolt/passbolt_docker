FROM php:7-fpm-alpine

LABEL MAINTAINER diego@passbolt.com

ENV PASSBOLT_VERSION 1.6.5
ENV PASSBOLT_URL https://github.com/passbolt/passbolt_api/archive/v${PASSBOLT_VERSION}.tar.gz

ARG PHP_EXTENSIONS="gd \
      intl \
      mcrypt \
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

RUN apk add --no-cache $PHP_EXTENSIONS \
      $PHP_GNUPG_BUILD_DEPS \
      sed \
      nginx \
      gpgme \
      gnupg1 \
      mysql-client

RUN apk add --no-cache $PHP_GNUPG_DEPS \
    && pecl install gnupg \
    && echo "extension=gnupg.so" > /etc/php7/conf.d/gnupg.ini \
    && apk del $PHP_GNUPG_DEPS \
    && curl -sS https://getcomposer.org/installer | php \
    && mv composer.phar /usr/local/bin/composer

RUN mkdir /var/www/passbolt \
    && curl -sSL $PASSBOLT_URL | tar zxf - -C /var/www/passbolt --strip-components 1 \
    && chown -R nginx:nginx /var/www/passbolt \
    && chmod -R a-w /var/www/passbolt \
    && chmod -R +w /var/www/passbolt/app/tmp \
    && chmod -R +w /var/www/passbolt/app/webroot/img/public

COPY conf/passbolt.conf /etc/nginx/conf.d/default.conf
COPY bin/docker-entrypoint.sh /docker-entrypoint.sh

EXPOSE 80 443

CMD ["/docker-entrypoint.sh"]
