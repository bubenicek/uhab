#!/bin/bash

source ./config.sh

curl $CURL_OPTIONS -X GET $URL_API/system/info | jq

