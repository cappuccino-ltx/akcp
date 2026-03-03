


#include <server.hh>
#include <iostream>
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
    int port = std::stoi(args[1]);
    kcp::server server;
    server_global = &server;
    server.set_connect_callback(on_connect);
    server.set_message_callback(on_message);
    server.enable_muliti_thread();
    server.start(port);
    return 0;    
}