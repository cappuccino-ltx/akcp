

#include "client.hh"


namespace kcp{

client::client(){
    init();
}
void client::set_connect_callback(const std::function<void(channel_view,bool)>& callback){
    kcp_thread_->set_connect_callback(callback);
}
void client::set_message_callback(const std::function<void(channel_view,packet)>& callback){
    kcp_thread_->set_message_callback(callback);
}

void client::connect(const std::string& host,int port){
    kcp_thread_->connect(host, port);
}
void client::stop(){
    kcp_thread_->manager_->timer_task(std::bind(&client::stop_internal,this), KCP_DISCONNECT_WAIT_TIMEOUT);
}

void client::stop_internal(){
    stop_.store(true, std::memory_order_release);
    kcp_thread_->stop();
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
}
void client::init(){
    kcp_thread_ = std::make_shared<kcp_thread>(std::ref(stop_));
    kcp_thread_->start();
}


} // namespace kcp