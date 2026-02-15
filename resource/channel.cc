

#include "channel.hh"
#include "channel_manager.hh"
#include "connection.hh"

namespace kcp{

channel::~channel(){}

void channel::send(const char* data, size_t size){
    auto m = manager_.lock();
    if (m->whthin_the_current_thread()){
        conn_.send(data, size);
    }else {
        packet pack = std::make_shared<std::vector<uint8_t>>(data, data + size);
        put_message_queue(pack);
    }
}

void channel::send(std::vector<uint8_t> && message){
    auto m = manager_.lock();
    if (m->whthin_the_current_thread()){
        conn_.send((const char*)message.data(), message.size());
    }else {
        packet pack = std::make_shared<std::vector<uint8_t>>(std::move(message));
        put_message_queue(pack);
    }
}
void channel::disconnect(){
    auto m = manager_.lock();
    if (m->whthin_the_current_thread()){
        conn_.disconnect();
    }else {
        m->post(std::bind(&connection::disconnect, &conn_));
    }
}

void channel::put_message_queue(const packet& pack){
    send_queue_.put(pack);
    if(!send_scheduled.exchange(true, std::memory_order_acq_rel)){
        manager_.lock()->post(std::bind(&channel::handler_send_message,this));
    }
}
void channel::handler_send_message(){
    send_scheduled.store(false,std::memory_order_release);
    while(send_queue_.readable()){
        packet pack;
        if(send_queue_.try_get(pack) && pack){
            conn_.send_packet(pack);
        }
    }
}


channel::channel(uint32_t conv, const udp::endpoint& peer, const std::weak_ptr<channel_manager>& manager)
    :conn_(conv, peer)
    ,manager_(manager)
    
{
    conn_.set_message_callback(channel::message_callback, this);
}
// external call , no need to delete
void channel::do_timeout(){
    // timeout operator,
    if (connect_callback_){
        connect_callback_(shared_from_this(),false);
    }
}
void channel::set_send_callback(void(* callback)(void*, const udp::endpoint&,const char*, size_t),void* ctx){
    conn_.set_send_callback(callback, ctx);
}
void channel::set_async_send_callback(void(* callback)(void*, const udp::endpoint&,const packet&),void* ctx){
    conn_.set_async_send_callback(callback, ctx);
}
void channel::set_connect_callback(const std::function<void(channel_view,bool)> & callback){
    connect_callback_ = callback;
}
void channel::set_message_callback(const std::function<void(channel_view,packet)> & callback){
    message_callback_ = callback;
}

void channel::set_remove_cahnnel_callback(const std::function<void(uint32_t)> & callback){
    remove_channel_callback_ = callback;
}

void channel::message_callback(void* self,packet pack){
    channel* self_ = static_cast<channel*>(self);
    self_->message_callback_(self_->shared_from_this(),pack);
}
// connect layer call ,need to delete
void channel::connect_callback(void* self,bool linked){
    channel* self_ = static_cast<channel*>(self);
    if (self_->connect_callback_){
        self_->connect_callback_(self_->shared_from_this(),linked);
    }
    if (self_->remove_channel_callback_){
        self_->remove_channel_callback_(self_->conn_.get_conv());
    }
}
void channel::update_callback(void* self, uint32_t clock){
    channel* self_ = static_cast<channel*>(self);
    self_->conn_.update(clock);
}

uint64_t channel::check(uint64_t clock){
    return conn_.check(clock);
}


} // namespace kcp