#include "timer.hh"
#include "time.hh"

namespace kcp{

timer::timer(asio::io_context& io_ctx,std::atomic_bool& stop)
    :timer_(io_ctx)
    ,stop_(stop)
{
    arm_to_top(0);
}

void timer::push(uint64_t clock, uint32_t conv){
    Item it{.next_time_ = clock,.conv = conv};
    heap_.push(it);
    if (clock < next_timeout_){
        timer_.cancel();
    }
    return ;
}
void timer::set_update_callback(void(* callback)(void*, uint32_t, uint64_t), void* ctx){
    update_callbacl_ = callback;
    update_ctx_ = ctx;
    return ;
}
void timer::stop(){
    std::priority_queue<Item,std::vector<Item>,Compare> temp;
    heap_.swap(temp);
    timer_.cancel();
    return ;
}
void timer::push_internal(uint64_t clock, uint32_t conv){
    Item it{.next_time_ = clock,.conv = conv};
    tasks_.push_back(it);
    return ;
}

void timer::handler(boost::system::error_code ec){
    uint64_t now = util::time::clock_64();
    if (ec == asio::error::operation_aborted) {
        if (!stop_ || !heap_.empty()){
            arm_to_top(now);
        }
        return;
    }
    handler_timeout(now);
    handler_push_internal();
    if (!stop_ || !heap_.empty()){
        arm_to_top(now);
    }
    return ;
}
void timer::handler_push_internal(){
    for (auto& item : tasks_) {
        heap_.push(item);
    }
    tasks_.clear();
    return ;
}
void timer::handler_timeout(uint64_t now){
    while(!heap_.empty()) {
        Item top = heap_.top();
        if (top.next_time_ > now){
            break;
        }
        heap_.pop();
        if (update_callbacl_){
            update_callbacl_(update_ctx_,top.conv,now);
        }
    }
    return ;
}
void timer::arm_to_top(uint64_t now){
    if (heap_.empty()) {
        next_timeout_ = MAX_TIMEOUT;
    }else {
        next_timeout_ = heap_.top().next_time_;
    }
    timer_.expires_after(std::chrono::milliseconds( next_timeout_ - now));
    timer_.async_wait(std::bind(&timer::handler,this,_1));
    return ;
}


bool timer::Compare::operator()(const timer::Item& i1,const timer::Item& i2){
    return i1.next_time_ > i2.next_time_;
}

} // namespace kcp