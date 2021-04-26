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

void test_sync_call() {
    std::cout << "test_sync_call start!" << std::endl;

    rpc_client client("127.0.0.1", 9000);
    bool r = client.connect();
    if (!r) {
        std::cout << "connect failed!" << std::endl;
        return;
    }

    {
        auto result = client.call<std::string>("echo", "task1");
        std::cout << result << std::endl;
    }

    {
        try {
            auto result = client.call<50, person>("get_person");
            std::cout << result.name << std::endl;
            client.close();
            bool r = client.connect();
            result = client.call<50, person>("get_person");
            std::cout << result.name << std::endl;
        } catch (const std::exception & ex) {
            std::cout << ex.what() << std::endl;
        }
    }

    std::getchar();
    std::cout << "test_sync_call finished!" << std::endl;
}

void test_async_call() {
    std::cout << "test_async_call start!" << std::endl;

    rpc_client client;
    bool r = client.connect("127.0.0.1", 9000);
    if (!r) {
        std::cout << "connect failed!" << std::endl;
        return;
    }

    for (size_t i = 0; i < 100; i=i+2) {
        std::string task1 = "task_" + std::to_string(i);
        // zero means no timeout check, no param means using default timeout(5s)
        client.async_call<0>("echo", 
            [](boost::system::error_code ec, string_view data) {
                auto str = as<std::string>(data);
                std::cout << "echo " << str << std::endl;
            }, task1);

        std::string task2 = "task_" + std::to_string(i+1);
        // set timeout 500ms
        client.async_call<5000>("async_echo",
            [](boost::system::error_code ec, string_view data) {
                if (ec) {
                    std::cout << "error code: " << ec.value()
                              << ", error msg: "<< ec.message() << std::endl;
                } else {
                    auto str = as<std::string>(data);
                    std::cout << "async_echo " << str << std::endl;
                }
            }, task2);
    }

    // client.run();

    std::getchar();
    std::cout << "test_async_call finished!" << std::endl;
}

void test_ssl() {
    std::cout << "test_ssl start!" << std::endl;

    bool is_ssl = true;
    rpc_client client;
    client.set_error_callback([](boost::system::error_code ec) {
        std::cout << ec.message() << "" << std::endl;
    });

#ifdef CINATRA_ENABLE_SSL
    std::cout << "enable ssl!" << std::endl;
    client.set_ssl_context_callback([](boost::asio::ssl::context& ctx) {
        ctx.set_verify_mode(boost::asio::ssl::context::verify_peer);
        ctx.load_verify_file("server.crt");
    });
#endif

    bool r = client.connect("127.0.0.1", 9000, is_ssl);
    if (!r) {
        std::cout << "client connect failed!" << std::endl;
        return;
    }

    for (size_t i = 0; i < 100; i++) {
        try {
            auto result = client.call<std::string>("echo", "purecpp");
            std::cout << result << " sync" << std::endl;
        } catch (const std::exception& e) {
            std::cout << e.what() << " sync" << std::endl;
        }        

        auto future = client.async_call<CallModel::future>("echo", "purecpp");
        auto status = future.wait_for(std::chrono::milliseconds(5000));
        if (status == std::future_status::timeout) {
            std::cout << "timeout future" << std::endl;
        } else {
            auto result1 = future.get();
            std::cout << result1.as<std::string>() << " future" << std::endl;
        }

        client.async_call("echo", [](boost::system::error_code ec, string_view data) {
            if (ec) {                
                std::cout << "error code: " << ec.value()
                          << ", error msg: "<< ec.message() << std::endl;
                return;
            } else {
                auto result = as<std::string>(data);
                std::cout << result << " async" << std::endl;
            }
        }, "purecpp");
    }

    std::getchar();
    std::cout << "test_ssl finished!" << std::endl;
}

