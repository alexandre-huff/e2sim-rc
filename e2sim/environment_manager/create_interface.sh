#!/bin/bash

podman run --rm -v "${PWD}:/local" openapitools/openapi-generator-cli generate \
    -i  /local/E2Node_UE_api.yml \
    -g cpp-pistache-server \
    -o /local/src
