#pragma once

#include<shared_mutex>
#include<queue>


using std::queue;

namespace Luna{
    namespace ThreadPool{

template<typename T>
class blocking_queue : protected queue<T>{

public:
    using wlock = std::unique_lock<std::shared_mutex>;
    using rlock = std::shared_lock<std::shared_mutex>;

public:
    blocking_queue() = default;
    ~blocking_queue(){
        clear();
    }

public:
    blocking_queue(const blocking_queue&) = delete;
    blocking_queue(blocking_queue&&) = delete;
    blocking_queue& operator=(const blocking_queue&) = delete;
    blocking_queue& operator=(blocking_queue&&) = delete;

public:
    void empty(){
        rlock lock(mtx_);
        return queue<T>::empty();
    }

    size_t size(){
        rlock lock(mtx_);
        return queue<T>::size();
    }

public:

    void clear(){
        wlock lock(mtx_);
        while(!queue<T>::empty()){
            queue<T>::pop();
        }
    }

    void push(const T& obj){
        wlock lock(mtx_);
        queue<T>::push(obj);
    }

    bool pop(T& holder){
        wlock lock(mtx_);
        if(queue<T>::empty()){
            return false;
        }else{
            holder = std::move(queue<T>::front());
            queue<T>::pop();
        }
        return true;
    }

    template<typename... Args>
    void emplace(Args&&... args){
        wlock lock(mtx_);
        queue<T>::emplace(std::forward<Args>(args)...);
    }

private:
    mutable std::shared_mutex mtx_;
};

    }//ThreadPool namespace
}//Luna namespace