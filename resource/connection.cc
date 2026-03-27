

#include "connection.hh"
#include "channel.hh"
#include "common.hh"
#include "time.hh"
#include "channel_manager.hh"

namespace kcp{

uint32_t connection::connection_timeout = CONNECTION_TIMEOUT;
bool connection::is_low_delay = true;

connection::connection(uint32_t conv, const udp::endpoint& peer)
    :context_(conv, peer)
{
    context_.set_receive_callback(&connection::receive_callback, this);
}

uint32_t connection::get_conv() {
    return context_.get_conv();
}
bool connection::is_alive(uint64_t clock){
    return clock - context_.get_last_time() < connection_timeout;
}
void connection::set_channel(const std::shared_ptr<channel>& chann){
    channel_ = chann;
}

void connection::update(uint32_t clock){
    context_.update(clock);
    return ;
}
void connection::flush() {
    context_.flush();
    return ;
}
void connection::input(const char* data, size_t bytes, const udp::endpoint& peer){
    context_.input(data, bytes, peer);
    push_task(true, false);
    return ;
}
uint64_t connection::check(uint64_t clock){
    return context_.check(clock);
}

void connection::set_send_callback(void(* callback)(void*, const udp::endpoint&,const char*, size_t),void* ctx){
    context_.set_send_callback(callback,ctx);
    return ;
}
void connection::set_async_send_callback(void(* callback)(void*, const udp::endpoint&,const packet&),void* ctx){
    context_.set_async_send_callback(callback,ctx);
    return ;
}
void connection::set_message_callback(void(* callback)(void*,packet),void* ctx) {
    message_ctx_ = ctx;
    message_callback_ = callback;
    return ;
}
void connection::set_connect_callback(void(* callback)(void*,bool),void* ctx){
    connect_ctx_ = ctx;
    connect_callback_ = callback;
    return ;
}
void connection::push_task(bool read_opt, bool write_opt) {
    if(connection::is_low_delay == false) {
        return ;
    }
    if (read_opt){
        uint64_t now = util::time::clock_64();
        if (check(now) <= now){
            write_opt = true;
        }else {
            return;
        }
    }
    auto chann = channel_.lock();
    chann->send_push |= write_opt;
    chann->manager_.lock()->add_event_channel(chann);
}
bool connection::send(const char* data, size_t size){
    push_task(false, true);
    return context_.send(data, size);
}
bool connection::send_packet(const packet& pack){
    push_task(false, true);
    return context_.send_packet(pack);
}
void connection::keepalive(){
    send(KCP_KEEPALIVE_REQUEST,KCP_PACKAGE_SIZE);
    return ;
}
void connection::disconnect(){
    send(KCP_DISCONNECT_REQUEST,KCP_PACKAGE_SIZE);
    return ;
}
void connection::set_timeout(uint32_t milliseconds){
    connection_timeout = milliseconds;
    return ;
}
void connection::disable_low_latency(){
    is_low_delay = false;
}

void connection::receive_callback(void* self, packet pack){
    connection* self_ = static_cast<connection*>(self);
    // keepalive packet handler
    if (pack->size() == KCP_PACKAGE_SIZE){
        if (strcmp((char*)pack->data(), KCP_DISCONNECT_REQUEST) == 0){
            self_->send(KCP_DISCONNECT_RESPONSE, KCP_PACKAGE_SIZE);
            if (self_->connect_callback_){
                self_->connect_callback_(self_->connect_ctx_, false);
            }
            return ;
        }else if (strcmp((char*)pack->data(), KCP_DISCONNECT_RESPONSE) == 0){
            // this is package response , discard
            if (self_->connect_callback_){
                self_->connect_callback_(self_->connect_ctx_, false);
            }
            return ;
        }else if(strcmp((char*)pack->data(), KCP_KEEPALIVE_REQUEST) == 0){
            // this is request , respond 
            self_->send(KCP_KEEPALIVE_RESPONSE,sizeof(KCP_KEEPALIVE_RESPONSE));
            return;
        }else if (strcmp((char*)pack->data(), KCP_KEEPALIVE_RESPONSE) == 0){
            // this is response , discard
            return ;
        }
    }
    if(self_->message_callback_) {
        self_->message_callback_(self_->message_ctx_, pack);
    }
    return ;
}


} // namespace kcp