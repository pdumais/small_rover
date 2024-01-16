#!/bin/bash
set -x

docker run --rm -it --privileged -v $(pwd)/../common:/build/main/common -v $(pwd):/build -u $UID -w /build -e HOME=/tmp espressif/idf:latest idf.py build

