#!/bin/sh

docker run -ti -v `pwd`:/mnt -v `pwd`/nginx.conf:/etc/nginx/nginx.conf -p 8022:80 nginx 
