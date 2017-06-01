#!/bin/bash

set -eo pipefail

gpg_private_key=/var/www/passbolt/app/Config/gpg/serverkey.private.asc
gpg_public_key=/var/www/passbolt/app/Config/gpg/serverkey.asc
gpg=$(which gpg)

core_config='/var/www/passbolt/app/Config/core.php'
db_config='/var/www/passbolt/app/Config/database.php'
app_config='/var/www/passbolt/app/Config/app.php'
ssl_key='/etc/ssl/certs/certificate.key'
ssl_cert='/etc/ssl/certs/certificate.crt'

gpg_gen_key() {
  # Need to increase entropy in order to generate the GPG key reliably
  find / > /dev/null&
  
  su -m -c "$gpg --batch --gen-key <<EOF
    Key-Type: 1
		Key-Length: ${key_length:-2048}
		Subkey-Type: 1
		Subkey-Length: ${subkey_length:-2048}
		Name-Real: ${key_name:-Passbolt default user}
		Name-Email: ${key_email:-passbolt@yourdomain.com}
		Expire-Date: ${key_expiration:-0}
		%commit
EOF" -ls /bin/bash nginx

  su -m -c "$gpg --armor --export-secret-keys $key_email > $gpg_private_key" -ls /bin/bash nginx
  su -m -c "$gpg --armor --export $key_email > $gpg_public_key" -ls /bin/bash nginx
}

gpg_import_key() {

  local key_id=$(su -m -c "gpg --with-colons $gpg_private_key | grep sec |cut -f5 -d:" -ls /bin/bash nginx)

  su -m -c "$gpg --batch --import $gpg_public_key" -ls /bin/bash nginx
  su -m -c "gpg -K $key_id" -ls /bin/bash nginx || su -m -c "$gpg --batch --import $gpg_private_key" -ls /bin/bash nginx
}

core_setup() {
  #Env vars:
  # salt
  # cipherseed
  # url

  local default_salt='DYhG93b0qyJfIxfs2guVoUubWwvniR2G0FgaC9mi'
  local default_seed='76859309657453542496749683645'
  local default_url='example.com'

  cp $core_config{.default,}
  sed -i s:$default_salt:${salt:-$default_salt}:g $core_config
  sed -i s:$default_seed:${cipherseed:-$default_seed}:g $core_config
  sed -i s:$default_url:${url:-$default_url}:g $core_config
}

db_setup() {
  #Env vars:
  # db_host
  # db_user
  # db_pass
  # db_name

  local default_host='localhost'
  local default_user='user'
  local default_pass='password'
  local default_db='database_name'

  cp $db_config{.default,}
  sed -i s:$default_host:${db_host:-db}:g $db_config
  sed -i s:$default_user:${db_user:-passbolt}:g $db_config
  sed -i s:$default_pass\',:${db_pass:-P4ssb0lt}\',:g $db_config
  sed -i s:$default_db:${db_name:-passbolt}:g $db_config
}

app_setup() {
  #Env vars:
  # fingerprint
  # registration
  # ssl

  local default_home='/home/www-data/.gnupg'
  local default_public_key='unsecure.key'
  local default_private_key='unsecure_private.key'
  local default_fingerprint='2FC8945833C51946E937F9FED47B0811573EE67E'
  local default_ssl='force'
  local default_registration='public'
  local gpg_home='/var/lib/nginx/.gnupg'
  local auto_fingerprint=$(su -m -c "$gpg --fingerprint |grep fingerprint| awk '{for(i=4;i<=NF;++i)printf \$i}'" -ls /bin/bash nginx)

  cp $app_config{.default,}
  sed -i s:$default_home:$gpg_home:g $app_config
  sed -i s:$default_public_key:serverkey.asc:g $app_config
  sed -i s:$default_private_key:serverkey.private.asc:g $app_config
  sed -i s:$default_fingerprint:${fingerprint:-$auto_fingerprint}:g $app_config
  sed -i "/force/ s:true:${ssl:-true}:" $app_config
}

gen_ssl_cert() {
  openssl req -new -newkey rsa:4096 -days 365 -nodes -x509 \
    -subj "/C=FR/ST=Denial/L=Springfield/O=Dis/CN=www.example.com" \
    -keyout $ssl_key -out $ssl_cert
}

install() {
  local database=${db_host:-$(grep -m1 -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' $db_config)}
  tables=$(mysql -u ${db_user:-passbolt} -h $database -p -BN -e "SHOW TABLES FROM passbolt" -p${db_pass:-P4ssb0lt} |wc -l)

  if [ $tables -eq 0 ]; then
    su -c "/var/www/passbolt/app/Console/cake install --send-anonymous-statistics true --no-admin" -ls /bin/bash nginx
  else
    echo "Enjoy! ☮"
  fi
}

php_fpm_setup() {
  sed -i '/^user\s/ s:nobody:nginx:g' /etc/php5/php-fpm.conf
  sed -i '/^group\s/ s:nobody:nginx:g' /etc/php5/php-fpm.conf
  cp /etc/php5/php-fpm.conf /etc/php5/fpm.d/www.conf
  sed -i '/^include\s/ s:^:#:' /etc/php5/fpm.d/www.conf
}

if [ ! -f $gpg_private_key ] || [ ! -f $gpg_public_key ]; then
  gpg_gen_key
else
  gpg_import_key
fi

if [ ! -f $core_config ]; then
  core_setup
fi

if [ ! -f $db_config ]; then
  db_setup
fi

if [ ! -f $app_config ]; then
  app_setup
fi

if [ ! -f $ssl_key ] && [ ! -f $ssl_cert ]; then
  gen_ssl_cert
fi

php_fpm_setup

install

php-fpm5
nginx -g "pid /tmp/nginx.pid; daemon off;"
