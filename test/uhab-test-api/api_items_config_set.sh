#!/bin/bash

source ./config.sh

if [ "$1" == "" ]; then
   echo Enter items repository name
   exit
fi

if [ "$2" == "" ]; then
   echo Enter configuration filename
   exit
fi

repo=$1
filename=$2

curl $CURL_OPTIONS -H "Content-Type: application/octet-stream" --data-binary @$filename --http-request PUT $URL_API/items/config/$repo

