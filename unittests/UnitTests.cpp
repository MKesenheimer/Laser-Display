#include "GameLibrary/Algorithms.h"
#include "GameLibrary/Sort.h"
#include "GameLibrary/vector.h"
#include "GameLibrary/matrix.h"
#include "GameLibrary/operators.h"
#include "GameLibrary/Fit.h"
#include "GameLibrary/vector.h"
#include "GameLibrary/matrix.h"
#include "GameLibrary/operators.h"
#include <vector>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>

#include <gtest/gtest.h>

int add(int a, int b) {return a + b;}

TEST(Addition, CanAddTwoNumbers) {
  EXPECT_TRUE(add(2, 2) == 4);
}

TEST(Algorithms, TravelingSalesmanProblem) {
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

    int length = Algorithms::travelingSalesmanProblem(graph, s);
    EXPECT_TRUE(length == 80);
}

TEST(Algorithms, LineDistanceSquare) {
    Line<int> p = Line<int>({XYPoint<int>({1, 2}), XYPoint<int>({3, 4})});
    Line<int> q = Line<int>({XYPoint<int>({5, 6}), XYPoint<int>({7, 8})});
    std::array<int, 4> d = Algorithms::lineDistanceSquare<int>(p, q);

    EXPECT_EQ(32, d[0]);
    EXPECT_EQ(72, d[1]);
    EXPECT_EQ(8,  d[2]);
    EXPECT_EQ(32, d[3]);
}

TEST(Algorithms, HoughLinesReinterpretCast) {
    cv::Vec4i l1 = {1, 2, 3, 4};
    cv::Vec4i l2 = {5, 6, 7, 8};
    //std::cout << "l1 = " << l1[0] << ", " << l1[1] << ", " << l1[2] << ", " << l1[3] << std::endl;
    //std::cout << "l2 = " << l2[0] << ", " << l2[1] << ", " << l2[2] << ", " << l2[3] << std::endl;

    Line<int>* line1 = reinterpret_cast<Line<int>*>(l1.val);
    Line<int>* line2 = reinterpret_cast<Line<int>*>(l2.val);

    // result should be 
    // l1 = 1, 2, 3, 4
    // l2 = 5, 6, 7, 8
    // d  = 32, 72, 8, 32
    std::array<int, 4> d = Algorithms::lineDistanceSquare((*line1), (*line2));
   // std::cout << "d  = " << d[0] << ", " << d[1] << ", " << d[2] << ", " << d[3] << std::endl;
    EXPECT_EQ(32, d[0]);
    EXPECT_EQ(72, d[1]);
    EXPECT_EQ(8,  d[2]);
    EXPECT_EQ(32, d[3]);

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
    //std::cout << "l1 = " << (*lines[0])[0].first << ", " << (*lines[0])[0].second << ", " << (*lines[0])[1].first << ", " << (*lines[0])[1].second << std::endl;
    //std::cout << "l2 = " << (*lines[1])[0].first << ", " << (*lines[1])[0].second << ", " << (*lines[1])[1].first << ", " << (*lines[1])[1].second << std::endl;
    //std::cout << "houghLines[0][0] = " << houghLines[0][0] << std::endl;
    EXPECT_EQ(0, (*lines[0])[0].first);
    EXPECT_EQ(1, (*lines[0])[0].second);
    EXPECT_EQ(3, (*lines[0])[1].first);
    EXPECT_EQ(4, (*lines[0])[1].second);
    EXPECT_EQ(5, (*lines[1])[0].first);
    EXPECT_EQ(6, (*lines[1])[0].second);
    EXPECT_EQ(7, (*lines[1])[1].first);
    EXPECT_EQ(8, (*lines[1])[1].second);
    EXPECT_EQ(0, houghLines[0][0]);

    // TODO
    // third test
    //std::vector<Line<int>> lines2; 
}

TEST(Algorithms, MinimalLineLineDistance) {
    Line<int> p = Line<int>({XYPoint<int>({1, 2}), XYPoint<int>({3, 4})});
    Line<int> q = Line<int>({XYPoint<int>({5, 6}), XYPoint<int>({7, 8})});
    auto test = Algorithms::minimalLineDistance<int>(p, q);
    // result should be "minimal distance = 8, P1 = (3, 4), P2 = (5, 6)""
    //std::cout << "minimal distance = " << std::get<0>(test) << ", P1 = (" << std::get<1>(test)->first << ", " << std::get<1>(test)->second << ")" << ", P2 = (" << std::get<2>(test)->first << ", " << std::get<2>(test)->second << ")" << std::endl;
    EXPECT_EQ(8, std::get<0>(test));
    EXPECT_EQ(3, std::get<1>(test)->first);
    EXPECT_EQ(4, std::get<1>(test)->second);
    EXPECT_EQ(5, std::get<2>(test)->first);
    EXPECT_EQ(6, std::get<2>(test)->second);
}

