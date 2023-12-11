FROM debian:bookworm-slim

LABEL maintainer="Passbolt SA <contact@passbolt.com>"

ARG PASSBOLT_DISTRO="buster"
ARG PASSBOLT_COMPONENT="stable"
ARG PASSBOLT_FLAVOUR="ce"
ARG PASSBOLT_SERVER_KEY="https://download.passbolt.com"
ARG PASSBOLT_REPO_URL="https://download.passbolt.com"
ARG PASSBOLT_REPO_KEY_PATH="/usr/share/keyrings/passbolt-repository.gpg"
ARG PASSBOLT_SOURCES_LIST_PATH="/etc/apt/sources.list.d/passbolt.sources"

ENV PASSBOLT_PKG_KEY=0xDE8B853FC155581D
ENV PHP_VERSION=8.2
ENV GNUPGHOME=/var/lib/passbolt/.gnupg
ENV PASSBOLT_FLAVOUR=$PASSBOLT_FLAVOUR
ENV PASSBOLT_PKG="passbolt-$PASSBOLT_FLAVOUR-server"

SHELL ["/bin/bash", "-o", "pipefail", "-c"]
RUN apt-get update \
  && DEBIAN_FRONTEND=non-interactive apt-get -y install \
  ca-certificates \
  gnupg \
  curl \
  && curl -s $PASSBOLT_SERVER_KEY/pub.key |\
  gpg --dearmor | tee $PASSBOLT_REPO_KEY_PATH > /dev/null \
  && chmod 644 $PASSBOLT_REPO_KEY_PATH \
  && echo -e "Types: deb \nURIs: $PASSBOLT_REPO_URL/$PASSBOLT_FLAVOUR/debian \nSuites: $PASSBOLT_DISTRO \nComponents: $PASSBOLT_COMPONENT \nSigned-By: $PASSBOLT_REPO_KEY_PATH" > $PASSBOLT_SOURCES_LIST_PATH \
  && apt-get update \
  && DEBIAN_FRONTEND=non-interactive apt-get -y install --no-install-recommends \
  nginx \
  $PASSBOLT_PKG \
  supervisor \
  && rm -f /etc/passbolt/jwt/* \
  && rm /etc/nginx/sites-enabled/default \
  && cp /usr/share/passbolt/examples/nginx-passbolt-ssl.conf /etc/nginx/snippets/passbolt-ssl.conf \
  && sed -i 's,;clear_env = no,clear_env = no,' /etc/php/$PHP_VERSION/fpm/pool.d/www.conf \
  && sed -i 's,# include __PASSBOLT_SSL__,include /etc/nginx/snippets/passbolt-ssl.conf;,' /etc/nginx/sites-enabled/nginx-passbolt.conf \
  && sed -i '/listen \[\:\:\]\:443 ssl http2;/a listen 443 ssl http2;' /etc/nginx/snippets/passbolt-ssl.conf \
  && sed -i 's,__CERT_PATH__,/etc/ssl/certs/certificate.crt;,' /etc/nginx/snippets/passbolt-ssl.conf \
  && sed -i 's,__KEY_PATH__,/etc/ssl/certs/certificate.key;,' /etc/nginx/snippets/passbolt-ssl.conf \
  && sed -i 's,www-data.*$,root su -s /bin/bash -c ". /etc/environment \&\& $PASSBOLT_BASE_DIR/bin/cron" www-data >/proc/1/fd/1 2>\&1,' /etc/cron.d/$PASSBOLT_PKG \
  && sed -i 's/# server_tokens/server_tokens/' /etc/nginx/nginx.conf \
  && ln -sf /dev/stdout /var/log/nginx/passbolt-access.log \
  && ln -sf /dev/stderr /var/log/nginx/passbolt-error.log \
  && ln -sf /dev/stderr /var/log/passbolt/error.log \
  && ln -sf /dev/stderr /var/log/php$PHP_VERSION-fpm.log

COPY conf/supervisor/cron.conf /etc/supervisor/conf.d/cron.conf
COPY conf/supervisor/nginx.conf /etc/supervisor/conf.d/nginx.conf
COPY conf/supervisor/php.conf /etc/supervisor/conf.d/php.conf
COPY scripts/entrypoint/docker-entrypoint.sh /docker-entrypoint.sh
COPY scripts/entrypoint/passbolt/entrypoint.sh /passbolt/entrypoint.sh
COPY scripts/entrypoint/passbolt/env.sh /passbolt/env.sh
COPY scripts/entrypoint/passbolt/deprecated_paths.sh /passbolt/deprecated_paths.sh
COPY scripts/entrypoint/passbolt/entropy.sh /passbolt/entropy.sh
COPY scripts/wait-for.sh /usr/bin/wait-for.sh

# Docker API does not support buildkit so we
# need to do this workaround https://github.com/docker/for-linux/issues/1136
RUN chmod 0644 /etc/supervisor/conf.d/* \
  && chmod 0700 /docker-entrypoint.sh \
  && chmod 0700 /passbolt/* \
  && chmod 0700 /usr/bin/wait-for.sh

EXPOSE 80 443

WORKDIR /usr/share/php/passbolt

CMD ["/docker-entrypoint.sh"]
