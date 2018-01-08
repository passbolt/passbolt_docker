#!/bin/bash

set -eo pipefail

base_path='/var/www/passbolt'
gpg_private_key="$base_path/config/gpg/serverkey.private.asc"
gpg_public_key="$base_path/config/gpg/serverkey.asc"
app_config="$base_path/config/app.php"
ssl_key='/etc/ssl/certs/certificate.key'
ssl_cert='/etc/ssl/certs/certificate.crt'

gpg_gen_key() {
  local key_email="${KEY_EMAIL:-passbolt@yourdomain.com}"
  local key_name="${KEY_NAME:-Passbolt default user}"
  local key_length="${KEY_LENGTH:-4096}"
  local subkey_length="${SUBKEY_LENGTH:-4096}"
  local expiration="${KEY_EXPIRATION:-0}"

  su -m -c "gpg --batch --gen-key <<EOF
    Key-Type: 1
		Key-Length: $key_length
		Subkey-Type: 1
		Subkey-Length: $subkey_length
    Name-Real: $key_name
    Name-Email: $key_email
    Expire-Date: $expiration
		%commit
EOF" -ls /bin/bash nginx

  su -m -c "gpg --armor --export-secret-keys $key_email > $gpg_private_key" -ls /bin/bash nginx
  su -m -c "gpg --armor --export $key_email > $gpg_public_key" -ls /bin/bash nginx
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
  if [ ! -f $app_config ] && [ ! -L $app_config ]; then
    cp $base_path/config/app.default.php $app_config
  fi
  tables=$(mysql -u "$DATABASE_USER" -h "$DB_HOST" -P "$DB_PORT" -p -BN -e "SHOW TABLES FROM $DB_NAME" -p"$DB_PASS" |wc -l)
  if [ "$tables" -eq 0 ]; then
    su -c "/var/www/passbolt/bin/cake passbolt install --no-admin" -ls /bin/bash nginx
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
}

if [ ! -f $gpg_private_key ] && [ ! -L $gpg_private_key ] || \
   [ ! -f $gpg_public_key ] && [ ! -L $gpg_public_key ]; then
  gpg_gen_key
  gpg_import_key
else
  gpg_import_key
fi

if [ ! -f $ssl_key ] && [ ! -L $ssl_key ] && \
   [ ! -f $ssl_cert ] && [ ! -L $ssl_cert ]; then
  gen_ssl_cert
fi

#gpg_auto_fingerprint=$(gpg --fingerprint "$key_email" | grep fingerprint | awk '{for(i=4;i<=NF;++i)printf \$i}')
install
email_cron_job
/usr/bin/supervisord -n -c /etc/supervisord.conf
