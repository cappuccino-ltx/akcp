

#include "server.hh"
#include "common.hh"
#include "connection.hh"
#include "packet.hh"
#include <functional>

namespace kcp{

server::server(){}

void server::set_connect_callback(const std::function<void(channel_view,bool)>& callback){
    connect_callback_ = callback;
    return ;
}
void server::set_message_callback(const std::function<void(channel_view,packet)>& callback){
    message_callback_ = callback;
    return ;
}
void server::set_buffer_pool(const std::function<packet(size_t)>& back){
    buffer_pool_interface::set_packet_get_callback(back);
}

void server::enable_muliti_thread(int n){
#if defined (__linux__)
    threads_.reserve(n);
    for(int i = 0; i < n; i++){
        threads_.emplace_back(std::ref(stop_));
    }
#endif
    return ;
}
void server::set_connection_timeout(uint32_t second){
    connection::set_timeout(second * 1000);
}

void server::start(int port){
    if(threads_.empty()) {
        threads_.emplace_back(std::ref(stop_));
    }
    int size = threads_.size();
    for(int i = 0; i < size; i++){
        threads_[i].set_connect_callback(connect_callback_);
        threads_[i].set_message_callback(message_callback_);
    }
    for (int i = 1; i < size; i++) {
        threads_[i].start(port);
    }
    threads_[0].start(port,false);
    return ;
}

void server::stop(){
    std::function<void()> back = std::bind(&server::stop_internal,this);
    if (threads_[0].manager_->whthin_the_current_thread()) {
        threads_[0].manager_->timer_task(std::move(back), KCP_DISCONNECT_WAIT_TIMEOUT);
    }else {
        threads_[0].manager_->post([this,back](){
            threads_[0].manager_->timer_task(
                std::move(const_cast<std::function<void()>&>(back))
                ,KCP_DISCONNECT_WAIT_TIMEOUT
            );
        });
    }
    return ;
}
void server::stop_internal(){
    stop_.store(true, std::memory_order_release);
    int size = threads_.size();
    for(int i = 0; i < size; i++){
        threads_[i].stop();
    }
    return ;
}


}; // namespace kcp