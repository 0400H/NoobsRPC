#include "test.hpp"

int main() {
    test_connect();
    test_sync_call();
    // test_async_call();
    test_sync_demo();
    test_async_demo();
    test_ssl();
    test_upload();
    test_download();
    // test_threads();
    // test_multiple_thread();
    // test_subscribe();
    test_sync_performance();
    test_async_performance();
    test_multi_client_performance(20, test_sync_performance);
    test_multi_client_performance(20, test_async_performance);
    // test_benchmark();

    return 0;
}