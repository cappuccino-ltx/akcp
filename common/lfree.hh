#pragma once 

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <type_traits>
#include <vector>
#include <atomic>
#include <memory>
#include <thread>
#include <functional>
#include <condition_variable>
#include <optional>

namespace lfree{

#define DEFAULT_RETRY 10
#define DEFAULT_STEAL_THREAD_NUM  4

enum queue_size {
    K003 = 1llu << 5, // 32
    K01 = 1llu << 7,  // 128
    K05 = 1llu << 9,  // 512
    K1 = 1llu << 10,  // 1024
    K2 = 1llu << 11,  // 2048
    K4 = 1llu << 12,  // 4096
    K8 = 1llu << 13,  // 8192
    K16 = 1llu << 14, // 16384
    K32 = 1llu << 15, // 32768
    K65 = 1llu << 16  // 65536
    // 可以添加自定义大小
}; // enum class queue_size
namespace util{
static inline size_t get_proper_size(std::size_t n){
    if (n <= queue_size::K003) return queue_size::K003;
    else if (n <= queue_size::K01) return queue_size::K01;
    else if (n <= queue_size::K05) return queue_size::K05;
    else if (n <= queue_size::K1) return queue_size::K1;
    else if (n <= queue_size::K2) return queue_size::K2;
    else if (n <= queue_size::K4) return queue_size::K4;
    else if (n <= queue_size::K8) return queue_size::K8;
    else if (n <= queue_size::K16) return queue_size::K16;
    else if (n <= queue_size::K32) return queue_size::K32;
    else return queue_size::K65;
}
}
namespace detail{
struct steal;
}
class base_queue{
protected:
    friend struct detail::steal;
    virtual void task_handle() = 0;
};

// If T is a custom type, default construction is required.
// 如果T是自定义类型,需要有默认构造.
template<class T>
class ring_queue : public base_queue{
public:
    ring_queue(std::size_t size = queue_size::K2, int try_count = DEFAULT_RETRY)
        :producer{ 0 }
        ,consumer{ 0 }
        ,retry{ try_count }
    {
        buff.resize(util::get_proper_size(size));
        sequence = new std::atomic<uint64_t>[buff.size()];
        for(std::size_t i = 0; i < buff.size(); i++) {
            sequence[i].store(i,std::memory_order_relaxed);
        }
    }
    virtual ~ring_queue(){
        run.store(false,std::memory_order_release);
        p_cond.notify_all();
        c_cond.notify_all();
        // 等待所有线程结束
        std::unique_lock<std::mutex> lock{ p_mtx };
        p_cond.wait(lock, [&](){return readable() == false; });
        lock.unlock();
        delete [] sequence;
    }
    // 不允许拷贝和赋值
    ring_queue(const ring_queue&) = delete;
    ring_queue& operator=(const ring_queue&) = delete;
    ring_queue(ring_queue&&) = delete;
    ring_queue& operator=(ring_queue&&) = delete;

public:
    // 移动版本的put
    void put(T&& in) {
        if constexpr (std::is_move_assignable_v<T>){
            _put(std::move(in));
        }else {
            put(in);
        }
    }
    void put(const T& in) {
        int yield_count = 1;
        while(try_put(in) == false){
            if (writable()) {
                continue;
            }
            if (yield_count){
                yield_count--;
                std::this_thread::yield();
            }else {
                std::unique_lock<std::mutex> lock{ p_mtx };
                p_cond.wait(lock, [&](){ return writable(); });
            }
        }
    }
    
    bool get(T& out) {
        int yield_count = 1;
        while(try_get(out) == false){
            if (readable()) {
                continue;
            }
            if (yield_count){
                yield_count--;
                std::this_thread::yield();
            }else {
                std::unique_lock<std::mutex> lock{ c_mtx };
                c_cond.wait(lock, [&](){return readable() || !run.load(std::memory_order_acquire); });
            }
            if(readable() == false && !run.load(std::memory_order_acquire) ){
                return false;
            }
        }
        return true;
    }
    

    bool try_put(const T& in) {
        return _put(in);
    }
    bool try_get(T& out) {
        return _get(out);
    }

