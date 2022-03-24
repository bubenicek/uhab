#!/bin/bash

source ./config.sh

if [ "$1" == "" ]; then
   echo Enter repository name
   exit
fi

curl $CURL_OPTIONS --http-request GET $URL_API/sitemaps/config/$1

