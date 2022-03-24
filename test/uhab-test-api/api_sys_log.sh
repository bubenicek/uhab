#!/bin/bash

source ./config.sh

curl $CURL_OPTIONS --http-request GET $URL_API/system/log