    bool readable(){
        uint64_t tail = consumer.load(std::memory_order_acquire);
        uint64_t index = tail & (buff.size() - 1);
        uint64_t prom = sequence[index].load(std::memory_order_acquire);
        // 如果可读
        if (prom == tail + 1){
            return true;
        }
        return false;
    }
    bool writable(){
        uint64_t head = producer.load(std::memory_order_acquire);
        uint64_t index = head & (buff.size() - 1);
        uint64_t prom = sequence[index].load(std::memory_order_acquire);
        // 如果可写
        if (prom == head){
            return true;
        }
        return false;
    }
    // 求队列中元素个数，是一个瞬间值。
    size_t size() const {
        uint64_t head = producer.load(std::memory_order_acquire);
        uint64_t tail = consumer.load(std::memory_order_acquire);
        return head - tail;
    }
    void quit(){
        run.store(false,std::memory_order_release);
        p_cond.notify_all();
        c_cond.notify_all();
    }

protected:
    // cas失败一定次数之后,或者队列满了之后,会调用这个接口,可以进行休眠或者等待
    // 如果在其中正确处理了数据的插入, 返回true
    virtual bool failed_try_put(const T& in) {
        return false;
    }
    // cas失败一定次数之后,或者队列为空之后,会调用这个接口,可以进行休眠或者等待
    // 如果在其中正确处理了数据的读取, 返回true
    virtual bool failed_try_get(T& out) {
        return false;
    }

    virtual void task_handle() {
        // null task
    };

private:
    void _put(T&& in) {
        while(1) {
            uint64_t head = producer.load(std::memory_order_relaxed);
            uint64_t index = head & (buff.size() - 1);
            uint64_t prom = sequence[index].load(std::memory_order_acquire);
            // 如果可写
            if (prom == head){
                // 占位
                if (producer.compare_exchange_weak(head, head + 1,std::memory_order_release,std::memory_order_relaxed)){
                    // 可写
                    buff[index].emplace(std::move(in));
                    sequence[index].store(head + 1,std::memory_order_release);
                    c_cond.notify_one();
                    return ;
                }
            }else if (prom < head){
                // 未读,不可写,需要唤醒消费者
                c_cond.notify_all();
            }
        }
    }
    bool _put(const T& in) {
        int retry_count = 0;
        while(1) {
            uint64_t head = producer.load(std::memory_order_relaxed);
            uint64_t index = head & (buff.size() - 1);
            uint64_t prom = sequence[index].load(std::memory_order_acquire);
            // 如果可写
            if (prom == head){
                // 占位
                if (producer.compare_exchange_weak(head, head + 1,std::memory_order_release,std::memory_order_relaxed)){
                    // 可写
                    buff[index].emplace(in);
                    sequence[index].store(head + 1,std::memory_order_release);
                    c_cond.notify_one();
                    return true;
                }else {
                    // failed
                    if (++retry_count == retry){
                        retry_count = 0;
                        if (failed_try_put(in)){
                            // 正确处理数据,直接返回
                            c_cond.notify_one();
                            return true;
                        }
                        return false;
                        
                    }
                }
            }else if (prom < head){
                // 当前位置未读,不可写,需要唤醒消费者
                c_cond.notify_all();
                if (failed_try_put(in)){
                    // 正确处理数据,直接返回
                    c_cond.notify_one();
                    return true;
                }
                return false;
            }
        }
        return false;
    }
    bool _get(T& out) {
        int retry_count = 0;
        while(1){
            uint64_t tail = consumer.load(std::memory_order_relaxed);
            uint64_t index = tail & (buff.size() - 1);
            uint64_t prom = sequence[index].load(std::memory_order_acquire);
            // 如果可读
            if (prom == tail + 1){
                // 占位
                if (consumer.compare_exchange_weak(tail, tail + 1,std::memory_order_release,std::memory_order_relaxed)){
                    // 可读
                    out = std::move(*buff[index]);
                    buff[index].reset();
                    sequence[index].store(tail + buff.size(),std::memory_order_release);
                    p_cond.notify_one();
                    return true;
                }else {
                    if (failed_try_get(out)){
                        // 正确处理数据,直接返回
                        p_cond.notify_one();
                        return true;
                    }
                    if (++retry_count == retry){
                        retry_count = 0;
                        return false;
                    }
                }
            }else if (prom == tail) {
                // 当前位置没有数据,不可读,需要唤醒生产者
                p_cond.notify_all();
                if (failed_try_get(out)){
                    // 正确处理数据,直接返回
                    p_cond.notify_one();
                    return true;
                }
                return false;
            }
        }
        return false;
    }
    
protected:

protected:
    int retry;

private:
    
