#include "config/config.h"

#include <gtest/gtest.h>

namespace {

class configTest : public ::testing::Test { };

TEST_F(configTest, good_test_json) {
    module_t *m;
    int ans = model_read_configuration("../config/tests/good_test.json", &m);
    ASSERT_EQ(ans, 3);
    for (int i = 0; i < ans; ++i)
        module_description_free(&m[i]);
    //delete[] m;
}

TEST_F(configTest, ill_test_json) {
    module_t *m;
    int ans = model_read_configuration("../config/tests/ill_test.json", &m);
    #ifdef CONFIG_CHECK
    EXPECT_EQ(ans, -1);
    #else
    EXPECT_EQ(ans, 3);
    #endif
    for (int i = 0; i < ans; ++i)
        module_description_free(&m[i]);
    //delete[] m;
}

TEST_F(configTest, bad_test_json) {
    module_t *m;
    int ans = model_read_configuration("../config/tests/bad_test.json", &m);
    EXPECT_EQ(ans, -1);
    for (int i = 0; i < ans; ++i)
        module_description_free(&m[i]);
}

TEST_F(configTest, non_existing_test_json) {
    module_t *m;
    int ans = model_read_configuration("../config/tests/not_test.json", &m);
    EXPECT_EQ(ans, -1);
    for (int i = 0; i < ans; ++i)
        module_description_free(&m[i]);
}

TEST_F(configTest, empty_test_json) {
    module_t *m;
    int ans = model_read_configuration("../config/tests/empty_test.json", &m);
    EXPECT_EQ(ans, -1);
    for (int i = 0; i < ans; ++i)
        module_description_free(&m[i]);
}

}; // namespace

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
