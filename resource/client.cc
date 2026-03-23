

#include "client.hh"
#include "packet.hh"


namespace kcp{

client::client(){
    init();
}
void client::set_connect_callback(const std::function<void(channel_view,bool)>& callback){
    kcp_thread_->set_connect_callback(callback);
    return ;
}
void client::set_message_callback(const std::function<void(channel_view,packet)>& callback){
    kcp_thread_->set_message_callback(callback);
    return ;
}
void client::set_buffer_pool(const std::function<packet(size_t)>& back){
    buffer_pool_interface::set_packet_get_callback(back);
}
void client::connect(const std::string& host,int port){
    kcp_thread_->connect(host, port);
    return ;
}
void client::stop(){
    kcp_thread_->manager_->timer_task(std::bind(&client::stop_internal,this), KCP_DISCONNECT_WAIT_TIMEOUT);
    return ;
}

void client::stop_internal(){
    stop_.store(true, std::memory_order_release);
    kcp_thread_->stop();
    return ;
}
void client::receive_callback(void* self,const udp::endpoint& point,const char* data,size_t size){
    client* self_ = static_cast<client*>(self);
    if (size == KCP_PACKAGE_SIZE) {
        uint32_t conv = protocol::parse_conv_from_response(data);
        if (conv != 0){
            channel_manager::push_new_link_callback(self_->kcp_thread_->manager_.get(),conv,point);
            return ;
        }
    }
    packet pack = std::make_shared<std::vector<uint8_t>>(data, data + size);
    channel_manager::push_package_callback(self_->kcp_thread_->manager_.get(),point,data,size);
    return ;
}
void client::init(){
    kcp_thread_ = std::make_shared<kcp_thread>(std::ref(stop_));
    kcp_thread_->start();
    return ;
}


} // namespace kcp