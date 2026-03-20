#include "context.hh"
#include "common.hh"
#include "ikcp.h"
#include "time.hh"
#include <atomic>
#include <cstdint>
namespace kcp{

std::atomic_uint32_t context::conv_global = KCP_CONV_MIN;

context::context(unsigned int conv, const udp::endpoint& peer)
    :peer_(peer),
    conv_(conv),
    last_packet_recv_time_(util::time::clock_32())
{
    kcp_ = ikcp_create(conv_, this);
    kcp_->output = context::send_callback;
    ikcp_nodelay(kcp_, KCP_NODELAY, KCP_INTERVAL, KCP_RESEND, KCP_NC);
}
context::~context(){
    ikcp_release(kcp_);
}

uint32_t context::get_conv(){
    return conv_;
}
uint64_t context::get_last_time(){
    return last_packet_recv_time_;
}
udp::endpoint context::get_host(){
    return peer_;
}


void context::set_send_callback(void(* callback)(void*, const udp::endpoint&,const char*, size_t),void* ctx){
    send_callback_ = callback;
    send_ctx_ = ctx;
    return ;
}

void context::set_async_send_callback(void(* callback)(void*, const udp::endpoint&,const packet&),void* ctx){
    async_send_callback_ = callback;
    send_ctx_ = ctx;
    return ;
}

void context::set_receive_callback(void(* callback)(void*,packet),void* ctx){
    receive_callback_ = callback;
    receive_ctx_ = ctx;
    return ;
}

void context::update(uint64_t clock){
    ikcp_update(kcp_, clock);
    return ;
}

void context::input(const char* data, size_t bytes, const udp::endpoint& peer){
    last_packet_recv_time_ = util::time::clock_64();
    peer_ = peer;
    ikcp_input(kcp_, data, bytes);
    while(1){ 
        int readable_size = ikcp_peeksize(kcp_);
        if (readable_size > 0) {
            packet pack(std::make_shared<std::vector<uint8_t>>(readable_size));
            int size = ikcp_recv(kcp_, (char*)pack->data(), pack->size());
            if (size > 0 && receive_callback_) {
                receive_callback_(receive_ctx_,pack);
            }
        }else {
            return ;
        }
    }
    return ;
}
uint64_t context::check(uint64_t clock){
    uint32_t next_32 = ikcp_check(kcp_, (uint32_t)clock);
    uint64_t next_64 = util::time::clock_32_to_64(clock, next_32);
    return next_64;
}

bool context::send(const char* data, size_t size){
    int ret = ikcp_send(kcp_, data, size);
    update(util::time::clock_32());
    return ret >= 0;
}
bool context::send_packet(const packet& pack){
    return send((const char *)pack->data(),pack->size());
}


int context::send_callback(const char *buf, int len, ikcpcb *kcp, void *user){
    context* this_ = static_cast<context*>(user);
    if (this_->async_send_callback_) {
        packet pack = std::make_shared<std::vector<uint8_t>>(buf, buf + len);
        this_->async_send_callback_(this_->send_ctx_, this_->peer_, pack);
    } else if (this_->send_callback_){
        this_->send_callback_(this_->send_ctx_, this_->peer_, buf, len);
    }
    return 0;
}

uint32_t context::generate_conv_global() {
    uint32_t conv = conv_global.load(std::memory_order_acquire);
    while(1){
        uint32_t next = (conv + 1 > KCP_CONV_MAX) ? (KCP_CONV_MIN) : (conv + 1);
        bool ret = conv_global.compare_exchange_strong(conv,next,std::memory_order_acq_rel,std::memory_order_relaxed);
        if (ret) {
            // success
            return conv;
        }
    }
    return 0;
}

uint32_t context::get_conv_from_packet(const char* data){
    return ikcp_getconv(data);
}


} // namespace kcp