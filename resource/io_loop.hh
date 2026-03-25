#pragma once

#include "common.hh"
#include "io_socket.hh"

namespace kcp{

class io_loop{
    friend class kcp_thread;
    friend class client;
public:
    explicit io_loop(const std::shared_ptr<asio::io_context>& io_, std::atomic_bool& stop,int port);
    explicit io_loop(const std::shared_ptr<asio::io_context>& io_, std::atomic_bool& stop);
    void start();
    void* get_default_sokcet();
    void* create_new_client_socket(std::atomic_bool& stop);
    void set_receive_callback(void(* callback)(void*,void*,const udp::endpoint&,const char*,size_t), void* ctx);
    static void send_message_internal(void* socket, const udp::endpoint& remote_endpoint,void* data, size_t size);
    

private:
    std::shared_ptr<asio::io_context> context_;
    std::vector<std::shared_ptr<io_socket>> sockets_;
}; //  class io_loop

} // namespace kcp