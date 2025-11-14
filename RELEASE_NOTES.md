Announcing the immediate availability of passbolt's docker repository 4.1.1.

This release adds docker secrets support for EMAIL_TRANSPORT_DEFAULT_PASSWORD
and EMAIL_TRANSPORT_DEFAULT_USERNAME environment variables thanks to @Shtiggs

It also includes a warning message when APP_FULL_BASE_URL environment variable
is not set to mitigate host header injection attacks. You can obtain more
information about the subject in the following link: <https://owasp.org/www-project-web-security-testing-guide/v42/4-Web_Application_Security_Testing/07-Input_Validation_Testing/17-Testing_for_Host_Header_Injection>
