// sudo port install opencv4 eigen
// add 
//    export PKG_CONFIG_PATH="/opt/local/lib/opencv4/pkgconfig":$PKG_CONFIG_PATH
// to your shell profile.
//
// compile: 
//    g++ -std=c++17 $(pkg-config --cflags --libs opencv4) laser-display.cpp -o laser-display


#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <string>
#include <sstream>
#include <stdio.h>
#include <chrono>

#include <vector>
#include <tuple>
#include <array>

#include <SDL.h>
#include <SDL_ttf.h>
#include "SDLTools/Utilities.h"
#include "SDLTools/Timer.h"
#include "GameLibrary/Point.h"
#include "GameLibrary/Renderer.h"
#include "GameLibrary/Algorithms.h"
#include "unittests/UnitTests.h"
#include "ocv-auxiliary.h"

const int FRAMES_PER_SECOND = 20; // Fps auf 20 festlegen
const std::string image("test3.png");

// TODO: nach Utilities oder GameLibrary verschieben
std::string intToStr(int a) {
    std::stringstream ss;
    ss << a;
    return ss.str();
}

// TODO: nach Utilities oder GameLibrary verschieben
template<typename T> 
T constrain(T value, T min, T max) {
    return std::min(std::max(value, min), max);
}

int main() {
    // take records of frame number
    int frame = 0;
    // Framecap an oder ausschalten
    bool cap = true;

    // Timer zum Festlegen der FPS
    sdl::auxiliary::Timer fps;
    // Timer zum Errechnen der weltweit vergangenen Zeit
    sdl::auxiliary::Timer worldtime;
    worldtime.start();

    // initialize random generator
    sdl::auxiliary::Utilities::seed(time(NULL));

    // Initialize SDL_ttf
    if (TTF_Init() != 0) {
        sdl::auxiliary::Utilities::logSDLError(std::cout, "TTF_Init");
        return -1;
    }

    //Start up SDL and make sure it went ok
    if (SDL_Init(SDL_INIT_VIDEO) != 0){
        sdl::auxiliary::Utilities::logSDLError(std::cout, "SDL_Init");
        return -1;
    }

    // set the screen dimensions -> sets the global variabls Renderer::screen_width and Renderer::screen_height
    Renderer::setDimensions(960, 540);

    // Set up our window and renderer, this time let's put our window in the center of the screen
    SDL_Window *window = SDL_CreateWindow("Laser-Display", SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED, Renderer::screen_width, Renderer::screen_height, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        sdl::auxiliary::Utilities::logSDLError(std::cout, "SDL_CreateWindow");
        SDL_Quit();
        return -1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
        sdl::auxiliary::Utilities::logSDLError(std::cout, "SD_CreateRenderer");
        sdl::auxiliary::Utilities::cleanup(window);
        SDL_Quit();
        return -1;
    }

    // read image for the first time to get its dimensions
    cv::Mat img = cv::imread(image);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR24, SDL_TEXTUREACCESS_STREAMING, img.cols, img.rows);
    if (renderer == NULL) {
        sdl::auxiliary::Utilities::logSDLError(std::cout, "SDL_CreateTexture");
        sdl::auxiliary::Utilities::cleanup(window, renderer);
        SDL_Quit();
        return -1;
    }

    // setup text rendering
    TTF_Font* font = TTF_OpenFont("lazy.ttf", 16);
    if (font == NULL) {
        sdl::auxiliary::Utilities::logSDLError(std::cout, "TTF_OpenFont");
        sdl::auxiliary::Utilities::cleanup(window, renderer, texture);
        SDL_Quit();
        return -1;
    }
    SDL_Color textColor = {0, 255, 0};

#ifdef LUMAX_OUTPUT
    // open Lumax device
    int NumOfCards = Lumax_GetPhysicalDevices();
    void* lumaxHandle = NULL;
    printf("Number of MiniLumax devices: %i\n", NumOfCards);
    if (NumOfCards > 0)
    {
        lumaxHandle = Lumax_OpenDevice(1, 0);
        printf("Lumax_OpenDevice returned handle: 0x%lx\n", (unsigned long)lumaxHandle);
        if (lumaxHandle == NULL){
            sdl::auxiliary::Utilities::logSDLError(std::cout, "Lumax_OpenDevice");
            sdl::auxiliary::Utilities::cleanup(window, renderer, texture);
            SDL_Quit();
            return 1;
        }
    }

    LumaxRenderer lumaxRenderer;
    lumaxRenderer.mirrorFactX = -1;
    lumaxRenderer.mirrorFactY = 1;
    // scaling of the laser output in respect to the SDL screen
    lumaxRenderer.scalingX = 0.5;
    lumaxRenderer.scalingY = 0.5;
    lumaxRenderer.swapXY = 0;
