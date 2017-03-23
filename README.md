# PASSBOLT DEBIAN DOCKER CONTAINER

ATTENTION : THIS IS A DEMO CONTAINER. DO NOT USE IT IN PRODUCTION.

How to use it
-------------
1) First, download passbolt source code from git.
```
	git clone https://github.com/passbolt/passbolt.git
```

2) Then, configure the container.
There is a configuration file located in /conf/conf.sh

It contains the following options :

- PASSBOLT_DIR : path to passbolt source code.
- MYSQL_HOST : mysql host. Keep it as 'localhost' to let the container handle the database.
- MYSQL_ROOT_PASSWORD : root password of mysql. It is only useful if MYSQL_HOST is set to localhost.
- MYSQL_USERNAME : valid username for the database.
- MYSQL_PASSWORD : valid password for the database.
- MYSQL_DATABASE : name of the database to be used.
- ADMIN_USERNAME : email of the admin user.
- ADMIN_FIRST_NAME : first name of the admin user.
- ADMIN_LAST_NAME : last name of the admin user.

Enter the values corresponding to your settings. The most important setting is PASSBOLT_DIR. You can keep the default values for the rest.

3) Generate the gpg server key.
```
	cd /path/to/docker/files
	./bin/generate_gpg_server_key.sh
```

4) (optional) Configure the smtp server.

In the PASSBOLT_DIR, edit the file app/Config/email.php.

If you don't configure a smtp server, emails notifications won't be sent. User won't be able to finalize their registration.

5) Finally, you can build and run the container.
```
	cd /path/to/docker/files
	docker build -t passbolt_debian .
	./launch-container.sh
```

6) To gain access to the application with the domain name passbolt.docker you must add an alias on your system targeting the passbolt container.

```
sudo sh -c 'echo "$(docker inspect --format='{{.NetworkSettings.Networks.bridge.IPAddress}}' passbolt) passbolt.docker" >> /etc/hosts'
```

7) Registration ending
If a smtp server has been configured you will receive a registration email at the email you defined in the conf.sh file.

If no smtp server has been configured, you can still finalize the registration process. Take a look at the end of the docker logs,
you will find the admin user registration link.
```
docker logs passbolt | awk '/The user has been registered with success/{print $0}'
```

Behavior
--------
By default the container will create a new database and use it to install passbolt.
However, in case an external database is provided in the settings, it will try to use it.
A few consideration :
- There should be a valid username, password and database on the mysql server.
- If the database exists but without passbolt installed, then passbolt will be installed normally.
- If the database exists and already has a passbolt installed, then no db installation will be done and the existing data will be kept.
