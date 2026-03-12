function gpg_gen_key() {
  key_email="${PASSBOLT_KEY_EMAIL:-passbolt@yourdomain.com}"
  key_name="${PASSBOLT_KEY_NAME:-Passbolt default user}"
  key_length="${PASSBOLT_KEY_LENGTH:-3072}"
  subkey_length="${PASSBOLT_SUBKEY_LENGTH:-3072}"
  expiration="${PASSBOLT_KEY_EXPIRATION:-0}"

  su -c "gpg --homedir $GNUPGHOME --batch --no-tty --gen-key <<EOF
    Key-Type: RSA
		Key-Length: $key_length
		Key-Usage: sign,cert
		Subkey-Type: RSA
		Subkey-Length: $subkey_length
		Subkey-Usage: encrypt
    Name-Real: $key_name
    Name-Email: $key_email
    Expire-Date: $expiration
    %no-protection
		%commit
EOF" -ls /bin/bash www-data

  su -c "gpg --homedir $GNUPGHOME --armor --export-secret-keys $key_email > $gpg_private_key" -ls /bin/bash www-data
  su -c "gpg --homedir $GNUPGHOME --armor --export $key_email > $gpg_public_key" -ls /bin/bash www-data
}

function gpg_import_key() {
  su -c "gpg --homedir $GNUPGHOME --batch --import $gpg_public_key" -ls /bin/bash www-data
  su -c "gpg --homedir $GNUPGHOME --batch --import $gpg_private_key" -ls /bin/bash www-data
}

function gen_ssl_cert() {
  openssl req -new -newkey rsa:4096 -days 365 -nodes -x509 \
    -subj '/C=FR/ST=Denial/L=Springfield/O=Dis/CN=www.passbolt.local' \
    -addext "subjectAltName = DNS:www.passbolt.local" \
    -keyout "$ssl_key" -out "$ssl_cert"
}

function get_subscription_file() {
  # Look for subscription key on possible paths
  for path in "${subscription_key_file_paths[@]}"; do
    if [ -f "${path}" ]; then
      SUBSCRIPTION_FILE="${path}"
      return 0
    fi
  done

  return 1
}

function import_subscription() {
  if [ "${PASSBOLT_FLAVOUR}" == 'ce' ]; then
    return
  fi

  if get_subscription_file; then
    echo "Subscription file found: ${SUBSCRIPTION_FILE}"
    su -c "/usr/share/php/passbolt/bin/cake passbolt subscription_import --file ${SUBSCRIPTION_FILE}" -s /bin/bash www-data
  elif [ -n "${SUBSCRIPTION_KEY}" ]; then
    echo "Using SUBSCRIPTION_KEY environment variable"
    echo "${SUBSCRIPTION_KEY}" > "${subscription_key_file_paths[0]}"
    chown www-data:www-data ${subscription_key_file_paths[0]}
    chmod 640 ${subscription_key_file_paths[0]}
    echo "Subscription key file created at ${subscription_key_file_paths[0]}"
    su -c "/usr/share/php/passbolt/bin/cake passbolt subscription_import --file \"${subscription_key_file_paths[0]}\"" -s /bin/bash www-data
  fi
}


function install_command() {
  echo "Installing passbolt"
  su -c '/usr/share/php/passbolt/bin/cake passbolt install --no-admin' -s /bin/bash www-data
}

function clear_cake_cache_engines() {
  echo "Clearing cake caches"
  for engine in "${@}"; do
    su -c "/usr/share/php/passbolt/bin/cake cache clear _cake_${engine}_" -s /bin/bash www-data
  done
}

function migrate_command() {
  echo "Running migrations"
  su -c '/usr/share/php/passbolt/bin/cake passbolt migrate --no-clear-cache' -s /bin/bash www-data
  clear_cake_cache_engines model core translations
}

function jwt_keys_creation() {
  if [[ "${PASSBOLT_PLUGINS_JWT_AUTHENTICATION_ENABLED}" == "false" ]]; then
    return 0
  fi

  if [[ ! -f "$passbolt_config/jwt/jwt.key" || ! -f "$passbolt_config/jwt/jwt.pem" ]]; then
    mkdir -p "$passbolt_config/jwt"
    chmod 770 "$passbolt_config/jwt"
    chown www-data:www-data "$passbolt_config/jwt"
    su -c '/usr/share/php/passbolt/bin/cake passbolt create_jwt_keys' -s /bin/bash www-data
  fi
}

function jwt_keys_permissions_adjustments() {
  if [[ "${PASSBOLT_PLUGINS_JWT_AUTHENTICATION_ENABLED}" == "false" ]]; then
    return 0
  fi

  if [[ -f "$passbolt_config/jwt/jwt.key" && -f "$passbolt_config/jwt/jwt.pem" ]]; then
    chmod 640 "$passbolt_config/jwt/jwt.key"
    chmod 640 "$passbolt_config/jwt/jwt.pem"
    chmod 750 "$passbolt_config/jwt"
    chown -Rf root:www-data "$passbolt_config/jwt"
  fi
}

function install() {
  if [ ! -f "$passbolt_config/app.php" ]; then
    su -c "cp $passbolt_config/app.default.php $passbolt_config/app.php" -s /bin/bash www-data
  fi

  if [[ ("${PASSBOLT_GPG_SERVER_KEY_FINGERPRINT_FORCE}" == "true") ||
    (-z "${PASSBOLT_GPG_SERVER_KEY_FINGERPRINT+xxx}" &&
    ! -f "$passbolt_config/passbolt.php") ]]; then
    gpg_auto_fingerprint="$(su -c "gpg --homedir $GNUPGHOME --list-keys --with-colons ${PASSBOLT_KEY_EMAIL:-passbolt@yourdomain.com} |grep fpr |head -1| cut -f10 -d:" -ls /bin/bash www-data)"
    export PASSBOLT_GPG_SERVER_KEY_FINGERPRINT=$gpg_auto_fingerprint
  fi

  import_subscription || true
  jwt_keys_creation
  jwt_keys_permissions_adjustments || echo "[WARN] An attempt to adjust the JWT keypair permission failed. This may be expected if you mount your own keypair as read-only and may be fine as long as the Passbolt server can read said keypair. You can use the health-check command to find any issue with your instance: https://www.passbolt.com/docs/hosting/troubleshooting/logs/#api" >&2
  install_command || migrate_command && echo "Enjoy! ☮"
  check_fullbase_url
}
