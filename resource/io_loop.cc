

#include "io_loop.hh"
#include "common.hh"

namespace kcp{


io_loop::io_loop(const std::shared_ptr<asio::io_context>& ctx, std::atomic_bool& stop,int port)
    :context_(ctx)
{
    socket_ = std::make_shared<io_socket>(context_,std::ref(stop),port);
}
io_loop::io_loop(const std::shared_ptr<asio::io_context>& ctx, std::atomic_bool& stop)
    :context_(ctx)
{
    socket_ = std::make_shared<io_socket>(context_,std::ref(stop));
}
void io_loop::start(){
    socket_->async_receive();
    return ;
}

void* io_loop::get_default_sokcet(){
    return socket_.get();
}
void* io_loop::create_new_client_socket(std::atomic_bool& stop){
    std::shared_ptr<io_socket> socket = std::make_shared<io_socket>(context_,std::ref(stop));
    socket->set_receive_callback(socket_->receive_callback, socket_->receive_ctx);
    socket->set_send_callback(socket_->send_callback, socket_->send_ctx);
    socket->async_receive();
    sockets_[socket.get()] = socket;
    return socket.get();
}
void io_loop::remove_client_socket(void* socket){
    io_socket* tar = static_cast<io_socket*>(socket);
    if (tar != socket_.get() && sockets_.count(tar)) {
        sockets_[tar]->socket_.cancel();
        sockets_.erase(tar);
    }
}
void io_loop::set_receive_callback(void(* callback)(void*,void*,const udp::endpoint&,const char*,size_t), void* ctx){
    socket_->set_receive_callback(callback, ctx);
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