void test_add() {
    std::cout << "test_add start!" << std::endl;

    try {
        rpc_client client("127.0.0.1", 9000);
        bool r = client.connect();
        if (!r) {
            std::cout << "connect failed!" << std::endl;
            return;
        }

        {
            auto result = client.call<int>("add", 1, 2);
            std::cout << result << std::endl;
        }

        {
            auto result = client.call<2000, int>("add", 1, 2);
            std::cout << result << std::endl;
        }
    } catch (const std::exception & e) {
        std::cout << e.what() << std::endl;
    }

    std::getchar();
    std::cout << "test_add finished!" << std::endl;
}

void test_translate() {
    std::cout << "test_translate start!" << std::endl;

    try {
        rpc_client client("127.0.0.1", 9000);
        bool r = client.connect();
        if (!r) {
            std::cout << "connect failed!" << std::endl;
            return;
        }

        auto result = client.call<std::string>("translate", "hello");
        std::cout << result << std::endl;
    } catch (const std::exception & e) {
        std::cout << e.what() << std::endl;
    }

    std::getchar();
    std::cout << "test_translate finished!" << std::endl;
}

void test_hello() {
    std::cout << "test_hello start!" << std::endl;

    try {
        rpc_client client("127.0.0.1", 9000);
        bool r = client.connect();
        if (!r) {
            std::cout << "connect failed!" << std::endl;
            return;
        }

        client.call("hello", "purecpp");
    } catch (const std::exception & e) {
        std::cout << e.what() << std::endl;
    }

    std::getchar();
    std::cout << "test_hello finished!" << std::endl;
}

void test_get_person_name() {
    std::cout << "test_get_person_name start!" << std::endl;

    try {
        rpc_client client("127.0.0.1", 9000);
        bool r = client.connect();
        if (!r) {
            std::cout << "connect failed!" << std::endl;
            return;
        }

        auto result = client.call<std::string>("get_person_name", person{ 1, "tom", 20 });
        std::cout << result << std::endl;
    } catch (const std::exception & e) {
        std::cout << e.what() << std::endl;
    }

    std::getchar();
    std::cout << "test_get_person_name finished!" << std::endl;
}

void test_get_person() {
    std::cout << "test_get_person start!" << std::endl;

    try {
        rpc_client client("127.0.0.1", 9000);
        bool r = client.connect();
        if (!r) {
            std::cout << "connect failed!" << std::endl;
            return;
        }

        auto result = client.call<person>("get_person");
        std::cout << result.name << std::endl;
    } catch (const std::exception & e) {
        std::cout << e.what() << std::endl;
    }
    std::getchar();
    std::cout << "test_get_person finished!" << std::endl;
}

void test_sync_demo() {
    test_add();
    test_translate();
    test_hello();
    test_get_person_name();
    test_get_person();
}

void test_async_demo() {
    std::cout << "test_async_demo start!" << std::endl;

    rpc_client client("127.0.0.1", 9000);
    bool r = client.connect();
    if (!r) {
        std::cout << "connect failed!" << std::endl;
        return;
    }

    client.set_error_callback([](boost::system::error_code ec) { std::cout << ec.message() << std::endl; });

    auto f = client.async_call<FUTURE>("get_person");
    if (f.wait_for(std::chrono::milliseconds(50)) == std::future_status::timeout) {
        std::cout << "timeout" << std::endl;
    } else {
        auto p = f.get().as<person>();
        std::cout << p.name << std::endl;
    }

    auto fu = client.async_call<FUTURE>("hello", "purecpp");
    fu.get().as(); //no return

    //sync call
    client.call("hello", "purecpp");
    auto p = client.call<person>("get_person");

    std::getchar();
    std::cout << "test_async_demo finished!" << std::endl;
}

