Describe "secret_file_to_path function"
  # Mocks
  function mkdir() {
    echo > /dev/null
  }
  function ln() {
    echo "ln $1 $2 $3"
  }

  function environment() {
    echo "public.key" > /tmp/public.key
    export PASSBOLT_GPG_SERVER_KEY_PUBLIC_FILE="/tmp/public.key"
  }
  Before "environment"
  Include "./entrypoint/passbolt/env.sh"
  It "should create the symlink to the secret file"
    
    When call secret_file_to_path 'PASSBOLT_GPG_SERVER_KEY_PUBLIC_FILE' '/etc/passbolt/gpg/serverkey.asc'
    The status should be success
    The output should include "ln -s /tmp/public.key /etc/passbolt/gpg/serverkey.asc"
  End

  function environment() {
    export PASSBOLT_GPG_SERVER_KEY_PUBLIC_FILE="NOT_A_FILE"
  }
  Before "environment"
  Include "./entrypoint/passbolt/env.sh"
  It "should NOT create the symlink to the secret file"
    When call secret_file_to_path 'PASSBOLT_GPG_SERVER_KEY_PUBLIC_FILE' '/etc/passbolt/gpg/serverkey.asc'
    The status should be success
    The output should be blank
  End
End

Describe "env_from_file function"

  function environment() {
    cat << EOF >> /tmp/passbolt-username
    passbolt-username
EOF
    export DATASOURCES_DEFAULT_USERNAME_FILE="/tmp/passbolt-username"
    export DATASOURCES_DEFAULT_USERNAME="username"
  }
  function cleanup() {
    unset DATASOURCES_DEFAULT_USERNAME_FILE="/tmp/passbolt-username"
    unset DATASOURCES_DEFAULT_USERNAME="username"
    rm -f /tmp/passbolt-*
  }
  Before "environment"
  After "cleanup"
  Include "./entrypoint/passbolt/env.sh"
  It "should return an error because none of the variables are empty"
    When call env_from_file 'DATASOURCES_DEFAULT_USERNAME' 
    The status should be failure
    The output should include "Error: Both DATASOURCES_DEFAULT_USERNAME and DATASOURCES_DEFAULT_USERNAME_FILE are set (but are exclusive)"
  End

  function cleanup() {
    unset DATASOURCES_DEFAULT_PASSWORD_FILE
    rm -f /tmp/passbolt-*
  }
  function environment() {
    echo "passbolt-password" > /tmp/passbolt-password
    export DATASOURCES_DEFAULT_PASSWORD_FILE="/tmp/passbolt-password"
  }

  Before "environment"
  Include "./entrypoint/passbolt/env.sh"
  After "cleanup"
  It "should set the right value on the DATASOURCES_DEFAULT_PASSWORD variable using the DATASOURCES_DEFAULT_PASSWORD_FILE variable"
    function check() {
      env_from_file 'DATASOURCES_DEFAULT_PASSWORD'
      if [ "$DATASOURCES_DEFAULT_PASSWORD" != "passbolt-password" ]; then
        echo "Error: DATASOURCES_DEFAULT_PASSWORD is not set to the expected value"
        return 1
      fi
    }
    When call check
    The status should be success
  End

  function environment() {
    export DATASOURCES_DEFAULT_PASSWORD="password"
  }
  Before "environment"
  Include "./entrypoint/passbolt/env.sh"
  It "should not change the DATASOURCES_DEFAULT_PASSWORD if it is set"
    function check() {
      env_from_file 'DATASOURCES_DEFAULT_PASSWORD'
      if [ "$DATASOURCES_DEFAULT_PASSWORD" != "password" ]; then
        echo "Error: DATASOURCES_DEFAULT_PASSWORD is not set to the expected value"
        return 1
      fi
    }
    When call check
    The status should be success
  End

End
