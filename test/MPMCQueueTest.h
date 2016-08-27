/* 
 * File:   MPMCQueueTest.h
 * Author: Barath Kannan
 *
 * Created on 27 August 2016, 9:28 PM
 */

#ifndef MPMCQUEUETEST_H
#define MPMCQUEUETEST_H

#include <gtest/gtest.h>
#include <CONQ/MPMCQueue.hpp>
#include <thread>
#include <mutex>
#include "BasicTimer.h"

class MPMCQueueTest : public testing::Test{
public:
    virtual void SetUp();
    virtual void TearDown();
protected:
    BasicTimer bt1;
    BasicTimer bt2;
    std::mutex _writeMutex;
};

#endif /* MPMCQUEUETEST_H */

