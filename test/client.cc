


#include <chrono>
#include <client.hh>
#include <iostream>
#include <thread>

kcp::channel_view channel_global;

void on_connect(kcp::channel_view channel, bool linked){
    if(linked) {
        channel_global = channel;
        std::cout << "连接成功" << std::endl;
    }else {
        channel_global = nullptr;
        std::cout << "连接断开" << std::endl;
    }
}

void on_message(kcp::channel_view channel, kcp::packet packet){
    std::cout << "收到消息 : " << (const char*)packet->data() << std::endl;
}

int main() {

    kcp::client client;
    client.set_connect_callback(on_connect);
    client.set_message_callback(on_message);
    client.start();
    client.connect("127.0.0.1", 8080);
    int count = 10;
    std::string message = "hello!!!!";
    while(count -- ){
        if(channel_global){
            channel_global->send(message.data(),message.size());
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    channel_global->disconnect();
    // client.stop();
    return 0;
}