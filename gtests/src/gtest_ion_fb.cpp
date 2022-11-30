#include <gtest/gtest.h>
#include <cstdlib>
#include <string>
#include "gtest_helper.h"

class IonFBTests : public ::testing::Test {
    public:
    const char* testBinaryName = "ion_fb";
    void SetUp() override { chdir("/data/nativetest64/unrestricted"); }
    void TearDown() override { chdir("/"); }
};

TEST_F(IonFBTests, TestMakeFb) {
    runSubTest(testBinaryName, "make-fb");
}

TEST_F(IonFBTests, TestClone) {
    runSubTest(testBinaryName, "clone");
}

TEST_F(IonFBTests, TestMmap) {
    runSubTest(testBinaryName, "mmap");
}
