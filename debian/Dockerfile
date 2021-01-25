FROM debian:buster-slim

LABEL maintainer="Passbolt SA <contact@passbolt.com>"

ENV PASSBOLT_PKG_KEY=0xDE8B853FC155581D
ENV PHP_VERSION=7.3

ARG PASSBOLT_PKG=passbolt-ce-server
ARG PASSBOLT_REPO_URL="https://download.passbolt.com/ce/debian"
ARG PASSBOLT_DISTRO="buster"
ARG PASSBOLT_COMPONENT="stable"

RUN apt-get update \
    && DEBIAN_FRONTEND=non-interactive apt-get -y install \
      ca-certificates \
      gnupg \
    && apt-key adv --keyserver keys.gnupg.net --recv-keys $PASSBOLT_PKG_KEY \
    && echo "deb $PASSBOLT_REPO_URL $PASSBOLT_DISTRO $PASSBOLT_COMPONENT" > /etc/apt/sources.list.d/passbolt.list \
    && apt-get update \
    && DEBIAN_FRONTEND=non-interactive apt-get -y install --no-install-recommends \
      nginx \
      $PASSBOLT_PKG \
      supervisor \
    && rm /etc/nginx/sites-enabled/default \
    && mkdir /run/php \
    && cp /usr/share/passbolt/examples/nginx-passbolt-ssl.conf /etc/nginx/snippets/passbolt-ssl.conf \
    && sed -i 's,;clear_env = no,clear_env = no,' /etc/php/$PHP_VERSION/fpm/pool.d/www.conf \
    && sed -i 's,# include __PASSBOLT_SSL__,include /etc/nginx/snippets/passbolt-ssl.conf;,' /etc/nginx/sites-enabled/nginx-passbolt.conf \
    && sed -i 's,ssl on;,listen 443 ssl;,' /etc/nginx/snippets/passbolt-ssl.conf \
    && sed -i 's,__CERT_PATH__,/etc/ssl/certs/certificate.crt;,' /etc/nginx/snippets/passbolt-ssl.conf \
    && sed -i 's,__KEY_PATH__,/etc/ssl/certs/certificate.key;,' /etc/nginx/snippets/passbolt-ssl.conf

COPY conf/supervisor/*.conf /etc/supervisor/conf.d/
COPY bin/docker-entrypoint.sh /docker-entrypoint.sh
COPY scripts/wait-for.sh /usr/bin/wait-for.sh

EXPOSE 80 443

CMD ["/docker-entrypoint.sh"]
