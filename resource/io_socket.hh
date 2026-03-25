#pragma once

#include <memory>
#if defined(__linux__)
#include <sys/socket.h>
#include <bits/types/struct_iovec.h>
#endif
#include <atomic>
#include <vector>
#include "common.hh"

namespace kcp{


class io_socket{
    friend class io_loop;
public:
    explicit io_socket(const std::shared_ptr<asio::io_context>& ctx, std::atomic_bool& stop, int port);
    explicit io_socket(const std::shared_ptr<asio::io_context>& ctx, std::atomic_bool& stop);
    void init() ;
    ~io_socket();
    void async_receive();
    void receive();
    void async_send_packet(const udp::endpoint& remote_endpoint,const packet& message);
    void send_packet(const udp::endpoint& remote_endpoint,const packet& message);
    void send(const udp::endpoint& remote_endpoint,void* data, size_t size);
    void set_receive_callback(void(* callback)(void*,void*,const udp::endpoint&,const char*,size_t),void* ctx);
    void set_send_callback(void(* callback)(void*,bool,size_t),void* ctx);
    static void send_message_callback(void* self, const udp::endpoint& remote_endpoint,const char* data, size_t size);
    static void async_send_message_callback(void* self, const udp::endpoint& remote_endpoint,const packet& pack);

private:
    void handle_receive(const boost::system::error_code& error, std::size_t size);
    void handle_send(packet send_buffer,const boost::system::error_code& error, std::size_t size);

private:

#if defined(__linux__)
    void send_message(const udp::endpoint& remote_endpoint,const packet& message);
    void register_send_task();
    void do_batch_send();
    void do_batch_read();
#endif

private:
#if defined(__linux__)
    std::shared_ptr<asio::io_context> ctx_;
#endif
    udp::socket socket_;
    std::vector<uint8_t> buffer_;
    udp::endpoint remote_endpoint_;
    void* receive_ctx { nullptr };
    void* send_ctx { nullptr };
    void(*receive_callback)(void*,void*,const udp::endpoint&,const char*,size_t){ nullptr };
    void(*send_callback)(void*,bool,size_t) { nullptr };
#if defined(__linux__)
    struct send_data{
        packet message;
        udp::endpoint address;
    };
    std::vector<send_data> message_queue_;
    struct batch_read{
        struct mmsghdr msgs[BATCH_IO_BUFFER_NUM];
        struct iovec iovs[BATCH_IO_BUFFER_NUM];
    };
    std::shared_ptr<batch_read> batch_read_ { nullptr };
    struct batch_write{
        struct mmsghdr msgs[BATCH_IO_BUFFER_NUM];
        struct iovec iovecs[BATCH_IO_BUFFER_NUM];
        char buffers[BATCH_IO_BUFFER_NUM][BATCH_IO_BUFFER_SIZE]; // 每个包的最大长度
        struct sockaddr_in addrs[BATCH_IO_BUFFER_NUM];
    };
    std::shared_ptr<batch_write> batch_write_ { nullptr };
#endif
    // control variable
    std::atomic_bool& stop_;
}; // class io_socket

} // namespace kcp