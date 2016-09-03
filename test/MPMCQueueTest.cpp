#include "MPMCQueueTest.h"
#include <iostream>
#include <chrono>

using CONQ::MPMCQueue;
using ::testing::Values;
using ::testing::Combine;
using std::cout;
using std::endl;

std::ostream& operator<<(std::ostream& os, const std::chrono::duration<double>& t) {
    double val;
    if ((val = std::chrono::duration<double>(t).count()) > 1.0){
        os << val << " seconds";
    }
    else if ((val = std::chrono::duration<double, std::milli>(t).count()) > 1.0){
        os << val << " milliseconds";
    }
    else if ((val = std::chrono::duration<double, std::micro>(t).count()) > 1.0){
        os << val << " microseconds";
    }
    else{
        val = std::chrono::duration<double, std::nano>(t).count();
        os << val << " nanoseconds";
    }
    return os;
}

void QueueTest::SetUp(){
    auto tupleParams = GetParam();
    _params = TestParameters{::testing::get<0>(tupleParams), ::testing::get<1>(tupleParams), ::testing::get<2>(tupleParams)};
    readers.resize(_params.nReaders, BasicTimer());
    writers.resize(_params.nWriters, BasicTimer());
    cout << "Readers: " << _params.nReaders << endl;
    cout << "Writers: " << _params.nWriters << endl;
    cout << "Elements: " << _params.nElements << endl;
}

void QueueTest::TearDown(){
    cout << "Enqueue:" << endl;
    auto writeDur = writers[0].getElapsedDuration();
    auto writeMax = writers[0].getElapsedDuration();
    int toMeasure = 1;
    for (size_t i=1; i<writers.size(); ++i){
        auto dur = writers[i].getElapsedDuration();
        if (dur > std::chrono::nanoseconds(1)){
            writeDur += dur;
            if (dur > writeMax) writeMax = dur;
            ++toMeasure;
        }
    }
    writeDur/=toMeasure;
    cout << "Max write thread duration: " << writeMax << endl;
    cout << "Average write thread duration: " << writeDur << endl;
    writeDur/=_params.nElements;
    cout << "Average time per enqueue: " << writeDur << endl;
    
    cout << "Dequeue:" << endl;
    auto readDur = readers[0].getElapsedDuration();
    auto readMax = readers[0].getElapsedDuration();
    toMeasure = 1;
    for (size_t i=1; i<readers.size(); ++i){
        auto dur = readers[i].getElapsedDuration();
        if (dur > std::chrono::nanoseconds(1)){
            readDur += dur;
            if (dur > readMax) readMax = dur;
            ++toMeasure;
        }        
    }
    readDur/=toMeasure;
    cout << "Max read thread duration: " << readMax << endl;
    cout << "Average read thread duration: " << readDur << endl;
    readDur/=_params.nElements;
    cout << "Average time per dequeue: " << readDur << endl;
}

TEST_P(QueueTest, Busy){
    std::vector<std::thread> l;
    for (size_t i=0; i<_params.nReaders; ++i){
        l.emplace_back([&, i](){
            readers[i].start();
            uint64_t res;
            for (size_t j=0; j<_params.nElements/_params.nReaders; ++j){
                while (!q.mcDequeue(res));
            }
            if (i == 0){
                size_t remainder = _params.nElements - ((_params.nElements/_params.nReaders)*_params.nReaders);
                for (size_t j=0; j<remainder; ++j){
                    while (!q.mcDequeue(res));
                }
            }
            readers[i].stop();
        });
    }
    for (size_t i=0; i<_params.nWriters; ++i){
        l.emplace_back([&, i](){
            writers[i].start();
            for (size_t j=0; j<_params.nElements/_params.nWriters; ++j){
                q.mpEnqueue(j);
            }
            if (i == 0){
                size_t remainder = _params.nElements - ((_params.nElements/_params.nWriters)*_params.nWriters);
                for (size_t j=0; j<remainder; ++j){
                    q.mpEnqueue(j);
                }
            }
            writers[i].stop();
        });
    }
    for (size_t i=0; i<_params.nReaders + _params.nWriters; ++i){
        l[i].join();
    }
}

TEST_P(QueueTest, BoundedBusy){
    std::vector<std::thread> l;
    for (size_t i=0; i<_params.nReaders; ++i){
        l.emplace_back([&, i](){
            readers[i].start();
            uint64_t res;
            for (size_t j=0; j<_params.nElements/_params.nReaders; ++j){
                while (!bq.mcDequeue(res));
            }
            if (i == 0){
                size_t remainder = _params.nElements - ((_params.nElements/_params.nReaders)*_params.nReaders);
                for (size_t j=0; j<remainder; ++j){
                    while (!bq.mcDequeue(res));
                }
            }
            readers[i].stop();
        });
    }
    for (size_t i=0; i<_params.nWriters; ++i){
        l.emplace_back([&, i](){
            writers[i].start();
            for (size_t j=0; j<_params.nElements/_params.nWriters; ++j){
                while (!bq.mpEnqueue(j));
            }
            if (i == 0){
                size_t remainder = _params.nElements - ((_params.nElements/_params.nWriters)*_params.nWriters);
                for (size_t j=0; j<remainder; ++j){
                    while (!bq.mpEnqueue(j));
                }
            }
            writers[i].stop();
        });
    }
    for (size_t i=0; i<_params.nReaders + _params.nWriters; ++i){
        l[i].join();
    }
}

TEST_P(QueueTest, BinaryExponentialBackoff){
    std::vector<std::thread> l;
    for (size_t i=0; i<_params.nReaders; ++i){
        l.emplace_back([&, i](){
            readers[i].start();
            uint64_t res;
            auto waitTime = std::chrono::nanoseconds(1);
            for (size_t j=0; j<_params.nElements/_params.nReaders; ++j){
                waitTime = std::chrono::nanoseconds(1);
                while (!q.mcDequeue(res)){
                    std::this_thread::sleep_for(waitTime);
                    waitTime*=2;
                }
            }
            if (i == 0){
                size_t remainder = _params.nElements - ((_params.nElements/_params.nReaders)*_params.nReaders);
                for (size_t j=0; j<remainder; ++j){
                    waitTime = std::chrono::nanoseconds(1);
                    while (!q.mcDequeue(res)){
                        std::this_thread::sleep_for(waitTime);
                        waitTime*=2;
                    }
                }
            }
            readers[i].stop();
        });
    }
    for (size_t i=0; i<_params.nWriters; ++i){
        l.emplace_back([&, i](){
            writers[i].start();
            for (size_t j=0; j<_params.nElements/_params.nWriters; ++j){
                q.mpEnqueue(j);
            }
            if (i == 0){
                size_t remainder = _params.nElements - ((_params.nElements/_params.nWriters)*_params.nWriters);
                for (size_t j=0; j<remainder; ++j){
                    q.mpEnqueue(j);
                }
            }
            writers[i].stop();
        });
    }
    for (size_t i=0; i<_params.nReaders + _params.nWriters; ++i){
        l[i].join();
    }
}

INSTANTIATE_TEST_CASE_P(
        MPMC_Benchmark,
        QueueTest,
        testing::Combine(
        Values(1, 2, 4, 8, 16, 32), //readers
        Values(1, 2, 4, 8, 16, 32), //writers
        Values(1e6, 1e7, 1e8, 1e9) //elements
        )
        );