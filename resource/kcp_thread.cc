
#include "kcp_thread.hh"

namespace kcp{


kcp_thread::kcp_thread(std::atomic<bool>& stop,bool enable_muliti_thread, int n, int index)
    :thread_(std::make_shared<std::thread>(std::bind(&kcp_thread::handler, this, std::ref(stop),enable_muliti_thread,n,index)))
{}
kcp_thread::~kcp_thread(){
    if(thread_){
        thread_->join();
    }
}

// thread execution function
void kcp_thread::handler(std::atomic<bool>& stop,bool enable_muliti_thread, int n, int index){
    manager_ = std::make_shared<channel_manager>(std::ref(stop),enable_muliti_thread,n,index);
    manager_->hook_timer();
    manager_->context_->run();
}


} // namespace kcp