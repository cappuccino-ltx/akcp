#pragma once 



#include <cstddef>
#include <unordered_map>
#include <vector>
#include <functional>
#include <lockfree.hh>

namespace buffer_pool{

#define interval_begin_default 128
#define interval_end_default 1024
#define queue_size_default 128
#define buffer_get_push_retry 3

// 容器池,不用大量重复申请和释放对象
template<typename T>
class BufferPool{
private:
    BufferPool(int queue_size, int interval_begin, int interval_end){
        size_t begin = lockfree::util::glign_to_2_index(interval_begin);
        size_t end = lockfree::util::glign_to_2_index(interval_end);
        buffer_size_min = begin;
        buffer_size_max = end;
        for (size_t i = begin; i <= end; i <<= 1){
            queue_map.insert({i,std::unique_ptr<queue>(new queue(
                std::thread::hardware_concurrency(),
                lockfree::util::get_proper_size(queue_size)
            ))});
        }
    }
public:
    using Buffer = std::shared_ptr<std::vector<T>>;
    Buffer get_buffer(size_t size){
        std::vector<T>* buf = nullptr;
        int index = lockfree::util::glign_to_2_index(size);
        if (index < buffer_size_min) {
            index = buffer_size_min;
        }
        if (index <= buffer_size_max) {
            queue_ptr& q = queue_map[index];
            int retry = buffer_get_push_retry;
            while(retry--) {
                if(q->try_get(buf)){
                    break;
                }
            }
        }
        if (!buf){
            buf = new std::vector<T>;
            buf->reserve(index);
        }
        return Buffer{ buf, std::bind(&BufferPool::delete_opt, this, std::placeholders::_1) };
    }

    static BufferPool<T>& get_instance(int queue_size = queue_size_default, int interval_begin = interval_begin_default, int interval_end = interval_end_default){
        static BufferPool<T> pool(queue_size, interval_begin, interval_end);
        return pool;
    }

    ~BufferPool(){
        std::vector<T>* ptr = nullptr;
        for (auto& q : queue_map){
            while (q.second->size_approx()){
                if (q.second->try_get(ptr)){
                    delete ptr;
                    ptr = nullptr;
                }
            }
        }
    }

private:
    void delete_opt(std::vector<T>* buff){
        size_t size = buff->capacity();
        if (size & (size - 1)) {
            delete buff;
            return ;
        }
        if (size > buffer_size_max || size < buffer_size_min) {
            delete buff;
            return ;
        }
        if (queue_map.count(size) == 0) {
            delete buff;
            return ;
        }
        buff->clear();
        int retry = buffer_get_push_retry;
        while(retry--){
            if (queue_map[size]->try_put(buff)){
                return ;
            }
        }
        delete buff;
    }

private:
    using queue = lockfree::concurrent_queue<std::vector<T>*>;
    using queue_ptr = std::unique_ptr<queue>;
    std::unordered_map<int, queue_ptr> queue_map;
    uint32_t buffer_size_max;
    uint32_t buffer_size_min;
};

template<typename T>
inline BufferPool<T>& init_pool(int queue_size = queue_size_default, int interval_begin = interval_begin_default, int interval_end = interval_end_default){
    return BufferPool<T>::get_instance(queue_size,interval_begin,interval_end);
}

template<typename T>
inline typename BufferPool<T>::Buffer get_buffer(size_t capacity){
    return BufferPool<T>::get_instance().get_buffer(capacity);
}

} // namespace memory_reuse