    std::atomic<uint64_t>* sequence;
    std::vector<std::optional<T>> buff;
    // index;
    std::atomic<uint64_t> producer;
    std::atomic<uint64_t> consumer;
    std::mutex p_mtx;
    std::mutex c_mtx;
    std::condition_variable p_cond;
    std::condition_variable c_cond;
    std::atomic<bool> run = { true };
};

namespace detail{


struct steal{
    steal(int thread_num, int task_queue_size)
        :threads(thread_num)
        ,run(thread_num > 0)
    {
        for (int i = 0; i < thread_num; ++i) {
            threads[i] = std::make_shared<std::thread>(std::bind(&steal::handle, this));
        }
    }
    ~steal(){
        run.store(false,std::memory_order_release);
        cond.notify_all();
        for (int i = 0; i < threads.size(); ++i) {
            threads[i]->join();
        }
    }

    void handle(){
        while(run.load(std::memory_order_relaxed)){
            {
                // 睡眠
                std::unique_lock<std::mutex> lock(mtx);
                cond.wait(lock,[&](){
                    return task_count.load(std::memory_order_relaxed) || !run.load(std::memory_order_relaxed);
                });
            }
            while(tasks.readable()){
                base_queue* task;
                bool suc = tasks.try_get(task);
                // 执行任务
                if(suc){
                    task->task_handle();
                }
            }
        }
    }
    void push(base_queue* task){
        tasks.put(task);
        cond.notify_one();
    }
    void notify(){
        cond.notify_all();
    }

    bool readable(){
        return tasks.readable();
    }

    bool running(){
        return run.load(std::memory_order_acquire);
    }
    
    std::vector<std::shared_ptr<std::thread>> threads; // thread list
    ring_queue<base_queue*> tasks;
    std::atomic<int> task_count { 0 };
    std::atomic<bool> run { true };
    std::mutex mtx;
    std::condition_variable cond;
}; // struct steal 

inline steal& global_steal(int thread_num = DEFAULT_STEAL_THREAD_NUM, int task_queue_size = queue_size::K1){
    static steal gs{ thread_num, task_queue_size };
    return gs;
}
} // namespace detail

inline void init_steal(int thread_num, int task_queue_size){
    detail::global_steal(thread_num, task_queue_size);
}

template<class T>
class queue : public ring_queue<T>{
public:
    queue(std::size_t size = queue_size::K2, std::size_t steal_queue_size = queue_size::K1)
        :ring_queue<T>(size)
        ,steal_queue(steal_queue_size)
    {}

    ~queue(){
        if(qqueue.readable()){
            detail::global_steal().notify();
        }
    }

    bool readable(){
        return ring_queue<T>::readable() || steal_queue.readable() || detail::global_steal().readable() || qqueue.readable();
    }



protected:
    // cas失败一定次数之后,或者队列满了之后,会调用这个接口,可以进行休眠或者等待
    // 如果在其中正确处理了数据的插入, 返回true
    virtual bool failed_try_put(const T& in) override {
        // 定义局部的threadlocal队列
        static thread_local std::shared_ptr<ring_queue<T>> local_queue = std::make_shared<ring_queue<T>>(K003);
        // 如果steal线程是启动状态 让steal线程插入任务
        detail::steal& gs = detail::global_steal();
        if (gs.running()){
            // 把数据直接插入到本地队列中,这个时候是不会出现cas竞争的
            local_queue->put(in);
            // 将local_queue的地址注册到qqueue中
            qqueue.put(local_queue);
            // 让任务窃取线程调用task_headle
            gs.push(this);
            return true;
        }
        return false;
    }
    // cas失败一定次数之后,或者队列为空之后,会调用这个接口,可以进行休眠或者等待
    // 如果在其中正确处理了数据的读取, 返回true
    virtual bool failed_try_get(T& out) override {
        // 尝试从本地队列中获取
        return steal_queue.try_get(out);
    }
    // 任务窃取线程从对应的本地队列中获取元素,插入到steal_queue中,
    virtual void task_handle() override {
        // 获取注册了任务的本地队列地址
        std::shared_ptr<ring_queue<T>> q;
        qqueue.get(q);
        auto local_q = q;
        if (local_q.get() == nullptr) return ;
        while(local_q->readable()) {
            T temp;
            if (local_q->try_get(temp)){
                steal_queue.put(temp);
            }
        }
    }

private:
    ring_queue<T> steal_queue;
    ring_queue<std::shared_ptr<ring_queue<T>>> qqueue {K003};
};

} // namespace lfree