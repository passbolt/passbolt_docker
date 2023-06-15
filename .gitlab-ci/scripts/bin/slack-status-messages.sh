#!/bin/bash

# Variables required
# CI_PROJECT_NAME
# CI_PIPELINE_ID
# SLACK_CHANNEL_ID
# SLACK_WEBHOOK

title="$1"
url="$2"

curl -X POST -H 'Content-type: application/json' $SLACK_WEBHOOK \
--data-binary @- <<EOF
{
  "channel": "$SLACK_CHANNEL_ID",
  "attachments": [
    {
      "color": "#36A64F",
      "title": "$title",
      "attachment_type": "default",
      "actions": [
        {
          "name": "Logs",
          "text": "Logs",
          "type": "button",
          "style": "default",
          "url": "$url"
        },
        {
          "name": "DockerHub",
          "text": "DockerHub",
          "type": "button",
          "style": "primary",
          "url": "https://hub.docker.com/r/passbolt/passbolt/tags"
        }
      ]
    }
  ]
}
EOF

