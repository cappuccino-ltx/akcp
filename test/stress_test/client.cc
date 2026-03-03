
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
int client_num = 1;
int times = 10;
int interval = 60;
std::string message;

// test arg
std::atomic_bool stop = false;
int tick_interval = 20; // send message time interval

// result
std::atomic_int64_t request_count = 0;
std::atomic_int64_t response_count = 0;
std::atomic_int64_t packet_loss = 0;
std::atomic_int connect_count = 0;

void send_message(kcp::channel_view channel){
    ++request_count;
    channel->send(message.c_str(),message.size());
    if (stop.load(std::memory_order_acquire)) {
        channel->disconnect();
        return;
    }
    auto back = std::bind(send_message, channel);
    channel->timer_task(back, tick_interval);
}

void on_connect(kcp::channel_view channel, bool linked){
    if(linked) {
        ++connect_count;
        // std::cout << "连接成功" << std::endl;
        send_message(channel);
    }else {
        // std::cout << "连接断开" << std::endl;
    }
}

void on_message(kcp::channel_view channel, kcp::packet packet){
    // std::cout << "收到消息 : " << (const char*)packet->data() << std::endl;
    // if(packet->size() == message.size() && (strcmp((char*)packet->data(), message.c_str()) == 0)) {
        ++response_count;
    // }
}

void test(){
    std::cout << "test preparation ..." << std::endl;
    // std::list<kcp::client> clients;
    kcp::client* clients = new kcp::client[client_num];
    tick_interval = 1000 / interval;
    if (interval > 1000) {
        tick_interval = 1;
    }

    for (int i = 0; i < client_num; i++) {
        clients[i].set_connect_callback(on_connect);
        clients[i].set_message_callback(on_message);
    }
    std::cout << "test begin" << std::endl;
    for (int i = 0; i < client_num; i++) {
        clients[i].connect(ip, port);
    }
    auto next = std::chrono::steady_clock::now();
    auto end = next + std::chrono::seconds(times);
    std::this_thread::sleep_until(end);
    stop = true;
    for (int i = 0; i < client_num; i++) {
        clients[i].stop();
    }
    std::cout << "test end" << std::endl;
    // wait disconnection
    std::this_thread::sleep_for(std::chrono::milliseconds(KCP_RTT));
    // summary
    packet_loss = request_count - response_count;
}



void result(){
    Json::Value root;
    root["1.settings"]["test time"] = times;
    root["1.settings"]["client number(preset)"] = client_num;
    root["1.settings"]["client number(success)"] = connect_count.load();
    root["1.settings"]["request rate(/s)"] = interval;
    root["1.settings"]["total speed"] = interval * connect_count.load();
    root["1.settings"]["package size"] = message.size();
    root["2.client result"]["pps"]["server rx(/s)"] = request_count / times;
    root["2.client result"]["pps"]["server tx(/s)"] = (request_count - packet_loss) / times;
    root["2.client result"]["pps"]["total(/s)"] = (request_count * 2 - packet_loss) / times;
    root["2.client result"]["bps"]["server rx(b/s)"] = request_count * message.size() / times ;// / 1024 / 1024;
    root["2.client result"]["bps"]["server tx(b/s)"] = (request_count - packet_loss) * message.size() / times;// / 1024 / 1024;
    root["2.client result"]["bps"]["total(/s)"] = (request_count * 2 - packet_loss) * message.size() / times;// / 1024 / 1024;
    root["2.client result"]["loss"] = packet_loss.load();
    // Manual filling
    root["3.server"]["cpu avg(%)"] = 0;
    root["3.server"]["cpu max(%)"] = 0;
    root["3.server"]["memory avg(mb)"] = 0;
    root["3.server"]["memory max(mb)"] = 0;
    // 写入文件
    std::ofstream ofs("./result.json");
    auto writer = Json::StreamWriterBuilder().newStreamWriter();
    writer->write(root, &ofs);
    writer->write(root, &std::cout);
    std::cout << std::endl;
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
    if (argc >= 4) client_num = atoi(args[3]);
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