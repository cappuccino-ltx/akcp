
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
std::string ip ;
int port;
int times = 30;
int interval = 60;
std::string message;

// test arg
std::atomic_bool stop{false};
int tick_interval = 20; // send message time interval

// global result
std::atomic_int64_t request_count{ 0};
std::atomic_int64_t response_count{ 0};
std::atomic_int connect_count { 0};
std::vector<size_t> p1000;
std::mutex global_mtx;

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
        std::cout << "client,bps(MB/s),pps(k/s),loss(all),p50(ms),p99(ms),p999(ms)\n" << std::fixed << std::setprecision(3) ;
    }
    void push(int client, double bps, double pps, int loss, int p50, int p99, int p999){
        out << client << "," << bps << "," << pps << "," << loss << "," << p50 << "," << p99 << "," << p999 << "\n";
        std::cout << client << "," << bps << "," << pps << "," << loss << "," << p50 << "," << p99 << "," << p999 << "\n";
    }
    ~result(){
        out.close();
    }
    std::ofstream out;
} result_;

kcp::packet get_buffer(size_t size){
    return buffer_pool::get_buffer<uint8_t>(size);
}

void thread_quie_hook(){
    // 在这里进行数据的写入
    {
        request_count += request_count_local;
        response_count += response_count_local;
        connect_count += connect_count_local;
        std::unique_lock<std::mutex> lock(global_mtx);
        for (int i = 0; i < 1000; i++) {
            p1000[i] += p1000_local[i];
        }
    }
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
    auto back = std::bind(send_message, channel);
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

void test(int client ){
    // 数据的清理
    request_count = 0;
    response_count = 0;
    connect_count = 0;
    p1000.clear();
    p1000.resize(1000,0);

    int thread_num = 7;
    int core_start = 0;
    int one_thread_client = client / thread_num;
    std::vector<std::shared_ptr<kcp::client>> clients;
    for (int i = 0; i < thread_num; i++) {
        clients.push_back(std::make_shared<kcp::client>(core_start + i));
        clients[i]->set_buffer_pool(get_buffer);
        clients[i]->set_thread_quit_callback(thread_quie_hook);
        // clients[i]->disable_low_latency();
    }
    tick_interval = 1000 / interval;
    if (interval > 1000) {
        tick_interval = 1;
    }

    for (int i = 0; i < thread_num; i++) {
        clients[i]->set_connect_callback(on_connect);
        clients[i]->set_message_callback(on_message);
    }
    stop = false;
    for (int i = 0; i < thread_num; i++) {
        int current_thread_client_n = 0;
        if (i == thread_num - 1) {
            current_thread_client_n = client - one_thread_client * (thread_num - 1);
        }else {
            current_thread_client_n = one_thread_client;
        }
        for (int j = 0;j < current_thread_client_n; j++){
            clients[i]->connect(ip, port,true);
        }
    }
    auto next = std::chrono::steady_clock::now();
    auto end = next + std::chrono::seconds(times);
    std::this_thread::sleep_until(end);
    stop = true;
    for (int i = 0; i < thread_num; i++) {
        clients[i]->stop();
    }
    // summary
    int loss = request_count - response_count;
    double pps = (request_count + response_count) / 1000.0 / times;
    double bps =  (request_count + response_count) * message.size() / 1024.0 / 1024.0 / times;
    int p50 = 0;
    int p99 = 0;
    int p999 = 0;

    size_t count = 0;
    for (int i = 0; i < 1000; i++) {
        count += p1000[i];
        if (p50 == 0 && count >= response_count * 0.5){
            p50 = i + 1;
        }
        if (p99 == 0 && count >= response_count * 0.99) {
            p99 = i + 1;
        }
        if (p999 == 0 && count >= response_count * 0.999) {
            p999 = i + 1;
        }
    }
    result_.push(client, bps, pps, loss, p50, p99, p999);
}


int main(int argc, char* args[]) {
    if (argc < 3) {
        std::cout << "usage : " << args[0] << " ip port clients" << std::endl;
        std::cout << "for example : " << args[0] << " 127.0.0.1 8080 100" << std::endl;
        std::cout << "The meaning is that 100 clients continuously request for 30 seconds, 60 requests per second, size of the package is 128bytes." << std::endl;
        return 1;
    }
    ip = args[1];
    port = atoi(args[2]);

    message.resize(128,' ');
    buffer_pool::init_pool<uint8_t>(1000,128,2000);
    // start test
    for(int i = 1000; i <= 11000; i += 1000) {
        test(i);
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    return 0;
}