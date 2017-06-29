# Change Log
All notable changes to this project will be documented in this file.
This project adheres to [Semantic Versioning](http://semver.org/).

## [Unreleased](https://github.com/passbolt/passbolt_api/compare/v1.6.1...HEAD)

## [1.6.1](https://github.com/passbolt/passbolt_api/compare/v1.6.0...v1.6.1) - 2017-06-29
### Fixed
- PASSBOLT-2158: corrected management of fullbaseurl throug url env variable.
- PASSBOLT-2164: corrected typo on email_transport env variable. [#24](https://github.com/passbolt/passbolt_docker/issues/24)
- PASSBOLT-2166: http to https redirection fixed. [#19](https://github.com/passbolt/passbolt_docker/issues/19)
- PASSBOLT-2167: healthcheck does not work on container. [#26](https://github.com/passbolt/passbolt_docker/issues/26)

### Added

- PASSBOLT-2165: Added TLS support through email_tls env variable. [#25](https://github.com/passbolt/passbolt_docker/issues/25)

## [1.6.0](https://github.com/passbolt/passbolt_api/compare/v1.5.1...v1.6.0) - 2017-06-23
### Added
- Added email set up support though environment variables
- Added cronjob to send queued emails
- Automated builds on the docker hub
- Updated README documentation

### Fixed
- Image build fails when using alpine:latest. Switched to alpine:3.5
- Deletion of passwords on docker image
- Avoid importing already imported secret keys on the gpg keyring