#endif

    // logic
    bool quit = false;
    bool reset = false;

    // opencv specific
    /*int lightThreshold = 0;
    int interThreshold = 40;
    int minLineLength = 90;
    int maxLineGap = 10;
    int blursize = 5;
    int upperThreshold = 300;
    int lowerThreshold = 100;*/
    int rResolution = 1;
    float thetaResolution = CV_PI / 180;

    int lightThreshold = 0;
    int interThreshold = 10;
    int minLineLength = 0;
    int maxLineGap = 10;
    int blursize = 11;
    int upperThreshold = 100;
    int lowerThreshold = 200;

    // the event structure
    SDL_Event e;
    while (!quit){
        // start the fps timer
        fps.start();

        // read any events that occured, for now we'll just quit if any event occurs
        while (SDL_PollEvent(&e)) {
            // if user closes the window
            if (e.type == SDL_QUIT)
                quit = true;
            // if user presses any key
            else if (e.type == SDL_KEYDOWN)
                if (e.key.keysym.sym == SDLK_c)
                    cap = !cap;
                if (e.key.keysym.sym == SDLK_SPACE)
                    reset = true;
        }

        // handle keyboard inputs (no lags and delays!)
        const uint8_t* keystate = SDL_GetKeyboardState(NULL);
        if (keystate[SDL_SCANCODE_E]) {
            lightThreshold++;
        }
        if (keystate[SDL_SCANCODE_D]) {
            lightThreshold = (lightThreshold >= 1 ? lightThreshold - 1 : lightThreshold);
        }
        if (keystate[SDL_SCANCODE_R]) {
            interThreshold++;
        }
        if (keystate[SDL_SCANCODE_F]) {
            interThreshold = (interThreshold >= 1 ? interThreshold - 1 : interThreshold);
        }
        if (keystate[SDL_SCANCODE_T]) {
            minLineLength++;
        }
        if (keystate[SDL_SCANCODE_G]) {
            minLineLength = (minLineLength >= 1 ? minLineLength - 1 : minLineLength);   
        }
        if (keystate[SDL_SCANCODE_Y]) {
            maxLineGap++;
        }
        if (keystate[SDL_SCANCODE_H]) {
            maxLineGap = (maxLineGap >= 1 ? maxLineGap - 1 : maxLineGap);
        }
        if (keystate[SDL_SCANCODE_U]) {
            blursize += 2;
        }
        if (keystate[SDL_SCANCODE_J]) {
            blursize = (blursize >= 3 ? blursize - 2 : blursize);
        }
        if (keystate[SDL_SCANCODE_I]) {
            upperThreshold++;
        }
        if (keystate[SDL_SCANCODE_K]) {
            upperThreshold = (upperThreshold >= 1 ? upperThreshold - 1 : upperThreshold);
        }
        if (keystate[SDL_SCANCODE_O]) {
            lowerThreshold++;
        }
        if (keystate[SDL_SCANCODE_L]) {
            lowerThreshold = (lowerThreshold >= 1 ? lowerThreshold - 1 : lowerThreshold);
        }
        
        // measure time
        auto start_time = std::chrono::high_resolution_clock::now();
        std::vector<Point<float>> points;

        // Reading image
        cv::Mat img = cv::imread(image);

        // Convert to graycsale
        cv::Mat img_gray;
        cv::cvtColor(img, img_gray, cv::COLOR_BGR2GRAY);
        // Blur the image for better edge detection
        cv::Mat img_blur;
        cv::GaussianBlur(img, img_blur, cv::Size(blursize, blursize), 0);

        // Canny edge detection
        cv::Mat edges;
        cv::Canny(img_blur, edges, lowerThreshold, upperThreshold, 3, false);
        // convert back to RGB image
        cv::Mat dst;
        cv::cvtColor(edges, dst, cv::COLOR_GRAY2RGB);
        cv::Mat lines = dst.clone(); //cv::Mat::zeros(dst.rows, dst.cols, CV_64F);
        lines.setTo(cv::Scalar(0, 0, 0));

        // Standard Hough Line Transform
        std::vector<cv::Vec4i> houghLines; // HoughLinesP: will hold the results of the detection
        HoughLinesP(edges, houghLines, rResolution, thetaResolution, interThreshold, minLineLength, maxLineGap); // runs the actual detection

        // sort the lines (TSP problem)
        //unittests::testSortLines();
        auxiliary::sortLines(houghLines);

        // Draw the lines
        for(size_t i = 0; i < houghLines.size(); ++i) {
            cv::Vec4i l = houghLines[i];
            cv::line(lines, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(255, 255, 255), 1, cv::LINE_AA);
        }
        // use lines as mask and multiply original image with mask
        cv::bitwise_and(img, lines, dst);

        // Draw the background black
        SDL_RenderClear(renderer);        
        boxRGBA(renderer, 0, 0, Renderer::screen_width, Renderer::screen_height, 10, 10, 10, 255);

        // render the image
        //SDL_UpdateTexture(texture, NULL, (void*)dst.data, dst.step1());
        //SDL_RenderCopy(renderer, texture, NULL, NULL);

        // apply the lines to the renderers
        points.reserve(15000); // TODO
        for(size_t i = 0; i < houghLines.size(); ++i) {
            cv::Vec4i l = houghLines[i];
            cv::Vec3b intensity1 = dst.at<cv::Vec3b>(cv::Point(constrain<int>(l[0], 0, dst.cols - 1), constrain<int>(l[1], 0, dst.rows - 1)));
            cv::Vec3b intensity2 = dst.at<cv::Vec3b>(cv::Point(constrain<int>(l[2], 0, dst.cols - 1), constrain<int>(l[3], 0, dst.rows - 1)));
            int blue  = constrain<int>((intensity1.val[0] + intensity2.val[0]) / 2, 0, 255);
            int green = constrain<int>((intensity1.val[1] + intensity2.val[1]) / 2, 0, 255);
            int red   = constrain<int>((intensity1.val[2] + intensity2.val[2]) / 2, 0, 255);
            if ((blue + green + red) >= lightThreshold) {
                // Points for Laser output
                points.push_back({(float)l[0], (float)l[1], 0, 0, 0, 255, false});
                points.push_back({(float)l[0], (float)l[1], blue, green, red, 255, false});
                points.push_back({(float)l[2], (float)l[3], blue, green, red, 255, false});
                points.push_back({(float)l[2], (float)l[3], 0, 0, 0, 255, false});
                // Points for SDL output
                l[0] = Renderer::transform(l[0], 0, img.cols, 0, Renderer::screen_width);
                l[1] = Renderer::transform(l[1], 0, img.rows, 0, Renderer::screen_height);
                l[2] = Renderer::transform(l[2], 0, img.cols, 0, Renderer::screen_width);
                l[3] = Renderer::transform(l[3], 0, img.rows, 0, Renderer::screen_height);
                lineRGBA(renderer, (int)l[2], (int)l[3], (int)l[0], (int)l[1], red, green, blue, 255);
            }
        }

#ifdef LUMAX_OUTPUT
        Renderer::drawPoints(points, lumaxRenderer);
        Renderer::sendPointsToLumax(lumaxHandle, lumaxRenderer, 3000);
#endif

        // measure time
        auto end_time = std::chrono::high_resolution_clock::now();
        auto time = end_time - start_time;
        std::cout << "Extracted " << houghLines.size() << " lines and ";
        std::cout << "generated " << points.size() << " points. ";
        std::cout << "Took " << time/std::chrono::milliseconds(1) << "ms to run.\n";

        // build text for displaying values
        std::string str = "FPS: " +  intToStr(1000.0f * frame / worldtime.getTicks());
        sdl::auxiliary::Utilities::renderText(str, font, textColor, renderer, 25, 25);
        str = "(e+, d-): Light threshold = " + intToStr(lightThreshold);
        sdl::auxiliary::Utilities::renderText(str, font, textColor, renderer, 25, 50);
        str = "(r+, f-): Intersection threshold = " + intToStr(interThreshold);
        sdl::auxiliary::Utilities::renderText(str, font, textColor, renderer, 25, 75);
        str = "(t+, g-): Min line length = " + intToStr(minLineLength);
        sdl::auxiliary::Utilities::renderText(str, font, textColor, renderer, 25, 100);
        str = "(z+, h-): Max line gap = " + intToStr(maxLineGap);
        sdl::auxiliary::Utilities::renderText(str, font, textColor, renderer, 25, 125);
        str = "(u+, j-): Blur = " + intToStr(blursize);
        sdl::auxiliary::Utilities::renderText(str, font, textColor, renderer, 25, 150);
        str = "(i+, k-): Upper threshold = " + intToStr(upperThreshold);
        sdl::auxiliary::Utilities::renderText(str, font, textColor, renderer, 25, 175);
        str = "(o+, l-): Lower threshold = " + intToStr(lowerThreshold);
        sdl::auxiliary::Utilities::renderText(str, font, textColor, renderer, 25, 200);

       // FPS
        if (worldtime.getTicks() > 1000 ) {
            worldtime.start();
            frame = 0;
        }
        

        // apply the renderer to the screen
        SDL_RenderPresent(renderer);

        // increment the frame number
        frame++;
        // apply the fps cap
        if ((cap == true) && (fps.getTicks() < 1000 / FRAMES_PER_SECOND) ) {
            SDL_Delay((1000 / FRAMES_PER_SECOND) - fps.getTicks() );
        }
    }

#ifdef LUMAX_OUTPUT
    Lumax_StopFrame(lumaxHandle);
#endif

    // Destroy the various items
    sdl::auxiliary::Utilities::cleanup(renderer, window, texture);
    TTF_CloseFont(font);
    SDL_Quit();

    return 0;
}
