#!/usr/bin/env bash

set -euo pipefail

gpg_private_key="${PASSBOLT_GPG_SERVER_KEY_PRIVATE:-/var/www/passbolt/config/gpg/serverkey_private.asc}"
gpg_public_key="${PASSBOLT_GPG_SERVER_KEY_PUBLIC:-/var/www/passbolt/config/gpg/serverkey.asc}"

ssl_key='/etc/ssl/certs/certificate.key'
ssl_cert='/etc/ssl/certs/certificate.crt'

export GNUPGHOME="/home/www-data/.gnupg"

gpg_gen_key() {
  key_email="${PASSBOLT_KEY_EMAIL:-passbolt@yourdomain.com}"
  key_name="${PASSBOLT_KEY_NAME:-Passbolt default user}"
  key_length="${PASSBOLT_KEY_LENGTH:-2048}"
  subkey_length="${PASSBOLT_SUBKEY_LENGTH:-2048}"
  expiration="${PASSBOLT_KEY_EXPIRATION:-0}"

  su -m -c "gpg --batch --no-tty --gen-key <<EOF
    Key-Type: default
		Key-Length: $key_length
		Subkey-Type: default
		Subkey-Length: $subkey_length
    Name-Real: $key_name
    Name-Email: $key_email
    Expire-Date: $expiration
    %no-protection
		%commit
EOF" -ls /bin/bash www-data

  su -c "gpg --batch --yes --pinentry-mode loopback --quick-gen-key --passphrase '' $key_email" -ls /bin/bash www-data

  su -c "gpg --armor --export-secret-keys $key_email > $gpg_private_key" -ls /bin/bash www-data
  su -c "gpg --armor --export $key_email > $gpg_public_key" -ls /bin/bash www-data
}

gpg_import_key() {
  su -c "gpg --batch --import $gpg_public_key" -ls /bin/bash www-data
  su -c "gpg --batch --import $gpg_private_key" -ls /bin/bash www-data
}

gen_ssl_cert() {
  openssl req -new -newkey rsa:4096 -days 365 -nodes -x509 \
    -subj '/C=FR/ST=Denial/L=Springfield/O=Dis/CN=www.passbolt.local' \
    -keyout $ssl_key -out $ssl_cert
}

install() {
  tables=$(mysql \
    -u "${DATASOURCES_DEFAULT_USERNAME:-passbolt}" \
    -h "${DATASOURCES_DEFAULT_HOST:-localhost}" \
    -P "${DATASOURCES_DEFAULT_PORT:-3306}" \
    -BN -e "SHOW TABLES FROM ${DATASOURCES_DEFAULT_DATABASE:-passbolt}" \
    -p"${DATASOURCES_DEFAULT_PASSWORD:-P4ssb0lt}" |wc -l)
  app_config="/var/www/passbolt/config/app.php"

  if [ ! -f "$app_config" ]; then
    su -c 'cp /var/www/passbolt/config/app.default.php /var/www/passbolt/config/app.php' -s /bin/bash www-data
  fi

  if [ -z "${PASSBOLT_GPG_SERVER_KEY_FINGERPRINT+xxx}" ]; then
    gpg_auto_fingerprint="$(su -c "gpg --list-keys --with-colons ${PASSBOLT_KEY_EMAIL:-passbolt@yourdomain.com} |grep fpr |head -1| cut -f10 -d:" -ls /bin/bash www-data)"
    export PASSBOLT_GPG_SERVER_KEY_FINGERPRINT=$gpg_auto_fingerprint
  fi

  if [ "$tables" -eq 0 ]; then
    su -c '/var/www/passbolt/bin/cake passbolt install --no-admin --force' -s /bin/bash www-data
  else
    su -c '/var/www/passbolt/bin/cake migrations migrate' -s /bin/bash www-data
    echo "Enjoy! ☮"
  fi
}

email_cron_job() {
  printenv > /etc/environment
  sed -i 's/=\(.*\)/="\1"/g' /etc/environment
  cron_task='/etc/cron.d/passbolt_email'
  echo "* * * * * su -c \"source /etc/environment ; /var/www/passbolt/bin/cake EmailQueue.sender\" -s /bin/bash www-data >> /var/log/cron.log 2>&1" >> $cron_task

  crontab /etc/cron.d/passbolt_email
}

if [ ! -f "$gpg_private_key" ] && [ ! -L "$gpg_private_key" ] || \
   [ ! -f "$gpg_public_key" ] && [ ! -L "$gpg_public_key" ]; then
  gpg_gen_key
  gpg_import_key
else
  gpg_import_key
fi

if [ ! -f "$ssl_key" ] && [ ! -L "$ssl_key" ] && \
   [ ! -f "$ssl_cert" ] && [ ! -L "$ssl_cert" ]; then
  gen_ssl_cert
fi

install
email_cron_job

/usr/bin/supervisord -n
