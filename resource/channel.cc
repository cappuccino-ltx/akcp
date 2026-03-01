

#include "channel.hh"
#include "channel_manager.hh"
#include "common.hh"
#include "connection.hh"
#include <cstdlib>
#include <functional>

namespace kcp{

channel::~channel(){}

void channel::send(const char* data, size_t size){
    auto m = manager_.lock();
    if (!m) {
        abort();
    }
    if (m->whthin_the_current_thread()){
        conn_.send(data, size);
    }else {
        packet pack = std::make_shared<std::vector<uint8_t>>(data, data + size);
        m->post(std::bind(&connection::send_packet,&conn_,pack));
    }
    return ;
}

void channel::send(std::vector<uint8_t> && message){
    auto m = manager_.lock();
    if (!m) {
        abort();
    }
    if (m->whthin_the_current_thread()){
        conn_.send((const char*)message.data(), message.size());
    }else {
        packet pack = std::make_shared<std::vector<uint8_t>>(std::move(message));
        m->post(std::bind(&connection::send_packet,&conn_,pack));
    }
    return ;
}
void channel::disconnect(){
    auto m = manager_.lock();
    if (!m) {
        abort();
    }
    if (m->whthin_the_current_thread()){
        conn_.disconnect();
    }else {
        m->post(std::bind(&connection::disconnect, &conn_));
    }
    return ;
}


channel::channel(uint32_t conv, const udp::endpoint& peer, const std::weak_ptr<channel_manager>& manager)
    :conn_(conv, peer)
    ,manager_(manager)
    
{
    conn_.set_message_callback(channel::message_callback, this);
    conn_.set_connect_callback(channel::connect_callback, this);
}
// external call , no need to delete
void channel::do_timeout(){
    // timeout operator,
    if (connect_callback_){
        connect_callback_(shared_from_this(),false);
    }
    return ;
}
void channel::set_send_callback(void(* callback)(void*, const udp::endpoint&,const char*, size_t),void* ctx){
    conn_.set_send_callback(callback, ctx);
    return ;
}
void channel::set_async_send_callback(void(* callback)(void*, const udp::endpoint&,const packet&),void* ctx){
    conn_.set_async_send_callback(callback, ctx);
    return ;
}
void channel::set_connect_callback(const std::function<void(channel_view,bool)> & callback){
    connect_callback_ = callback;
    return ;
}
void channel::set_message_callback(const std::function<void(channel_view,packet)> & callback){
    message_callback_ = callback;
    return ;
}

void channel::set_remove_cahnnel_callback(void(*callback)(void*,uint32_t),void* ctx){
    remove_channel_ctx_ = ctx;
    remove_channel_callback_ = callback;
    return ;
}
void channel::timer_task(std::function<void()>&& task, uint32_t milliseconds){
    std::shared_ptr<channel_manager> m = manager_.lock();
    if(!m) {
        return ;
    }
    m->timer_task(std::move(task), milliseconds);
    return ;
}

void channel::message_callback(void* self,packet pack){
    channel* self_ = static_cast<channel*>(self);
    self_->message_callback_(self_->shared_from_this(),pack);
    return ;
}
// connect layer call ,need to delete
void channel::connect_callback(void* self,bool linked){
    channel* self_ = static_cast<channel*>(self);
    if (self_->connect_callback_){
        self_->connect_callback_(self_->shared_from_this(),linked);
    }
    if (self_->remove_channel_callback_){
        self_->manager_.lock()->timer_task(
            std::bind(
                self_->remove_channel_callback_,
                self_->remove_channel_ctx_, 
                self_->conn_.get_conv()
            ),
            KCP_DISCONNECT_WAIT_TIMEOUT
        );
        // self_->remove_channel_callback_(self_->remove_channel_ctx_, self_->conn_.get_conv());
    }
    return ;
}
void channel::update_callback(void* self, uint32_t clock){
    channel* self_ = static_cast<channel*>(self);
    self_->conn_.update(clock);
    return ;
}

uint64_t channel::check(uint64_t clock){
    return conn_.check(clock);
}


} // namespace kcp