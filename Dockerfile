FROM alpine:3.6

MAINTAINER diego@passbolt.com

ENV PASSBOLT_VERSION 1.6.5
ENV PASSBOLT_URL https://github.com/passbolt/passbolt_api/archive/v${PASSBOLT_VERSION}.tar.gz

ARG BASE_PHP_DEPS="php7-curl \
      php7-common \
      php7-gd \
      php7-intl \
      php7-json \
      php7-mcrypt \
      php7-mysqli \
      php7-xsl \
      php7-fpm \
      php7-phar \
      php7-posix \
      php7-xml \
      php7-openssl \
      php7-zlib \
      php7-ctype \
      php7-pdo \
      php7-pdo_mysql \
      php7-pear \
      php7-session \
      php7-iconv \
      php7-mbstring"

ARG BASE_NODE_DEPS="nodejs \
    nodejs-npm"

ARG PHP_GNUPG_DEPS="php7-dev \
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

ARG BASE_PHP_DEV_DEPS="php7-tokenizer \
    php7-xmlwriter \
    php7-simplexml \
    php7-xdebug \
    git \
    openssh"

RUN apk add --no-cache $BASE_PHP_DEPS \
      $BASE_NODE_DEPS \
      $BASE_PHP_DEV_DEPS \
      sed \
      tar \
      bash \
      curl \
      nginx \
      gpgme \
      gnupg1 \
      recode \
      libxml2 \
      openssl \
      libpcre32 \
      mysql-client \
      ca-certificates

RUN apk add --no-cache $PHP_GNUPG_DEPS \
    #https://bugs.alpinelinux.org/issues/5378
    && sed -i "s/ -n / /" $(which pecl) \
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
