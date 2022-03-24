#!/bin/bash

source ./config.sh

if [ "$1" == "" ]; then
   echo Enter binding name
   exit
fi

if [ "$2" == "" ]; then
   echo Enter configuration filename
   exit
fi

binding=$1
filename=$2

curl $CURL_OPTIONS -H "Content-Type: application/octet-stream" --data-binary @$filename --http-request PUT $URL_API/bindings/config/$binding

