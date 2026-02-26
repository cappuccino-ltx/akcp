
#include <chrono>
#include <client.hh>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <jsoncpp/json/writer.h>
#include <list>
#include <thread>
#include <jsoncpp/json/json.h>
#include <fstream>

// arg
std::string ip ;
int port;
int clients = 1;
int times = 10;
int interval = 60;
std::string message;

// result
std::atomic_int64_t request_count = 0;
std::atomic_int64_t packet_loss = 0;
std::atomic_int64_t connect_count = 0;

void on_connect(kcp::channel_view* user_channel, std::atomic_bool* is_linked,kcp::channel_view channel, bool linked){
    if(linked) {
        connect_count++;
        *is_linked = true;
        *user_channel = channel;
        std::cout << "连接成功" << std::endl;
    }else {
        std::cout << "连接断开" << std::endl;
    }
}

void on_message(size_t* count, kcp::channel_view channel, kcp::packet packet){
    // std::cout << "收到消息 : " << (const char*)packet->data() << std::endl;
    // if(packet->size() == message.size() && (strcmp((char*)packet->data(), message.c_str()) == 0)) {
    //     ++*count;
    // }
    ++*count;
}

void client_test(){
    kcp::client client;
    kcp::channel_view channel;
    std::atomic_bool is_linked = false;
    size_t echo_count = 0;
    size_t send_count = 0;

    auto connection_callback = std::bind(on_connect,&channel,&is_linked,kcp::_1,kcp::_2);
    auto message_callback = std::bind(on_message,&echo_count,kcp::_1,kcp::_2);
    client.set_connect_callback(connection_callback);
    client.set_message_callback(message_callback);

    client.connect(ip, port);
    
    float per_tick = interval / 1000.0f;
    float acc = 0.0f;

    int tick = 1000 / interval;
    if (interval > 1000) {
        tick = 1;
    }
    while(!is_linked.load()){
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    auto next = std::chrono::steady_clock::now();
    auto end = next + std::chrono::seconds(times);
    while (next <= end) {
        int send_n = 1;
        if (per_tick > 1) {
            acc += per_tick;
            send_n = (int)acc;
            acc -= send_n;
        }
        // send
        for (int i = 0; i < send_n; i++) {
            send_count++;
            channel->send(message.data(),message.size());
        }

        next += std::chrono::milliseconds(tick);
        std::this_thread::sleep_until(next);
    }
    channel->disconnect();
    client.stop();
    // wait disconnection
    std::this_thread::sleep_for(std::chrono::milliseconds(KCP_RTT));
    // summary
    request_count += send_count;
    packet_loss += send_count - echo_count;
}


void test(){
    std::list<std::thread> threads;
    for (int i = 0; i < clients; i++){
        threads.emplace_back(client_test);
    }
    for(auto & t : threads){
        t.join();
    }
    threads.clear();
}

void result(){
    Json::Value root;
    root["test time"] = times;
    root["client number"] = clients;
    root["request rate"] = interval;
    root["total speed"] = interval * clients;
    root["package size"] = message.size();
    root["result"]["pps"]["server rx(/s)"] = request_count / times;
    root["result"]["pps"]["server tx(/s)"] = (request_count - packet_loss) / times;
    root["result"]["pps"]["total(/s)"] = (request_count * 2 - packet_loss) / times;
    root["result"]["bps"]["server rx(b/s)"] = request_count * message.size() / times ;// / 1024 / 1024;
    root["result"]["bps"]["server tx(b/s)"] = (request_count - packet_loss) * message.size() / times;// / 1024 / 1024;
    root["result"]["bps"]["total(/s)"] = (request_count * 2 - packet_loss) * message.size() / times;// / 1024 / 1024;
    root["result"]["loss"] = packet_loss.load();
    // Manual filling
    root["server"]["cpu avg(%)"] = 0;
    root["server"]["cpu max(%)"] = 0;
    root["server"]["memory avg(mb)"] = 0;
    root["server"]["memory max(mb)"] = 0;
    // 写入文件
    std::ofstream ofs("./result.json");
    auto writer = Json::StreamWriterBuilder().newStreamWriter();
    writer->write(root, &ofs);
    delete writer;
    ofs.close();
}


int main(int argc, char* args[]) {
    if (argc < 3) {
        std::cout << "usage : " << args[0] << " ip port clients timer interval package_size" << std::endl;
        std::cout << "for example : " << args[0] << " 127.0.0.1 8080 100 30 60 128" << std::endl;
        std::cout << "The meaning is that 100 clients continuously request for 30 seconds, 60 requests per second, size of the package is 128bytes." << std::endl;
        return 1;
    }
    ip = args[1];
    port = atoi(args[2]);
    if (argc >= 4) clients = atoi(args[3]);
    if (argc >= 5) times = atoi(args[4]);
    if (argc >= 6) interval = atoi(args[5]);
    if (argc >= 7) message.resize(atoi(args[6]),' ');
    else message.resize(128,' ');
    // start test
    test();
    // print result 
    result();
    return 0;
}