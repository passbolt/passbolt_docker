#!/bin/bash

set -eo pipefail

gpg_private_key=/var/www/passbolt/app/Config/gpg/serverkey.private.asc
gpg_public_key=/var/www/passbolt/app/Config/gpg/serverkey.asc
gpg=$(which gpg)

core_config='/var/www/passbolt/app/Config/core.php'
db_config='/var/www/passbolt/app/Config/database.php'
app_config='/var/www/passbolt/app/Config/app.php'
email_config='/var/www/passbolt/app/Config/email.php'
ssl_key='/etc/ssl/certs/certificate.key'
ssl_cert='/etc/ssl/certs/certificate.crt'

gpg_gen_key() {
  su -c "$gpg --batch --gen-key <<EOF
    Key-Type: 1
		Key-Length: ${KEY_LENGTH:-2048}
		Subkey-Type: 1
		Subkey-Length: ${SUBKEY_LENGTH:-2048}
		Name-Real: ${KEY_NAME:-Passbolt default user}
		Name-Email: ${KEY_EMAIL:-passbolt@yourdomain.com}
		Expire-Date: ${KEY_EXPIRATION:-0}
    %no-protection
		%commit
EOF" -ls /bin/bash www-data

  su -c "$gpg --armor --export-secret-keys $KEY_EMAIL > $gpg_private_key" -ls /bin/bash www-data
  su -c "$gpg --armor --export $KEY_EMAIL > $gpg_public_key" -ls /bin/bash www-data
}

gpg_import_key() {

  local key_id=$(su -c "gpg --with-colons $gpg_private_key | grep sec |cut -f5 -d:" -ls /bin/bash www-data)

  su -c "$gpg --batch --import $gpg_public_key" -ls /bin/bash www-data
  su -c "gpg -K $key_id" -ls /bin/bash www-data || su -c "$gpg --batch --import $gpg_private_key" -ls /bin/bash www-data
}

core_setup() {
  #Env vars:
  # SALT
  # CIPHERSEED
  # URL

  local default_salt='DYhG93b0qyJfIxfs2guVoUubWwvniR2G0FgaC9mi'
  local default_seed='76859309657453542496749683645'
  local default_url='http://passbolt.local'

  cp $core_config{.default,}
  sed -i s:$default_salt:${SALT:-$default_salt}:g $core_config
  sed -i s:$default_seed:${CIPHERSEED:-$default_seed}:g $core_config
  sed -i "/example.com/ s:\/\/::" $core_config
  sed -i "s|http://example.com|${URL:-$default_url}|g" $core_config
}

db_setup() {
  #Env vars:
  # DB_HOST
  # DB_USER
  # DB_PASS
  # DB_NAME

  local default_host='localhost'
  local default_user='user'
  local default_pass='password'
  local default_db='database_name'

  cp $db_config{.default,}
  sed -i "/$default_host/a\ \t\t'port' => '${DB_PORT:-3306}'," $db_config
  sed -i s:$default_host:${DB_HOST:-db}:g $db_config
  sed -i s:$default_user:${DB_USER:-passbolt}:g $db_config
  sed -i s:$default_pass\',:${DB_PASS:-P4ssb0lt}\',:g $db_config
  sed -i s:$default_db:${DB_NAME:-passbolt}:g $db_config
}

app_setup() {
  #Env vars:
  # FINGERPRINT
  # REGISTRATION
  # SSL

  local default_home='/home/www-data/.gnupg'
  local default_public_key='unsecure.key'
  local default_private_key='unsecure_private.key'
  local default_fingerprint='2FC8945833C51946E937F9FED47B0811573EE67E'
  local auto_fingerprint=$(su -c "$gpg --fingerprint |grep fingerprint| awk '{for(i=4;i<=NF;++i)printf \$i}'" -ls /bin/bash www-data)

  cp $app_config{.default,}
  sed -i s:$default_public_key:serverkey.asc:g $app_config
  sed -i s:$default_private_key:serverkey.private.asc:g $app_config
  sed -i s:$default_fingerprint:${FINGERPRINT:-$auto_fingerprint}:g $app_config
  sed -i "/force/ s:true:${SSL:-true}:" $app_config
  sed -i "/'registration'/{n; s:false:${REGISTRATION:-false}:}" $app_config
}

