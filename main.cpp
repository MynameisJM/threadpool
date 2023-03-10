#include "src/threadpool.h"
#include<iostream>
#include<Windows.h>

using namespace Luna::ThreadPool;
int main(){
    threadpool<blocking_queue<std::function<void()>>> tp;
    tp.init(4);
    tp.async([]{
        std::cout<<"threadpool start...\n";
    });
    for (int i = 0; i < 1000; ++i) {
        tp.async([i] {
            Sleep(1000);
            std::cout << "task[" << i << "]:This is threadpool doing...\n";
            });
        if (i == 100) {
            tp.terminate();
        }
    }

    tp.async([] {
        std::cout << "task[!" << "]:Stop threadpool...\n";
        });
    return 1;
}