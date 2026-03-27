#pragma once 
#include <cstring>
#include <memory>
#include "common.hh"
#include "context.hh"

namespace kcp{
class channel;
class connection{
    friend class channel_container;
public:
    explicit connection(uint32_t conv, const udp::endpoint& peer);

    uint32_t get_conv();
    bool is_alive(uint64_t clock);
    void set_channel(const std::shared_ptr<channel>& chann);
    void update(uint32_t clock);
    void flush();
    void input(const char* data, size_t bytes, const udp::endpoint& peer);
    uint64_t check(uint64_t clock);

    void set_send_callback(void(* callback)(void*, const udp::endpoint&,const char*, size_t),void* ctx);
    void set_async_send_callback(void(* callback)(void*, const udp::endpoint&,const packet&),void* ctx);
    void set_message_callback(void(* callback)(void*,packet),void* ctx);
    void set_connect_callback(void(* callback)(void*,bool),void* ctx);

    bool send(const char* data, size_t size);
    bool send_packet(const packet& pack);
    void push_task(bool read_opt, bool write_opt);
    
    void keepalive();
    void disconnect();
    static void set_timeout(uint32_t milliseconds);
    static void disable_low_latency();

private:
    static void receive_callback(void* ptr, packet pack);

private:
    std::weak_ptr<channel> channel_;
    context context_;
    void* message_ctx_ { nullptr };
    void* connect_ctx_ { nullptr };
    void(* message_callback_)(void*,packet) { nullptr };
    void(* connect_callback_)(void*,bool) { nullptr };
    static uint32_t connection_timeout;
    static bool is_low_delay;
}; // class connection

} // namespace kcp