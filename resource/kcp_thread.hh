#pragma once 

#include "channel_manager.hh"
#include <chrono>
#include <thread>

namespace kcp{

class kcp_thread{
    friend class server;
    friend class client;
public:
    kcp_thread(std::atomic<bool>& stop,bool enable_muliti_thread = false, int n = 0, int index = 0);
    ~kcp_thread();
    void stop(){
        if (manager_) {
            manager_->stop();
        }
    }

private:
    // thread execution function
    void handler(std::atomic<bool>& stop,bool enable_muliti_thread, int n, int index);
    std::shared_ptr<channel_manager> get_manager(){
        while(!manager_){
            std::this_thread::sleep_for(std::chrono::milliseconds(UPDATE_INTERVAL));
        }
        return manager_;
    }

private:
    std::shared_ptr<channel_manager> manager_;
    std::shared_ptr<std::thread> thread_;
}; // class kcp_thread

} // namespace kcp