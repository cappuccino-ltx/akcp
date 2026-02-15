

#include "io_loop.hh"

namespace kcp{


io_loop::io_loop(const std::shared_ptr<asio::io_context>& io_, std::atomic_bool& stop,int port){
    if (io_){
        context_ = io_;
    }else {
        context_ = std::make_shared<asio::io_context>();
    }
    socket_ = std::make_shared<io_socket>(*context_,std::ref(stop),port);
}
io_loop::io_loop(const std::shared_ptr<asio::io_context>& io_, std::atomic_bool& stop){
    if (io_){
        context_ = io_;
    }else {
        context_ = std::make_shared<asio::io_context>();
    }
    socket_ = std::make_shared<io_socket>(*context_,std::ref(stop));
}

void io_loop::enable_reader(){
    socket_->async_receive();
}
void io_loop::start(){
    socket_->async_receive();
    context_->run();
}

void io_loop::set_receive_callback(void(* callback)(void*,const udp::endpoint&,const char*,size_t), void* ctx){
    socket_->set_receive_callback(callback, ctx);
}


void io_loop::send_message_internal(const udp::endpoint& remote_endpoint,void* data, size_t size){
    socket_->send(remote_endpoint, data, size);
}
void io_loop::connect(const std::string& host, int port){
    udp::resolver res(*context_);
    udp::endpoint point(*res.resolve(host,std::to_string(port)).begin());
    packet pack = std::make_shared<std::vector<uint8_t>>((const char*)KCP_CONNECT_REQUEST,(const char*)KCP_CONNECT_REQUEST + KCP_PACKAGE_SIZE);
    socket_->async_send_packet(point, pack);
}

} // namespace kcp