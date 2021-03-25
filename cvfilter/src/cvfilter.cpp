#include <stdio.h>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/types.hpp>

#include <iostream>

#include "cvfilter/cvparams.h"
#include "cvfilter/cvfilter.h"

void gammaCorrection(cv::Mat &src, cv::Mat &dst, float gamma)
{
  unsigned char lut[256];
  for (int i = 0; i < 256; i++)
  {
    lut[i] = cv::saturate_cast<uchar>(pow((float)(i / 255.0), gamma) * 255.0f);
  }
  dst = src.clone();

  const int channels = dst.channels();
  switch (channels)
  {
    case 1:
    {
      cv::MatIterator_<uchar> it, end;
      for (it = dst.begin<uchar>(), end = dst.end<uchar>(); it != end; it++)
      *it = lut[(*it)];
      break;
    }
    case 3:
    {
      cv::MatIterator_<cv::Vec3b> it, end;
      for (it = dst.begin<cv::Vec3b>(), end = dst.end<cv::Vec3b>(); it != end; it++)
      {
        (*it)[0] = lut[((*it)[0])];
        (*it)[1] = lut[((*it)[1])];
        (*it)[2] = lut[((*it)[2])];
      }
      break;
    }
  }
}


void prefilterImage(cv::Mat origImage, cv::Mat &prefImage) {

    cv::Mat croppedImage = origImage(cv::Rect(0, TEST_CONFIG_H1, origImage.cols, TEST_CONFIG_H2 - TEST_CONFIG_H1 ));

    cv::Mat greyImage;
    cv::cvtColor(croppedImage, greyImage, cv::COLOR_BGR2GRAY);

    cv::Mat gammaImage;
    //adjust_gamma -- gamma correction  //TODO: use fast pow algorithm from topcon sitara (need to find code or write it)
    gammaCorrection(greyImage, gammaImage, TEST_GAMMA);

    //PY//width = int(frame.shape[1] * self.C["SCALE"] / 100)
    //PY//height = int(frame.shape[0] * self.C["SCALE"] / 100)
    //PY//dsize = (width, height)
    //PY//frame = cv2.resize(frame, dsize)

    cv::Mat resizeImage;
    double scale = TEST_SCALE / 100.;
    cv::resize(gammaImage, resizeImage, cv::Size(greyImage.cols * scale, greyImage.rows * scale));

    cv::Mat thresImage;
    //PY//frame_thresh = cv2.adaptiveThreshold(frame, self.C["MAX_VALUE_THRESH"], cv2.ADAPTIVE_THRESH_GAUSSIAN_C,
    //PY//                                        cv2.THRESH_BINARY_INV, self.C["BLOCK_SIZE_THRESH"], self.C["C_THRESH"])
    cv::adaptiveThreshold(resizeImage, thresImage, TEST_MAX_VALUE_THRESH, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, TEST_BLOCK_SIZE_THRESH, TEST_C_THRESH);

    cv::Mat blurImage;
    //PY//frame_thresh = cv2.blur(frame_thresh, (5, 5))
    cv::blur(thresImage, blurImage, cv::Size(5,5));
     
    int verticalsize = 5;
    //PY//vertical_structure = cv2.getStructuringElement(cv2.MORPH_RECT, (1, verticalsize))
    cv::Mat vertical_structure = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(1,verticalsize));	
        
    cv::Mat erodeImage;
    //PY//frame_thresh = cv2.erode(frame_thresh, vertical_structure, (-1, -1))
    cv::erode(blurImage, erodeImage, vertical_structure);

    cv::Mat pImage;
    //PY//frame_thresh = cv2.dilate(frame_thresh, vertical_structure, (-1, -1))
    cv::dilate(erodeImage, prefImage, vertical_structure);

#if 0
    cv::imwrite("../data/grey.bmp", greyImage);
    cv::imwrite("../data/resize.bmp", resizeImage);
    cv::imwrite("../data/thres.bmp", thresImage);
    cv::imwrite("../data/blur.bmp", blurImage);
    cv::imwrite("../data/pref.bmp", prefImage);
#endif
}

//comparison contours
bool compareContourAreas (std::vector<cv::Point> contour1, std::vector<cv::Point> contour2) {
    double i = fabs(contourArea(cv::Mat(contour1)));
    double j = fabs(contourArea(cv::Mat(contour2)));
    return (i < j);
}

void findMaxContour(cv::Mat pref, std::vector<cv::Point>  &biggestContour) {
    //PY// mask = np.zeros((self.prefiltered.shape[:2]), np.uint8)
    cv::Mat mask = cv::Mat::zeros(pref.cols, pref.rows, CV_8UC1);
    cv::Mat equ;
    //PY//equ = cv2.equalizeHist(self.prefiltered)
    cv::equalizeHist(pref, equ);
    //PY//_, thresh = cv2.threshold(equ, 0, 255, cv2.THRESH_BINARY + cv2.THRESH_OTSU) //first of both skipped
    cv::Mat thresh;
    cv::threshold(equ, thresh, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

    //PY//kernel = np.ones((10, 10), np.uint8)
    cv::Mat kernel = cv::Mat::ones(10, 10, CV_8UC1);
    //PY//opening = cv2.morphologyEx(thresh, cv2.MORPH_OPEN, kernel)
    cv::Mat opening;
    cv::morphologyEx(thresh, opening, cv::MORPH_OPEN, kernel);

    //PY//contours, hierarchy = cv2.findContours(opening, cv2.RETR_TREE, cv2.CHAIN_APPROX_NONE) //vectors
    std::vector<std::vector<cv::Point> > contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(opening, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_NONE);

    // Drawing is not actual
    //PY// for cnt in contours:
    //PY//      x, y, w, h = cv2.boundingRect(cnt)
    //PY//      cv2.drawContours(mask, [cnt], 0, (255, 255, 255), -1)

    //PY//res = cv2.bitwise_and(self.prefiltered, self.prefiltered, mask=mask)
    cv::Mat grey;
    cv::bitwise_and(pref, pref, grey);
    //PY//gray = res
    //PY//_, thresh = cv2.threshold(gray, 0, 255, cv2.THRESH_BINARY + cv2.THRESH_OTSU)
    cv::threshold(grey, thresh, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

    cv::Mat blur;
    cv::GaussianBlur(thresh, blur, cv::Size(5, 5), 0);
    //PY//contours, hierarchy = cv2.findContours(blur, cv2.RETR_TREE, cv2.CHAIN_APPROX_NONE)
    cv::findContours(blur, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_NONE);

    //PY//cnt = []
    //PY//if contours:
    //PY//    cnt = max(contours, key=cv2.contourArea)  
    std::sort(contours.begin(), contours.end(), compareContourAreas);
    biggestContour = contours[contours.size()-1];
}

float findRadius(cv::Mat image) {
    float radius = .0;

    cv::Mat pref;
    std::vector<cv::Point>  biggestContour;

    prefilterImage(image, pref);

    findMaxContour(pref, biggestContour);

    //PY//((x, y), radius) = cv2.minEnclosingCircle(self.cnt)
    cv::Point2f center;
    cv::minEnclosingCircle(biggestContour, center, radius);
    return radius;
}

