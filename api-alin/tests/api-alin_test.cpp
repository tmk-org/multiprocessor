#define _GNU_SOURCE
#include "api/api_parser.h"

#include <gtest/gtest.h>

/* Example functions, side effect -- can change exit_flag */
void apiUninited(const char *cmd, int *exit_flag) { *exit_flag = 1; }
void apiInit(const char *cmd, int *exit_flag)     { *exit_flag = 2; }
void apiStart(const char *cmd, int *exit_flag)    { *exit_flag = 3; }
void apiFinish(const char *cmd, int *exit_flag)   { *exit_flag = 4; }
void apiStatus(const char *cmd, int *exit_flag)   { *exit_flag = 5; }
void apiGrab(const char *cmd, int *exit_flag)     { *exit_flag = 6; }
void apiLive(const char *cmd, int *exit_flag)     { *exit_flag = 7; }
void apiControl(const char *cmd, int *exit_flag)  { *exit_flag = 8; }
void apiClean(const char *cmd, int *exit_flag)    { *exit_flag = 9; }
void apiStop(const char *cmd, int *exit_flag)     { *exit_flag = 10;}

namespace {

class apiTest : public ::testing::Test { };

TEST_F(apiTest, good_api_test) {
    int exit_flag = 0;
    executeAPICommand("get,/api/client/1/grab", &exit_flag);
    EXPECT_EQ(exit_flag, 6);
    executeAPICommand("post,/api/client/1/status", &exit_flag);
    EXPECT_EQ(exit_flag, 5);
    executeAPICommand("get,/api/client/1/stop,abc", &exit_flag);
    EXPECT_EQ(exit_flag, 10);
}

TEST_F(apiTest, bad_api_test) {
    int exit_flag = 0;
    executeAPICommand("aaaaaaaaaa", &exit_flag);
    EXPECT_EQ(exit_flag, 0);
    executeAPICommand("/api/client/1/control", &exit_flag);
    EXPECT_EQ(exit_flag, 0);
    //executeAPICommand("get,/api/server/start,abc", &exit_flag); // ??
    //EXPECT_EQ(exit_flag, 0);
}

}; // namespace

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
