#pragma once 

#include <atomic>
#include <common.hh>
#include <functional>
#include <protocol.hh>
#include "kcp_thread.hh"


namespace kcp{

class client{
public:
    client();
    void set_connect_callback(const std::function<void(channel_view,bool)>& callback);
    void set_message_callback(const std::function<void(channel_view,packet)>& callback);
    void connect(const std::string& host,int port);
    void stop();

private:
    void stop_internal();
    static void receive_callback(void* self,const udp::endpoint& point,const char* data,size_t size);
    void init();

private:
// control variable
    std::atomic_bool stop_ {false};
    std::shared_ptr<kcp_thread> kcp_thread_;
}; // class client

} // namespace kcp