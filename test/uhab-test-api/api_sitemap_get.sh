#!/bin/bash

source ./config.sh

page=$1

if [ "$1" == "" ]; then
   page=0
fi

curl $CURL_OPTIONS --http-request GET $URL_API/sitemaps/test/$page | jq

