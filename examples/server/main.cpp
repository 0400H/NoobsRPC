#include <rest_rpc.hpp>
using namespace rest_rpc;
using namespace rpc_service;

#include <fstream>
#include <chrono>

#include "event.h"

#define __RESTRPC_VERBOSE__

event g_event;

std::string echo(rpc_conn conn, const std::string& src) {
#ifdef __RESTRPC_VERBOSE__
    std::cout << "func: " << __FUNCTION__ << std::endl;
#endif
    g_event.increase();
    return src;
}

// if you want to response later, you can use async model, you can control when to response
void async_echo(rpc_conn conn, const std::string& src) {
#ifdef __RESTRPC_VERBOSE__
    std::cout << "func: " << __FUNCTION__ << std::endl;
#endif
    g_event.increase();
    // note: you need keep the request id at that time, and pass it into the async thread
    auto req_id = conn.lock()->request_id();
    
    std::thread thd([conn, req_id, src] {
        // std::this_thread::sleep_for(std::chrono::seconds(1));
        auto conn_sp = conn.lock();
        if (conn_sp) {
            conn_sp->pack_and_response(req_id, std::move(src));
        }
    });
    thd.detach();
}

struct dummy {
    int add(rpc_conn conn, int a, int b) {
#ifdef __RESTRPC_VERBOSE__
    std::cout << "func: " << __FUNCTION__ << std::endl;
#endif
        g_event.increase();
        return a + b;
    }
};

struct person {
    int id;
    std::string name;
    int age;

    MSGPACK_DEFINE(id, name, age);
};

person get_person(rpc_conn conn) {
#ifdef __RESTRPC_VERBOSE__
    std::cout << "func: " << __FUNCTION__ << std::endl;
#endif
    g_event.increase();
    return { 1, "tom", 20 };
}

std::string get_person_name(rpc_conn conn, const person& p) {
#ifdef __RESTRPC_VERBOSE__
    std::cout << "func: " << __FUNCTION__ << std::endl;
#endif
    g_event.increase();
    return p.name;
}

void upload(rpc_conn conn, const std::string& filename, const std::string& content) {
#ifdef __RESTRPC_VERBOSE__
    std::cout << "func: " << __FUNCTION__ << std::endl;
#endif
    g_event.increase();
    std::cout << content.size() << std::endl;
    std::ofstream file(filename, std::ios::binary);
    file.write(content.data(), content.size());
}

std::string download(rpc_conn conn, const std::string& filename) {
#ifdef __RESTRPC_VERBOSE__
    std::cout << "func: " << __FUNCTION__ << std::endl;
#endif
    g_event.increase();
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        return "";
    }

    file.seekg(0, std::ios::end);
    size_t file_len = file.tellg();
    file.seekg(0, std::ios::beg);
    std::string content;
    content.resize(file_len);
    file.read(&content[0], file_len);
    std::cout << file_len << std::endl;

    return content;
}

double get_latency(rpc_conn conn, const std::chrono::system_clock::time_point& start) {
#ifdef __RESTRPC_VERBOSE__
    std::cout << "func: " << __FUNCTION__ << std::endl;
#endif
    g_event.increase();
    auto stop = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    double time_ms = 1000 * double(duration.count()) 
                   * std::chrono::microseconds::period::num
                   / std::chrono::microseconds::period::den;
    return time_ms;
}


void start_server(bool with_ssl=true) {
    rpc_server* server = nullptr;
    if (with_ssl) {
        server = new rpc_server(9000, std::thread::hardware_concurrency(), { "server->crt", "server->key" });
    } else {
        server = new rpc_server(9000, std::thread::hardware_concurrency());
    }

    dummy d;
    person p = { 1, "tom", 20 };

    server->register_handler("echo", echo);
    server->register_handler<Async>("async_echo", async_echo);
    server->register_handler("add", &dummy::add, &d);
    server->register_handler("get_person", get_person);
    server->register_handler("get_person_name", get_person_name);
    server->register_handler("upload", upload);
    server->register_handler("download", download);
    server->register_handler("get_latency", get_latency);
    server->register_handler("publish", [&server](rpc_conn conn, std::string key, std::string token, std::string val) {
        server->publish(std::move(key), std::move(val));
    });
    server->register_handler("publish_by_token", [&server](rpc_conn conn, std::string key, std::string token, std::string val) {
        server->publish_by_token(std::move(key), std::move(token), std::move(val));
    });

    std::thread thd([&server, &p] {
        while (true) {
            server->publish("key", "hello subscriber");
            auto list = server->get_token_list();
            for (auto& token : list) {                
                server->publish_by_token("key", token, p);
                server->publish_by_token("key1", token, "hello subscriber1");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });

    server->run();
}

int main() {
    start_server(false);
    return 0;
}