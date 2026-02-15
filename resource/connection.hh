#pragma once 

#include <common.hh>
#include <cstring>
#include "context.hh"

namespace kcp{

class connection{
public:
    explicit connection(uint32_t conv, const udp::endpoint& peer);

    uint32_t get_conv();
    uint32_t is_alive(uint32_t clock);

    void update(uint32_t clock);
    void input(const char* data, size_t bytes, const udp::endpoint& peer);
    uint64_t check(uint64_t clock);

    void set_send_callback(void(* callback)(void*, const udp::endpoint&,const char*, size_t),void* ctx);
    void set_async_send_callback(void(* callback)(void*, const udp::endpoint&,const packet&),void* ctx);
    void set_message_callback(void(* callback)(void*,packet),void* ctx);
    void set_connect_callback(void(* callback)(void*,bool),void* ctx);

    bool send(const char* data, size_t size);
    bool send_packet(const packet& pack);

    void keepalive();
    void disconnect();
    static void set_timeout(uint32_t time);

private:
    static void receive_callback(void* ptr, packet pack);

private:
    context context_;
    void* message_ctx_ { nullptr };
    void* connect_ctx_ { nullptr };
    void(* message_callback_)(void*,packet) { nullptr };
    void(* connect_callback_)(void*,bool) { nullptr };
    static uint32_t connection_timeout;
}; // class connection

} // namespace kcp