#pragma once 

#include "channel_manager.hh"
#include "io_loop.hh"
#include <memory>
#include <thread>

namespace kcp{

class kcp_thread{
    friend class server;
    friend class client;
public:
    kcp_thread(std::atomic<bool>& stop);
    ~kcp_thread();
    void stop();
    void set_connect_callback(const std::function<void(channel_view,bool)>& callback);
    void set_message_callback(const std::function<void(channel_view,packet)>& callback);
    void start(int port = -1, bool is_separate_thread = true);
    void connect(const std::string& host, int port);
private:
    static void receive_callback(void* self,const udp::endpoint& point,const char* data,size_t size);

private:
    // thread execution function
    void handler();
    std::shared_ptr<channel_manager> get_manager();

private:
    std::shared_ptr<channel_manager> manager_;
    std::shared_ptr<io_loop> loop_;
    // control variable
    std::atomic_bool& stop_;
    // callback function 
    std::function<void(channel_view,bool)> connect_callback_;
    std::function<void(channel_view,packet)> message_callback_;
    // thread 
    std::shared_ptr<std::thread> thread_;
}; // class kcp_thread

} // namespace kcp