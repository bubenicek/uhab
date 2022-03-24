#!/bin/bash

source ./config.sh

if [ "$1" == "" ]; then
   echo Enter binding name
   exit
fi

BINDING=$1
FILENAME=/tmp/binding.cfg

# Load config
curl $CURL_OPTIONS --http-request GET $URL_API/bindings/config/$1 > $FILENAME
if [ $? -ne 0 ]
then
  echo "Load configuration failed"
  exit 0
fi

vi $FILENAME
echo vim_ret: $?

# Save config
curl $CURL_OPTIONS -H "Content-Type: application/octet-stream" --data-binary @$FILENAME --http-request PUT $URL_API/bindings/config/$BINDING
if [ $? -ne 0 ]
then
  echo "Save configuration failed"
  exit 0
fi

echo "Binding configuration saved"

