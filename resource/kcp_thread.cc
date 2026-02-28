
#include "kcp_thread.hh"
#include "address.hh"
#include "common.hh"
#include "number.hh"
#include "protocol.hh"
#include <chrono>
#include <functional>
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
void kcp_thread::start(int port, bool is_separate_thread){
    std::shared_ptr<channel_manager> manager;
    if (is_separate_thread) {
        thread_ = std::make_shared<std::thread>(std::bind(&kcp_thread::handler, this));
        manager = get_manager();
    }else {
        manager_ = std::make_shared<channel_manager>(std::ref(stop_));
        manager = manager_;
    }
    if (port < 0) {
        loop_ = std::make_shared<io_loop>(manager->context_, std::ref(stop_));
    }else {
        loop_ = std::make_shared<io_loop>(manager->context_, std::ref(stop_), port);
    }
    // manager -> socket
    manager->set_send_callback(io_socket::send_message_callback, loop_->socket_.get());
    // socket -> manager
    loop_->set_receive_callback(kcp_thread::receive_callback, this);
    // manager -> user
    // connnect
    manager->set_connect_callback(connect_callback_);
    // message
    manager->set_message_callback(message_callback_);
    loop_->start();
    if (is_separate_thread) {
        return ;
    }
    manager_->context_->run();
    return ;
}
void kcp_thread::connect(const std::string& host, int port){
    if(manager_->whthin_the_current_thread()) {
        connect_internal(host, port);
    }else {
        manager_->post(std::bind(
            &kcp_thread::connect_internal,
            this,
            host,
            port
        ));
    }
    return ;
}
void kcp_thread::connect_internal(const std::string& host, int port){
    udp::resolver res(*manager_->context_);
    udp::endpoint point(*res.resolve(host,std::to_string(port)).begin());
    loop_->send_message_internal(point, (void*)KCP_CONNECT_REQUEST, KCP_PACKAGE_SIZE);
    add_connect(host,port);
    return ;
}
void kcp_thread::receive_callback(void* self,const udp::endpoint& point,const char* data,size_t size){
    kcp_thread* self_ = static_cast<kcp_thread*>(self);
    if (size == KCP_PACKAGE_SIZE) {
        // server
        if (strcmp(data,KCP_CONNECT_REQUEST) == 0){
            uint32_t new_conv = context::generate_conv_global();
            channel_manager::push_half_link_callback(self_->manager_.get(),new_conv,point);
            std::string send_data = protocol::format_connect_response(new_conv);
            self_->loop_->send_message_internal(point,send_data.data(),KCP_PACKAGE_SIZE);
            return ;
        }
        // client and server create channel and client ack
        uint32_t conv = protocol::parse_conv_from_response(data);
        if (conv != 0) {
            if (data[0] == 1 && data[1] == 2 && data[12] == 3 && data[13] == 4) {
                // client ack
                std::string send_data = protocol::format_connect_response_ack(conv);
                self_->loop_->send_message_internal(point,send_data.data(),KCP_PACKAGE_SIZE);
                // remove timer
                self_->remove_connect(point);
            }
            // else if (data[0] == 4 && data[1] == 3 && data[12] == 2 && data[13] == 1){
            //     // server
            // }
            channel_manager::push_new_link_callback(self_->manager_.get(),conv,point);
            return;
        }
    }
    packet pack = std::make_shared<std::vector<uint8_t>>(data, data + size);
    channel_manager::push_package_callback(self_->manager_.get(),point,data,size);
    return ;
}

// thread execution function
void kcp_thread::handler(){
    manager_ = std::make_shared<channel_manager>(std::ref(stop_));
    manager_->context_->run();
    return ;
}
std::shared_ptr<channel_manager> kcp_thread::get_manager(){
    while(!manager_){
        std::this_thread::sleep_for(std::chrono::milliseconds(util::number::random(10, 20)));
    }
    return manager_;
}

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
void kcp_thread::add_connect(const std::string& ip, int port){
    std::string host;
    util::address::host_to_string(ip, port, &host);
    connect_.insert(host);
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
    timeout = kcp_timewait(timeout);
    if (timeout < 0) {
        return ;
    }
    add_connect_time_task(host, timeout);
    return ;
}

} // namespace kcp