

#include "client.hh"
#include "packet.hh"


namespace kcp{

client::client(int core){
    init(core);
}
void client::set_connect_callback(const std::function<void(channel_view,bool)>& callback){
    kcp_thread_->set_connect_callback(callback);
    return ;
}
void client::set_message_callback(const std::function<void(channel_view,packet)>& callback){
    kcp_thread_->set_message_callback(callback);
    return ;
}
void client::set_thread_quit_callback(const std::function<void()>& back){
    kcp_thread_->set_thread_quit_callback(back);
}
void client::set_thread_start_callback(const std::function<void()>& back){
    kcp_thread_->set_thread_start_callback(back);
}
void client::set_buffer_pool(const std::function<packet(size_t)>& back){
    buffer_pool_interface::set_packet_get_callback(back);
}
void client::connect(const std::string& host,int port, bool is_sole_socket){
    kcp_thread_->connect(host, port, is_sole_socket);
    return ;
}
void client::disable_low_latency(int heartbeat_time){
    connection::disable_low_latency();
    context::set_interval(heartbeat_time);
}
void client::stop(){
    kcp_thread_->manager_->timer_task(std::bind(&client::stop_internal,this), KCP_DISCONNECT_WAIT_TIMEOUT);
    kcp_thread_->close_wait();
    return ;
}

void client::stop_internal(){
    stop_.store(true, std::memory_order_release);
    kcp_thread_->stop();
    return ;
}
void client::init(int core){
    kcp_thread_ = std::make_shared<kcp_thread>(std::ref(stop_));
    kcp_thread_->start(-1,core);
    return ;
}


} // namespace kcp