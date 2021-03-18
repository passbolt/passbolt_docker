# Change Log
All notable changes to this project will be documented in this file.
This project adheres to [Semantic Versioning](http://semver.org/).

## [Unreleased](https://github.com/passbolt/passbolt_docker/compare/v3.1.0...HEAD)

## [3.1.0](https://github.com/passbolt/passbolt_docker/compare/v3.0.2...v3.1.0) - 2021-03-18

- This is a sync release. Check [changes](https://github.com/passbolt/passbolt_api/compare/v3.0.2...v3.1.0) for passbolt_api
- Fix dev Dockerfile ln command syntax

## [3.0.2](https://github.com/passbolt/passbolt_docker/compare/v3.0.1...v3.0.2) - 2021-03-12

- This is a sync release. Check [changes](https://github.com/passbolt/passbolt_api/compare/v3.0.1...v3.0.2) for passbolt_api

## [3.0.1](https://github.com/passbolt/passbolt_docker/compare/v3.0.0...v3.0.1) - 2021-02-24

- This is a sync release. Check [changes](https://github.com/passbolt/passbolt_api/compare/v3.0.0...v3.0.1) for passbolt_api

## [3.0.0](https://github.com/passbolt/passbolt_docker/compare/v2.13.5...v3.0.0) - 2021-02-23

We are happy to announce the release of passbolt docker 3.0.0!

This release contains passbolt-api 3.0.0 as well as some new additions and deprection
notices.

Passbolt docker images now rely on passbolt's debian package. As a result the dockerfiles
are now using debian-slim as base images and not longer rely on docker php library images.

As a result of using debian packages some paths such as /var/www/passbolt are going to be
deprecated. This release still supports both paths by symlinking so users should not
be impacted by the path changes. We strongly recommend that you update your volumes
accordingly.

We have also released a rootless image that runs entirely under www-data user and uses
supercronic instead of plain cron to run the background tasks. We aim to make a transition
to rootless images by default to make our docker images a bit more secure by default.
However, rootless alternatives are still considered beta.

As with this release passbolt images are no longer tagged with the '-debian' suffix. Instead:

- Passbolt docker CE images will be tagged as: passbolt/passbolt:<version>-ce
- Passbolt docker CE rootless images will be tagged as: passbolt/passbolt:<version>-ce-non-root
- Passbolt docker pro images will be tagged as: passbolt/passbolt:<version>-pro
- Passbolt docker pro rootless images will be tagged as: passbolt/passbolt:<version>-pro-non-root

You can still find the old Dockerfiles on the dev/ directory as they are still quite
handy for development purposes.

### Added

- New debian package based docker images
- New rootless images
- Supercronic introduced on rootless images

### Changed

- Passbolt installation uses official passbolt debian packages
- /var/www/passbolt files are now in /usr/share/php/passbolt
- /var/www/passbolt/config files are no in /etc/passbolt
- Default workdir is now /usr/share/php/passbolt
- Old docker images moved to dev/ directory
- debian Dockerfiles moved to debian/ directory
- Deprecation message is shown on startup of the containers if old paths detected

## [2.13.5](https://github.com/passbolt/passbolt_docker/compare/v2.13.1...v2.13.5) - 2020-08-04

- Passbolt api bumped to 2.13.5

## [2.13.1](https://github.com/passbolt/passbolt_docker/compare/v2.13.0...v2.13.1) - 2020-07-07

- Passbolt api bumped to 2.13.1

## [2.13.0](https://github.com/passbolt/passbolt_docker/compare/v2.12.1...v2.13.0) - 2020-06-23

- Passbolt api bumped to 2.13.0
- PHP version pinned to 7.3.19

## [2.12.1](https://github.com/passbolt/passbolt_docker/compare/v2.12.0...v2.12.1) - 2020-04-14

### Changed

- Dockerfile pins specific php version for better control
- Passbolt code version bumped to 2.12.1

## [2.12.0](https://github.com/passbolt/passbolt_docker/compare/v2.11.0...v2.12.0) - 2019-12-06

### Changed

- Timeout set to 0 in wait-for.sh. Fixes [#133](https://github.com/passbolt/passbolt_docker/issues/133)

## [2.11.0](https://github.com/passbolt/passbolt_docker/compare/v2.10.0...v2.11.0) - 2019-08-08

### Changed

- Base Container switched to php7.3 and debian 10
- Entrypoint runs supervisor with exec to take over PID 1
- Minor changes: Maintainer label

## [2.10.0](https://github.com/passbolt/passbolt_docker/compare/v2.9.0...v2.10.0) - 2019-05-16

- This is a sync release. Check [changes](https://github.com/passbolt/passbolt_api/compare/v2.9.0...v2.10.0) for passbolt_api

## [2.9.0](https://github.com/passbolt/passbolt_docker/compare/v2.8.4...v2.9.0) - 2019-04-24

### Added

- Build arg to pass headers to curl
- Removed git as dev dependency

### Fixed

- Fix typo in Readme [#125](https://github.com/passbolt/passbolt_docker/pull/125)

## [2.8.4](https://github.com/passbolt/passbolt_docker/compare/v2.8.3...v2.8.4) - 2019-04-17

This is a sync release. Check [changes](https://github.com/passbolt/passbolt_api/compare/v2.8.3...v2.8.4) for passbolt_api

## [2.8.3](https://github.com/passbolt/passbolt_docker/compare/v2.8.2...v2.8.3) - 2019-04-02

### Added

- Documentation for PASSBOLT_PLUGINS_EXPORT_ENABLED and PASSBOLT_PLUGINS_IMPORT_ENABLED

## [2.8.2](https://github.com/passbolt/passbolt_docker/compare/v2.8.1...v2.8.2) - 2019-04-01

This is a sync release. Check [changes](https://github.com/passbolt/passbolt_api/compare/v2.8.1...v2.8.2) for passbolt_api

## [2.8.1](https://github.com/passbolt/passbolt_docker/compare/v2.7.1...v2.8.1) - 2019-04-01

### Added

- Documentation for new env variable APP_BASE to use passbolt in a subfolder
- Added wait-for.sh to the docker image as part of [#123](https://github.com/passbolt/passbolt_docker/pull/123)

### Fixed

- Now passbolt checks for the mysql status outside of the docker image [#97](https://github.com/passbolt/passbolt_docker/issues/97)

## [2.7.1](https://github.com/passbolt/passbolt_docker/compare/v2.7.0...v2.7.1) - 2019-02-13

### Added

- Use php.ini-production for saner defaults in php

## [2.7.0](https://github.com/passbolt/passbolt_docker/compare/v2.5.0...v2.7.0) - 2019-02-12

### Fixed

- Added small warning message when entropy is low at container startup [#121](https://github.com/passbolt/passbolt_docker/issues/121)

## [2.5.0](https://github.com/passbolt/passbolt_docker/compare/v2.4.0...v2.5.0) - 2018-11-15

### Added

- Enabled opcache extension to increase overall performance

## [2.4.0](https://github.com/passbolt/passbolt_docker/compare/v2.3.0...v2.4.0) - 2018-10-15

### Added

- Merged: Adding SSL configuration for mysql/mariadb [#111](https://github.com/passbolt/passbolt_docker/pull/111)

### Fixed

- Minor fix: Remove duplicate arg PHP_EXTENSIONS from Dockerfile

## [2.3.0](https://github.com/passbolt/passbolt_docker/compare/v2.2.0...v2.3.0) - 2018-09-03

See [Changes](https://github.com/passbolt/passbolt_api/compare/v2.2.0...v2.3.0) for passbolt_api

## [2.2.0](https://github.com/passbolt/passbolt_docker/compare/v2.1.0...v2.2.0) - 2018-08-13

### Added

- Added [wait-for-it](https://github.com/vishnubob/wait-for-it) instead of wait for to eliminate netcat dependency

### Changed

- Merged: hide nginx and php version [#107](https://github.com/passbolt/passbolt_docker/pull/107)
- Merged: restrict MySQL port access [#109](https://github.com/passbolt/passbolt_docker/pull/109)
- Supervisor config files split into conf.d/{php.conf,nginx.conf,cron.conf}
- Default stdout logging is more verbose now allowing users to see more details on the requests

## [2.1.0](https://github.com/passbolt/passbolt_docker/compare/v2.0.7...v2.1.0) - 2018-06-14

### Fixed

- cron EmailQueue.sender job fails if db password contains certain characters [#105](https://github.com/passbolt/passbolt_docker/issues/105)

## [2.0.7](https://github.com/passbolt/passbolt_docker/compare/v2.0.5...v2.0.7) - 2018-05-09

Sync release. See release notes on [https://github.com/passbolt/passbolt_api](passbolt api repo)

## [2.0.5](https://github.com/passbolt/passbolt_docker/compare/v2.0.4...v2.0.5) - 2018-05-08

### Fixed

- Nginx configuration file root directive for passbolt

## [2.0.4](https://github.com/passbolt/passbolt_docker/compare/v2.0.2...v2.0.4) - 2018-04-26

### Fixed

- Authentication plugin 'caching_sha2_password' cannot be loaded [#103](https://github.com/passbolt/passbolt_docker/issues/103)

### Changed

- MariaDB as default SQL backend option in docker-compose files related with [#103](https://github.com/passbolt/passbolt_docker/issues/103)
- Replace php copy with curl for use with proxy [#102](https://github.com/passbolt/passbolt_docker/pull/102)
- Documentation requirements moved up in the README

## [2.0.3](https://github.com/passbolt/passbolt_docker/compare/v2.0.2...v2.0.3) - 2018-04-20

### Fixed

- Updated path for images volume. [#101](https://github.com/passbolt/passbolt_docker/pull/101)

### Changed
- Run passbolt migrate task instead of cake migrations migrate

## [2.0.2](https://github.com/passbolt/passbolt_docker/compare/v2.0.1...v2.0.2) - 2018-04-17

### Fixed

- Unable to load a jpeg image as avatar. [#100](https://github.com/passbolt/passbolt_docker/issues/100)
- docker-entrypoint.sh adds email-sending-job everytime you restart the container. [#98](https://github.com/passbolt/passbolt_docker/issues/98)

### Changed

- Removed composer binary after dependency installation.

## [2.0.1](https://github.com/passbolt/passbolt_docker/compare/v2.0.0...v2.0.1) - 2018-04-09

- Decrypt bug fix. Check https://github.com/passbolt/passbolt_api

## [2.0.0](https://github.com/passbolt/passbolt_docker/compare/v2.0.0-rc2...v2.0.0) - 2018-04-09

### Changed

- Base image switched to php:7-fpm (debian based) due performance issues with passbolt and alpine based images
- Web user is now www-data
- Supervisor provides better logging to stdout
- Upload max filesize increased to 5M for avatar uploads
- README documentation updated
- Composer file loads images directory in passbolt container as a docker volume

### Added

- Added composer installer signature check according to official composer docs [#91](https://github.com/passbolt/passbolt_docker/pull/91)

## [1.6.10](https://github.com/passbolt/passbolt_docker/compare/v1.6.9-1...v1.6.10) - 2018-03-28

### Fixed

- chown fails with Docker Secrets, can't start container [#89](https://github.com/passbolt/passbolt_docker/pull/89)

### Changed

The container base image has been migrated from alpine to debian. The reason behind this change
is that we have detected slower performance in alpine based images. Changing the image introduced a few changes
in the structure of the container:
- Bigger images
- www user is now www-data user
- cron jobs are managed as crontabs in /var/spool/cron/crontabs/root
- Permissions check on the passbolt base dir has been removed as it was a big performance penalty on startup times.
- Docker hub tags will now follow the PASSBOLT_VERSION-debian pattern

## [2.0.0-rc2](https://github.com/passbolt/passbolt_docker/compare/v2.0.0-rc1...v2.0.0-rc2) - 2018-02-20

### Changed

- README documentation updated
- PECL_PASSBOLT_EXTENSIONS, PASSBOLT_VERSION and PASSBOLT_URL are now a docker build arg

### Added

- Docker composer files to run passbolt_docker in different environments
- Codacy badges and reports

### Fixed

- Minor issues regarding bash syntax shellcheck SC2034 and SC2166
- Hadolint DL3003 fixed

## [2.0.0-rc1](https://github.com/passbolt/passbolt_docker/compare/v1.6.9-1...v2.0.0-rc1) - 2018-01-17

### Changed

- Moved away from plain alpine to php:7-fpm-alpine series
- Environment variables interface has been revamped and moved to application domain [default.php](https://github.com/passbolt/passbolt_api/blob/develop/config/default.php) and [app.default.php](https://github.com/passbolt/passbolt_api/blob/develop/config/app.default.php)
- PHP extensions management no longer using alpine packages
- Introduced [supervisord](http://supervisord.org/) for process monitoring
- Introduced testing framework for development purposes based on [rspec](http://rspec.info/)
- Reduced the dependencies installed in Dockerfile
- Default user moved from nginx to www-data
- Slightly changed paths of gpg serverkeys (<PASSBOLT_ROOT>/app/Config/gpg/serverkey.private.asc -> <PASSBOLT_ROOT>/config/gpg/serverkey_private.asc)
- Refactor or docker-entrypoint.sh:
  - Moved away from bash to sh
  - Make it compliant with [shellcheck](https://github.com/koalaman/shellcheck)
  - Removed search and replace commands

## [1.6.9-1](https://github.com/passbolt/passbolt_docker/compare/v1.6.9...v1.6.9-1) - 2018-01-15

### Fixed

- Fix bug in how the email 'client' is edited. [#84](https://github.com/passbolt/passbolt_docker/pull/84)

## [1.6.9](https://github.com/passbolt/passbolt_docker/compare/v1.6.5+1...v1.6.9) - 2018-01-14

This release provides the last passbolt_api 1.x series release along with several pull requests
and fixes.

### Fixed

- Unable to access default installation with http [#59](https://github.com/passbolt/passbolt_docker/issues/59)
- Check and correct the permissions and ownership of /var/www/passbolt [#67](https://github.com/passbolt/passbolt_docker/issues/67)
- cp: Unrecognized option -T [#75](https://github.com/passbolt/passbolt_docker/issues/75)
- turn URL config independent from SSL var [#76](https://github.com/passbolt/passbolt_docker/pull/76)
- Set the default MySQL port to 3306 [#77](https://github.com/passbolt/passbolt_docker/pull/77)
- Add environment variable to set email client [#81](https://github.com/passbolt/passbolt_docker/pull/81)


## [1.6.5+1](https://github.com/passbolt/passbolt_docker/compare/v1.6.5...v1.6.5+1) - 2017-11-14

### Fixed

- Introduce EMAIL_AUTH=false Environment variable [#71](https://github.com/passbolt/passbolt_docker/pull/71)
- Fixed https in App.fullBaseUrl for SSL=false. [#73]( https://github.com/passbolt/passbolt_docker/pull/73)

## [1.6.5](https://github.com/passbolt/passbolt_docker/compare/v1.6.3...v1.6.5) - 2017-09-14

- Refer to [passbolt_api CHANGELOG](https://github.com/passbolt/passbolt_api/blob/master/CHANGELOG.md) for a list of new features and fixes.

### Fixed
- PASSBOLT-2406: change to LABEL, add docker-compose file for testing [#69](https://github.com/passbolt/passbolt_docker/pull/69)
- PASSBOLT-2407: Check for email cron before setting it [#63](https://github.com/passbolt/passbolt_docker/issues/63)
- PASSBOLT-2408: Strict Transport Security (HSTS)	Invalid Server provided more than one HSTS header [#65](https://github.com/passbolt/passbolt_docker/issues/65)
- PASSBOLT-2410: nginx config sub optimal [#66](https://github.com/passbolt/passbolt_docker/issues/66)

## [1.6.3](https://github.com/passbolt/passbolt_docker/compare/v1.6.2+1...v1.6.3) - 2017-08-31

- Refer to [passbolt_api CHANGELOG](https://github.com/passbolt/passbolt_api/blob/master/CHANGELOG.md) for a list of new features and fixes.

## [1.6.2+1](https://github.com/passbolt/passbolt_docker/compare/v1.6.2...v1.6.2+1) - 2017-08-16

### Fixed
- PASSBOLT-2295: Added environment variable DB_PORT for non standard database ports (reopened) [#43](https://github.com/passbolt/passbolt_docker/issues/43)

## [1.6.2](https://github.com/passbolt/passbolt_docker/compare/v1.6.1+1...v1.6.2) - 2017-08-16

### Added

- PASSBOLT-2295: Added environment variable DB_PORT for non standard database ports. [#43](https://github.com/passbolt/passbolt_docker/issues/43)
- PASSBOLT-2321: Upgraded passbolt container to latest stable alpine (3.6)

### Fixed
- PASSBOLT-2319: Fullbaseurl parameter was not changing when specifying URL [#50](https://github.com/passbolt/passbolt_docker/issues/50)
- PASSBOLT-2320: TLS value on email.php should not be quoted PR[#53](https://github.com/passbolt/passbolt_docker/pull/53)

## [1.6.1+1](https://github.com/passbolt/passbolt_docker/compare/v1.6.1...v1.6.1+1) - 2017-07-31

### Notes
This release aims to distribute passbolt-1.6.1 and include most relevant community contributions
The most notable change from user perspective is the switch from lowercase to uppercase environment variables. Users will
have to review their previous scripts and update any environment variable to match the new naming convention. Please refer to PR#39

### Added
- PASSBOLT-2276: ENV-Variable uppercase convention PR[#39](https://github.com/passbolt/passbolt_docker/pull/39)
- PASSBOLT-2279: Allow Config files to be symbolic links PR[#32](https://github.com/passbolt/passbolt_docker/pull/32)
- PASSBOLT-2278: Allow no db environment variable setting PR[#20](https://github.com/passbolt/passbolt_docker/pull/20)
- PASSBOLT-2280: On MacOS systems note you should access it using https PR[#35](https://github.com/passbolt/passbolt_docker/pull/35)

### Fixed
- PASSBOLT-2159: Added registration env support PR[#37](https://github.com/passbolt/passbolt_docker/pull/37)

## [1.6.1](https://github.com/passbolt/passbolt_docker/compare/v1.6.0...v1.6.1) - 2017-06-29
### Fixed
- PASSBOLT-2158: corrected management of fullbaseurl throug url env variable.
- PASSBOLT-2164: corrected typo on email_transport env variable. [#24](https://github.com/passbolt/passbolt_docker/issues/24)
- PASSBOLT-2166: http to https redirection fixed. [#19](https://github.com/passbolt/passbolt_docker/issues/19)
- PASSBOLT-2167: healthcheck does not work on container. [#26](https://github.com/passbolt/passbolt_docker/issues/26)

### Added

- PASSBOLT-2165: Added TLS support through email_tls env variable. [#25](https://github.com/passbolt/passbolt_docker/issues/25)

## [1.6.0](https://github.com/passbolt/passbolt_docker/compare/v1.5.1...v1.6.0) - 2017-06-23
### Added
- Added email set up support though environment variables
- Added cronjob to send queued emails
- Automated builds on the docker hub
- Updated README documentation

### Fixed
- Image build fails when using alpine:latest. Switched to alpine:3.5
- Deletion of passwords on docker image
- Avoid importing already imported secret keys on the gpg keyring
