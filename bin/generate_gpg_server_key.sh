#!/bin/sh

DIRNAME=`dirname $0`/..
gpg --batch --armor --gen-key $DIRNAME/conf/gpg_server_key_settings.conf
mv $DIRNAME/gpg_server_key_public.key $DIRNAME/conf
mv $DIRNAME/gpg_server_key_private.key $DIRNAME/conf
