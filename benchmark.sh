unset http_proxy https_proxy

examples/build/basic_server &
sleep 5s
examples/build/basic_client &
sleep 30s
kill -9 `pgrep basic_`
