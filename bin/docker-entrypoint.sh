#!/usr/bin/env sh

set -eo pipefail

gpg_private_key="${PASSBOLT_GPG_SERVER_KEY_PRIVATE:-/var/www/passbolt/config/gpg/serverkey_private.asc}"
gpg_public_key="${PASSBOLT_GPG_SERVER_KEY_PUBLIC:-/var/www/passbolt/config/gpg/serverkey.asc}"

ssl_key='/etc/ssl/certs/certificate.key'
ssl_cert='/etc/ssl/certs/certificate.crt'

gpg_gen_key() {
  key_email="${PASSBOLT_KEY_EMAIL:-passbolt@yourdomain.com}"
  key_name="${PASSBOLT_KEY_NAME:-Passbolt default user}"
  key_length="${PASSBOLT_KEY_LENGTH:-2048}"
  subkey_length="${PASSBOLT_SUBKEY_LENGTH:-2048}"
  expiration="${PASSBOLT_KEY_EXPIRATION:-0}"

  su -m -c "gpg --batch --gen-key <<EOF
    Key-Type: 1
		Key-Length: $key_length
		Subkey-Type: 1
		Subkey-Length: $subkey_length
    Name-Real: $key_name
    Name-Email: $key_email
    Expire-Date: $expiration
		%commit
EOF" -ls /bin/sh www-data

  su -c "gpg --armor --export-secret-keys $key_email > $gpg_private_key" -ls /bin/sh www-data
  su -c "gpg --armor --export $key_email > $gpg_public_key" -ls /bin/sh www-data
}

gpg_import_key() {
  key_id=""
  key_id=$(su -m -c "gpg --with-colons $gpg_private_key | grep sec |cut -f5 -d:" -ls /bin/sh www-data)
  su -c "gpg --batch --import $gpg_public_key" -ls /bin/sh www-data
  su -c "gpg -K $key_id" -ls /bin/sh www-data || su -m -c "gpg --batch --import $gpg_private_key" -ls /bin/sh www-data
}

gen_ssl_cert() {
  openssl req -new -newkey rsa:4096 -days 365 -nodes -x509 \
    -subj '/C=FR/ST=Denial/L=Springfield/O=Dis/CN=www.passbolt.local' \
    -keyout $ssl_key -out $ssl_cert
}

install() {
  tables=""
  tables=$(mysql \
    -u "$DATASOURCES_DEFAULT_USERNAME" \
    -h "$DATASOURCES_DEFAULT_HOST" \
    -P "$DATASOURCES_DEFAULT_PORT" \
    -BN -e "SHOW TABLES FROM $DATASOURCES_DEFAULT_DATABASE" \
    -p"$DATASOURCES_DEFAULT_PASSWORD" |wc -l)

  if [ "$tables" -eq 0 ]; then
    su -c 'cp /var/www/passbolt/config/app.default.php /var/www/passbolt/config/app.php' -s /bin/sh www-data
    su -c '/var/www/passbolt/bin/cake passbolt install --no-admin --force' -s /bin/sh www-data
  else
    echo "Enjoy! ☮"
  fi
}

email_cron_job() {
  root_crontab='/etc/crontabs/root'
  cron_task_dir='/etc/periodic/1min'
  cron_task='/etc/periodic/1min/email_queue_processing'
  process_email="/var/www/passbolt/app/Console/cake EmailQueue.sender --quiet"

  mkdir -p $cron_task_dir
  echo "* * * * * run-parts $cron_task_dir" >> $root_crontab
  echo "#!/bin/sh" > $cron_task
  chmod +x $cron_task
  echo "su -c \"$process_email\" -s /bin/sh www-data" >> $cron_task
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

gpg_auto_fingerprint="$(su -c "gpg --with-fingerprint $gpg_public_key | grep fingerprint | awk '{for(i=4;i<=NF;++i)printf \$i}'" -ls /bin/sh www-data)"
export PASSBOLT_GPG_SERVER_KEY_FINGERPRINT=$gpg_auto_fingerprint
install
email_cron_job

/usr/bin/supervisord -n -c /etc/supervisord.conf
