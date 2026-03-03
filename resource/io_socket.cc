
#include "io_socket.hh"
#include <atomic>

#if defined(__linux__)
#include <sys/socket.h>
#elif defined(_WIN32) || defined(_WIN64)

#elif defined(__APPLE__) || defined(__MACH__)

#endif

namespace kcp{

io_socket::io_socket(const std::shared_ptr<asio::io_context>& ctx, std::atomic_bool& stop, int port)
    :socket_(*ctx,udp::v4())
    ,buffer_(RECEIVE_BUFFER_SIZE)
    ,stop_(stop)
{
    // set port reuse
    socket_.set_option(asio::socket_base::reuse_address(true));
#if defined(__linux__)
    int reuseport = 1;
    setsockopt(socket_.native_handle(), SOL_SOCKET, SO_REUSEPORT, &reuseport, sizeof(reuseport));
#else
    socket_.set_option(asio::socket_base::reuse_address(true));
#endif
    boost::asio::socket_base::receive_buffer_size receive_option(SERVER_SOCKET_RECEIVE_BUFFER_SIZE);
    socket_.set_option(receive_option);

    boost::asio::socket_base::send_buffer_size send_option(SERVER_SOCKET_SEND_BUFFER_SIZE);
    socket_.set_option(send_option);
    // bind
    asio::ip::udp::endpoint endpoint(asio::ip::udp::v4(), port);
    socket_.bind(endpoint);
}
io_socket::io_socket(const std::shared_ptr<asio::io_context>& ctx, std::atomic_bool& stop)
    :socket_(*ctx,udp::v4()),buffer_(RECEIVE_BUFFER_SIZE)
    ,stop_(stop)
{}
void io_socket::async_receive(){
    if (stop_.load(std::memory_order_acquire)){
        return;
    }
    boost::asio::socket_base::receive_buffer_size receive_option(CLIENT_SOCKET_RECEIVE_BUFFER_SIZE);
    socket_.set_option(receive_option);

    boost::asio::socket_base::send_buffer_size send_option(CLIENT_SOCKET_SEND_BUFFER_SIZE);
    socket_.set_option(send_option);

    socket_.async_receive_from(
        asio::buffer(buffer_.data(),buffer_.size()),
        remote_endpoint_,
        std::bind(&io_socket::handle_receive, this, _1, _2)
    );
    return ;
}
void io_socket::receive(){
    socket_.receive_from(
        asio::buffer(buffer_.data(),buffer_.size()),
        remote_endpoint_
    );
    return ;
}
void io_socket::async_send_packet(const udp::endpoint& remote_endpoint,const packet& message){
    socket_.async_send_to(
        asio::buffer(message->data(),message->size()),
        remote_endpoint,
        std::bind(&io_socket::handle_send, this, message, _1, _2)
    );
    return ;
}
void io_socket::send_packet(const udp::endpoint& remote_endpoint,const packet& message){
    send(remote_endpoint,message->data(), message->size());
    return ;
}
void io_socket::send(const udp::endpoint& remote_endpoint,void* data, size_t size){
    socket_.send_to(
        asio::buffer(data,size),
        remote_endpoint
    );
    return ;
}
void io_socket::set_receive_callback(void(* callback)(void*,const udp::endpoint&,const char*,size_t), void* ctx){
    receive_callback = callback;
    receive_ctx = ctx;
    return ;
}
void io_socket::set_send_callback(void(* callback)(void*,bool,size_t), void* ctx){
    send_callback = callback;
    send_ctx = ctx;
    return ;
}
void io_socket::send_message_callback(void* self, const udp::endpoint& remote_endpoint,const char* data, size_t size){
    io_socket* self_ = static_cast<io_socket*>(self);
    self_->send(remote_endpoint,(void*)data,size);
    return ;
}
void io_socket::async_send_message_callback(void* self, const udp::endpoint& remote_endpoint,const packet& pack){
    io_socket* self_ = static_cast<io_socket*>(self);
    self_->async_send_packet(remote_endpoint,pack);
    return ;
}

void io_socket::handle_receive(const boost::system::error_code& error, std::size_t size){
    if (!error && receive_callback){
        receive_callback(receive_ctx,remote_endpoint_,(char*)buffer_.data(),size);
    }
    async_receive();
    return ;
}
void io_socket::handle_send(packet send_buffer,const boost::system::error_code& error, std::size_t size){
    if (send_callback){
        send_callback(send_ctx,!error,size);
    }
    return ;
}


} // namespace kcp