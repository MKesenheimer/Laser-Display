#include "GameLibrary/Algorithms.h"
#include <vector>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>

// TODO: use google unittests
namespace unittests {
    void testTravelingSalesmanProblem() {
        // graph to test:
        //             (1)
        //             /|\
        //            / | \
        //           /  |  \
        //          /   |20 \
        //         /    |    \
        //      10/    (4)    \15
        //       /     / \     \
        //      /   /25     \30 \
        //     / /             \ \
        //   (2) --------------- (3)
        //              35

        std::vector<std::vector<int>> graph;
        graph.push_back(std::vector<int>({ 0, 10, 15, 20 }));
        graph.push_back(std::vector<int>({ 10, 0, 35, 25 }));
        graph.push_back(std::vector<int>({ 15, 35, 0, 30 }));
        graph.push_back(std::vector<int>({ 20, 25, 30, 0 }));
        int s = 0;

        // result should be 80;
        std::cout << Algorithms::travelingSalesmanProblem(graph, s) << std::endl;
    }

    // uncomment if distanceSquare4i was moved to Algorithms.
    void testLineDistanceSquare() {
        Line<int> p = Line<int>({XYPoint<int>({1, 2}), XYPoint<int>({3, 4})});
        Line<int> q = Line<int>({XYPoint<int>({5, 6}), XYPoint<int>({7, 8})});
        std::array<int, 4> d = Algorithms::lineDistanceSquare<int>(p, q);

        //  result should be 32, 72, 8, 32;
        std::cout << d[0] << ", " << d[1] << ", " << d[2] << ", " << d[3] << std::endl;
    }

    void testHoughLinesReinterpretCast() {
        cv::Vec4i l1 = {1, 2, 3, 4};
        cv::Vec4i l2 = {5, 6, 7, 8};
        std::cout << "l1 = " << l1[0] << ", " << l1[1] << ", " << l1[2] << ", " << l1[3] << std::endl;
        std::cout << "l2 = " << l2[0] << ", " << l2[1] << ", " << l2[2] << ", " << l2[3] << std::endl;

        Line<int>* line1 = reinterpret_cast<Line<int>*>(l1.val);
        Line<int>* line2 = reinterpret_cast<Line<int>*>(l2.val);

        std::array<int, 4> d = Algorithms::lineDistanceSquare((*line1), (*line2));
        std::cout << "d  = " << d[0] << ", " << d[1] << ", " << d[2] << ", " << d[3] << std::endl;
    }

    void testMinimalLineLineDistance() {
        Line<int> p = Line<int>({XYPoint<int>({1, 2}), XYPoint<int>({3, 4})});
        Line<int> q = Line<int>({XYPoint<int>({5, 6}), XYPoint<int>({7, 8})});
        auto test = Algorithms::minimalLineDistance<int>(p, q);

        // result should be "minimal distance = 8, P1 = (3, 4), P2 = (5, 6)""
        std::cout << "minimal distance = " << std::get<0>(test) << ", P1 = (" << std::get<1>(test)->first << ", " << std::get<1>(test)->second << ")" << ", P2 = (" << std::get<2>(test)->first << ", " << std::get<2>(test)->second << ")" << std::endl;
    }

    void testMinimalPointLineDistance() {
        XYPoint<int> p = XYPoint<int>({3, 4});
        Line<int> q = Line<int>({XYPoint<int>({7, 8}), XYPoint<int>({5, 6})});
        int d = Algorithms::minimalPLineDistance<int>(p, q);

        // result should be "minimal distance = 8"
        std::cout << "minimal distance = " << d << std::endl;
    }

    void testMinimalPointLinePoint() {
        XYPoint<int> p = XYPoint<int>({3, 4});
        Line<int> q = Line<int>({XYPoint<int>({7, 8}), XYPoint<int>({5, 6})});
        auto point = Algorithms::minimalPLinePoint<int>(p, q);

        // result should be "minimal point P = (5, 6)""
        std::cout << "minimal point = (" << point->first << ", " << point->second << ")" << std::endl;
    }

    void testSortLines() {
        std::vector<cv::Vec4i> houghLines;
        /*houghLines.push_back({1, 1, 2, 2});
        houghLines.push_back({3, 4, 4, 4});
        houghLines.push_back({4, 2, 3, 2});
        houghLines.push_back({4, 3, 5, 4});*/
        /*(1, 1, 2, 2)
          (3, 2, 4, 2)
          (4, 3, 5, 4)
          (4, 4, 3, 4)*/
    
        houghLines.push_back({3, 2, 4, 2});
        houghLines.push_back({1, 1, 2, 2});
        houghLines.push_back({3, 4, 4, 4});
        houghLines.push_back({4, 3, 5, 4});

        for (size_t k = 0; k < houghLines.size() - 1; ++k) {
            Line<int>* line0 = reinterpret_cast<Line<int>*>(houghLines[k].val);
            const XYPoint<int>& point0 = (*line0)[0];
            const XYPoint<int>& point1 = (*line0)[1];
            std::cout << "step 0: start line = (" << point0.first << ", " << point0.second << ", " << point1.first << ", " << point1.second << ")" << std::endl;

            // find closest line
            std::vector<cv::Vec4i>::iterator closest = std::min_element(houghLines.begin() + k + 1, houghLines.end(),
                [point1](cv::Vec4i& a, cv::Vec4i& b) {
                    Line<int>* lineA = reinterpret_cast<Line<int>*>(a.val);
                    Line<int>* lineB = reinterpret_cast<Line<int>*>(b.val);
                    return Algorithms::minimalPLineDistance<int>(point1, *lineA) < Algorithms::minimalPLineDistance<int>(point1, *lineB);
                }
            );
            std::cout << "step 1: closest line = " << std::distance(houghLines.begin(), closest) << std::endl;
            
            // determine closest point of closest line
            Line<int>* nextLine = reinterpret_cast<Line<int>*>(closest->val);
            Line<int>::const_iterator nextPoint = Algorithms::minimalPLinePoint<int>(point1, *nextLine);
            std::cout << "step 2: closest point = (" << nextPoint->first << ", " << nextPoint->second << ")" << std::endl;

            // if the points of the line are not in the correct order, swap them
            // TODO
            if (nextPoint != nextLine->begin())
                std::iter_swap(nextLine->begin(), nextLine->begin() + 1);

            // move the element we found to the current position k + 1
            size_t i = std::distance(houghLines.begin(), closest);
            std::iter_swap(houghLines.begin() + k + 1, houghLines.begin() + i);
        }

        std::cout << "After sorting:" << std::endl;
        for (size_t k = 0; k < houghLines.size(); ++k) {
            cv::Vec4i l = houghLines[k];
            std::cout << "(" << l[0] << ", " << l[1] << ", " << l[2] << ", " << l[3] << ")" << std::endl;
        }        
    }
}