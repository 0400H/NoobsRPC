#include <iostream>
#include <fstream>
#include <string>
#include <chrono>

#include <rest_rpc.hpp>

using namespace rest_rpc;
using namespace rest_rpc::rpc_service;

struct person {
    int id;
    std::string name;
    int age;

    MSGPACK_DEFINE(id, name, age);
};

bool client_connect(rpc_client & client,
                  bool is_ssl=true,
                  unsigned short port=9000,
                  const std::string& host="127.0.0.1",
                  size_t timeout = 3) {
    client.set_error_callback([](boost::system::error_code ec) {
        std::cout << ec.message() << "" << std::endl;
    });

    if (is_ssl) {
        std::cout << "start with ssl!" << std::endl;
#ifdef CINATRA_ENABLE_SSL
    client.set_ssl_context_callback([](boost::asio::ssl::context& ctx) {
        ctx.set_verify_mode(boost::asio::ssl::context::verify_peer);
        ctx.load_verify_file("server.crt");
    });
#endif
    } else {
        std::cout << "start without ssl!" << std::endl;
    }

    return client.connect(host, port, is_ssl, timeout);
}

void test_connect(bool is_ssl=true) {
    std::cout << "test_connect start!" << std::endl;

    rpc_client client;
    client.enable_auto_reconnect(); //automatic reconnect
    client.enable_auto_heartbeat(); //automatic heartbeat
    client_connect(client, is_ssl, 9000, "127.0.0.1");

    int count = 0;
    while (true) {
        if (client.has_connected()) {
            std::cout << "connected ok" << std::endl;
            break;
        } else {
            std::cout << "connected failed: " << count++ << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::getchar();
    std::cout << "test_connect finished!" << std::endl;
}

void test_sync_call(bool is_ssl=true) {
    std::cout << "test_sync_call start!" << std::endl;

    rpc_client client;
    bool r = client_connect(client, is_ssl, 9000, "127.0.0.1");
    if (!r) {
        std::cout << "connect failed!" << std::endl;
        return;
    }

    {
        std::cout << "test service echo start!" << std::endl;
        auto result = client.call<std::string>("echo", "service: echo");
        std::cout << result << std::endl;
        std::cout << "test service echo finished!" << std::endl;
    }

    {
        std::cout << "test service get_person start!" << std::endl;
        try {
            auto result = client.call<0, person>("get_person");
            std::cout << result.name << std::endl;

            client.close();
            bool r = client_connect(client, is_ssl, 9000, "127.0.0.1");
            if (!r) {
                std::cout << "connect failed!" << std::endl;
                return;
            }

            result = client.call<50, person>("get_person");
            std::cout << result.name << std::endl;
        } catch (const std::exception & ex) {
            std::cout << ex.what() << std::endl;
        }
        std::cout << "test service get_person finished!" << std::endl;
    }

    {
        std::cout << "test service get_person_name start!" << std::endl;
        try {
            rpc_client client;
            bool r = client_connect(client, is_ssl, 9000, "127.0.0.1");
            if (!r) {
                std::cout << "connect failed!" << std::endl;
                return;
            }

            auto result = client.call<std::string>("get_person_name", person{ 1, "Tom", 20 });
            std::cout << result << std::endl;
        } catch (const std::exception & e) {
            std::cout << e.what() << std::endl;
        }
        std::cout << "test service get_person_name finished!" << std::endl;
    }

    std::getchar();
    std::cout << "test_sync_call finished!" << std::endl;
}

void test_async_call(bool is_ssl=true) {
    std::cout << "test_async_call start!" << std::endl;

    rpc_client client;
    bool r = client_connect(client, is_ssl, 9000, "127.0.0.1");
    if (!r) {
        std::cout << "connect failed!" << std::endl;
        return;
    }

    {
        for (size_t i = 0; i < 100; i=i+2) {
            std::string task1 = "task_" + std::to_string(i);
            // zero means no timeout check, no param means using default timeout(5s)
            client.async_call<0>("echo", 
                [](const boost::system::error_code &ec, string_view data) {
                    if (ec) {
                        std::cout << "error code: " << ec.value()
                                  << ", error msg: "<< ec.message() << std::endl;
                    } else {
                        auto str = as<std::string>(data);
                        std::cout << "echo " << str << std::endl;
                    }
                }, task1);

            std::string task2 = "task_" + std::to_string(i+1);
            // set timeout 500ms
            client.async_call<500>("async_echo",
                [](const boost::system::error_code &ec, string_view data) {
                    if (ec) {
                        std::cout << "error code: " << ec.value()
                                  << ", error msg: "<< ec.message() << std::endl;
                    } else {
                        auto str = as<std::string>(data);
                        std::cout << "async_echo " << str << std::endl;
                    }
                }, task2);
        }
    }

    {
        client.set_error_callback([](boost::system::error_code ec) { std::cout << ec.message() << std::endl; });

        auto f = client.async_call<FUTURE>("get_person");
        if (f.wait_for(std::chrono::milliseconds(50)) == std::future_status::timeout) {
            std::cout << "timeout" << std::endl;
        } else {
            auto p = f.get().as<person>();
            std::cout << p.name << std::endl;
        }
    }

    // client.run();

    std::getchar();
    std::cout << "test_async_call finished!" << std::endl;
}

void test_upload(bool is_ssl=true) {
    std::cout << "test_upload start!" << std::endl;

    rpc_client client;
    bool r = client_connect(client, true, 9000, "127.0.0.1", 1);
    if (!r) {
        std::cout << "connect failed!" << std::endl;
        return;
    }

    std::ifstream file(__FILE__, std::ios::binary);
    file.seekg(0, std::ios::end);
    size_t file_len = file.tellg();
    file.seekg(0, std::ios::beg);
    std::string conent;
    conent.resize(file_len);
    file.read(&conent[0], file_len);
    std::cout << file_len << std::endl;

    {
        auto f = client.async_call<FUTURE>("upload", "upload_file", conent);
        if (f.wait_for(std::chrono::milliseconds(500)) == std::future_status::timeout) {
            std::cout << "timeout" << std::endl;
        } else {
            f.get().as();
            std::cout << "ok" << std::endl;
        }
    }

    std::getchar();
    std::cout << "test_upload finished!" << std::endl;
}

void test_download(bool is_ssl=true) {
    std::cout << "test_download start!" << std::endl;

    rpc_client client;
    bool r = client_connect(client, true, 9000, "127.0.0.1", 1);
    if (!r) {
        std::cout << "connect failed!" << std::endl;
        return;
    }

    auto f = client.async_call<FUTURE>("download", "test");
    if (f.wait_for(std::chrono::milliseconds(500)) == std::future_status::timeout) {
        std::cout << "timeout" << std::endl;
    } else {
        auto content = f.get().as<std::string>();
        std::cout << content.size() << std::endl;
        std::ofstream file("test", std::ios::binary);
        file.write(content.data(), content.size());
    }

    std::getchar();
    std::cout << "test_download finished!" << std::endl;
}

void wait_for_notification(rpc_client & client) {
    client.async_call<0>("sub", [&client](const boost::system::error_code &ec, string_view data) {
        auto str = as<std::string>(data);
        std::cout << str << std::endl;

        wait_for_notification(client);
    });
}

void test_subscribe(bool is_ssl=true) {
    std::cout << "test_subscribe start!" << std::endl;

    rpc_client client;
    client.enable_auto_reconnect();
    client.enable_auto_heartbeat();
    bool r = client_connect(client, is_ssl, 9000, "127.0.0.1");
    if (!r) {
        return;
    }

    client.subscribe("key", [](string_view data) {
        std::cout << data << "" << std::endl;
    });

    client.subscribe("key", "048a796c8a3c6a6b7bd1223bf2c8cee05232e927b521984ba417cb2fca6df9d1", [](string_view data) {
        msgpack_codec codec;
        person p = codec.unpack<person>(data.data(), data.size());
        std::cout << p.name << std::endl;
    });

    client.subscribe("key1", "048a796c8a3c6a6b7bd1223bf2c8cee05232e927b521984ba417cb2fca6df9d1", [](string_view data) {
        std::cout << data << std::endl;
    });

    std::thread thd([&client] {
        auto loop = 100;
        while (loop > 0) {
            loop -= 1;
            try {
                if (client.has_connected()) {
                    auto r = client.call<std::string>("echo", "test subscribe");
                    std::cout << "add result: " << r << "" << std::endl;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            } catch (const std::exception& ex) {
                std::cout << ex.what() << "" << std::endl;
            }
        }
    });

    thd.detach();

    /*rpc_client client1;
    bool r1 = client1.connect("127.0.0.1", 9000);
    if (!r1) {
        return;
    }

    person p{10, "jack", 21};
    client1.publish("key", "hello subscriber");
    client1.publish_by_token("key", "sub_key", p);
    
    std::thread thd([&client1, p] {
        while (true) {
            try {
                client1.publish("key", "hello subscriber");
                client1.publish_by_token("key", "unique_token", p);                
            }
            catch (const std::exception& ex) {
                std::cout << ex.what() << "" << std::endl;
            }
        }
    });
*/

    std::getchar();
    std::cout << "test_subscribe finished!" << std::endl;
}

void test_threads(bool is_ssl=true) {
    std::cout << "test_threads start!" << std::endl;

    rpc_client client;
    bool r = client_connect(client, is_ssl, 9000, "127.0.0.1");
    if (!r) {
        return;
    }

    size_t scale = 1000;

    {
        for (size_t i = 0; i < scale; i++) {
            auto future = client.async_call<FUTURE>("echo", "test");
            auto status = future.wait_for(std::chrono::seconds(2));
            if (status == std::future_status::timeout) {
                std::cout << "timeout" << std::endl;
            } else if (status == std::future_status::ready) {
                std::string content = future.get().as<std::string>();
            }
            std::this_thread::sleep_for(std::chrono::microseconds(2));
        }
        std::cout << "without thread finished!" << std::endl;
    }

    {
        std::thread thd([scale, &client] {
            for (size_t i = scale; i < 2*scale; i++) {
                auto future = client.async_call<FUTURE>("echo", "service: echo");
                auto status = future.wait_for(std::chrono::seconds(2));
                if (status == std::future_status::timeout) {
                    std::cout << "timeout" << std::endl;
                } else if (status == std::future_status::ready) {
                    std::string content = future.get().as<std::string>();
                }
                std::this_thread::sleep_for(std::chrono::microseconds(2));
            }
            std::cout << "child thread 1 finished!" << std::endl;
        });
        thd.join();
        // thd.detach();
    }

    std::getchar();
    std::cout << "test_threads finished!" << std::endl;
}

void test_multiple_thread(bool is_ssl=true) {
    std::cout << "test_multiple_thread start!" << std::endl;

    person p = { 1, "Tom", 20 };
    std::vector<std::shared_ptr<rpc_client>> clients;
    std::vector<std::shared_ptr<std::thread>> threads;
    for (int j = 0; j < 4; ++j) {
        auto client = std::make_shared<rpc_client>();
        clients.push_back(client);
        bool r = client->connect("127.0.0.1", 9000, is_ssl);
        if (!r) {
            return;
        }

        for (size_t i = 0; i < 2; i++) {
            threads.emplace_back(std::make_shared<std::thread>([&client, &p] {
                for (size_t i = 0; i < 10000; i++) {
                    client->async_call<0>("get_person_name",
                        [](const boost::system::error_code &ec, string_view data) {
                            if (ec) {
                                std::cout << "error code: " << ec.value()
                                        << ", error msg: "<< ec.message() << std::endl;
                            } else {
                                auto str = as<std::string>(data);
                                std::cout << "echo " << str << std::endl;
                            }
                        }, p);

                    //auto future = client->async_call<FUTURE>("get_person_name", p);
                    //auto status = future.wait_for(std::chrono::seconds(2));
                    //if (status == std::future_status::deferred) {
                    //    std::cout << "deferred" << std::endl;
                    //}
                    //else if (status == std::future_status::timeout) {
                    //    std::cout << "timeout" << std::endl;
                    //}
                    //else if (status == std::future_status::ready) {
                    //}

                    //client->call<std::string>("get_person_name", p);
                }
            }));
        }
    }

    for (auto& thd : threads) {
        thd->detach();
    }

    std::getchar();
    std::cout << "test_multiple_thread finished!" << std::endl;
}

void test_get_latency(bool is_ssl=true) {
    std::cout << "test_get_latency start!" << std::endl;

    rpc_client client;
    bool r = client_connect(client, is_ssl, 9000, "127.0.0.1");
    if (!r) {
        return;
    }

    {
        for (size_t i = 0; i < 10000; i++) {
            auto r = client.call<10, double>("get_latency", std::chrono::system_clock::now());
            std::cout << "sync get latency: " << r << " ms" << std::endl;
        }
    }

    // {
    //     for (size_t i = 0; i < 10000; i++) {
    //         client.async_call<5000>("get_latency",
    //                     [i](const boost::system::error_code &ec, string_view data) {
    //                         if (ec) {
    //                             std::cout << "error code: " << ec.value()
    //                                     << ", error msg: "<< ec.message() << std::endl;
    //                         } else {
    //                             std::cout << "non error: " << i << std::endl;
    //                         }
    //                 }, std::chrono::system_clock::now());
    //         std::cout << "async post latency: " << r << std::endl;
    //     }
    // }

    std::getchar();
    std::cout << "test_get_latency finished!" << std::endl;
}

void test_benchmark(bool is_ssl=true){
    std::cout << "test_benchmark start!" << std::endl;

    rpc_client client;
    bool r = client_connect(client, is_ssl, 9000, "127.0.0.1");
    if (!r) {
        return;
    }

    {
        for (size_t i = 0; i < 10000; i++) {
            client.async_call<100>("echo",
                [i](const boost::system::error_code &ec, string_view data) {
                    if (ec) {
                        std::cout << "error code: " << ec.value()
                                << ", error msg: "<< ec.message() << std::endl;
                    } else {
                        std::cout << "non error: " << i << std::endl;
                    }
            }, "benchmark 1");
        }
    }

    {
        for (size_t i = 0; i < 100000; i++) {
            client.async_call<500>("async_echo",
                [i](const boost::system::error_code &ec, string_view data) {
                    if (ec) {
                        std::cout << "error code: " << ec.value()
                                  << ", error msg: " << ec.message() << std::endl;
                    } else {
                        std::cout << "non error: " << i << std::endl;
                    }
            }, "benchmark 2");
        }
    }

    {
        for (size_t i = 0; i < 300000; i++) {
            client.async_call<5000>("async_echo",
                [i](const boost::system::error_code &ec, string_view data) {
                    if (ec) {
                        std::cout << "error code: " << ec.value()
                                  << ", error msg: " << ec.message() << std::endl;
                    } else {
                        std::cout << "non error: " << i << std::endl;
                    }
            }, "benchmark 3");
        }
    }

    // client.run();

    std::getchar();
    std::cout << "test_benchmark finished!" << std::endl;
}


void test_sync_performance(bool is_ssl=true) {
    std::cout << "test_sync_performance start!" << std::endl;

    rpc_client client;
    bool r = client_connect(client, is_ssl, 9000, "127.0.0.1");
    if (!r) {
        std::cout << "connect failed!" << std::endl;
        return;
    }

    try {
        person p = { 1, "Tom", 20 };
        for (size_t i = 0; i < 10000; i++) {
            client.call<std::string>("get_person_name", p);
        }
        std::cout << "finish" << std::endl;
    } catch (const std::exception & ex) {
        std::cout << ex.what() << std::endl;
    }

    std::getchar();
    std::cout << "test_sync_performance finished!" << std::endl;
}

void test_async_performance(bool is_ssl=true) {
    std::cout << "test_async_performance start!" << std::endl;

    rpc_client client;
    bool r = client_connect(client, is_ssl, 9000, "127.0.0.1");
    if (!r) {
        std::cout << "connect failed!" << std::endl;
        return;
    }

    {
        person p = {1, "Tom", 20};
        std::vector<std::future<req_result>> futures;
        // for (size_t i = 0; i < 10000; i++) {
        //     auto future = client.async_call<FUTURE>("get_person_name", p);
        //     auto status = future.wait_for(std::chrono::milliseconds(10));
        //     if (status == std::future_status::deferred) {
        //         std::cout << "deferred" << std::endl;
        //     } else if (status == std::future_status::timeout) {
        //         std::cout << "timeout" << std::endl;
        //     } else if (status == std::future_status::ready) {
        //     }
        // }
        for (size_t i = 0; i < 10000; i++) {
            futures.push_back(client.async_call<FUTURE>("get_person_name", p));
        }

        for (auto& future : futures) {
            auto status = future.wait_for(std::chrono::milliseconds(100));
            if (status == std::future_status::deferred) {
                std::cout << "deferred" << std::endl;
            } else if (status == std::future_status::timeout) {
                std::cout << "timeout" << std::endl;
            } else if (status == std::future_status::ready) {
            }
        }
    }

    std::getchar();
    std::cout << "test_async_performance finished!" << std::endl;
}

template <typename T>
void test_multi_client_performance(size_t n, T & func, bool is_ssl=true) {
    std::cout << "test_multi_client_performance start!" << std::endl;

    std::vector<std::shared_ptr<std::thread>> group;
    for (int i = 0; i < n; ++i) {
        group.emplace_back(std::make_shared<std::thread>([&func, is_ssl](){ return func(is_ssl); }));
    }
    for (auto& p : group) {
        p->detach();
    }

    std::getchar();
    std::cout << "test_multi_client_performance finished!" << std::endl;
}