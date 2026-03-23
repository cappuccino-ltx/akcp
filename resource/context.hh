#pragma once 

#include <atomic>
#include <cstdint>
#include "common.hh"
#include "ikcp.h"


namespace kcp{

class context{
public:
    explicit context(uint32_t conv, const udp::endpoint& peer);
    ~context();

    uint32_t get_conv();
    uint64_t get_last_time();
    udp::endpoint get_host();

    void set_send_callback(void(* callback)(void*, const udp::endpoint&,const char*, size_t),void* ctx);
    void set_async_send_callback(void(* callback)(void*, const udp::endpoint&,const packet&),void* ctx);
    void set_receive_callback(void(* callback)(void*,packet),void* ctx);

    void update(uint64_t clock);
    void input(const char* data, size_t bytes, const udp::endpoint& peer);
    bool send(const char* data, size_t size);
    bool send_packet(const packet& pack);
    uint64_t check(uint64_t clock);
    uint64_t next_update_time();
    static uint32_t get_conv_from_packet(const char* data);
    static uint32_t generate_conv_global();

private:
    static int send_callback(const char *buf, int len, ikcpcb *kcp, void *user);

private:
    uint32_t conv_;
    udp::endpoint peer_;
    ikcpcb* kcp_ { nullptr };
    void* send_ctx_ { nullptr };
    void* receive_ctx_ { nullptr };
    void(* send_callback_)(void*, const udp::endpoint&,const char *,size_t) { nullptr };
    void(* async_send_callback_)(void*, const udp::endpoint&,const packet&) { nullptr };
    void(* receive_callback_)(void*,packet) { nullptr };
    uint64_t last_packet_recv_time_ {0};
    static std::atomic_uint32_t conv_global;
    
}; // class connection


} // namespace kcp