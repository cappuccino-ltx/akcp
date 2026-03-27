
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
#if defined(__linux__)
    ,ctx_(ctx)
#endif
    ,buffer_(RECEIVE_BUFFER_SIZE)
    ,stop_(stop)
{
    // set port reuse
#if defined(__linux__)
    int reuseport = 1;
    setsockopt(socket_.native_handle(), SOL_SOCKET, SO_REUSEPORT, &reuseport, sizeof(reuseport));
#else
    socket_.set_option(asio::socket_base::reuse_address(true));
#endif
    // resize buffer
    boost::asio::socket_base::receive_buffer_size receive_option(SERVER_SOCKET_RECEIVE_BUFFER_SIZE);
    socket_.set_option(receive_option);
    boost::asio::socket_base::send_buffer_size send_option(SERVER_SOCKET_SEND_BUFFER_SIZE);
    socket_.set_option(send_option);

    // bind
    asio::ip::udp::endpoint endpoint(asio::ip::udp::v4(), port);
    socket_.bind(endpoint);
    init();
}
io_socket::io_socket(const std::shared_ptr<asio::io_context>& ctx, std::atomic_bool& stop)
    :socket_(*ctx,udp::v4()),buffer_(RECEIVE_BUFFER_SIZE)
#if defined(__linux__)
    ,ctx_(ctx)
#endif
    ,stop_(stop)
{
    init();
}
io_socket::~io_socket(){
    socket_.close();
}
void io_socket::init() {
#if defined (__linux__)
    batch_read_ = std::make_shared<batch_read>();
    batch_write_ = std::make_shared<batch_write>();
#endif
}
void io_socket::async_receive(){
    if (stop_.load(std::memory_order_acquire)){
        return;
    }
#if defined(__linux__)
    socket_.async_wait(udp::socket::wait_read,[this](boost::system::error_code ec){
        if (!ec) {
            do_batch_read();
        }else {
            return ;
        }
        async_receive();
    });
    return;
#else
    socket_.async_receive_from(
        asio::buffer(buffer_.data(),buffer_.size()),
        remote_endpoint_,
        std::bind(&io_socket::handle_receive, this, _1, _2)
    );
    return ;
#endif
}
void io_socket::receive(){
    socket_.receive_from(
        asio::buffer(buffer_.data(),buffer_.size()),
        remote_endpoint_
    );
    return ;
}
void io_socket::async_send_packet(const udp::endpoint& remote_endpoint,const packet& message){
#if defined (__linux__)
    return send_message(remote_endpoint, message);
#else
    socket_.async_send_to(
        asio::buffer(message->data(),message->size()),
        remote_endpoint,
        std::bind(&io_socket::handle_send, this, message, _1, _2)
    );
    return ;
#endif
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
void io_socket::set_receive_callback(void(* callback)(void*,void*,const udp::endpoint&,const char*,size_t), void* ctx){
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
        receive_callback(receive_ctx,this,remote_endpoint_,(char*)buffer_.data(),size);
    }else if (error){
        return;
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

#if defined(__linux__)

void io_socket::send_message(const udp::endpoint& remote_endpoint,const packet& message){
    register_send_task();
    message_queue_.push_back({message, remote_endpoint});
}
void io_socket::register_send_task(){
    if (message_queue_.empty() == false) {
        return;
    }
    ctx_->post([this](){
        this->do_batch_send();
    });
}
void io_socket::do_batch_send(){
    int fd = socket_.native_handle();
    while(message_queue_.empty() == false){

        int count = std::min((int)message_queue_.size(), BATCH_IO_BUFFER_NUM);
        for (int i = 0; i < count; i++){
            batch_read_->iovs[i].iov_base = message_queue_[i].message->data();
            batch_read_->iovs[i].iov_len = message_queue_[i].message->size();

            batch_read_->msgs[i].msg_hdr.msg_iov = &batch_read_->iovs[i];
            batch_read_->msgs[i].msg_hdr.msg_iovlen = 1;
            batch_read_->msgs[i].msg_hdr.msg_name = const_cast<sockaddr*>(message_queue_[i].address.data()); // 目标 sockaddr
            batch_read_->msgs[i].msg_hdr.msg_namelen = sizeof(sockaddr_in);
            batch_read_->msgs[i].msg_hdr.msg_control = nullptr;
            batch_read_->msgs[i].msg_hdr.msg_controllen = 0;
            batch_read_->msgs[i].msg_hdr.msg_flags = 0;
        }
        // sent
        int sent_n = sendmmsg(fd, batch_read_->msgs, count, 0);
        if (sent_n > 0) {
            message_queue_.erase(message_queue_.begin(),message_queue_.begin() + sent_n);
        }
    }
}
void io_socket::do_batch_read(){
    memset(batch_write_->msgs, 0, sizeof(batch_write_->msgs));
    for (int i = 0; i < BATCH_IO_BUFFER_NUM; ++i) {
        batch_write_->iovecs[i].iov_base = batch_write_->buffers[i];
        batch_write_->iovecs[i].iov_len = sizeof(batch_write_->buffers[i]);
        batch_write_->msgs[i].msg_hdr.msg_iov = &batch_write_->iovecs[i];
        batch_write_->msgs[i].msg_hdr.msg_iovlen = 1;
        batch_write_->msgs[i].msg_hdr.msg_name = &batch_write_->addrs[i];
        batch_write_->msgs[i].msg_hdr.msg_namelen = sizeof(batch_write_->addrs[i]);
    }
    int sockfd = socket_.native_handle();
    int read_n = recvmmsg(sockfd, batch_write_->msgs, BATCH_IO_BUFFER_NUM, MSG_DONTWAIT, nullptr);
    if (read_n > 0) {
        for (int i = 0; i < read_n; ++i) {
            // 4. 处理每一个读取到的数据包
            std::string data(batch_write_->buffers[i], batch_write_->msgs[i].msg_len);
            receive_callback(
                receive_ctx, 
                this, 
                udp::endpoint(
                    boost::asio::ip::address_v4(ntohl(batch_write_->addrs[i].sin_addr.s_addr)), 
                    ntohs(batch_write_->addrs[i].sin_port)
                ),
                batch_write_->buffers[i],
                batch_write_->msgs[i].msg_len);
        }
    }
}
#endif


} // namespace kcp