email_setup() {
  #Env vars:
  # EMAIL_TRANSPORT
  # EMAIL_FROM
  # EMAIL_HOST
  # EMAIL_PORT
  # EMAIL_TIMEOUT
  # EMAIL_AUTH
  # EMAIL_USERNAME
  # EMAIL_PASSWORD
  # EMAIL_CLIENT
  # EMAIL_TLS

  local default_transport='Smtp'
  local default_from='contact@passbolt.com'
  local default_host='smtp.mandrillapp.com'
  local default_port='587'
  local default_timeout='30'
  local default_username="''"
  local default_password="''"
  local default_client=null

  cp $email_config{.default,}
  sed -i s:$default_transport:${EMAIL_TRANSPORT:-Smtp}:g $email_config
  sed -i s:$default_from:${EMAIL_FROM:-contact@mydomain.local}:g $email_config
  sed -i s:$default_host:${EMAIL_HOST:-localhost}:g $email_config
  sed -i s:$default_port:${EMAIL_PORT:-587}:g $email_config
  sed -i s:$default_timeout:${EMAIL_TIMEOUT:-30}:g $email_config
  if [ "$EMAIL_AUTH" = "false" ] ; then
	sed -i "0,/"$default_username"/s:"$default_username":null:" $email_config
	sed -i "0,/"$default_password"/s:"$default_password":null:" $email_config
  else
  	sed -i "0,/"$default_username"/s:"$default_username":'${EMAIL_USERNAME:-email_user}':" $email_config
	sed -i "0,/"$default_password"/s:"$default_password":'${EMAIL_PASSWORD:-email_password}':" $email_config
  fi
  if [ -n "$EMAIL_CLIENT" ] ; then
    sed -i "0,/'client'/s:'client' => $default_client:'client' => '$EMAIL_CLIENT':" $email_config
  fi
  sed -i "0,/tls/s:false:${EMAIL_TLS:-false}:" $email_config

}

gen_ssl_cert() {
  openssl req -new -newkey rsa:4096 -days 365 -nodes -x509 \
    -subj "/C=FR/ST=Denial/L=Springfield/O=Dis/CN=www.passbolt.local" \
    -keyout $ssl_key -out $ssl_cert
}

install() {
  local database_host=${DB_HOST:-$(cat $db_config | grep -m1 "'host'" | sed -r "s/\s*'host' => '(.*)',/\1/")}
  local database_port=${DB_PORT:-$(cat $db_config | grep -m1 "'port' => '\d" | sed -r "s/\s*'port' => '(.*)',/\1/")}
  local database_user=${DB_USER:-$(cat $db_config | grep -m1 "'login'" | sed -r "s/\s*'login' => '(.*)',/\1/")}
  local database_pass=${DB_PASS:-$(cat $db_config | grep -m1 "'password'" | sed -r "s/\s*'password' => '(.*)',/\1/")}
  local database_name=${DB_NAME:-$(cat $db_config | grep -m1 "'database'" | sed -r "s/\s*'database' => '(.*)',/\1/")}
  tables=$(mysql -u ${database_user:-passbolt} -h $database_host -P ${database_port:-3306} -p -BN -e "SHOW TABLES FROM ${database_name:-passbolt}" -p${database_pass:-P4ssb0lt} |wc -l)

  if [ $tables -eq 0 ]; then
    su -c "/var/www/passbolt/app/Console/cake install --send-anonymous-statistics true --no-admin" -ls /bin/bash www-data
  else
    echo "Enjoy! ☮"
  fi
}

email_cron_job() {
  cron_task='/etc/cron.d/passbolt_email'
  echo "* * * * * su -c \"/var/www/passbolt/app/Console/cake EmailQueue.sender\" -s /bin/bash www-data >> /var/log/cron.log 2>&1" >> $cron_task

  crontab /etc/cron.d/passbolt_email

  cron -f -l &
}

if [ ! -f $gpg_private_key ] && [ ! -L $gpg_private_key ] || \
   [ ! -f $gpg_public_key ] && [ ! -L $gpg_public_key ]; then
  gpg_gen_key
else
  gpg_import_key
fi

if [ ! -f $core_config ] && [ ! -L $core_config ]; then
  core_setup
fi

if [ ! -f $db_config ] && [ ! -L $db_config ]; then
  db_setup
fi

if [ ! -f $app_config ] && [ ! -L $app_config ]; then
  app_setup
fi

if [ ! -f $email_config ] && [ ! -L $email_config ]; then
  email_setup
fi

if [ ! -f $ssl_key ] && [ ! -L $ssl_key ] && \
   [ ! -f $ssl_cert ] && [ ! -L $ssl_cert ]; then
  gen_ssl_cert
fi

install

php-fpm &

email_cron_job

nginx -g "daemon off;"
