

#include "channel_container.hh"
#include "time.hh"

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
void channel_container::clear(){
    channels_.clear();
    return ;
}
std::shared_ptr<channel> channel_container::insert(uint32_t conv, const udp::endpoint& peer, const std::weak_ptr<channel_manager>& manager){
    std::shared_ptr<channel> chann(new channel(conv,peer,manager));
    channels_[conv] = chann;
    uint64_t timing = chann->check(util::time::clock_32());
    timer_.push(timing, conv);
    return chann;
}
void channel_container::remove(uint32_t conv){
    channels_.erase(conv);
    return ;
}
void channel_container::remove_callback(void* self, uint32_t conv){
    channel_container* self_ = static_cast<channel_container*>(self);
    self_->channels_.erase(conv);
    return ;
}

size_t channel_container::size(){
    return channels_.size();
}

void channel_container::stop(){
    clear();
    timer_.stop();
    return ;
}
void channel_container::update_callback(void* self, uint32_t conv ,uint64_t clock){
    channel_container* self_ = static_cast<channel_container*>(self);
    std::shared_ptr<channel> chann_ = self_->find(conv);
    if (!chann_) {
        // the connection has been disconnected
        return ;
    }
    if (!chann_->conn_.is_alive(clock)) {
        chann_->do_timeout();
        return self_->remove(conv);
    }
    chann_->conn_.update(clock);
    uint64_t timing = chann_->conn_.check(clock);
    self_->timer_.push_internal(timing, conv);
    return ;
}


} // namespace kcp