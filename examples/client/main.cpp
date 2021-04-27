#include "test.hpp"

int main() {
    bool with_ssl = false;

    test_connect(with_ssl);
    test_sync_call(with_ssl);
    test_async_call(with_ssl);
    test_threads(with_ssl);
    // test_multiple_thread(with_ssl);
    test_upload(with_ssl);
    test_download(with_ssl);
    test_subscribe(with_ssl);
    test_get_latency(with_ssl);
    test_benchmark(with_ssl);
    test_sync_performance(with_ssl);
    test_async_performance(with_ssl);
    test_multi_client_performance(20, test_sync_performance, with_ssl);
    test_multi_client_performance(20, test_async_performance, with_ssl);

    return 0;
}