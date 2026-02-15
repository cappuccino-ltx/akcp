#pragma once 

#include <atomic>
#include <common.hh>
#include <protocol.hh>
#include "kcp_thread.hh"
#include "io_loop.hh"


namespace kcp{

class client{
public:
    client(){}
    void set_connect_callback(const std::function<void(channel_view,bool)>& callback){
        connect_callback_ = callback;
    }
    void set_message_callback(const std::function<void(channel_view,packet)>& callback){
        message_callback_ = callback;
    }
    void start(){
        kcp_thread_ = std::make_shared<kcp_thread>(std::ref(stop_));
        auto manager = kcp_thread_->get_manager();
        loop_ = std::make_shared<io_loop>(manager->context_, std::ref(stop_));
        // set callback function
        // manager -> socket
        manager->set_send_callback(io_socket::send_message_callback, loop_->socket_.get());
        // socket -> manager
        loop_->set_receive_callback(client::receive_callback, this);
        // manager -> user
        // connnect
        manager->set_connect_callback(connect_callback_);
        // message
        manager->set_message_callback(message_callback_);

        loop_->enable_reader();
    }
    void connect(const std::string& host,int port){
        loop_->connect(host, port);
    }
    void stop(){
        stop_.store(true, std::memory_order_release);
    }

private:
    static void receive_callback(void* self,const udp::endpoint& point,const char* data,size_t size){
        client* self_ = static_cast<client*>(self);
        if (size == KCP_PACKAGE_SIZE) {
            uint32_t conv = protocol::parse_conv_from_response(data);
            if (conv != 0){
                channel_manager::push_new_link_callback(self_->kcp_thread_->manager_.get(),conv,point);
                return ;
            }
        }
        packet pack = std::make_shared<std::vector<uint8_t>>(data, data + size);
        channel_manager::push_package_callback(self_->kcp_thread_->manager_.get(),point,data,size);
    }

private:
// control variable
    std::atomic_bool stop_ {false};
    std::shared_ptr<io_loop> loop_;
    std::shared_ptr<kcp_thread> kcp_thread_;
    // callback function 
    std::function<void(channel_view,bool)> connect_callback_;
    std::function<void(channel_view,packet)> message_callback_;
}; // class client

} // namespace kcp