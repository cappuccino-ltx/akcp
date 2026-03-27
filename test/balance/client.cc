
#include <chrono>
#include <client.hh>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ios>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <iomanip>
#include <fstream>
#include "buffer_pool.hh"
#include "common.hh"
#include <time.hh>

// arg
std::string ip = "127.0.0.1";
int port = 8080;
int short_test_times = 60 * 10; // 10分钟
int long_test_times = 60 * 60 * 14; // 14小时
int test_client = 1000; // 1000
int interval = 60;
int tick_interval = 1000 / 60; // send message time interval
std::string message;

// thread local result
thread_local uint64_t request_count_local {0};
thread_local uint64_t response_count_local {0};
thread_local uint64_t connect_count_local {0};
thread_local std::vector<size_t> p1000_local(1000,1);

struct result{

    result(const std::string& filename = "./result.csv"){
        out.open(filename);
        if (out.is_open() == false) {
            return;
        }
        out << "client,bps(MB/s),pps(k/s),loss(all),p50(ms),p99(ms),p999(ms)\n" << std::fixed << std::setprecision(3) ;
    }
    void push(int client, double bps, double pps, int loss, int p50, int p99, int p999){
        out << client << "," << bps << "," << pps << "," << loss << "," << p50 << "," << p99 << "," << p999 << "\n";
    }
    ~result(){
        out.close();
    }
    std::ofstream out;
} result_;

kcp::packet get_buffer(size_t size){
    return buffer_pool::get_buffer<uint8_t>(size);
}

void thread_quie_hook(int time){
    // 在这里进行数据的写入
    // summary
    int loss = request_count_local - response_count_local;
    double pps = (request_count_local + response_count_local) / 1000.0 / time;
    double bps =  (request_count_local + response_count_local) * message.size() / 1024.0 / 1024.0 / time;
    int p50 = 0;
    int p99 = 0;
    int p999 = 0;

    size_t count = 0;
    for (int i = 0; i < 1000; i++) {
        count += p1000_local[i];
        if (p50 == 0 && count >= response_count_local * 0.5){
            p50 = i + 1;
        }
        if (p99 == 0 && count >= response_count_local * 0.99) {
            p99 = i + 1;
        }
        if (p999 == 0 && count >= response_count_local * 0.999) {
            p999 = i + 1;
        }
    }
    result_.push(test_client, bps, pps, loss, p50, p99, p999);
    
}
struct Client{
    std::shared_ptr<kcp::client> client;
    std::atomic_bool stop{false };
    Client(int client_n, int time,int core){
        client = std::make_shared<kcp::client>(core);
        client->set_buffer_pool(get_buffer);
        client->set_thread_quit_callback(std::bind(thread_quie_hook,time));
        // client->disable_low_latency();

        client->set_connect_callback(std::bind(&Client::on_connect,this,kcp::_1, kcp::_2));
        client->set_message_callback(std::bind(&Client::on_message,this,kcp::_1, kcp::_2));
    }
    ~Client(){
        client->stop();
    }
    void send_message(kcp::channel_view channel){
        
        if (stop.load(std::memory_order_acquire)) {
            channel->disconnect();
            return;
        }
        kcp::packet pack = get_buffer(128);
        pack->resize(128,0);
        *((uint64_t*)(pack->data())) = kcp::util::time::clock_64();
        channel->send(pack);
        ++request_count_local;
        auto back = std::bind(&Client::send_message,this, channel);
        channel->timer_task(back, tick_interval);
    }

    void on_connect(kcp::channel_view channel, bool linked){
        if(linked) {
            ++connect_count_local;
            // std::cout << "连接成功" << std::endl;
            send_message(channel);
        }else {
            // std::cout << "连接断开" << std::endl;
        }
    }

    void on_message(kcp::channel_view channel, kcp::packet packet){
        // std::cout << "收到消息 : " << (const char*)packet->data() << std::endl;
        if(packet->size() == message.size()) {
            ++response_count_local;
            int delay = kcp::util::time::clock_64() - *((uint64_t*)(packet->data()));
            if (delay >= 0 && delay < 1000) {
                p1000_local[delay]++;
            }else {
                std::cout << "delay error : " << delay << std::endl;
            }
        }else {
            std::cout << "packet size != message size" << std::endl;
        }
    }
};
void test(int client_n , int time, int core){

    Client client(client_n,time, core);
    auto next = std::chrono::steady_clock::now();
    auto end = next + std::chrono::seconds(time);
    for (int j = 0;j < client_n; j++){
        client.client->connect(ip, port,true);
    }
    std::this_thread::sleep_until(end);
    client.stop = true;
}


int main(int argc, char* args[]) {
    message.resize(128,' ');
    buffer_pool::init_pool<uint8_t>(3000,128,2000);
    
    std::thread t1([](){
        test(test_client, long_test_times, 1);
    });
    std::thread t2([](){
        test(test_client, long_test_times, 2);
    });
    std::thread t3([](){
        test(test_client, long_test_times, 3);
    });
    std::thread t4([](){
        int n = long_test_times / short_test_times;
        for (int i = 0; i < n; i++) {
            test(test_client, short_test_times, 4);
        }
    });
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    return 0;
}