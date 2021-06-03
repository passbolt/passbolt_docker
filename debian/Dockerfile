FROM debian:buster-slim

LABEL maintainer="Passbolt SA <contact@passbolt.com>"

ARG PASSBOLT_REPO_URL="https://download.passbolt.com/ce/debian"
ARG PASSBOLT_DISTRO="buster"
ARG PASSBOLT_COMPONENT="stable"
ARG PASSBOLT_PKG=passbolt-ce-server
ARG PASSBOLT_FLAVOUR="ce"
ARG PASSBOLT_SERVER_KEY="hkps://keys.mailvelope.com "

ENV PASSBOLT_PKG_KEY=0xDE8B853FC155581D
ENV PHP_VERSION=7.3
ENV GNUPGHOME=/var/lib/passbolt/.gnupg
ENV PASSBOLT_FLAVOUR=$PASSBOLT_FLAVOUR

RUN apt-get update \
    && DEBIAN_FRONTEND=non-interactive apt-get -y install \
      ca-certificates \
      gnupg \
    && apt-key adv --keyserver $PASSBOLT_SERVER_KEY --recv-keys $PASSBOLT_PKG_KEY \
    && echo "deb $PASSBOLT_REPO_URL $PASSBOLT_DISTRO $PASSBOLT_COMPONENT" > /etc/apt/sources.list.d/passbolt.list \
    && apt-get update \
    && DEBIAN_FRONTEND=non-interactive apt-get -y install --no-install-recommends \
      nginx \
      $PASSBOLT_PKG \
      supervisor \
      curl \
    && rm /etc/nginx/sites-enabled/default \
    && mkdir /run/php \
    && cp /usr/share/passbolt/examples/nginx-passbolt-ssl.conf /etc/nginx/snippets/passbolt-ssl.conf \
    && sed -i 's,;clear_env = no,clear_env = no,' /etc/php/$PHP_VERSION/fpm/pool.d/www.conf \
    && sed -i 's,# include __PASSBOLT_SSL__,include /etc/nginx/snippets/passbolt-ssl.conf;,' /etc/nginx/sites-enabled/nginx-passbolt.conf \
    && sed -i '/listen \[\:\:\]\:443 ssl http2;/a listen 443 ssl http2;' /etc/nginx/snippets/passbolt-ssl.conf \
    && sed -i 's,__CERT_PATH__,/etc/ssl/certs/certificate.crt;,' /etc/nginx/snippets/passbolt-ssl.conf \
    && sed -i 's,__KEY_PATH__,/etc/ssl/certs/certificate.key;,' /etc/nginx/snippets/passbolt-ssl.conf \
    && sed -i 's,www-data.*$,www-data exec /bin/bash -c ". /etc/environment \&\& $PASSBOLT_BASE_DIR/bin/cron",' /etc/cron.d/$PASSBOLT_PKG \
    && sed -i 's/# server_tokens/server_tokens/' /etc/nginx/nginx.conf \
    && ln -sf /dev/stdout /var/log/nginx/passbolt-access.log \
    && ln -sf /dev/stderr /var/log/nginx/passbolt-error.log \
    && ln -sf /dev/stderr /var/log/passbolt/error.log \
    && ln -sf /dev/stderr /var/log/php7.3-fpm.log \
    && crontab /etc/cron.d/$PASSBOLT_PKG

COPY conf/supervisor/cron.conf /etc/supervisor/conf.d/cron.conf
COPY conf/supervisor/nginx.conf /etc/supervisor/conf.d/nginx.conf
COPY conf/supervisor/php.conf /etc/supervisor/conf.d/php.conf
COPY debian/bin/docker-entrypoint.sh /docker-entrypoint.sh
COPY scripts/wait-for.sh /usr/bin/wait-for.sh

EXPOSE 80 443

WORKDIR /usr/share/php/passbolt

CMD ["/docker-entrypoint.sh"]
