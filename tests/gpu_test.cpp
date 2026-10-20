#include "gpu.hpp"
#include "gpu_factory.hpp"
#include <gtest/gtest.h>
#include <vector>
#include <memory>

GpuFactory gpuFac;
GPU gpu = *(gpuFac.makeGPUs().at(0));

TEST(GPU, utilization){
    auto a = gpuFac.makeGPUs();
    EXPECT_NO_THROW(gpu.utilization());
    EXPECT_TRUE(gpu.utilization() >= 0);
}

TEST(GPU, VRAM){
    EXPECT_NO_THROW(gpu.VRAM());
    EXPECT_TRUE(gpu.VRAM() > 0);
}

TEST(GPU, sharedVRAM){
    EXPECT_NO_THROW(gpu.sharedVRAM());
    EXPECT_TRUE(gpu.sharedVRAM() > 0);
}
