FROM alpine:3.6

LABEL maintainer="diego@passbolt.com"

ENV PASSBOLT_VERSION 1.6.9
ENV PASSBOLT_URL https://github.com/passbolt/passbolt_api/archive/v${PASSBOLT_VERSION}.tar.gz

ARG BASE_PHP_DEPS="php5-curl \
      php5-common \
      php5-gd \
      php5-intl \
      php5-json \
      php5-mcrypt \
      php5-mysql \
      php5-xsl \
      php5-fpm \
      php5-phar \
      php5-posix \
      php5-xml \
      php5-openssl \
      php5-zlib \
      php5-ctype \
      php5-pdo \
      php5-pdo_mysql \
      php5-pear"

ARG PHP_GNUPG_DEPS="php5-dev \
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

RUN apk add --no-cache $BASE_PHP_DEPS \
      sed \
      coreutils \
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

RUN apk add --no-cache $PHP_GNUPG_DEPS  \
    && ln -s /usr/bin/php5 /usr/bin/php \
    && ln -s /usr/bin/phpize5 /usr/bin/phpize \
    && sed -i "s/ -n / /" $(which pecl) \
    && pecl install gnupg \
    && pecl install redis \
    && echo "extension=gnupg.so" > /etc/php5/conf.d/gnupg.ini \
    && echo "extension=redis.so" > /etc/php5/conf.d/redis.ini \
    && apk del $PHP_GNUPG_DEPS \
    && curl -sS https://getcomposer.org/installer | php \
    && mv composer.phar /usr/local/bin/composer \
    && mkdir /var/www/passbolt \
    && curl -sSL $PASSBOLT_URL | tar zxf - -C /var/www/passbolt --strip-components 1 \
    && chown -R nginx:nginx /var/www/passbolt \
    && chmod -R +w /var/www/passbolt/app/tmp \
    && chmod +w /var/www/passbolt/app/webroot/img/public

COPY conf/passbolt.conf /etc/nginx/conf.d/default.conf
COPY bin/docker-entrypoint.sh /docker-entrypoint.sh

EXPOSE 80 443

CMD ["/docker-entrypoint.sh"]
