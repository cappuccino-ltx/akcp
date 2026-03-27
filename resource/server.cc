

#include "server.hh"
#include "common.hh"
#include "connection.hh"
#include "kcp_thread.hh"
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
        threads_.push_back(std::move(std::unique_ptr<kcp_thread>(new kcp_thread(std::ref(stop_)))));
    }
#endif
    return ;
}
void server::disable_low_latency(int heartbeat_time){
    connection::disable_low_latency();
    context::set_interval(heartbeat_time);
}
void server::set_connection_timeout(uint32_t second){
    connection::set_timeout(second * 1000);
}

void server::set_thread_quit_callback(const std::function<void()>& back){
    for (int i = threads_.size(); i >= 0; i--) {
        threads_[i]->set_thread_quit_callback(back);
    }
}
void server::set_thread_start_callback(const std::function<void()>& back){
    for (int i = threads_.size(); i >= 0; i--) {
        threads_[i]->set_thread_start_callback(back);
    }
}

void server::start(int port, int core_begin, int core_end){
    if(threads_.empty()) {
        threads_.push_back(std::move(std::unique_ptr<kcp_thread>(new kcp_thread(std::ref(stop_)))));
    }
    int size = threads_.size();
    for(int i = 0; i < size; i++){
        threads_[i]->set_connect_callback(connect_callback_);
        threads_[i]->set_message_callback(message_callback_);
    }
    for (int i = 0, j = core_begin; i < size; i++, j++) {
        if (j > core_end) j = core_begin;
        threads_[i]->start(port,j);
    }
    for (int i = 0; i < size; i++) {
        threads_[i]->close_wait();
    }
    return ;
}

void server::stop(){
    std::function<void()> back = std::bind(&server::stop_internal,this);
    if (threads_[0]->manager_->whthin_the_current_thread()) {
        threads_[0]->manager_->timer_task(std::move(back), KCP_DISCONNECT_WAIT_TIMEOUT);
    }else {
        threads_[0]->manager_->post([this,back](){
            threads_[0]->manager_->timer_task(
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
        threads_[i]->stop();
    }
    return ;
}


}; // namespace kcp