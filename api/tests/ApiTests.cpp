#include "gtest/gtest.h"

extern "C" {
#include "api/api.h"
}

TEST(ApiTest, generate_API_INIT) {
    const char *expected = "/api/server/";
    const char *request = apiGenerateCommandString(API_GET, API_GRAB, 0, "xxx", "yyy");
    ASSERT_STREQ(request, expected);
}
