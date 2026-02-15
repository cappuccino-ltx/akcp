

#include "channel_manager.hh"
#include <ctime>

namespace kcp{

int channel_manager::update_interval_ = UPDATE_INTERVAL;

channel_manager::channel_manager(std::atomic<bool>& stop,bool enable_muliti_thread, int n, int index)
    :context_(std::make_shared<asio::io_context>())
    ,timer_(*context_)
    ,stop_(stop)
    ,container_(*context_,stop_)
    ,cuurent_id(std::this_thread::get_id())
{
    if (enable_muliti_thread){
        packages_ = std::make_shared<lfree::ring_queue<std::shared_ptr<package>>>();
        new_link_ = std::make_shared<lfree::ring_queue<std::shared_ptr<link_data>>>(lfree::queue_size::K01);
    }else {
        // set callback
    }
}
// muliti-threaded mode
void channel_manager::push_package(const std::shared_ptr<package>& pack){
    packages_->put(pack);
}
void channel_manager::push_new_link(const std::shared_ptr<link_data>& pack){
    new_link_->put(pack);
}
// single-threaded mode
void channel_manager::push_package_callback(void* self,const udp::endpoint& point, const char* data, size_t size){
    channel_manager* self_ = static_cast<channel_manager*>(self);
    self_->receive_packet(point, data, size);
}
void channel_manager::push_new_link_callback(void* self, uint32_t conv, const udp::endpoint& peer){
    channel_manager* self_ = static_cast<channel_manager*>(self);
    self_->create_linked(conv, peer);
}
// set the callback function for sending messages in the socket layer to the channel
void channel_manager::set_send_callback(void(* callback)(void*, const udp::endpoint&,const char*, size_t),void* ctx){
    send_ctx_ = ctx;
    send_callback_ = callback;
}
void channel_manager::set_async_send_callback(void(* callback)(void*, const udp::endpoint&,const packet&),void* ctx){
    send_ctx_ = ctx;
    async_send_callback_ = callback;
}
// set the callback for receive message in the application layer to the channel
void channel_manager::set_connect_callback(const std::function<void(channel_view,bool)> & callback){
    connect_callback_ = callback;
}
void channel_manager::set_message_callback(const std::function<void(channel_view,packet)> & callback){
    message_callback_ = callback;
}
void channel_manager::set_update_interval(size_t val){
    channel_manager::update_interval_ = val;
}
void channel_manager::post(const std::function<void(void)>& task){
    asio::post(*context_,task);
}

// muliti-thread exchange
bool channel_manager::whthin_the_current_thread(){
    return std::this_thread::get_id() == cuurent_id;
}

void channel_manager::handler_packets(){
    if (packages_.get() == nullptr){
        return ;
    }
    while(packages_->readable()){ 
        std::shared_ptr<package> pack;
        bool ret = packages_->try_get(pack);
        if (ret){
            receive_packet(pack->first,(const char*)pack->second->data(),pack->second->size());
        }
    }
}
void channel_manager::handler_new_link(){
    if (new_link_.get() == nullptr){
        return ;
    }
    while(new_link_->readable()){ 
        std::shared_ptr<link_data> data;
        bool ret = new_link_->try_get(data);
        if (ret){
            create_linked(data->first, data->second);
        }
    }
}
void channel_manager::handler(){
    handler_new_link();
    handler_packets();
}

// process data and links
void channel_manager::receive_packet(const udp::endpoint& point, const char* data, size_t size){
    uint32_t conv = context::get_conv_from_packet(data);
    std::shared_ptr<channel> chann = container_.find(conv);
    if (chann.get() == nullptr){
        return ;
    }
    chann->conn_.input(data, size, point);
}
void channel_manager::create_linked(uint32_t conv, const udp::endpoint& peer){
    std::shared_ptr<channel> chann = container_.insert(conv, peer, shared_from_this());
    if (async_send_callback_) {
        chann->set_async_send_callback(async_send_callback_, send_ctx_);
    }else if (send_callback_){
        chann->set_send_callback(send_callback_,send_ctx_);
    }
    chann->set_message_callback(message_callback_);
    chann->set_connect_callback(connect_callback_);
    chann->set_remove_cahnnel_callback(std::bind(&channel_container::remove,&container_,_1));
    if (connect_callback_) {
        connect_callback_(chann->shared_from_this(),true);
    }
}

// timer
void channel_manager::hook_timer(){
    if(stop_.load(std::memory_order_acquire)){
        return ;
    }
    timer_.expires_after(std::chrono::milliseconds(update_interval_));
    timer_.async_wait(std::bind(&channel_manager::handler_timer,this));
}
void channel_manager::handler_timer(){
    hook_timer();
    uint32_t current_clock = util::time::clock_32();
    container_.update(current_clock);
}
    

} //namespace kcp