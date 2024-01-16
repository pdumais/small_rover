#!/bin/bash

docker run -v /dev/ttyUSB1:/dev/ttyUSB0 --rm -it --privileged -v $(pwd):/build -w /build -e HOME=/tmp espressif/idf:latest idf.py monitor -p /dev/ttyUSB0

