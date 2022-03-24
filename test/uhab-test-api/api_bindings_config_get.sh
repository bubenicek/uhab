#!/bin/bash

source ./config.sh

if [ "$1" == "" ]; then
   echo Enter binding name
   exit
fi


curl $CURL_OPTIONS --http-request GET $URL_API/bindings/config/$1

