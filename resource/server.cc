

#include "server.hh"

namespace kcp{

server::server(){}

void server::set_connect_callback(const std::function<void(channel_view,bool)>& callback){
    connect_callback_ = callback;
}
void server::set_message_callback(const std::function<void(channel_view,packet)>& callback){
    message_callback_ = callback;
}

void server::enable_muliti_thread(int n){
    threads_.reserve(n);
    for(int i = 0; i < n; i++){
        threads_.emplace_back(std::ref(stop_),true,n,i);
    }
}
void server::set_update_interval(int val){
    channel_manager::set_update_interval(val);
}

void server::start(int port){
    if (threads_.size()){
        // muliti thread
        start_muliti(port);
    }else {
        // single thread
        start_single(port);
    }
}
void server::stop(){
    stop_.store(true,std::memory_order_release);
}
void server::start_single(int port){
    manager_ = std::make_shared<channel_manager>(std::ref(stop_));
    loop_ = std::make_shared<io_loop>(manager_->context_, std::ref(stop_), port);
    // manager -> socket
    manager_->set_send_callback(io_socket::send_message_callback, loop_->socket_.get());
    // socket -> manager
    loop_->set_receive_callback(server::receive_callback_single, this);
    // manager -> user
    // connnect
    manager_->set_connect_callback(connect_callback_);
    // message
    manager_->set_message_callback(message_callback_);
    // start receive
    manager_->hook_timer();
    loop_->start();
}
void server::start_muliti(int port){
    loop_ = std::make_shared<io_loop>(nullptr, std::ref(stop_), port);
    // socket -> manager
    loop_->set_receive_callback(server::receive_callback, this);
    for (int i = 0; i < threads_.size(); i++) {
        // manager -> socket
        threads_[i].get_manager()->set_async_send_callback(io_socket::async_send_message_callback, loop_->socket_.get());
        // manager -> user
        // connnect
        threads_[i].get_manager()->set_connect_callback(connect_callback_);
        // message
        threads_[i].get_manager()->set_message_callback(message_callback_);
    }
    // start loop
    loop_->start();
}

void server::receive_callback(void* self,const udp::endpoint& point,const char* data,size_t size){
    server* self_ = static_cast<server*>(self);
    size_t thread_n = self_->threads_.size();
    if (size == KCP_PACKAGE_SIZE) {
        if (strcmp(data,KCP_CONNECT_REQUEST) == 0){
            uint32_t new_conv = context::generate_conv_global();
            std::shared_ptr<link_data> new_link = std::make_shared<link_data>(new_conv,point);
            self_->threads_[new_conv % thread_n].manager_->push_new_link(new_link);
            std::string send_data = protocol::format_connect_response(new_conv);
            self_->loop_->send_message_internal(point,send_data.data(),KCP_PACKAGE_SIZE);
            return ;
        }
    }
    packet pack = std::make_shared<std::vector<uint8_t>>(data, data + size);
    std::shared_ptr<package> receive_data = std::make_shared<package>(point,pack);
    uint32_t conv = context::get_conv_from_packet(data);
    self_->threads_[conv % thread_n].manager_->push_package(receive_data);
}

void server::receive_callback_single(void* self,const udp::endpoint& point,const char* data,size_t size){
    server* self_ = static_cast<server*>(self);
    if (size == KCP_PACKAGE_SIZE) {
        if (strcmp(data,KCP_CONNECT_REQUEST) == 0){
            uint32_t new_conv = context::generate_conv_global();
            channel_manager::push_new_link_callback(self_->manager_.get(),new_conv,point);
            std::string send_data = protocol::format_connect_response(new_conv);
            self_->loop_->send_message_internal(point,send_data.data(),KCP_PACKAGE_SIZE);
            return ;
        }
    }
    packet pack = std::make_shared<std::vector<uint8_t>>(data, data + size);
    channel_manager::push_package_callback(self_->manager_.get(),point,data,size);
}

}; // namespace kcp