void test_upload() {
    std::cout << "test_upload start!" << std::endl;

    rpc_client client("127.0.0.1", 9000);
    bool r = client.connect(1);
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
        auto f = client.async_call<FUTURE>("upload", "test", conent);
        if (f.wait_for(std::chrono::milliseconds(500)) == std::future_status::timeout) {
            std::cout << "timeout" << std::endl;
        } else {
            f.get().as();
            std::cout << "ok" << std::endl;
        }
    }

    {
        auto f = client.async_call<FUTURE>("upload", "test1", conent);
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

void test_download() {
    std::cout << "test_download start!" << std::endl;

    rpc_client client("127.0.0.1", 9000);
    bool r = client.connect(1);
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

void test_sync_performance() {
    std::cout << "test_sync_performance start!" << std::endl;

    rpc_client client("127.0.0.1", 9000);
    bool r = client.connect();
    if (!r) {
        std::cout << "connect failed!" << std::endl;
        return;
    }

    person p{ 1, "tom", 20 };

    try {
        for (size_t i = 0; i < 10000; i++) {
            client.call<std::string>("get_name", p);
        }
        std::cout << "finish" << std::endl;
    } catch (const std::exception & ex) {
        std::cout << ex.what() << std::endl;
    }

    std::getchar();
    std::cout << "test_sync_performance finished!" << std::endl;
}

template <typename T>
void test_multi_client_performance(size_t n, T & func) {
    std::cout << "test_multi_client_performance start!" << std::endl;

    std::vector<std::shared_ptr<std::thread>> group;
    for (int i = 0; i < n; ++i) {
        group.emplace_back(std::make_shared<std::thread>([&func]{ return func(); }));
    }
    for (auto& p : group) {
        p->join();
    }

    std::getchar();
    std::cout << "test_multi_client_performance finished!" << std::endl;
}

void test_async_performance() {
    std::cout << "test_async_performance start!" << std::endl;

    rpc_client client("127.0.0.1", 9000);
    bool r = client.connect();
    if (!r) {
        std::cout << "connect failed!" << std::endl;
        return;
    }

    person p{ 1, "tom", 20 };

    for (size_t i = 0; i < 10000; i++) {
        auto future = client.async_call<FUTURE>("get_name", p);
        auto status = future.wait_for(std::chrono::seconds(2));
        if (status == std::future_status::deferred) {
            std::cout << "deferred" << std::endl;
        } else if (status == std::future_status::timeout) {
            std::cout << "timeout" << std::endl;
        } else if (status == std::future_status::ready) {
        }
    }

    std::getchar();
    std::cout << "test_async_performance finished!" << std::endl;
}

void test_connect() {
    std::cout << "test_connect start!" << std::endl;

    rpc_client client;
    client.enable_auto_reconnect(); //automatic reconnect
    client.enable_auto_heartbeat(); //automatic heartbeat
    bool r = client.connect("127.0.0.1", 9000);
    int count = 0;
    while (true) {
        if (client.has_connected()) {
            std::cout << "connected ok" << std::endl;
            break;
        } else {
            std::cout << "connected failed: "<< count++<<"" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    //{
    //    rpc_client client;
    //    bool r = client.connect("127.0.0.1", 9000);
    //    int count = 0;
    //    while (true) {
    //        if (client.connect()) {
    //            std::cout << "connected ok" << std::endl;
    //            break;
    //        }
    //        else {
    //            std::cout << "connected failed: " << count++ << "" << std::endl;
    //        }
    //    }
    //}

    std::getchar();
    std::cout << "test_connect finished!" << std::endl;
}

void wait_for_notification(rpc_client & client) {
    client.async_call<0>("sub", [&client](boost::system::error_code ec, string_view data) {
        auto str = as<std::string>(data);
        std::cout << str << std::endl;

        wait_for_notification(client);
    });
}

void test_subscribe() {
    std::cout << "test_subscribe start!" << std::endl;

    rpc_client client;
    client.enable_auto_reconnect();
    client.enable_auto_heartbeat();
    bool r = client.connect("127.0.0.1", 9000);
    if (!r) {
        return;
    }

    client.subscribe("key", [](string_view data) {
        std::cout << data << "" << std::endl;
    });

    client.subscribe("key", "048a796c8a3c6a6b7bd1223bf2c8cee05232e927b521984ba417cb2fca6df9d1", [](string_view data) {
        msgpack_codec codec;
        person p = codec.unpack<person>(data.data(), data.size());
        std::cout << p.name << "" << std::endl;
    });

    client.subscribe("key1", "048a796c8a3c6a6b7bd1223bf2c8cee05232e927b521984ba417cb2fca6df9d1", [](string_view data) {
        std::cout << data << "" << std::endl;
    });

    bool stop = false;
    std::thread thd1([&client, &stop] {
        while (true) {
            try {
                if (client.has_connected()) {
                    int r = client.call<int>("add", 2, 3);
                    std::cout << "add result: " << r << "" << std::endl;
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            } catch (const std::exception& ex) {
                std::cout << ex.what() << "" << std::endl;
            }
        }
    });

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

void test_threads() {
    std::cout << "test_threads start!" << std::endl;

    rpc_client client;
    bool r = client.connect("127.0.0.1", 9000);
    if (!r) {
        return;
    }

    size_t scale = 1000;
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
    std::cout << "father thread finished!" << std::endl;

    std::thread thd1([scale, &client] {
        for (size_t i = scale; i < 2*scale; i++) {
            auto future = client.async_call<FUTURE>("echo", "test");
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
    
    std::thread thd2([scale, &client] {
        for (size_t i = 2*scale; i < 3*scale; i++) {
            client.async_call("get_int", [i](boost::system::error_code ec, string_view data) {
                if (ec) {
                    std::cout << "error code: " << ec.value()
                              << ", error msg: "<< ec.message() << std::endl;
                    return;
                }
                int r = as<int>(data);
                if (r != i) {
                    std::cout << "error not match" << std::endl;
                }
            }, i);
            std::this_thread::sleep_for(std::chrono::microseconds(2));
        }
        std::cout << "child thread 2 finished!" << std::endl;
    });

    std::getchar();
    std::cout << "test_threads finished!" << std::endl;
}

void test_multiple_thread() {
    std::cout << "test_multiple_thread start!" << std::endl;

    std::vector<std::shared_ptr<rpc_client>> cls;
    std::vector<std::shared_ptr<std::thread>> v;
    for (int j = 0; j < 4; ++j) {
        auto client = std::make_shared<rpc_client>();
        cls.push_back(client);
        bool r = client->connect("127.0.0.1", 9000);
        if (!r) {
            return;
        }

        for (size_t i = 0; i < 2; i++) {
            person p{ 1, "tom", 20 };
            v.emplace_back(std::make_shared<std::thread>([client] {
                person p{ 1, "tom", 20 };
                for (size_t i = 0; i < 10000; i++) {
                    client->async_call<0>("get_name",
                        [](boost::system::error_code ec, string_view data) {
                            if (ec) {
                                std::cout << "error code: " << ec.value()
                                          << ", error msg: "<< ec.message() << std::endl;
                            }
                        }, p);

                    //auto future = client->async_call<FUTURE>("get_name", p);
                    //auto status = future.wait_for(std::chrono::seconds(2));
                    //if (status == std::future_status::deferred) {
                    //    std::cout << "deferred" << std::endl;
                    //}
                    //else if (status == std::future_status::timeout) {
                    //    std::cout << "timeout" << std::endl;
                    //}
                    //else if (status == std::future_status::ready) {
                    //}

                    //client->call<std::string>("get_name", p);
                }
            }));
        }
    }

    std::getchar();
    std::cout << "test_multiple_thread finished!" << std::endl;
}

void test_benchmark(){
    std::cout << "test_benchmark start!" << std::endl;

    rpc_client client;
    bool r = client.connect("127.0.0.1", 9000);
    if (!r) {
        return;
    }

    for (size_t i = 0; i < 1000000; i++) {
        client.async_call<5000>("echo",
            [i](boost::system::error_code ec, string_view data) {
                if (ec) {
                    std::cout << "error code: " << ec.value()
                              << ", error msg: "<< ec.message() << std::endl;
                } else {
                    std::cout << "non error: " << i << std::endl;
                }
        }, "hello wolrd");
    }

    // client.run();

    std::getchar();
    std::cout << "test_benchmark finished!" << std::endl;
}