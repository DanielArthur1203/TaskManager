#include "gpu.hpp"
#include <gtest/gtest.h>

GPU gpu;

TEST(GPU, deviceIds){
    EXPECT_NO_THROW(gpu.deviceIds());
    EXPECT_TRUE(gpu.deviceIds().size() != 0);
}

TEST(GPU, utilization){
    EXPECT_NO_THROW(gpu.utilization());
    EXPECT_TRUE(gpu.utilization().size() != 0);
}

TEST(GPU, VRAM){
    EXPECT_NO_THROW(gpu.VRAM());
    EXPECT_TRUE(gpu.VRAM().size() != 0);
}