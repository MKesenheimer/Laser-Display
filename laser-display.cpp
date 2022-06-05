// sudo port install opencv4 eigen
// add 
//  export PKG_CONFIG_PATH="/opt/local/lib/opencv4/pkgconfig":$PKG_CONFIG_PATH
// to your shell profile.
//
// compile: 
//  g++ -std=c++17 $(pkg-config --cflags --libs opencv4) laser-display.cpp -o laser-display


#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <sstream>

std::string intToStr(int a) {
  std::stringstream ss;
  ss << a;
  return ss.str();
}

int main()
{
  int upperThreshold = 300;
  int lowerThreshold = 100;

  // Reading image
  cv::Mat img = cv::imread("test.jpg");

  // Convert to graycsale
  cv::Mat img_gray;
  cv::cvtColor(img, img_gray, cv::COLOR_BGR2GRAY);
  // Blur the image for better edge detection
  cv::Mat img_blur;
  cv::GaussianBlur(img, img_blur, cv::Size(3, 3), 0);

  // Canny edge detection
  cv::Mat edges;
  cv::Canny(img_blur, edges, lowerThreshold, upperThreshold, 3, false);
  // convert back to RGB image
  cv::cvtColor(edges, edges, cv::COLOR_GRAY2RGB);
  // use edges as mask and multiply original image with mask
  cv::Mat dst;
  cv::bitwise_and(img, edges, edges);

  // build text for displaying values
  std::string str = "Upper threshold = " + intToStr(upperThreshold);
  cv::putText(edges, str, cv::Point(50, 50), cv::FONT_HERSHEY_DUPLEX, 1, cv::Scalar(0, 255, 0), 1, false);
  str = "Lower threshold = " + intToStr(lowerThreshold);
  cv::putText(edges, str, cv::Point(50, 100), cv::FONT_HERSHEY_DUPLEX, 1, cv::Scalar(0, 255, 0), 1, false);  

  // Display canny edge detected image
  cv::imshow("Canny edge detection", edges);
  cv::waitKey(0);
  
  cv::destroyAllWindows();
  return 0;
}
