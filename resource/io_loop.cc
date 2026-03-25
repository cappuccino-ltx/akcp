

#include "io_loop.hh"
#include "common.hh"

namespace kcp{


io_loop::io_loop(const std::shared_ptr<asio::io_context>& ctx, std::atomic_bool& stop,int port)
    :context_(ctx)
{
    sockets_.emplace_back(std::make_shared<io_socket>(context_,std::ref(stop),port));
}
io_loop::io_loop(const std::shared_ptr<asio::io_context>& ctx, std::atomic_bool& stop)
    :context_(ctx)
{
    sockets_.emplace_back(std::make_shared<io_socket>(context_,std::ref(stop)));
}
void io_loop::start(){
    sockets_[0]->async_receive();
    return ;
}

void* io_loop::get_default_sokcet(){
    return sockets_[0].get();
}
void* io_loop::create_new_client_socket(std::atomic_bool& stop){
    std::shared_ptr<io_socket> socket = std::make_shared<io_socket>(context_,std::ref(stop));
    socket->set_receive_callback(sockets_[0]->receive_callback, sockets_[0]->receive_ctx);
    socket->set_send_callback(sockets_[0]->send_callback, sockets_[0]->send_ctx);
    sockets_.push_back(socket);
    socket->async_receive();
    return socket.get();
}

void io_loop::set_receive_callback(void(* callback)(void*,void*,const udp::endpoint&,const char*,size_t), void* ctx){
    sockets_[0]->set_receive_callback(callback, ctx);
    return ;
}

// static 
void io_loop::send_message_internal(void* socket, const udp::endpoint& remote_endpoint,void* data, size_t size){
    if (!socket) {
        return ;
    }
    io_socket* socket_ = static_cast<io_socket*>(socket);
    socket_->send(remote_endpoint, data, size);
    return ;
}

} // namespace kcp