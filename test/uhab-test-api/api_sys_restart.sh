#!/bin/bash

source ./config.sh

curl $CURL_OPTIONS --http-request PUT $URL_API/system/restart 

