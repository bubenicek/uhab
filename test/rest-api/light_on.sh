#!/bin/bash

source ./config.sh

curl -X POST --header "Content-Type: text/plain" --header "Accept: application/json" -d "ON" "http://$IP:8080/rest/items/LightObyvak1"

