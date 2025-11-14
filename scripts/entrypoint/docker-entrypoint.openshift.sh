#!/usr/bin/env bash

set -eo pipefail

passbolt_config="/etc/passbolt"
gpg_private_key="${PASSBOLT_GPG_SERVER_KEY_PRIVATE:-$passbolt_config/gpg/serverkey_private.asc}"
gpg_public_key="${PASSBOLT_GPG_SERVER_KEY_PUBLIC:-$passbolt_config/gpg/serverkey.asc}"

ssl_key='/etc/passbolt/certs/certificate.key'
ssl_cert='/etc/passbolt/certs/certificate.crt'

deprecation_message=""

subscription_key_file_paths=("/etc/passbolt/subscription_key.txt" "/etc/passbolt/license")

source $(dirname $0)/../passbolt/entrypoint-openshift.sh
source $(dirname $0)/../passbolt/env.sh
source $(dirname $0)/../passbolt/deprecated_paths.sh

manage_docker_env

check_deprecated_paths

if [ ! -f "$gpg_private_key" ] ||
  [ ! -f "$gpg_public_key" ]; then
  gpg_gen_key
  gpg_import_key
else
  gpg_import_key
fi

if [ ! -f "$ssl_key" ] && [ ! -L "$ssl_key" ] &&
  [ ! -f "$ssl_cert" ] && [ ! -L "$ssl_cert" ]; then
  gen_ssl_cert
fi

install

echo -e "$deprecation_message"

declare -p | grep -Ev 'BASHOPTS|BASH_VERSINFO|EUID|PPID|SHELLOPTS|UID' >/etc/environment

exec /usr/bin/supervisord -n
