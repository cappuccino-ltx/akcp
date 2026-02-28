
#include "timer_tasks.hh"
#include "time.hh"

namespace kcp{

timer_tasks::timer_tasks(asio::io_context& io_ctx,std::atomic_bool& stop)
    :timer_(io_ctx, stop)
{
    timer_.set_update_callback(timer_tasks::update_callback, this);
}
uint32_t timer_tasks::push(std::function<void()>&& task,uint32_t milliseconds){
    uint32_t ret = index_++;
    tasks.insert(std::make_pair(ret,std::move(task)));
    timer_.push(util::time::clock_64() + milliseconds, ret);
    return ret;
}
void timer_tasks::cancel(uint32_t index) {
    auto it = tasks.find(index);
    if (it == tasks.end()) {
        return ;
    }
    tasks.erase(it);
    return ;
}
void timer_tasks::update_callback(void* self, uint32_t index ,uint64_t clock){
    timer_tasks* self_ = static_cast<timer_tasks*>(self);
    auto it = self_->tasks.find(index);
    if (it == self_->tasks.end()) {
        return ;
    }
    if(it->second) {
        it->second();
    }
    self_->tasks.erase(it);
    return ;
}

} // namespace kcp