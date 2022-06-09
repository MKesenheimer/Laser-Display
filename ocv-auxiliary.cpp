#include "ocv-auxiliary.h"
#include "algorithm"
#include "GameLibrary/Point.h"
#include "GameLibrary/Algorithms.h"

namespace auxiliary {
    void sortLines(std::vector<cv::Vec4i>& houghLines) {
        if (houghLines.size() <= 1)
            return;
        for (size_t k = 0; k < houghLines.size() - 1; ++k) {
            Line<int>* line0 = reinterpret_cast<Line<int>*>(houghLines[k].val);
            const XYPoint<int>& point0 = (*line0)[0];
            const XYPoint<int>& point1 = (*line0)[1];

            // find closest line
            std::vector<cv::Vec4i>::iterator closest = std::min_element(houghLines.begin() + k + 1, houghLines.end(),
                [point1](cv::Vec4i& a, cv::Vec4i& b) {
                    Line<int>* lineA = reinterpret_cast<Line<int>*>(a.val);
                    Line<int>* lineB = reinterpret_cast<Line<int>*>(b.val);
                    return Algorithms::minimalPLineDistance<int>(point1, *lineA) < Algorithms::minimalPLineDistance<int>(point1, *lineB);
                }
            );

            // determine closest point of closest line
            Line<int>* nextLine = reinterpret_cast<Line<int>*>(closest->val);
            Line<int>::const_iterator nextPoint = Algorithms::minimalPLinePoint<int>(point1, *nextLine);

            // if the points of the line are not in the correct order, swap them
            if (nextPoint != nextLine->begin())
                std::iter_swap(nextLine->begin(), nextLine->begin() + 1);

            // move the element we found to the current position k + 1
            size_t i = std::distance(houghLines.begin(), closest);
            std::iter_swap(houghLines.begin() + k + 1, houghLines.begin() + i);
        }
    }
}