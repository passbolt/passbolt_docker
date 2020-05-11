#!/usr/bin/env bash
cp -v /etc/letsencrypt/live/${CERTBOT_DOMAIN}/cert.pem /etc/ssl/certs/certificate.crt
cp -v /etc/letsencrypt/live/${CERTBOT_DOMAIN}/privkey.pem /etc/ssl/certs/certificate.key
/etc/init.d/nginx reload