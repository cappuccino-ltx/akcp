


#include <chrono>
#include <client.hh>
#include <iostream>
#include <list>
#include <thread>

std::atomic<int> count {0};

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
    int n = 100;
    kcp::client client;
    client.set_connect_callback(on_connect);
    client.set_message_callback(on_message);
    for(int i = 0; i < n; i++) {
        // 第三个参数为 true , 代表创建不同的套接字发起请求
        client.connect("127.0.0.1", 8080, true); 
    }
    while(count != n) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    } 
    client.stop();
    
    return 0;
}
