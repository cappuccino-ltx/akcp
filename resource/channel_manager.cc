

#include "channel_manager.hh"
#include "channel.hh"
#include "time.hh"
#include <cstdint>
#include <ctime>

namespace kcp{

channel_manager::channel_manager(std::atomic<bool>& stop)
    :context_(std::make_shared<asio::io_context>())
    ,stop_(stop)
    ,container_(*context_,stop_)
    ,tasks_(*context_,stop_)
    ,cuurent_id(std::this_thread::get_id())
{}

std::shared_ptr<channel_manager> channel_manager::create(std::atomic<bool>& stop){
    std::shared_ptr<channel_manager> ptr = std::make_shared<channel_manager>(std::ref(stop));
    ptr->init();
    return ptr;
}
void channel_manager::init(){
    container_.set_manager(shared_from_this());
}

void channel_manager::push_package_callback(void* self, const udp::endpoint& point, const char* data, size_t size){
    channel_manager* self_ = static_cast<channel_manager*>(self);
    self_->receive_packet(point, data, size);
    return ;
}
void channel_manager::push_new_link_callback(void* self, void* socket, uint32_t conv, const udp::endpoint& peer){
    channel_manager* self_ = static_cast<channel_manager*>(self);
    self_->create_linked(socket, conv, peer);
    return ;
}
void channel_manager::push_half_link_callback(void* self, uint32_t conv, const udp::endpoint& peer){
    channel_manager* self_ = static_cast<channel_manager*>(self);
    self_->half_channel_.insert(std::make_pair(conv, peer));
    // set timeout task
    self_->timer_task(std::bind(&channel_manager::remove_half_link,self_,conv), HALF_CONNECTION_TIMEOUT);
    return ;
}
void channel_manager::remove_half_link(uint32_t conv) {
    auto it = half_channel_.find(conv);
    if(it == half_channel_.end()) {
        return ;
    }
    half_channel_.erase(it);
    return ;
}
// set the callback function for sending messages in the socket layer to the channel
void channel_manager::set_send_callback(void(* callback)(void*, const udp::endpoint&,const char*, size_t)){
    send_callback_ = callback;
    return ;
}
void channel_manager::set_async_send_callback(void(* callback)(void*, const udp::endpoint&,const packet&)){
    async_send_callback_ = callback;
    return ;
}
// set the callback for receive message in the application layer to the channel
void channel_manager::set_connect_callback(const std::function<void(channel_view,bool)> & callback){
    connect_callback_ = callback;
    return ;
}
void channel_manager::set_message_callback(const std::function<void(channel_view,packet)> & callback){
    message_callback_ = callback;
    return ;
}
void channel_manager::post(const std::function<void(void)>& task){
    asio::post(*context_,task);
    return ;
}
void channel_manager::timer_task(std::function<void()>&& task, uint32_t milliseconds){
    if(whthin_the_current_thread()) {
        tasks_.push(std::move(task), milliseconds);
    }else{
        post([this,task,milliseconds](){
            tasks_.push(std::move(const_cast<std::function<void()>&&>(task)), milliseconds);
        });
    }
    return ;
}
// muliti-thread exchange
bool channel_manager::whthin_the_current_thread(){
    return std::this_thread::get_id() == cuurent_id;
}
void channel_manager::stop(){
    container_.stop();
    context_->stop();
    return ;
}

void channel_manager::add_event_channel(const std::shared_ptr<channel>& chann){
    if (chann->queued) {
        return;
    }
    register_event_channel_call();
    chann->queued = true;
    event_channel_.push_back(chann);
}
void channel_manager::register_event_channel_call(){
    if (event_channel_.size()) {
        return;
    }
    context_->post([this](){
        this->hander_event_channel_update();
    });
}
void channel_manager::hander_event_channel_update(){
    uint64_t now = util::time::clock_64();
    for(auto& ptr : event_channel_) {
        ptr->conn_.update(now);
        if (ptr->send_push) {
            // send ...
            ptr->conn_.flush();
            ptr->send_push = false;
        }
        if (ptr->time_push) {
            // timer ...
            uint64_t next_time = ptr->conn_.check(now);
            container_.push_channel_timer(next_time, ptr->conn_.get_conv());
            ptr->time_push = false;
        }
        ptr->queued = false;
    }
    event_channel_.clear();
}

// process data and links
void channel_manager::receive_packet(const udp::endpoint& point, const char* data, size_t size){
    uint32_t conv = context::get_conv_from_packet(data);
    std::shared_ptr<channel> chann = container_.find(conv);
    if (chann.get() == nullptr){
        return;
    }
    chann->conn_.input(data, size, point);
    return ;
}
void channel_manager::create_linked(void* socket, uint32_t conv, const udp::endpoint& peer){
    std::shared_ptr<channel> chann = container_.insert(conv, peer, shared_from_this());
    if (async_send_callback_) {
        chann->set_async_send_callback(async_send_callback_,socket);
    }else if (send_callback_){
        chann->set_send_callback(send_callback_,socket);
    }
    chann->set_message_callback(message_callback_);
    chann->set_connect_callback(connect_callback_);
    chann->set_remove_cahnnel_callback(channel_container::remove_callback,&container_);
    if (connect_callback_) {
        connect_callback_(chann->shared_from_this(),true);
    }
    return ;
}

} //namespace kcp