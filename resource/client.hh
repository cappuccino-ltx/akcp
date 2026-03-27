#pragma once 

#include <atomic>
#include "common.hh"
#include <functional>
#include "kcp_thread.hh"


namespace kcp{

class client{
public:
    client(int core = -1);
    void set_connect_callback(const std::function<void(channel_view,bool)>& callback);
    void set_message_callback(const std::function<void(channel_view,packet)>& callback);
    void set_thread_quit_callback(const std::function<void()>& back);
    void set_thread_start_callback(const std::function<void()>& back);
    void connect(const std::string& host,int port, bool is_sole_socket = false);
    void stop();
    void disable_low_latency(int heartbeat_time = KCP_INTERVAL);
    void set_buffer_pool(const std::function<packet(size_t)>& back);

private:
    void stop_internal();
    void init(int core = -1);

private:
// control variable
    std::atomic_bool stop_ {false};
    std::shared_ptr<kcp_thread> kcp_thread_;
}; // class client

} // namespace kcp