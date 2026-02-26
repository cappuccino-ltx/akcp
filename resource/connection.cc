

#include "connection.hh"
#include "common.hh"

namespace kcp{

uint32_t connection::connection_timeout = CONNECTION_TIMEOUT;

connection::connection(uint32_t conv, const udp::endpoint& peer)
    :context_(conv, peer)
{
    context_.set_receive_callback(&connection::receive_callback, this);
}

uint32_t connection::get_conv() {
    return context_.get_conv();
}
uint32_t connection::is_alive(uint64_t clock){
    return clock - context_.get_last_time() < connection_timeout;
}

void connection::update(uint32_t clock){
    context_.update(clock);
}
void connection::input(const char* data, size_t bytes, const udp::endpoint& peer){
    context_.input(data, bytes, peer);
}
uint64_t connection::check(uint64_t clock){
    return context_.check(clock);
}

void connection::set_send_callback(void(* callback)(void*, const udp::endpoint&,const char*, size_t),void* ctx){
    context_.set_send_callback(callback, ctx);
}
void connection::set_async_send_callback(void(* callback)(void*, const udp::endpoint&,const packet&),void* ctx){
    context_.set_async_send_callback(callback, ctx);
}
void connection::set_message_callback(void(* callback)(void*,packet),void* ctx) {
    message_ctx_ = ctx;
    message_callback_ = callback;
}
void connection::set_connect_callback(void(* callback)(void*,bool),void* ctx){
    connect_ctx_ = ctx;
    connect_callback_ = callback;
}

bool connection::send(const char* data, size_t size){
    return context_.send(data, size);
}
bool connection::send_packet(const packet& pack){
    return context_.send_packet(pack);
}
void connection::keepalive(){
    send(KCP_KEEPALIVE_REQUEST,KCP_PACKAGE_SIZE);
}
void connection::disconnect(){
    send(KCP_DISCONNECT_REQUEST,KCP_PACKAGE_SIZE);
}
void connection::set_timeout(uint32_t milliseconds){
    connection_timeout = milliseconds;
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
}


} // namespace kcp