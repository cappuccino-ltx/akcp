


#include <server.hh>
#include <functional>
#include <iostream>

void on_connect(kcp::channel_view channel, bool linked){
    if(linked) {
        std::cout << "连接到来" << std::endl;
    }else {
        std::cout << "连接断开" << std::endl;
    }
}

void on_message(kcp::channel_view channel, kcp::packet packet){
    std::cout << "收到消息 : " << (const char*)packet->data() << std::endl; 
    channel->send(std::move(*packet));
}


int main() {
    kcp::server server;
    server.set_connect_callback(on_connect);
    server.set_message_callback(on_message);
    server.start(8080);
    return 0;    
}