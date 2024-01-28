#!/bin/bash

docker run -v /dev/ttyUSB1:/dev/ttyUSB0 --rm -it --privileged -v $(pwd)/../common:/build/main/common -v $(pwd):/build -w /build -e HOME=/tmp espressif/idf:release-v5.2 idf.py flash monitor -p /dev/ttyUSB0

