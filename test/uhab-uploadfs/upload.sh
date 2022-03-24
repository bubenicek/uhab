#!/bin/bash

source ./config.sh

function create_dir {
  if [ -n "$1" ]; then
     curl $CURL_OPTIONS --http-request POST $URL$1
     echo "Create dir '"$1"' res:"$?
  fi
}

function copy_file {
  if [ -n "$1" ]; then
    curl $CURL_OPTIONS -H "Content-Type: application/octet-stream" --data-binary @$1 --http-request PUT $URL$2
    echo "Copy file " $1 "->" $2 " res:"$?
  fi
}

##
## Copy files
##
files=`find ./rootfs -type f`
for file in $files
do
   src=$file
   dst=${file#./rootfs}
   copy_file $src $dst
done


