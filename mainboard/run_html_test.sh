#!/bin/sh

docker run -ti -v `pwd`:/mnt -v `pwd`/nginx.conf:/etc/nginx/nginx.conf -p 8001:80 nginx 
