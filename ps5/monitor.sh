#!/bin/bash

docker run -v /dev/ttyUSB0:/dev/ttyUSB0 --rm -it --privileged -v $(pwd):/build -w /build -e HOME=/tmp espressif/idf:release-v5.2 idf.py monitor -p /dev/ttyUSB0

