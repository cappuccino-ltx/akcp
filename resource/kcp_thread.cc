
#include "kcp_thread.hh"
#include "address.hh"
#include "common.hh"
#include "io_socket.hh"
#include "protocol.hh"
#include <cstdint>
#include <functional>
#include <future>
#include <thread>

namespace kcp{


kcp_thread::kcp_thread(std::atomic<bool>& stop)
    :stop_(stop)
    // ,thread_(std::make_shared<std::thread>(std::bind(&kcp_thread::handler, this)))
{}
kcp_thread::~kcp_thread(){
    if(thread_){
        thread_->join();
    }
}

void kcp_thread::stop(){
    if (manager_) {
        manager_->stop();
    }
    return ;
}
void kcp_thread::set_connect_callback(const std::function<void(channel_view,bool)>& callback){
    connect_callback_ = callback;
    if(manager_) {
        manager_->set_connect_callback(connect_callback_);
    }
    return ;
}
void kcp_thread::set_message_callback(const std::function<void(channel_view,packet)>& callback){
    message_callback_ = callback;
    if (manager_) {
        manager_->set_message_callback(message_callback_);
    }
    return ;
}

void kcp_thread::set_thread_quit_callback(const std::function<void()>& back) {
    thread_quit_callback_ = back;
}
void kcp_thread::set_thread_start_callback(const std::function<void()>& back) {
    thread_start_callback_ = back;
}
void kcp_thread::start(int port, int core){
    std::promise<bool> is_start;
    auto future = is_start.get_future();
    thread_ = std::make_shared<std::thread>(std::bind(
        &kcp_thread::handler, 
        this, 
        std::ref(is_start),
        std::ref(wait_end),
        core
    ));
    future.get();
    
    if (port < 0) {
        loop_ = std::make_shared<io_loop>(manager_->context_, std::ref(stop_));
        // remove 
        manager_->container_.remove_channel_socket_callback_ = std::bind(&io_loop::remove_client_socket,loop_.get(),kcp::_1);
    }else {
        loop_ = std::make_shared<io_loop>(manager_->context_, std::ref(stop_), port);
    }
    // manager -> socket
    // sync
    manager_->set_send_callback(io_socket::send_message_callback);
    // async
    manager_->set_async_send_callback(io_socket::async_send_message_callback);
    // socket -> manager
    loop_->set_receive_callback(kcp_thread::receive_callback, this);
    // manager -> user
    // connnect
    manager_->set_connect_callback(connect_callback_);
    // message
    manager_->set_message_callback(message_callback_);
    
    loop_->start();
    return ;
}
void kcp_thread::connect(const std::string& ip, int port, bool is_sole_socket){
    if(manager_->whthin_the_current_thread()) {
        connect_internal(ip, port, is_sole_socket);
    }else {
        manager_->post(std::bind(
            &kcp_thread::connect_internal,
            this,
            ip,
            port,
            is_sole_socket
        ));
    }
    return ;
}
void kcp_thread::close_wait(){
    wait_end.get_future().get();
}
void kcp_thread::connect_internal(const std::string& ip, int port, bool is_sole_socket){
    udp::resolver res(*manager_->context_);
    udp::endpoint point(*res.resolve(ip,std::to_string(port)).begin());
    if (is_sole_socket) {
        // create socket
        void* socket = loop_->create_new_client_socket(std::ref(stop_));
        io_loop::send_message_internal(socket, point, (void*)KCP_CONNECT_REQUEST, KCP_PACKAGE_SIZE);
        add_connect(socket, ip, port);
    }else {
        io_loop::send_message_internal(loop_->get_default_sokcet(), point, (void*)KCP_CONNECT_REQUEST, KCP_PACKAGE_SIZE);
        add_connect(loop_->get_default_sokcet(), ip, port);
    }
    return ;
}
void kcp_thread::reconnect(void* socket, const std::string& host){
    std::string ip;
    std::string port;
    util::address::string_to_host(host, &ip, &port);

    udp::resolver res(*manager_->context_);
    udp::endpoint point(*res.resolve(ip,port).begin());
    loop_->send_message_internal(socket, point, (void*)KCP_CONNECT_REQUEST, KCP_PACKAGE_SIZE);
    return ;
}
void kcp_thread::receive_callback(void* self, void* socket, const udp::endpoint& point,const char* data,size_t size){
    kcp_thread* self_ = static_cast<kcp_thread*>(self);
    if (size == KCP_PACKAGE_SIZE) {
        // server
        if (strcmp(data,KCP_CONNECT_REQUEST) == 0){
            uint32_t new_conv = context::generate_conv_global();
            channel_manager::push_half_link_callback(self_->manager_.get(),new_conv,point);
            std::string send_data = protocol::format_connect_response(new_conv);
            io_loop::send_message_internal(socket,point,(void*)send_data.data(),KCP_PACKAGE_SIZE);
            return ;
        }
        // client and server create channel and client ack
        uint32_t conv = protocol::parse_conv_from_response(data);
        if (conv != 0) {
            if (data[0] == 1 && data[1] == 2 && data[12] == 3 && data[13] == 4) {
                // client ack
                std::string send_data = protocol::format_connect_response_ack(conv);
                io_loop::send_message_internal(socket, point,(void*)send_data.data(),KCP_PACKAGE_SIZE);
                // remove timer
                self_->remove_connect(point);
            }
            // else if (data[0] == 4 && data[1] == 3 && data[12] == 2 && data[13] == 1){
            //     // server
            // }
            channel_manager::push_new_link_callback(self_->manager_.get(),socket,conv,point);
            return;
        }
    }
    packet pack = std::make_shared<std::vector<uint8_t>>(data, data + size);
    channel_manager::push_package_callback(self_->manager_.get(),point, data, size);
    return ;
}

// thread execution function
void kcp_thread::handler(std::promise<bool>& start, std::promise<bool>& end, int core){
    manager_ = channel_manager::create(std::ref(stop_));
    start.set_value(true);
    if(thread_start_callback_){
        thread_start_callback_();
    }
#if defined(__linux__)
    while(thread_.get() == nullptr){
        std::this_thread::yield();
    }
    if (core >= 0) {
        bind_to_core(core);
    }
#endif
    manager_->context_->run();
    if (thread_quit_callback_){
        thread_quit_callback_();
    }
    end.set_value(true);
    return ;
}
#if defined (__linux__)
bool kcp_thread::bind_to_core(int cpu_id) {
    // 1. 获取原生 pthread 句柄
    pthread_t native_handle = thread_->native_handle();
    // 2. 初始化 cpu_set_t 掩码
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);      // 清空集合
    CPU_SET(cpu_id, &cpuset); // 将目标核心放入集合
    // 3. 调用系统接口进行绑定
    // np 代表 Non-Portable（非标准/平台相关），在 Linux 下通用
    int rc = pthread_setaffinity_np(native_handle, sizeof(cpu_set_t), &cpuset);
    if (rc != 0) {
        return false;
    }
    return true;
}
#endif
void kcp_thread::remove_connect(const udp::endpoint& point){
    std::string host;
    util::address::point_to_string(point, &host);
    auto it = connect_.find(host);
    if (it == connect_.end()) {
        return;
    }
    connect_.erase(it);
    return ;
}
void kcp_thread::add_connect(void* socket, const std::string& ip, int port){
    std::string host;
    util::address::host_to_string(ip, port, &host);
    connect_.insert({host, socket});
    add_connect_time_task(host,kcp_timewait(0));
    return ;
}
void kcp_thread::add_connect_time_task(const std::string& host, int timeout){
    manager_->timer_task(std::bind(&kcp_thread::connect_time_task,this,host,timeout), timeout);
    return ;
}
void kcp_thread::connect_time_task(const std::string& host, int timeout){
    // query whether the connection has been established 
    if(connect_.count(host) == 0) {
        // the connection has been established 
        return ;
    }
    // reconnection
    reconnect(connect_[host], host);

    // add timeout task
    timeout = kcp_timewait(timeout);
    if (timeout < 0) {
        return ;
    }
    add_connect_time_task(host, timeout);
    return ;
}

} // namespace kcp