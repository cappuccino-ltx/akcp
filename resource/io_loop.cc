

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

void io_loop::set_receive_callback(void(* callback)(void*,const udp::endpoint&,const char*,size_t), void* ctx){
    socket_->set_receive_callback(callback, ctx);
    return ;
}

void io_loop::send_message_internal(const udp::endpoint& remote_endpoint,void* data, size_t size){
    socket_->send(remote_endpoint, data, size);
    return ;
}

} // namespace kcp