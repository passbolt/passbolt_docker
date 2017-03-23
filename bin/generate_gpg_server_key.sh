#!/bin/sh

DIRNAME=`dirname $0`/..
gpg --batch --armor --gen-key $DIRNAME/conf/gpg_server_key_settings.conf
gpg --armor --output $DIRNAME/gpg_server_key_private.key --export-secret-keys "$(cat $DIRNAME/conf/gpg_server_key_settings.conf | grep Name-Real | awk -F: '{gsub(/^[ \t]+|[ \t]+$/, "", $2); print $2}')"
gpg --armor --output conf/gpg_server_key_public.key --export "$(gpg --list-packets conf/gpg_server_key_private.key | grep keyid | awk -F: 'NR==1 {gsub(/^[ \t]+|[ \t]+$/, "", $2); print $2}')"
mv $DIRNAME/gpg_server_key_public.key $DIRNAME/conf
mv $DIRNAME/gpg_server_key_private.key $DIRNAME/conf
