


#include <cstdint>
#include <server.hh>
#include <iostream>
#include "buffer_pool.hh"

kcp::packet get_buffer(size_t size){
    return buffer_pool::get_buffer<uint8_t>(size);
}

kcp::server* server_global = nullptr;
void on_connect(kcp::channel_view channel, bool linked){
    if(linked) {
        // std::cout << "[" << std::this_thread::get_id() << "]连接到来" << std::endl;
    }else {
        // std::cout << "[" << std::this_thread::get_id() << "]连接断开" << std::endl;
        // server_global->stop();
    }
}

void on_message(kcp::channel_view channel, kcp::packet packet){
    // std::cout << "收到消息 : " << (const char*)packet->data() << std::endl; 
    channel->send(std::move(*packet));
}


int main(int argc , char* args[]) {
    if (argc != 2) {
        std::cout << "usage : " << args[0] << " port " << std::endl;
        return 1;
    }
    // 初始化 buffer poll
    buffer_pool::init_pool<uint8_t>(2000,100,2000);
    int port = std::stoi(args[1]);
    kcp::server server;
    server_global = &server;
    server.set_connect_callback(on_connect);
    server.set_message_callback(on_message);
    server.set_buffer_pool(get_buffer);
    server.enable_muliti_thread(5);
    server.disable_low_latency();
    server.start(port,7, 11);
    return 0;    
}