


#include <atomic>
#include <server.hh>
#include <functional>
#include <iostream>
#include <thread>
kcp::server* server_global = nullptr;
std::atomic_bool threads[3] = {false, false, false};
std::thread::id tids[3];
int count[3];
std::atomic_int count_total = 100;

void add_connection() {
    for(int i = 0; i < 3; i++) {
        if (!threads[i]) {
            threads[i] = true;
            tids[i] = std::this_thread::get_id();
            count[i] ++;
            break ;
        } else if (tids[i] == std::this_thread::get_id()) {
            count[i]++;
            break ;
        }
    }
    count_total--;
    if (!count_total) {
        server_global->stop();
    }
}


void on_connect(kcp::channel_view channel, bool linked){
    if(linked) {
        add_connection();
        std::cout << "[" << std::this_thread::get_id() << "]连接到来" << std::endl;
    }else {
        std::cout << "[" << std::this_thread::get_id() << "]连接断开" << std::endl;
    }
}

void on_message(kcp::channel_view channel, kcp::packet packet){
    // std::cout << "收到消息 : " << (const char*)packet->data() << std::endl; 
    channel->send(std::move(*packet));
}


int main() {
    kcp::server server;
    server_global = &server;
    server.set_connect_callback(on_connect);
    server.set_message_callback(on_message);
    server.enable_muliti_thread(3);
    server.start(8080);
    std::cout << "thread[0] : " << count[0] << std::endl;
    std::cout << "thread[1] : " << count[1] << std::endl;
    std::cout << "thread[2] : " << count[2] << std::endl;
    return 0;    
}