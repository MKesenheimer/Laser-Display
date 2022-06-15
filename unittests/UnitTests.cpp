#include "GameLibrary/Algorithms.h"
#include "GameLibrary/Sort.h"
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

        // result should be 
        // l1 = 1, 2, 3, 4
        // l2 = 5, 6, 7, 8
        // d  = 32, 72, 8, 32
        std::array<int, 4> d = Algorithms::lineDistanceSquare((*line1), (*line2));
        std::cout << "d  = " << d[0] << ", " << d[1] << ", " << d[2] << ", " << d[3] << std::endl;


        // second test
        std::vector<cv::Vec4i> houghLines = {l1, l2};
        std::vector<Line<int>*> lines;
      
        lines.push_back(reinterpret_cast<Line<int>*>(houghLines[0].val));
        lines.push_back(reinterpret_cast<Line<int>*>(houghLines[1].val));

        // modify values in both structures
        (*lines[0])[0].first = 0;
        houghLines[0][1] = 1;

        // result should be
        // l1 = 0, 1, 3, 4
        // l2 = 5, 6, 7, 8
        // houghLines[0][0] = 0
        std::cout << "l1 = " << (*lines[0])[0].first << ", " << (*lines[0])[0].second << ", " << (*lines[0])[1].first << ", " << (*lines[0])[1].second << std::endl;
        std::cout << "l2 = " << (*lines[1])[0].first << ", " << (*lines[1])[0].second << ", " << (*lines[1])[1].first << ", " << (*lines[1])[1].second << std::endl;
        std::cout << "houghLines[0][0] = " << houghLines[0][0] << std::endl;

        // TODO
        // third test
        //std::vector<Line<int>> lines2; 
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
        /* result:
          (1, 1, 2, 2)
          (3, 2, 4, 2)
          (4, 3, 5, 4)
          (4, 4, 3, 4)*/
    
        houghLines.push_back({4, 2, 3, 2});
        houghLines.push_back({3, 4, 4, 4});
        houghLines.push_back({1, 1, 2, 2});
        houghLines.push_back({5, 4, 4, 3});
        /* result:
          (4, 2, 3, 2)
          (2, 2, 1, 1)
          (3, 4, 4, 4)
          (5, 4, 4, 3)*/

        Sort::sortLines(houghLines);
        std::cout << "After sorting:" << std::endl;
        for (size_t k = 0; k < houghLines.size(); ++k) {
            cv::Vec4i l = houghLines[k];
            std::cout << "(" << l[0] << ", " << l[1] << ", " << l[2] << ", " << l[3] << ")" << std::endl;
        }
    }
}