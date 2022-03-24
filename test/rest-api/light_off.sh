#!/bin/bash

source ./config.sh

curl -X POST --header "Content-Type: text/plain" --header "Accept: application/json" -d "OFF" "http://$IP:8080/rest/items/LightObyvak1"

