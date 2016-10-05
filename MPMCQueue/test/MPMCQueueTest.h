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
#include <CONQ/BoundedMPMCQueue.hpp>
#include <CONQ/VectorBoundedMPMCQueue.hpp>
#include <CONQ/MPMCCacheQueue.hpp>
#include <CONQ/RotatorMPMCQueue.hpp>
#include <CONQ/VectorRotatorMPMCQueue.hpp>
#include <CONQ/BlockingVectorRotatorMPMCQueue.hpp>
#include "BasicTimer.h"

struct TestParameters{
    uint32_t nReaders;
    uint32_t nWriters;
    uint32_t nElements;
};

class QueueTest : public testing::Test, 
        public testing::WithParamInterface< ::testing::tuple<uint32_t, uint32_t, uint32_t> >{
public:
    virtual void SetUp();
    virtual void TearDown();
protected:
    std::vector<BasicTimer> readers;
    std::vector<BasicTimer> writers;
    TestParameters _params;
    char padding0[64];
    CONQ::MPMCQueue<uint64_t> q;
    char padding1[64];
    CONQ::BoundedMPMCQueue<uint64_t, 16777216> bq;
    char padding2[64];
    CONQ::VectorBoundedMPMCQueue<uint64_t> vbq{16777216};
    char padding3[64];
    CONQ::MPMCCacheQueue<uint64_t, 16777216> dq;
    char padding4[64];
    CONQ::RotatorMPMCQueue<uint64_t, 32> rq;
    char padding5[64];
    CONQ::VectorRotatorMPMCQueue<uint64_t> vrq{32};
    char padding6[64];
    CONQ::BlockingVectorRotatorMPMCQueue<uint64_t> bvrq{32};
};
#endif /* MPMCQUEUETEST_H */

