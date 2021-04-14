#include "config/config.h"

#include <gtest/gtest.h>
//#include <glog/logging.h>

namespace {

class configTest : public ::testing::Test { };

//const std::string test_folder = "../config/tests/"; // for running from "build"
//const std::string test_folder = "../../config/tests/"; // for running from "build/bin"
std::string test_folder;

TEST_F(configTest, modules_test) {
    module_t *m;
    module_t_clean_up(&m, -1, -1);
}

TEST_F(configTest, good_test_json) {
    module_t *m = NULL;
    int ans = model_read_configuration((test_folder + "good_test.json").c_str(), &m);
    ASSERT_EQ(ans, 3);
    module_t_clean_up(&m, ans, -1);
}

TEST_F(configTest, bad_tests_json) {
    module_t *m = NULL;
    int ans = model_read_configuration((test_folder + "bad_test.json").c_str(), &m);
    EXPECT_EQ(ans, -1);
    module_t_clean_up(&m, ans, -1);
    
    ans = model_read_configuration((test_folder + "not_test.json").c_str(), &m);
    EXPECT_EQ(ans, -1);
    module_t_clean_up(&m, ans, -1);
    
    ans = model_read_configuration((test_folder + "empty_test.json").c_str(), &m);
    EXPECT_EQ(ans, -1);
    module_t_clean_up(&m, ans, -1);
}

TEST_F(configTest, extra_check_test_json) {
    module_t *m = NULL;
    int ans = model_read_configuration((test_folder + "too_many_FIRST_types_test.json").c_str(), &m);
    EXPECT_EQ(ans, -1);
    module_t_clean_up(&m, ans, -1);
    
    ans = model_read_configuration((test_folder + "too_many_null_name_test.json").c_str(), &m);
    EXPECT_EQ(ans, -1);
    module_t_clean_up(&m, ans, -1);
    
    ans = model_read_configuration((test_folder + "ill_test.json").c_str(), &m);
    #ifdef CONFIG_EXTRA_CHECK
    EXPECT_EQ(ans, -1);
    #else
    EXPECT_EQ(ans, 3);
    #endif
    module_t_clean_up(&m, ans, -1);
}
}; // namespace

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Please, use: " << argv[0] << " ../config/tests/" << std::endl;
        test_folder = "../config/tests/"; //"/home/alin/test/multiprocessor/config/tests/";
    } else {
        test_folder = argv[1];
    }
    ::testing::InitGoogleTest/*(NULL, static_cast<char **>(NULL));*/(&argc, argv);
    //google::InitGoogleLogging/*(NULL);*/(argv[1]);
    //google::InstallFailureSignalHandler();
    return RUN_ALL_TESTS();
}
