#include <vector>
#include <opencv2/opencv.hpp>

namespace auxiliary {
    void sortLines(std::vector<cv::Vec4i>& houghLines);
}