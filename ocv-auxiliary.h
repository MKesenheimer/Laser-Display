/*
 *  ocv-auxiliary.h
 *  Created by Matthias Kesenheimer on 08.06.22.
 *  Copyright 2022. All rights reserved.
 */
#pragma once
#include <vector>
#include <opencv2/opencv.hpp>

namespace auxiliary {
    void sortLines(std::vector<cv::Vec4i>& houghLines);
}