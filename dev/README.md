```
       ____                  __          ____          .-.
      / __ \____  _____ ____/ /_  ____  / / /_    .--./ /      _.---.,
     / /_/ / __ `/ ___/ ___/ __ \/ __ \/ / __/     '-,  (__..-`       \
    / ____/ /_/ (__  |__  ) /_/ / /_/ / / /_          \                |
   /_/    \__,_/____/____/_,___/\____/_/\__/           `,.__.   ^___.-/
                                                         `-./ .'...--`
  The open source password manager for teams                `'
  (c) 2023 Passbolt SA
  https://www.passbolt.com
```

# Setting up a working development environment using docker

Please note that these instructions are for setting a functional development environment only. Refer to the [installation guide](https://help.passbolt.com/hosting/install) if you want to use Passbolt securely to share passwords with your team.

## Prerequisites
  - [Git](https://git-scm.com/)
  - [Docker](https://docs.docker.com/get-docker/) v20 or newer

## Preparing Local Environment

1. Clone the repos
Fork the Passbolt API repository. Please read [Fork a repo](https://docs.github.com/en/get-started/quickstart/fork-a-repo?tool=webui) if you've never done this before.

Clone the forked repository onto your local machine:
```bash
git clone git@github.com:<YOUR_FORK_HERE>/passbolt_api.git
```

In addition to the Passbolt API repository, you'll also require the [passbolt_docker](https://github.com/passbolt/passbolt_docker) repository to spin up the stack using docker compose.
```bash
git clone https://github.com/passbolt/passbolt_docker.git
```

2. Copy the initial app.php into a new one for passbolt_api (the new file will be used by the passbolt server)
```
cd passbolt_api
cp config/app.default.php config/app.php
```

3. Run composer install to update all the dependencies. A new vendor directory will be created with all the required libraries
```
cd passbolt_api
docker run --rm --interactive --tty --volume $PWD:/app composer install --ignore-platform-reqs
```

4. Map the passbolt.local to the localhost in the /etc/hosts
```
127.0.0.1   passbolt.local
```

5. Copy the .env.example file into .env and replace the PATH_TO_PASSBOLT_API variable with the path to the passbolt_api repository on your machine

6. Spin-up the docker-compose containers (mariadb and passbolt server)
```
cd passbolt_docker
docker-compose -f dev/docker-compose-dev.yml up -d
```

7. Create the first user (the administrator) by replacing the below command with your own data. More details [here](https://help.passbolt.com/hosting/install/ce/docker).
```
cd passbolt_docker
docker-compose -f dev/docker-compose-ce.yaml exec passbolt /bin/bash -c \
  'su -m -c "/var/www/passbolt/bin/cake passbolt register_user -u myuser@passbolt.local \
   -f name  -l lastname  -r admin" -s /bin/sh www-data'
```

8. Copy-paste the output in the browser and you are ready!
