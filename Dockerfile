FROM debian:buster-slim

LABEL maintainer="Passbolt SA <contact@passbolt.com>"

ENV PASSBOLT_PKG_KEY=0xDE8B853FC155581D
ENV PASSBOLT_PKG=passbolt-ce-server
ENV PHP_VERSION=7.3
ENV GNUPGHOME=/var/lib/passbolt/.gnupg


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
      supervisor \
      php-apcu

RUN sed -i 's,listen 80;,listen 8080;,' /etc/nginx/sites-enabled/nginx-passbolt.conf \
    && rm /etc/nginx/sites-enabled/default \
    && cp /usr/share/passbolt/examples/nginx-passbolt-ssl.conf /etc/nginx/snippets/passbolt-ssl.conf \
    && sed -i 's,;clear_env = no,clear_env = no,' /etc/php/$PHP_VERSION/fpm/pool.d/www.conf \
    && sed -i 's,# include __PASSBOLT_SSL__,include /etc/nginx/snippets/passbolt-ssl.conf;,' /etc/nginx/sites-enabled/nginx-passbolt.conf \
    && sed -i 's,ssl on;,listen 4443 ssl;,' /etc/nginx/snippets/passbolt-ssl.conf \
    && sed -i 's,__CERT_PATH__,/etc/passbolt/certs/certificate.crt;,' /etc/nginx/snippets/passbolt-ssl.conf \
    && sed -i 's,__KEY_PATH__,/etc/passbolt/certs/certificate.key;,' /etc/nginx/snippets/passbolt-ssl.conf \
    && sed -i '/user www-data;/d' /etc/nginx/nginx.conf \
    && sed -i 's,/run/nginx.pid,/tmp/nginx.pid,' /etc/nginx/nginx.conf \
    && sed -i "/^http {/a \    proxy_temp_path /tmp/proxy_temp;\n    client_body_temp_path /tmp/client_temp;\n    fastcgi_temp_path /tmp/fastcgi_temp;\n    uwsgi_temp_path /tmp/uwsgi_temp;\n    scgi_temp_path /tmp/scgi_temp;\n" /etc/nginx/nginx.conf \
    && sed -i 's,listen = /run/php/php7.3-fpm.sock,listen = 127.0.0.1:9000,' /etc/php/7.3/fpm/pool.d/www.conf \
    && sed -i 's,unix:/run/php/php7.3-fpm.sock,127.0.0.1:9000,' /etc/nginx/sites-enabled/nginx-passbolt.conf \
    && sed -i 's,pid = /run/php/php7.3-fpm.pid,pid = /tmp/php7.3-fpm.pid,' /etc/php/7.3/fpm/php-fpm.conf \
    && sed -i 's,/var/run/supervisor.sock,/tmp/supervisor.sock,' /etc/supervisor/supervisord.conf \
# nginx user must own the cache and etc directory to write cache and tweak the nginx config
    #&& chown -R www-data:0 /var/cache/nginx \
    #&& chmod -R g+w /var/cache/nginx \
    && chown -R www-data:0 /etc/nginx \
    && chmod -R g+w /etc/nginx \
    && mkdir /etc/passbolt/certs \
    && chown www-data:0 /etc/passbolt/certs \
    && chown www-data:0 /var/log/supervisor \
    && chown -R www-data:0 /var/log/nginx \
    && ln -sf /dev/stdout /var/log/nginx/passbolt-access.log \
    && ln -sf /dev/stderr /var/log/nginx/passbolt-error.log \
    && ln -sf /dev/stderr /var/log/passbolt/error.log \
    && ln -sf /dev/stderr /var/log/php7.3-fpm.log \
    && chown -R www-data:0 /var/log/supervisor \
    && touch /var/www/.profile \
    && chown www-data:www-data /var/www/.profile

COPY conf/supervisor/*.conf /etc/supervisor/conf.d/
COPY bin/docker-entrypoint.sh /docker-entrypoint.sh
COPY scripts/wait-for.sh /usr/bin/wait-for.sh

EXPOSE 8080 4443

USER www-data

CMD ["/docker-entrypoint.sh"]
