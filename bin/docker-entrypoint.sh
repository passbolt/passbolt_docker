#!/bin/bash

set -eo pipefail

gpg_private_key=/var/www/passbolt/config/gpg/serverkey.private.asc
gpg_public_key=/var/www/passbolt/config/gpg/serverkey.asc

app_config='/var/www/passbolt/app/Config/app.php'
ssl_key='/etc/ssl/certs/certificate.key'
ssl_cert='/etc/ssl/certs/certificate.crt'

gpg_gen_key() {
  gpg --batch --gen-key <<EOF
    Key-Type: 1
		Key-Length: ${KEY_LENGTH:-2048}
		Subkey-Type: 1
		Subkey-Length: ${SUBKEY_LENGTH:-2048}
		Name-Real: ${KEY_NAME:-Passbolt default user}
		Name-Email: ${KEY_EMAIL:-passbolt@yourdomain.com}
		Expire-Date: ${KEY_EXPIRATION:-0}
		%commit
EOF

  gpg --armor --export-secret-keys "$KEY_EMAIL" > "$gpg_private_key"
  gpg --armor --export "$KEY_EMAIL" > "$gpg_public_key"
  gpg_auto_fingerprint=$(gpg --fingerprint "$KEY_EMAIL" | grep fingerprint | awk '{for(i=4;i<=NF;++i)printf \$i}')
}

gpg_import_key() {
  local key_id=""
  key_id=$(su -m -c "gpg --with-colons $gpg_private_key | grep sec |cut -f5 -d:" -ls /bin/bash nginx)

  su -m -c "gpg --batch --import $gpg_public_key" -ls /bin/bash nginx
  su -m -c "gpg -K $key_id" -ls /bin/bash nginx || su -m -c "gpg --batch --import $gpg_private_key" -ls /bin/bash nginx
}

gen_ssl_cert() {
  openssl req -new -newkey rsa:4096 -days 365 -nodes -x509 \
    -subj "/C=FR/ST=Denial/L=Springfield/O=Dis/CN=www.passbolt.local" \
    -keyout $ssl_key -out $ssl_cert
}

install() {
  tables=$(mysql -u "$DATABASE_USER" -h "$DB_HOST" -P "$DB_HOST" -p -BN -e "SHOW TABLES FROM $DB_NAME" -p"$DB_PASS" |wc -l)

  if [ "$tables" -eq 0 ]; then
    su -c "/var/www/passbolt/app/Console/cake install --send-anonymous-statistics true --no-admin" -ls /bin/bash nginx
  else
    echo "Enjoy! ☮"
  fi
}

email_cron_job() {
  local root_crontab='/etc/crontabs/root'
  local cron_task_dir='/etc/periodic/1min'
  local cron_task='/etc/periodic/1min/email_queue_processing'
  local process_email="/var/www/passbolt/app/Console/cake EmailQueue.sender --quiet"

  mkdir -p $cron_task_dir

  echo "* * * * * run-parts $cron_task_dir" >> $root_crontab
  echo "#!/bin/sh" > $cron_task
  chmod +x $cron_task
  echo "su -c \"$process_email\" -ls /bin/bash nginx" >> $cron_task

  crond -f -c /etc/crontabs
}


if [ ! -f $gpg_private_key ] && [ ! -L $gpg_private_key ] || \
   [ ! -f $gpg_public_key ] && [ ! -L $gpg_public_key ]; then
  su -c "gpg --list-keys" -ls /bin/bash nginx
  gpg_gen_key
  gpg_import_key
else
  gpg_import_key
fi

if [ ! -f $app_config ] && [ ! -L $app_config ]; then
  app_setup
fi

if [ ! -f $ssl_key ] && [ ! -L $ssl_key ] && \
   [ ! -f $ssl_cert ] && [ ! -L $ssl_cert ]; then
  gen_ssl_cert
fi

install

email_cron_job
