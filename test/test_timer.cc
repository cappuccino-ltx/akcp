

#include "time.hh"
#include "timer.hh"
#include <boost/asio/io_context.hpp>
#include <memory>

void handler(void* self, uint32_t conv, uint64_t now){
    std::cout << "定时任务被执行" << std::endl;
}

void test01() {
    kcp::asio::io_context ctx;
    std::atomic_bool stop = false;
    kcp::timer time(ctx, stop);
    time.set_update_callback(handler, nullptr);
    time.push(kcp::util::time::clock_64() + 2000, 0);
    ctx.run();
}

void test02(){
    // kcp::asio::io_context ctx;
    // kcp::asio::system_timer timer_;
}

int main() {
    
    return 0;
}