

#include "channel_container.hh"

namespace kcp{

channel_container::channel_container(asio::io_context& io_ctx,std::atomic_bool& stop)
    :timer_(io_ctx,stop)
{
    timer_.set_update_callback(channel_container::update_callback, this);
}

std::shared_ptr<channel> channel_container::find(uint32_t conv){
    auto it = channels_.find(conv);
    if (it == channels_.end()){
        return nullptr;
    }
    return it->second;
}
void channel_container::update(uint32_t clock){
    for(auto it = channels_.begin(); it != channels_.end();) {
        std::shared_ptr<channel>& channel = it->second;
        channel->conn_.update(clock);
        if (false == channel->conn_.is_alive(clock)){
            channel->do_timeout();
            it = channels_.erase(it);
            continue;
        }
        ++it;
    }
}
void channel_container::clear(){
    channels_.clear();
}
std::shared_ptr<channel> channel_container::insert(uint32_t conv, const udp::endpoint& peer, const std::weak_ptr<channel_manager>& manager){
    std::shared_ptr<channel> chann(new channel(conv,peer,manager));
    channels_[conv] = chann;
    return chann;
}
void channel_container::remove(uint32_t conv){
    channels_.erase(conv);
}

size_t channel_container::size(){
    return channels_.size();
}


} // namespace kcp