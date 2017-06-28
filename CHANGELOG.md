# Change Log
All notable changes to this project will be documented in this file.
This project adheres to [Semantic Versioning](http://semver.org/).

## [Unreleased]

### Fixed
- PASSBOLT-2158: corrected management of fullbaseurl throug url env variable
- PASSBOLT-2164: corrected typo on email_transport env variable
- PASSBOLT-2166: http to https redirection fixed

### Added

- PASSBOLT-2165: Added TLS support through email_tls env variable

## [1.6.0] - 2017-06-23
### Added
- Added email set up support though environment variables
- Added cronjob to send queued emails
- Automated builds on the docker hub
- Updated README documentation

### Fixed
- Image build fails when using alpine:latest. Switched to alpine:3.5
- Deletion of passwords on docker image
- Avoid importing already imported secret keys on the gpg keyring
