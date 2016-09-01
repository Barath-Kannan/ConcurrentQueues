#include "MPMCQueueTest.h"
#include <iostream>

using CONQ::MPMCQueue;
using std::cout;
using std::endl;

void MPMCQueueTest::SetUp(){}
void MPMCQueueTest::TearDown(){}

TEST_F(MPMCQueueTest, BasicTest){
    MPMCQueue<uint64_t> q;
    bt1.start();
    for (uint64_t i=0; i<1e8; ++i){
        q.spEnqueue(i);
    }
    bt1.stop();
    cout << "Time to enqueue: " << bt1 << endl;
    auto durPer = bt1.getElapsedDuration()/1e8;
    cout << std::chrono::duration<double, std::nano>(durPer).count() << " ns per enqueue" << endl;
    uint64_t res;
    bt1.start();
    for (uint64_t i=0; i<1e8; ++i){
        q.scDequeue(res);
    }
    bt1.stop();
    cout << "Time to dequeue: " << bt1 << endl;
    durPer = bt1.getElapsedDuration()/1e8;
    cout << std::chrono::duration<double, std::nano>(durPer).count() << " ns per dequeue" << endl;
}

TEST_F(MPMCQueueTest, ConcurrentEnqueueDequeueTest){
    MPMCQueue<uint64_t> q;
    std::thread t1([&](){
        bt1.start();
        for (uint64_t i=0; i<1e8; ++i){
            q.spEnqueue(i);
        }
        bt1.stop();
        std::lock_guard<std::mutex> lock(_writeMutex);
        cout << "Time to enqueue: " << bt1 << endl;
        auto durPer = bt1.getElapsedDuration()/1e8;
        cout << std::chrono::duration<double, std::nano>(durPer).count() << " ns per enqueue" << endl;
    });
    
    std::thread t2([&](){
        bt2.start();
        uint64_t res;
        for (uint64_t i=0; i<1e8; ++i){
            while (!q.scDequeue(res));
        }
        bt2.stop();
        std::lock_guard<std::mutex> lock(_writeMutex);
        cout << "Time to dequeue: " << bt2 << endl;
        auto durPer = bt2.getElapsedDuration()/1e8;
        cout << std::chrono::duration<double, std::nano>(durPer).count() << " ns per dequeue" << endl;
    });
    t1.join();
    t2.join();    
}

TEST_F(MPMCQueueTest, MultiEmit){
    MPMCQueue<uint64_t> q;
    std::vector<std::thread> l;
    l.emplace_back([&](){
        uint64_t res;
        bt2.start();
        for (uint64_t i=0; i<1e8; ++i){
            while (!q.scDequeue(res));
            //q.blockingDequeue(res);
        }
        bt2.stop();
        std::lock_guard<std::mutex> lock(_writeMutex);
        cout << "Time to dequeue: " << bt2 << endl;
        auto durPer = bt2.getElapsedDuration()/1e8;
        cout << std::chrono::duration<double, std::nano>(durPer).count() << " ns per dequeue" << endl;
    });
    bt1.start();
    for (uint32_t i=0; i<10; ++i){
        l.emplace_back([&](){
            for (uint64_t i=0; i<1e7; ++i){
                q.mpEnqueue(i);
            }
        });
    }
    bt1.stop();
    for (uint32_t i=1; i<11; ++i){
        l[i].join();
    }
    _writeMutex.lock();
    cout << "Time to enqueue: " << bt1 << endl;
    auto durPer = bt1.getElapsedDuration()/1e8;
    cout << std::chrono::duration<double, std::nano>(durPer).count() << " ns per enqueue" << endl;
    _writeMutex.unlock();
    l[0].join();
}

TEST_F(MPMCQueueTest, MultiEmitMultiConsume){
    MPMCQueue<uint64_t> q;
    std::vector<std::thread> l;
    
    bt1.start();
    for (uint32_t i=0; i<10; ++i){
        l.emplace_back([&](){
            uint64_t res;
            for (uint64_t i=0; i<1e8; ++i){
                while (!q.mcDequeue(res));
            }
        });
    }
    for (uint32_t i=0; i<10; ++i){
        l.emplace_back([&](){
            for (uint64_t i=0; i<1e8; ++i){
                q.mpEnqueue(i);
            }
        });
    }
    for (uint32_t i=0; i<20; ++i){
        l[i].join();
    }
    bt1.stop();
    cout << "Time to finish: " << bt1 << endl;
    auto durPer = bt1.getElapsedDuration()/1e9;
    cout << std::chrono::duration<double, std::nano>(durPer).count() << " ns per dequeue" << endl;
    cout << "Throughput: " << 1e9/bt1.getElapsedSeconds() << " messages/s" << endl;
}