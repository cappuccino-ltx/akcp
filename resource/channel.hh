#pragma once 

#include "connection.hh"
#include <memory>

namespace kcp{
// handle class provided for user use
class channel;
class channel_manager;
using channel_view = std::shared_ptr<channel>;
class channel : public std::enable_shared_from_this<channel>{
    friend class channel_manager;
    friend class channel_container;
    friend class connection;
    friend class timer;
public:
    ~channel();
    void send(const char* data, size_t size);
    void send(const std::string& message);
    void send(const std::vector<uint8_t> & message);
    void send(packet message);
    void disconnect();
    void timer_task(std::function<void()>&& task, uint32_t milliseconds);
    
private:
    channel(uint32_t conv, const udp::endpoint& peer, const std::weak_ptr<channel_manager>& manager);
    // external call , no need to delete
    void do_timeout();
    void set_send_callback(void(* callback)(void*, const udp::endpoint&,const char*, size_t), void* socket);
    void set_async_send_callback(void(* callback)(void*, const udp::endpoint&,const packet&), void* socket);
    void set_connect_callback(const std::function<void(channel_view,bool)> & callback);
    void set_message_callback(const std::function<void(channel_view,packet)> & callback);
    void set_remove_cahnnel_callback(void(*callback)(void*,uint32_t),void* ctx);
    
    static void message_callback(void* self,packet pack);
    // connect layer call ,need to delete
    static void connect_callback(void* self,bool linked);
    static void update_callback(void* self, uint32_t clock);
    void put_message_queue(const packet& pack);
    void handler_send_message();
    uint64_t check(uint64_t clock);
    uint64_t next_update_time();
    static std::shared_ptr<channel> create(uint32_t conv, const udp::endpoint& peer, const std::weak_ptr<channel_manager>& manager);
    void init();


private:
    std::weak_ptr<channel_manager> manager_;
    connection conn_;
    std::function<void(channel_view,bool)> connect_callback_;
    std::function<void(channel_view,packet)> message_callback_;
    // std::function<void(uint32_t)> remove_channel_callback_;
    void* remove_channel_ctx_ { nullptr };
    void(*remove_channel_callback_)(void*,uint32_t) { nullptr };
    bool timer_scheduled { false };// write_scheduled
}; // class channel

} // namespace kcp