TEST(Algorithms, MinimalPointLineDistance) {
    XYPoint<int> p = XYPoint<int>({3, 4});
    Line<int> q = Line<int>({XYPoint<int>({7, 8}), XYPoint<int>({5, 6})});
    int d = Algorithms::minimalPLineDistance<int>(p, q);
    // result should be "minimal distance = 8"
    //std::cout << "minimal distance = " << d << std::endl;
    EXPECT_EQ(8, d);
}
 
TEST(Algorithms, MinimalPointLinePoint) {
    XYPoint<int> p = XYPoint<int>({3, 4});
    Line<int> q = Line<int>({XYPoint<int>({7, 8}), XYPoint<int>({5, 6})});
    auto point = Algorithms::minimalPLinePoint<int>(p, q);
    // result should be "minimal point P = (5, 6)""
    //std::cout << "minimal point = (" << point->first << ", " << point->second << ")" << std::endl;
    EXPECT_EQ(5, point->first);
    EXPECT_EQ(6, point->second);
}

TEST(Algorithms, SortLines) {
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
    //std::cout << "After sorting:" << std::endl;
    for (size_t k = 0; k < houghLines.size(); ++k) {
        cv::Vec4i l = houghLines[k];
        switch (k) {
            case 0:
                EXPECT_EQ(4, l[0]);
                EXPECT_EQ(2, l[1]);
                EXPECT_EQ(3, l[2]);
                EXPECT_EQ(2, l[3]);
                break;
            case 1:
                EXPECT_EQ(2, l[0]);
                EXPECT_EQ(2, l[1]);
                EXPECT_EQ(1, l[2]);
                EXPECT_EQ(1, l[3]);
                break;
            case 2:
                EXPECT_EQ(3, l[0]);
                EXPECT_EQ(4, l[1]);
                EXPECT_EQ(4, l[2]);
                EXPECT_EQ(4, l[3]);
                break;
            case 3:
                EXPECT_EQ(5, l[0]);
                EXPECT_EQ(4, l[1]);
                EXPECT_EQ(4, l[2]);
                EXPECT_EQ(3, l[3]);
                break;
        }
        //std::cout << "(" << l[0] << ", " << l[1] << ", " << l[2] << ", " << l[3] << ")" << std::endl;
    }
}

TEST(Types, VectorMatrix) {
    math::vector<double> drow1 = { 1, 2};
    math::vector<double> drow2 = { 3, 4};
    math::matrix<double> dmat1 = { drow1, drow2 };

    math::vector<double> drow3 = { 5, 6};
    math::vector<double> drow4 = { 7, 8};

    math::matrix<double> dmat2 = { drow3, drow4 };
    math::matrix<double> dmat = dmat1 * dmat2;

    EXPECT_EQ(19, dmat(0, 0));
    EXPECT_EQ(22, dmat(0, 1));
    EXPECT_EQ(43, dmat(1, 0));
    EXPECT_EQ(50, dmat(1, 1));
}

TEST(GameLibrary, LinFit) {
    // given points pi = {xi, yi}
    math::vector<double> p1 = {0, 0};
    math::vector<double> p2 = {1, 1};
    math::vector<double> p3 = {2, 4};
    math::vector<double> p4 = {3, 9};

    math::matrix<double> P;
    P.push_back(p1);
    P.push_back(p2);
    P.push_back(p3);
    P.push_back(p4);

    // fit the polynomial c + b * x + a * x^2
    const size_t degree = 2;
    const size_t numberOfPoints = P.rows();

    // A is the coefficient matrix of the polynom
    math::matrix<double> A(numberOfPoints, degree + 1);
    math::vector<double> b(numberOfPoints);
    for (size_t i = 0; i < numberOfPoints; ++i) { // for every vector
        for (size_t j = 0; j <= degree; ++j) { // for every coefficient (degree)
            A(i, j) = std::pow(P(i, 0), j);
        }
        b[i] = P(i, 1);
    }

    // Solve the equation b = A * x for x
    math::vector<double> x = math::utilities::Fit::linFit<double>(b, A);

    EXPECT_NEAR(0, x[0], 0.001); // c = 0
    EXPECT_NEAR(0, x[1], 0.001); // b = 0
    EXPECT_NEAR(1, x[2], 0.001); // a = 1
}

TEST(GameLibrary, polyFit) {
    // given points pi = {xi, yi}
    math::vector<double> p1 = {0, 0};
    math::vector<double> p2 = {1, 1};
    math::vector<double> p3 = {2, 4};
    math::vector<double> p4 = {3, 9};

    math::matrix<double> P;
    P.push_back(p1);
    P.push_back(p2);
    P.push_back(p3);
    P.push_back(p4);

    // fit the polynomial c + b * x + a * x^2
    const size_t degree = 2;

    // Solve the equation b = A * x for x
    math::vector<double> coeff = math::utilities::Fit::polyFit<double>(P, degree);

    EXPECT_NEAR(0, coeff[0], 0.001); // c = 0
    EXPECT_NEAR(0, coeff[1], 0.001); // b = 0
    EXPECT_NEAR(1, coeff[2], 0.001); // a = 1
}