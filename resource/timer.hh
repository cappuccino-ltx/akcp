#pragma once 

#include <cstdint>
#include <queue>
#include "time.hh"
#include "channel.hh"
namespace kcp{


class timer{
public:
    timer(asio::io_context& io_ctx,std::atomic_bool& stop)
        :timer_(io_ctx)
        ,stop_(stop)
    {
        arm_to_top(0);
    }
    
    void push(uint64_t clock, uint32_t conv){
        Item it{.next_time_ = clock,.conv = conv};
        heap_.push(it);
        if (clock < next_timeout_){
            timer_.cancel();
        }
    }
    void set_update_callback(void(* callback)(void*, uint32_t, uint64_t), void* ctx){
        update_callbacl_ = callback;
        update_ctx_ = ctx;
    }
    void stop(){
        std::priority_queue<Item,std::vector<Item>,Compare> temp;
        heap_.swap(temp);
        timer_.cancel();
    }

private:
    void handler(boost::system::error_code ec){
        uint64_t now = util::time::clock_64();
        if (ec == asio::error::operation_aborted) {
            if (!stop_ || !heap_.empty()){
                arm_to_top(now);
            }
            return;
        }
        handler_timeout(now);
        if (!stop_ || !heap_.empty()){
            arm_to_top(now);
        }
    }
    void handler_timeout(uint64_t now){
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
    }
    void arm_to_top(uint64_t now){
        if (heap_.empty()) {
            next_timeout_ = MAX_TIMEOUT;
        }else {
            next_timeout_ = heap_.top().next_time_;
        }
        timer_.expires_after(std::chrono::milliseconds( next_timeout_ - now));
        timer_.async_wait(std::bind(&timer::handler,this,_1));
    }
    

private:
    struct Item{
        uint64_t next_time_;
        uint32_t conv;
    };
    struct Compare{
        bool operator()(const Item& i1,const Item& i2){
            return i1.next_time_ > i2.next_time_;
        }
    };
    std::priority_queue<Item,std::vector<Item>,Compare> heap_;
    void* update_ctx_;
    void(* update_callbacl_)(void*, uint32_t, uint64_t) {nullptr};
    asio::system_timer timer_;
    uint64_t next_timeout_ { MAX_TIMEOUT };
    std::atomic_bool& stop_;
};

} // namespace kcp