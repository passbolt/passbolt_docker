# PASSBOLT DEBIAN DOCKER CONTAINER

ERRATUM : THIS IS A DEMO CONTAINER. DO NOT USE IT IN PRODUCTION.

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

Enter the values corresponding to your settings. The most important setting is PASSBOLT_DIR. You can keep the default values for the rest.

3) Finally, you can build and run the container :
```
	cd /path/to/docker/files
	docker build -t passbolt_debian .
	./launch-container.sh
```

Behavior
--------
By default the container will create a new database and use it to install passbolt.
However, in case an external database is provided in the settings, it will try to use it.
A few consideration :
- There should be a valid username, password and database on the mysql server.
- If the database exists but without passbolt installed, then passbolt will be installed normally.
- If the database exists and already has a passbolt installed, then no db installation will be done and the existing data will be kept.


