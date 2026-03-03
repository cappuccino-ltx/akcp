


#include <chrono>
#include <client.hh>
#include <iostream>
#include <list>
#include <thread>
#include <unistd.h>

std::atomic<int> count = 0;

void on_connect(kcp::channel_view channel, bool linked){
    if(linked) {
        std::cout << "连接成功" << std::endl;
        std::string message = "hello!!!!";
        channel->send(message.data(),message.size());
    }else {
        std::cout << "连接断开" << std::endl;
    }
}

void on_message(kcp::channel_view channel, kcp::packet packet){
    std::cout << "收到消息 : " << (const char*)packet->data() << std::endl;
    channel->disconnect();
    count++;
}

int main() {

    std::list<kcp::client> clients;
    for(int i = 0; i < 100; i++) {
        clients.emplace_back();
        clients.back().set_connect_callback(on_connect);
        clients.back().set_message_callback(on_message);
    }
    for(auto& client: clients) {
        client.connect("127.0.0.1", 8080);
    }
    if(count != 100) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    } 

    for(auto& client: clients) {
        client.stop();
    }
    clients.clear();
    return 0;
}