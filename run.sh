set -e 

unset http_proxy https_proxy

cd examples/build

SERVER_CN="localhost"
openssl req -x509 -sha256 -nodes -days 365 -newkey rsa:2048 -keyout server.key -out server.crt -subj "/CN=${SERVER_CN}"

./basic_server &
sleep 3s

./basic_client &
(sleep 60s && kill -9 `pgrep basic_`) &
