#pragma once 


#include "channel_manager.hh"
#include "io_loop.hh"
#include <memory>
#include <thread>
#include <unordered_map>
#include <future>

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
    void start(int port = -1, int core = -1);
    void connect(const std::string& ip, int port, bool is_sole_socket);
    void close_wait();
private:
    static void receive_callback(void* self, void* socket, const udp::endpoint& point, const char* data, size_t size);

private:
    // thread execution function
    void handler(std::promise<bool>& start, std::promise<bool>& end, int core);
#if defined (__linux__)
    bool bind_to_core(int cpu_id);
#endif

private:
    void connect_internal(const std::string& ip, int port, bool is_sole_socket);
    void reconnect(void* socket,const std::string& ip);
    void remove_connect(const udp::endpoint& point);
    void add_connect(void* socket, const std::string& ip, int port);
    void add_connect_time_task(const std::string& host, int timeout);
    void connect_time_task(const std::string& host, int timeout);

private:
    std::shared_ptr<channel_manager> manager_;
    std::shared_ptr<io_loop> loop_;
    // control variable
    std::atomic_bool& stop_;
    std::unordered_map<std::string,void*> connect_;
    // callback function 
    std::function<void(channel_view,bool)> connect_callback_;
    std::function<void(channel_view,packet)> message_callback_;
    // thread 
    std::shared_ptr<std::thread> thread_;
    std::promise<bool> wait_end;
}; // class kcp_thread

} // namespace kcp