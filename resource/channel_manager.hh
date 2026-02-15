#pragma once 

#include <cstdint>
#include <lfree.hh>
#include <thread>
#include "channel_container.hh"

namespace kcp{

class channel_manager : public std::enable_shared_from_this<channel_manager>{
public:
    friend class kcp_thread;
    friend class server;
    friend class client;
public:
    channel_manager(std::atomic<bool>& stop,bool enable_muliti_thread = false, int n = 0, int index = 0);
    // muliti-threaded mode
    void push_package(const std::shared_ptr<package>& pack);
    void push_new_link(const std::shared_ptr<link_data>& pack);
    // single-threaded mode
    static void push_package_callback(void* self,const udp::endpoint& point, const char* data, size_t size);
    static void push_new_link_callback(void* self, uint32_t conv, const udp::endpoint& peer);
    // set the callback function for sending messages in the socket layer to the channel
    void set_send_callback(void(* callback)(void*, const udp::endpoint&,const char*, size_t),void* ctx);
    void set_async_send_callback(void(* callback)(void*, const udp::endpoint&,const packet&),void* ctx);
    // set the callback for receive message in the application layer to the channel
    // void set_receive_callback(void(* callback)(void*,packet),void* ctx);
    void set_connect_callback(const std::function<void(channel_view,bool)> & callback);
    void set_message_callback(const std::function<void(channel_view,packet)> & callback);
    static void set_update_interval(size_t val);
    void post(const std::function<void(void)>& task);
    bool whthin_the_current_thread();
    void stop(){
        container_.stop();
    }

private:
    // muliti-thread exchange
    void handler_packets();
    void handler_new_link();
    void handler();
    // process data and links
    void receive_packet(const udp::endpoint& point, const char* data, size_t size);
    void create_linked(uint32_t conv, const udp::endpoint& peer);
    // timer
    void hook_timer();
    void handler_timer();
    
private:
    // event loop and timer 
    std::shared_ptr<asio::io_context> context_;
    asio::system_timer timer_;
    // control switch
    std::atomic<bool>& stop_;
    // channel container 
    channel_container container_;
    std::thread::id cuurent_id;
    // lock free queue
    std::shared_ptr<lfree::ring_queue<std::shared_ptr<package>>> packages_;
    std::shared_ptr<lfree::ring_queue<std::shared_ptr<link_data>>> new_link_;
    // connection send message of callback
    void* send_ctx_ { nullptr };
    void(* send_callback_)(void*, const udp::endpoint&,const char *,size_t) { nullptr };
    void(* async_send_callback_)(void*, const udp::endpoint&,const packet&) { nullptr };

    std::function<void(channel_view,bool)> connect_callback_;
    std::function<void(channel_view,packet)> message_callback_;
    // update interval
    static int update_interval_;
}; // class connection_manager

} //namespace kcp