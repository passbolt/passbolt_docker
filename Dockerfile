FROM alpine

LABEL maintainer="diego@passbolt.com"

ARG PASSBOLT_VERSION="2.0.0-rc2"
ARG PASSBOLT_URL="https://github.com/passbolt/passbolt_api/archive/v${PASSBOLT_VERSION}.tar.gz"

ARG PHP_EXTENSIONS="php7-gd \
      php7-intl \
      php7-pdo_mysql \
      php7-xsl \
      php7-redis \
      php7-openssl \
      php7-json \
      php7-zlib \
      php7-phar \
      php7-mbstring \
      php7-ctype \
      php7-posix \
      php7-mcrypt \
      php7-iconv"

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
      file \
      php7-pear"

ARG PECL_PASSBOLT_EXTENSIONS="gnupg"

WORKDIR /var/www/passbolt
RUN apk add --no-cache $PHP_GNUPG_BUILD_DEPS \
      $PHP_EXTENSIONS \
      openssl \
      nginx \
      php7-fpm \
      gpgme \
      gnupg1 \
      mysql-client \
      supervisor \
      php7 \
      curl \
      git \
    && pecl install $PECL_PASSBOLT_EXTENSIONS \
    && echo "extension=gnupg.so" > /etc/php7/conf.d/20_gnupg.ini \
    && apk del $PHP_GNUPG_BUILD_DEPS \
    && php -r "copy('https://getcomposer.org/installer', 'composer-setup.php');" \
    && php composer-setup.php \
    && php -r "unlink('composer-setup.php');" \
    && mv composer.phar /usr/local/bin/composer \
    && curl -sSL $PASSBOLT_URL | tar zxf - -C . --strip-components 1 \
    && composer install -n --no-dev --optimize-autoloader \
    && apk del git \
    && chown -R nginx:nginx . \
    && chmod 775 $(find /var/www/passbolt/tmp -type d) \
    && chmod 664 $(find /var/www/passbolt/tmp -type f) \
    && chmod 775 $(find /var/www/passbolt/webroot/img/public -type d) \
    && chmod 664 $(find /var/www/passbolt/webroot/img/public -type f) \
    && sed -i 's/;daemonize = yes/daemonize = no/g' /etc/php7/php-fpm.conf \
    && sed -i 's/;clear_env = no/clear_env = no/g' /etc/php7/php-fpm.d/www.conf \
    && sed -i 's/user = nobody/user = nginx/g' /etc/php7/php-fpm.d/www.conf \
    && sed -i 's/group = nobody/user = nginx/g' /etc/php7/php-fpm.d/www.conf

COPY conf/passbolt.conf /etc/nginx/conf.d/default.conf
COPY conf/supervisord.conf /etc/supervisord.conf
COPY bin/docker-entrypoint.sh /docker-entrypoint.sh

EXPOSE 80 443

CMD ["/docker-entrypoint.sh"]
