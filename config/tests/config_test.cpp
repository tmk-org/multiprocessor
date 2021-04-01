#include "config/config.h"

#include <gtest/gtest.h>

namespace {

class configTest : public ::testing::Test { };

TEST_F(configTest, good_test_json) {
    module_t *m;
    ASSERT_EQ(create_config("../config/tests/good_test.json", &m), 3);
    delete[] m;
}

TEST_F(configTest, ill_test_json) {
    module_t *m;
    EXPECT_EQ(create_config("../config/tests/ill_test.json", &m), -1); // doesn't work, but it's ok
    delete[] m;
}

TEST_F(configTest, bad_test_json) {
    module_t *m;
    EXPECT_EQ(create_config("../config/tests/bad_test.json", &m), -1);
}

TEST_F(configTest, non_existing_test_json) {
    module_t *m;
    EXPECT_EQ(create_config("../config/tests/not_test.json", &m), -1);
}

TEST_F(configTest, empty_test_json) {
    module_t *m;
    EXPECT_EQ(create_config("../config/tests/empty_test.json", &m), -1);
}

}; // namespace

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
