```
       ____                  __          ____          .-.
      / __ \____  _____ ____/ /_  ____  / / /_    .--./ /      _.---.,
     / /_/ / __ `/ ___/ ___/ __ \/ __ \/ / __/     '-,  (__..-`       \
    / ____/ /_/ (__  |__  ) /_/ / /_/ / / /_          \                |
   /_/    \__,_/____/____/_,___/\____/_/\__/           `,.__.   ^___.-/
                                                         `-./ .'...--`
  The open source password manager for teams                `'
  (c) 2018 Passbolt SARL
  https://www.passbolt.com
```
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/0de4eaf7426944769a70a2d727a9012b)](https://www.codacy.com/app/passbolt/passbolt_docker?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=passbolt/passbolt_docker&amp;utm_campaign=Badge_Grade)
[![Docker Pulls](https://img.shields.io/docker/pulls/passbolt/passbolt.svg?style=flat-square)](https://hub.docker.com/r/passbolt/passbolt/tags/)
[![GitHub release](https://img.shields.io/github/release/passbolt/passbolt_docker.svg?style=flat-square)](https://github.com/passbolt/passbolt_docker/releases)
[![license](https://img.shields.io/github/license/passbolt/passbolt_docker.svg?style=flat-square)](https://github.com/passbolt/passbolt_docker/LICENSE)
[![Twitter Follow](https://img.shields.io/twitter/follow/passbolt.svg?style=social&label=Follow)](https://twitter.com/passbolt)

# If you are here for the distroless stack

First configure env/passbolt.env to match your desired domain name or other env variables you might need.

In order to run the distroless stack you need first to generate a gpg-key. This can be done like this:

```docker-compose run generate-key```

Once the process finishes it should output a GPG key fingerprint that you should add to the file
`env/passbolt.env`

Now you are good to go, launch the stack with:

```docker-compose up```

Register an admin user with the following command:

```
docker-compose run install-passbolt -c "/usr/share/php/passbolt/bin/cake passbolt register_user -u email_address -f name -l surname
-r admin"
```

The above command will output a one use link that you should paste on your browser to configure your browser extension

Where:

- email_address: should be your email address
- name: first name of the user
- surname: surname of the user

# Distroless compose caveats

Important the stack is for testing as you might expect there are some caveats:

- There is no cron job service so expect no emails
