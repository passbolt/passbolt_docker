# usage: file_env VAR [DEFAULT]
#    ie: file_env 'DATASOURCES_DEFAULT_USERNAME' 'example'
# (will allow for "$DATASOURCES_DEFAULT_USERNAME_FILE" to fill in the value of
#  "$DATASOURCES_DEFAULT_USERNAME" from a file, especially for Docker's secrets feature)
function env_from_file() {
  local var="$1"
  local fileVar="${var}_FILE"
  local default="${2:-}"
  if [ "${!var:-}" ] && [ "${!fileVar:-}" ]; then
    echo "Error: Both $var and $fileVar are set (but are exclusive)"
    return 1
  fi
  local val="$default"
  if [ "${!var:-}" ]; then
    val="${!var}"
  elif [ "${!fileVar:-}" ]; then
    val="$(<"${!fileVar}")"
  fi
  export "$var"="$val"
  unset "$fileVar"
}

function secret_file_to_path() {
  local secret_file="${!1}"
  local path="$2"
  if [ -f "$secret_file" ]; then
    mkdir -p "$(dirname "$path")"
    ln -s "$secret_file" "$path"
  fi
}

function manage_docker_env() {
  # Setup env variables from docker secrets
  env_from_file 'DATASOURCES_DEFAULT_PASSWORD'
  env_from_file 'DATASOURCES_DEFAULT_USERNAME'
  env_from_file 'DATASOURCES_DEFAULT_HOST'
  env_from_file 'DATASOURCES_DEFAULT_DATABASE'

  # Get docker secrets values if exist and set them on new paths
  secret_file_to_path 'PASSBOLT_GPG_SERVER_KEY_PUBLIC_FILE' "$gpg_public_key"
  secret_file_to_path 'PASSBOLT_GPG_SERVER_KEY_PRIVATE_FILE' "$gpg_private_key"
  secret_file_to_path 'PASSBOLT_SSL_SERVER_CERT_FILE' "$ssl_cert"
  secret_file_to_path 'PASSBOLT_SSL_SERVER_KEY_FILE' "$ssl_key"
}

function check_fullbase_url() {
  if [[ (-z "${APP_FULL_BASE_URL+xxx}") ]]; then
    local warning_color='\033[1;33m'
    local no_color='\033[0m'
    echo -e "${warning_color}WARNING:${no_color} APP_FULL_BASE_URL is not set. It can lead to host header injection attack"
    echo -e "${warning_color}WARNING:${no_color} More information: https://owasp.org/www-project-web-security-testing-guide/v42/4-Web_Application_Security_Testing/07-Input_Validation_Testing/17-Testing_for_Host_Header_Injection"
  fi
}
