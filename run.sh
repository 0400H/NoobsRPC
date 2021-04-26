#!/bin/bash

set -e 

if  [ ! -n "$1" ] ; then
    tag="all"
else
    tag=$1
fi

function server()
{
    SERVER_CN="localhost"
    openssl req -x509 -sha256 -nodes -days 365 -newkey rsa:2048 -keyout server.key -out server.crt -subj "/CN=${SERVER_CN}"
    ./basic_server &
}

function client()
{
    ./basic_client | tee -a ${filename}.log &
    sleep 60s && kill -9 `pgrep basic_`
}

filename=$(date '+%Y%m%d.%H%M%S').$(hostname)
unset http_proxy https_proxy
cd examples/build

if  [ "$tag" = "server" ] ; then
    rm -rf *.log
    server
elif [ "$tag" = "client" ] ; then
    client
elif [ "$tag" = "all" ] ; then
    server &
    sleep 5s
    client &
fi
