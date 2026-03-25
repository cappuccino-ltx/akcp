#pragma once 

#include <cstdint>
#include <thread>
#include <unordered_map>
#include "channel_container.hh"
#include "timer_tasks.hh"

namespace kcp{

class channel_manager : public std::enable_shared_from_this<channel_manager>{
public:
    friend class kcp_thread;
    friend class server;
    friend class client;
public:
    channel_manager(std::atomic<bool>& stop);
    static std::shared_ptr<channel_manager> create(std::atomic<bool>& stop);
    void init();
    
    // single-threaded mode
    static void push_package_callback(void* self, const udp::endpoint& point, const char* data, size_t size);
    static void push_new_link_callback(void* self,void * socket, uint32_t conv, const udp::endpoint& peer);
    static void push_half_link_callback(void* self, uint32_t conv, const udp::endpoint& peer);
    void remove_half_link(uint32_t conv);
    // set the callback function for sending messages in the socket layer to the channel
    void set_send_callback(void(* callback)(void*, const udp::endpoint&,const char*, size_t));
    void set_async_send_callback(void(* callback)(void*, const udp::endpoint&,const packet&));
    // set the callback for receive message in the application layer to the channel
    void set_connect_callback(const std::function<void(channel_view,bool)> & callback);
    void set_message_callback(const std::function<void(channel_view,packet)> & callback);
    void post(const std::function<void(void)>& task);
    void timer_task(std::function<void()>&& task, uint32_t milliseconds);
    bool whthin_the_current_thread();
    void stop();
    void add_event_channel(const std::shared_ptr<channel>& chann);
    void register_event_channel_call();
    void hander_event_channel_update();

private:
    // process data and links
    void receive_packet(const udp::endpoint& point, const char* data, size_t size);
    void create_linked(void* socket, uint32_t conv, const udp::endpoint& peer);
    
private:
    // event loop and timer 
    std::shared_ptr<asio::io_context> context_;
    // control switch
    std::atomic<bool>& stop_;
    std::thread::id cuurent_id;
    // connection send message of callback
    void(* send_callback_)(void*, const udp::endpoint&,const char *,size_t) { nullptr };
    void(* async_send_callback_)(void*, const udp::endpoint&,const packet&) { nullptr };
    std::function<void(channel_view,bool)> connect_callback_;
    std::function<void(channel_view,packet)> message_callback_;
    // half channel
    std::unordered_map<uint32_t,udp::endpoint> half_channel_;
    std::vector<std::shared_ptr<channel>> event_channel_;
    // channel container 
    channel_container container_;
    // timer tasks
    timer_tasks tasks_;
    
}; // class connection_manager

} //namespace kcp