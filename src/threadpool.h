#pragma once

#include "blocking_queue.h"

#include<condition_variable> // ?
#include<functional>
#include<future>
#include<memory>
#include<mutex>
#include<queue>
#include<shared_mutex>
#include<stdexcept> // ?
#include<thread>
#include<utility> //?
#include<vector>

namespace Luna{
    namespace ThreadPool{

template<typename QueueType 
    = blocking_queue<std::function<void()>>>
class threadpool{
public:
    using wlock = std::unique_lock<std::shared_mutex>;
    using rlock = std::shared_lock<std::shared_mutex>;

public:
    threadpool() = default;
    ~threadpool();
    threadpool(const threadpool&) = delete;
    threadpool(threadpool&&) = delete;
    threadpool& operator=(const threadpool&) = delete;
    threadpool& operator=(const threadpool&&) = delete;

public:
    void init(int num);
    void terminate();
    void cancel();

public:
    bool inited() const;
    bool is_running() const;
    int size() const;

private:
    bool _is_running() const{
        return inited_ && !stop_ && !cancel_;
    }

private:
    void spawn();

public:
    template<class F,class... Args>
    auto async(F&& f,Args&&... args) const->std::future<decltype(f(args...))>;

private:
    bool inited_ {false};
    bool stop_{false};
    bool cancel_{false};
    std::vector<std::thread> workers_;
    mutable QueueType tasks_;
    mutable std::shared_mutex mtx_;
    mutable std::condition_variable_any cond_;
    mutable std::once_flag once_;
};

template<typename QueueType>
inline threadpool<QueueType>::~threadpool(){
   terminate();
}

template<typename QueueType>
inline void threadpool<QueueType>::init(int num){
    std::call_once(once_,[this,num](){
        wlock lock(mtx_);
        stop_ = false;
        cancel_ = false;
        workers_.reserve(num);
        for(int i = 0; i < num; ++i){
            workers_.emplace_back(std::bind(&threadpool<QueueType>::spawn,this));
        }
        inited_ = true;
    });
}

template<typename QueueType>
inline void threadpool<QueueType>::terminate(){
    {
        wlock lock(mtx_);
        if(_is_running()){
            stop_ = true;
        }else{
            return ;
        }
    }
    cond_.notify_all();
    for(auto& worker : workers_){
        worker.join();  // ????????????????????????
    }
}

template<typename QueueType>
inline void threadpool<QueueType>::cancel(){
    {
        wlock lock(mtx_);
        if(_is_running()){
            cancel_ = true;
        }else{
            return ;
        }  // why we need to do this,why not just directly workers.join() ??
        //It may be that we need to tell other thread of the threadpool by setting stop_ , cancel_ flags.
    }
    tasks_.clear();
    cond_.notify_all();
    for(auto& worker : workers_){
        worker.join();
    }
}

template<typename QueueType>
inline bool threadpool<QueueType>::inited() const {
   rlock lock(mtx_);
   return inited_;
}

template<typename QueueType>
inline bool threadpool<QueueType>::is_running() const{
    rlock lock(mtx_);
    return _is_running();
}

template<typename QueueType>
inline int threadpool<QueueType>::size() const{
    rlock lock(mtx_);
    return workers_.size();
}

template<typename QueueType>
inline void threadpool<QueueType>::spawn(){
    for(;;){
        bool pop = false;
        std::function<void()> task;
        {
            wlock lock(mtx_);
            cond_.wait(lock,[this,&pop,&task]{
                pop = tasks_.pop(task);
                return cancel_ || stop_ || pop;
            });
        }
        if(cancel_ || (stop_ && !pop)){
            return ;  // if stop_ is true, but pop = true,we need to finshed task first.
        }
        task();
    }
}

template<typename QueueType>
template<class F,class... Args>
auto threadpool<QueueType>::async(F&& f,Args&&... args) const
    ->std::future<decltype(f(args...))>{
        using return_t = decltype(f(args...));
        using future_t = std::future<return_t>;
        using task_t   = std::packaged_task<return_t()>;

        {
            rlock lock(mtx_);
            if(stop_ || cancel_){
                throw std::runtime_error(
                    "Delegating task to a threadpool"
                    "that has been terminated or canceled."
                );
            }
        }

        auto bind_func = std::bind(std::forward<F>(f),std::forward<Args>(args)...);
        std::shared_ptr<task_t> task = std::make_shared<task_t>(std::move(bind_func));
        future_t fut = task->get_future();
        tasks_.emplace([task]()->void{(*task)();});
        cond_.notify_one();
        return fut;
    }

    }//ThreadPool End
}//Luna End


