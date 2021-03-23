#include "gtest/gtest.h"

#include "filter/cvfilter.h"

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

TEST(FilterTest, FindRadius) {
    cv::Mat image = cv::Mat::zeros(960, 1024, CV_8UC3);
    float expected = .0;
    float value = findRadius(image);
    ASSERT_EQ(value, expected);
}
