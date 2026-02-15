#pragma once

#include <atomic>
#include <common.hh>

namespace kcp{


class io_socket{
public:
    explicit io_socket(asio::io_context& context, std::atomic_bool& stop, int port);
    io_socket(asio::io_context& context, std::atomic_bool& stop);
    void async_receive();
    void receive();
    void async_send_packet(const udp::endpoint& remote_endpoint,const packet& message);
    void send_packet(const udp::endpoint& remote_endpoint,const packet& message);
    void send(const udp::endpoint& remote_endpoint,void* data, size_t size);
    void set_receive_callback(void(* callback)(void*,const udp::endpoint&,const char*,size_t),void* ctx);
    void set_send_callback(void(* callback)(void*,bool,size_t),void* ctx);
    static void send_message_callback(void* self, const udp::endpoint& remote_endpoint,const char* data, size_t size);
    static void async_send_message_callback(void* self, const udp::endpoint& remote_endpoint,const packet& pack);

private:
    void handle_receive(const boost::system::error_code& error, std::size_t size);
    void handle_send(packet send_buffer,const boost::system::error_code& error, std::size_t size);

private:
    udp::socket socket_;
    std::vector<byte> buffer_;
    udp::endpoint remote_endpoint_;
    void* receive_ctx { nullptr };
    void* send_ctx { nullptr };
    void(*receive_callback)(void*,const udp::endpoint&,const char*,size_t){ nullptr };
    void(*send_callback)(void*,bool,size_t) { nullptr };
    // control variable
    std::atomic_bool& stop_;
}; // class io_socket

} // namespace kcp