# Change Log
All notable changes to this project will be documented in this file.
This project adheres to [Semantic Versioning](http://semver.org/).

## [Unreleased](https://github.com/passbolt/passbolt_docker/compare/v1.6.9-1...HEAD)

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
