FROM debian:buster-slim

LABEL maintainer="Passbolt SA <contact@passbolt.com>"

ENV PASSBOLT_PKG_KEY=0xDE8B853FC155581D
ENV PASSBOLT_PKG=passbolt-ce-server

RUN apt-get update \
    && DEBIAN_FRONTEND=non-interactive apt-get -y install \
      ca-certificates \
      gnupg \
    && apt-key adv --keyserver keys.gnupg.net --recv-keys $PASSBOLT_PKG_KEY \
    && echo "deb https://download.passbolt.com/ce/debian buster stable" > /etc/apt/sources.list.d/passbolt.list \
    && apt-get update \
    && DEBIAN_FRONTEND=non-interactive apt-get -y install --no-install-recommends \
      nginx \
      $PASSBOLT_PKG \
      supervisor


COPY conf/supervisor/*.conf /etc/supervisor/conf.d/
COPY bin/docker-entrypoint.sh /docker-entrypoint.sh
COPY scripts/wait-for.sh /usr/bin/wait-for.sh

EXPOSE 80 443

CMD ["/docker-entrypoint.sh"]
