function gpg_gen_key() {
  key_email="${PASSBOLT_KEY_EMAIL:-passbolt@yourdomain.com}"
  key_name="${PASSBOLT_KEY_NAME:-Passbolt default user}"
  key_length="${PASSBOLT_KEY_LENGTH:-3072}"
  subkey_length="${PASSBOLT_SUBKEY_LENGTH:-3072}"
  expiration="${PASSBOLT_KEY_EXPIRATION:-0}"

  entropy_check

  gpg --homedir "$GNUPGHOME" --batch --no-tty --gen-key <<EOF
    Key-Type: default
		Key-Length: $key_length
		Subkey-Type: default
		Subkey-Length: $subkey_length
    Name-Real: $key_name
    Name-Email: $key_email
    Expire-Date: $expiration
    %no-protection
		%commit
EOF

  gpg --homedir "$GNUPGHOME" --armor --export-secret-keys "$key_email" > "$gpg_private_key"
  gpg --homedir "$GNUPGHOME" --armor --export "$key_email" > "$gpg_public_key"
}

function gpg_import_key() {
  gpg --homedir "$GNUPGHOME" --batch --import "$gpg_public_key"
  gpg --homedir "$GNUPGHOME" --batch --import "$gpg_private_key"
}

function gen_ssl_cert() {
  openssl req -new -newkey rsa:4096 -days 365 -nodes -x509 \
    -subj '/C=FR/ST=Denial/L=Springfield/O=Dis/CN=www.passbolt.local' \
    -addext "subjectAltName = DNS:www.passbolt.local" \
    -keyout "$ssl_key" -out "$ssl_cert"
}

function get_subscription_file() {
  if [ "${PASSBOLT_FLAVOUR}" == 'ce' ]; then
    return 1
  fi
  
  # Look for subscription key on possible paths
  for path in "${subscription_key_file_paths[@]}";
  do
    if [ -f "${path}" ]; then
      SUBSCRIPTION_FILE="${path}"
      return 0
    fi
  done

  return 1
}

function import_subscription() {
  if get_subscription_file; then
    echo "Subscription file found: $SUBSCRIPTION_FILE"
    /usr/share/php/passbolt/bin/cake passbolt subscription_import --file "$SUBSCRIPTION_FILE"
  fi
}

function install_command() {
  echo "Installing passbolt"
  /usr/share/php/passbolt/bin/cake passbolt install --no-admin
}

function clear_cake_cache_engines() {
  echo "Clearing cake caches"
  for engine in "${@}";
  do
    /usr/share/php/passbolt/bin/cake cache clear "_cake_${engine}_"
  done
}

function migrate_command() {
  echo "Running migrations"
  /usr/share/php/passbolt/bin/cake passbolt migrate --no-clear-cache
  clear_cake_cache_engines model core
}

function jwt_keys_creation() {
  if [[ $PASSBOLT_PLUGINS_JWT_AUTHENTICATION_ENABLED == "true" && ( ! -f $passbolt_config/jwt/jwt.key || ! -f $passbolt_config/jwt/jwt.pem ) ]]
  then 
    /usr/share/php/passbolt/bin/cake passbolt create_jwt_keys
    chmod 640 "$passbolt_config/jwt/jwt.key" && chown www-data:www-data "$passbolt_config/jwt/jwt.key" 
    chmod 640 "$passbolt_config/jwt/jwt.pem" && chown www-data:www-data "$passbolt_config/jwt/jwt.pem" 
  fi 
}

function install() {
  if [ ! -f "$passbolt_config/app.php" ]; then
    cp $passbolt_config/app.default.php $passbolt_config/app.php
  fi

  if [ -z "${PASSBOLT_GPG_SERVER_KEY_FINGERPRINT+xxx}" ] && [ ! -f  "$passbolt_config/passbolt.php" ]; then
    gpg_auto_fingerprint="$(gpg --homedir "$GNUPGHOME" --list-keys --with-colons ${PASSBOLT_KEY_EMAIL:-passbolt@yourdomain.com} |grep fpr |head -1| cut -f10 -d:)"
    export PASSBOLT_GPG_SERVER_KEY_FINGERPRINT=$gpg_auto_fingerprint
  fi

  import_subscription || true

  jwt_keys_creation
  install_command || migrate_command && echo "Enjoy! ☮"
}
