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
#include "SDLTools/CommandLineParser.h"
#include "GameLibrary/Point.h"
#include "GameLibrary/Renderer.h"
#include "GameLibrary/Algorithms.h"
#include "GameLibrary/Sort.h"

#include "unittests/UnitTests.h"

#define OCVSTEP 5
#define MEASURETIME

// TODO: read from command line
const int FRAMES_PER_SECOND = 20; // Fps auf 20 festlegen
const std::string fimage("images/doom-smallres.png");
const std::string fvideo("videos/doom.mp4");
// TODO: read from command line
enum InputType {
    image, video, camera
};
const InputType inputtype = InputType::image;
// TODO: read from command line
// TODO: if values are 0, use original values
const int width = 960;
const int height = 600;

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

template<typename T> 
T distanceSq(XYPoint<T> a, XYPoint<T> b) {
    T deltaX = a.first - b.first;
    T deltaY = a.second - b.second;
    return (deltaX * deltaX + deltaY * deltaY);
}

void usage(char* argv[])
{
    std::cout << "Usage:" << std::endl << argv[0] << std::endl;
    // TODO usage
    std::exit(-1);
}

int main(int argc, char* argv[]) {
    // Check if all necessary command line arguments were provided
    if (argc < 2 || sdl::auxiliary::CommandLineParser::cmdOptionExists(argv, argv + argc, "-h"))
        usage(argv);

    std::string input;
    if (sdl::auxiliary::CommandLineParser::cmdOptionExists(argv, argv + argc, "-i")) {
        input = sdl::auxiliary::CommandLineParser::getCmdOption(argv, argv + argc, "-i");
        sdl::auxiliary::CommandLineParser::normalizePath(input);
        std::cout << "Input: " << input << std::endl;
        // TODO: if png -> inputtype = image
    }

    

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

    // open the default camera
    //cv::VideoCapture capture(0);
    // open video file
    cv::VideoCapture capture(fvideo);
    if(!capture.isOpened()) {
        sdl::auxiliary::Utilities::logSDLError(std::cout, "VideoCapture");
        SDL_Quit();
        return 1;
    }

    // TODO: in Funktion auslagern
    // read image for the first time to get its dimensions
    cv::Mat img;
    if (inputtype == InputType::image) {
        // read image from file
        img = cv::imread(fimage);
    } else if (inputtype == InputType::video || inputtype == InputType::camera) {
        // get a new frame from camera
        capture >> img;
        // TODO: if crop
        cv::Rect crop(150, 50, img.cols - 2*150, img.rows - 150);
        // Crop the full image to that image contained by the rectangle crop
        img = img(crop);
    }
    // sets the global variabls Renderer::screen_width and Renderer::screen_height
    if (width == 0 && height == 0) {
        // use the original dimensions
        Renderer::setDimensions(img.cols, img.rows);
    } else {
        // use fixed dimensions
        Renderer::setDimensions(width, height);
    }

    // TODO: Window-Abmessungen automatisch gemäß den Abmessungen des eingelesenen Bildes setzen, sodass es immer ungefähr 900 * 600 hat
    // Set up our window and renderer, this time let's put our window in the center of the screen
    SDL_Window *window = SDL_CreateWindow("Laser-Display", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, Renderer::screen_width, Renderer::screen_height, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        sdl::auxiliary::Utilities::logSDLError(std::cout, "SDL_CreateWindow");
        SDL_Quit();
        return -1;
    }

    // load the SDL renderer and set the screen dimensions
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
        sdl::auxiliary::Utilities::logSDLError(std::cout, "SDL_CreateRenderer");
        sdl::auxiliary::Utilities::cleanup(window);
        SDL_Quit();
        return -1;
    }
    //SDL_RenderSetLogicalSize(renderer, Renderer::screen_width, Renderer::screen_height);

    // create a texture for displaying images
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR24, SDL_TEXTUREACCESS_STREAMING, img.cols, img.rows);
    if (renderer == NULL) {
        sdl::auxiliary::Utilities::logSDLError(std::cout, "SDL_CreateTexture");
        sdl::auxiliary::Utilities::cleanup(window, renderer);
        SDL_Quit();
        return -1;
    }

    // setup text rendering
    TTF_Font* font = TTF_OpenFont("fonts/lazy.ttf", 16);
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
        if (lumaxHandle == NULL) {
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
    lumaxRenderer.scalingX = 0.2;
    lumaxRenderer.scalingY = 0.15;
    lumaxRenderer.swapXY = 0;
#endif

    // logic
    bool quit = false;
    bool pause = false;
    bool blankMoves = false;
    // opencv specific
#if 0
    // doom picture
    int fillShortBlanks = 0;
    int lightThreshold = 50;
    int interThreshold = 40;
    int minLineLength = 0;
    int maxLineGap = 10;
    int blursize = 7;
    int upperThreshold = 300;
    int lowerThreshold = 200;
#endif

#if 0
    // zoidberg
    int fillShortBlanks = 50;
    int lightThreshold = 1;
    int interThreshold = 10;
    int minLineLength = 3;
    int maxLineGap = 1;
    int blursize = 9;
    int upperThreshold = 100;
    int lowerThreshold = 200;
#endif

#if 0
    // video capture
    int fillShortBlanks = 0;
    int lightThreshold = 50;
    int interThreshold = 30;
    int minLineLength = 0;
    int maxLineGap = 5;
    int blursize = 19;
    int upperThreshold = 30;
    int lowerThreshold = 10;
#endif

#if 1
    // doom video capture
    int fillShortBlanks = 10;
    int lightThreshold = 50;
    int interThreshold = 10;
    int minLineLength = 0;
    int maxLineGap = 4;
    int blursize = 17;
    int upperThreshold = 30;
    int lowerThreshold = 10;
#endif

    int rResolution = 1;
    float thetaResolution = CV_PI / 180; // 1°

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
                    pause = !pause;
        }

        // handle keyboard inputs (no lags and delays!)
        const uint8_t* keystate = SDL_GetKeyboardState(NULL);
        if (keystate[SDL_SCANCODE_B]) {
            blankMoves = !blankMoves;
        }
        if (keystate[SDL_SCANCODE_W]) {
            fillShortBlanks++;
        }
        if (keystate[SDL_SCANCODE_S]) {
            fillShortBlanks = (fillShortBlanks >= 2 ? fillShortBlanks - 1 : fillShortBlanks);
        }
        if (keystate[SDL_SCANCODE_E]) {
            lightThreshold++;
        }
        if (keystate[SDL_SCANCODE_D]) {
            lightThreshold = (lightThreshold >= 2 ? lightThreshold - 1 : lightThreshold);
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
            lowerThreshold = (lowerThreshold < upperThreshold ? lowerThreshold + 1 : upperThreshold - 1);
        }
        if (keystate[SDL_SCANCODE_L]) {
            lowerThreshold = (lowerThreshold >= 1 ? lowerThreshold - 1 : lowerThreshold);
        }

#ifdef MEASURETIME
        // measure time
        auto start_time = std::chrono::high_resolution_clock::now();
#endif

        if (!pause) {
            if (inputtype == InputType::image) {
                // read image from file
                img = cv::imread(fimage);
            } else if (inputtype == InputType::video || inputtype == InputType::camera) {
                // get a new frame from camera
                capture >> img;
                // TODO: if crop
                cv::Rect crop(150, 50, img.cols - 2*150, img.rows - 150);
                // Crop the full image to that image contained by the rectangle crop
                img = img(crop);
            }
        }
#if OCVSTEP == 0
        cv::Mat display = img.clone();
#endif

// TODO: test if HSV threshold may improve object detection
#if 0
        // HSV threshold detection
        // Convert from BGR to HSV colorspace
        cv::Mat imgHSV;
        cv::cvtColor(img, imgHSV, cv::COLOR_BGR2HSV);
        // Detect the object based on HSV Range Values
        cv::Mat img_threshold;
        int max_value = 255;
        int max_value_H = 360/2;
        int low_H = 0;
        int low_S = 0;
        int low_V = 0;
        int high_H = max_value_H;
        int high_S = max_value;
        int high_V = max_value;
        cv::inRange(imgHSV, cv::Scalar(low_H, low_S, low_V), cv::Scalar(high_H, high_S, high_V), img_threshold);
#if OCVSTEP == 1
        cv::Mat display = img_threshold.clone();
        //cv::cvtColor(display, display, cv::COLOR_HSV2BGR);
#endif
#endif

        // Blur the image for better edge detection
        cv::Mat img_blur;
        cv::GaussianBlur(img, img_blur, cv::Size(blursize, blursize), 0);
#if OCVSTEP == 2
        cv::Mat display = img_blur.clone();
#endif

        // Convert to graycsale
        cv::Mat img_gray;
        cv::cvtColor(img_blur, img_gray, cv::COLOR_BGR2GRAY);
#if OCVSTEP == 3
        cv::Mat display = img_gray.clone();
        // convert to original color space, preserving content
        cv::cvtColor(display, display, cv::COLOR_GRAY2RGB);
#endif

        // Canny edge detection
        cv::Mat edges;
        cv::Canny(img_gray, edges, lowerThreshold, upperThreshold, 3, false);
#if OCVSTEP == 4
        cv::Mat display = edges.clone();
        // convert to original color space, preserving content
        cv::cvtColor(display, display, cv::COLOR_GRAY2RGB);
#endif

        // dilate the lines (thicken)
        int dilationSize = 1;
        int erosionType = cv::MORPH_ELLIPSE; // MORPH_RECT, MORPH_CROSS, MORPH_ELLIPSE
        cv::Mat element = cv::getStructuringElement(erosionType, cv::Size(2*dilationSize + 1, 2*dilationSize+1), cv::Point(dilationSize, dilationSize));
        cv::dilate(edges, edges, element);
#if OCVSTEP == 5
        cv::Mat display = edges.clone();
        // convert to original color space, preserving content
        cv::cvtColor(display, display, cv::COLOR_GRAY2RGB);
#endif


        // probabilistic Hough Line Transform
        std::vector<cv::Vec4i> houghLines; // HoughLinesP: will hold the results of the detection
        HoughLinesP(edges, houghLines, rResolution, thetaResolution, interThreshold, minLineLength, maxLineGap); // runs the actual detection
        // sort the lines (TSP problem)
        Sort::sortLines(houghLines);

        // Draw the lines
        cv::Mat lines = edges.clone(); // copy to have a matrix with the right size
        lines.setTo(cv::Scalar(0, 0, 0));
        for(size_t i = 0; i < houghLines.size(); ++i) {
            cv::Vec4i l = houghLines[i];
            cv::line(lines, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(255, 255, 255), 1, cv::LINE_AA);
        }
        // convert to original color space, preserving content
        cv::cvtColor(lines, lines, cv::COLOR_GRAY2RGB);
#if OCVSTEP == 6
        cv::Mat display = lines.clone();
#endif

        // use lines as mask and multiply original image with mask
        cv::bitwise_and(img, lines, lines);
#if OCVSTEP == 7
        cv::Mat display = lines.clone();
#endif
        // Draw the background black
        SDL_RenderClear(renderer);        
        boxRGBA(renderer, 0, 0, Renderer::screen_width, Renderer::screen_height, 10, 10, 10, 255);

#ifdef OCVSTEP
        // render the image
        SDL_UpdateTexture(texture, NULL, (void*)display.data, display.step1());
        SDL_Rect destRect = {0, 0, Renderer::screen_width, Renderer::screen_height};
        //std::cout << destRect.w << ", " << destRect.h << std::endl;
        SDL_RenderCopy(renderer, texture, NULL, &destRect);
        //SDL_RenderCopyEx(renderer, texture, NULL, &destRect, 0, NULL, SDL_FLIP_NONE);
#endif

        // apply the lines to the renderers
        std::vector<Point<float>> points;
        points.reserve(15000); // TODO
        int lastLaser[2] = {Renderer::screen_width / 2, Renderer::screen_height / 2};
        int lastSDL[2] = {Renderer::screen_width / 2, Renderer::screen_height / 2};
        for(size_t i = 0; i < houghLines.size(); ++i) {
            cv::Vec4i l = houghLines[i];
            cv::Vec3b intensity1 = lines.at<cv::Vec3b>(cv::Point(constrain<int>(l[0], 0, lines.cols - 1), constrain<int>(l[1], 0, lines.rows - 1)));
            cv::Vec3b intensity2 = lines.at<cv::Vec3b>(cv::Point(constrain<int>(l[2], 0, lines.cols - 1), constrain<int>(l[3], 0, lines.rows - 1)));
            
            int blue  = constrain<int>((intensity1.val[0] + intensity2.val[0]) / 2, 0, 255);
            int green = constrain<int>((intensity1.val[1] + intensity2.val[1]) / 2, 0, 255);
            int red   = constrain<int>((intensity1.val[2] + intensity2.val[2]) / 2, 0, 255);

            // sort out dark lines
            if ((blue + green + red) >= lightThreshold) {
                // TODO: implement color correction function
                float colorFactor = 255 / std::max(blue, std::max(green, red));
                blue  *= colorFactor;
                green *= colorFactor;
                red   *= colorFactor;

                // TODO: DEBUG
                blue = 255;
                red = 255;
                green = 0 * 255;

                // Points for Laser output
                // TODO: check implementation
                if (std::sqrt(distanceSq(XYPoint<int>({l[0], l[1]}), XYPoint<int>({lastLaser[0], lastLaser[1]}))) > fillShortBlanks) {
                    // blank move
                    points.push_back({(float)lastLaser[0], (float)lastLaser[1], 0, 0, 0, 255, false});
                    points.push_back({(float)l[0], (float)l[1], 0, 0, 0, 255, false});
                }
                // laser line
                points.push_back({(float)l[0], (float)l[1], blue, green, red, 255, false});
                points.push_back({(float)l[2], (float)l[3], blue, green, red, 255, false});
                // store the last laser point
                lastLaser[0] = l[2];
                lastLaser[1] = l[3];

                // Points for SDL output: Transform from original image dimensions to dimensions of the renderer's screen
                l[0] = Renderer::transform(l[0], 0, img.cols, 0, Renderer::screen_width);
                l[1] = Renderer::transform(l[1], 0, img.rows, 0, Renderer::screen_height);
                l[2] = Renderer::transform(l[2], 0, img.cols, 0, Renderer::screen_width);
                l[3] = Renderer::transform(l[3], 0, img.rows, 0, Renderer::screen_height);
                // TODO: fillShortBlanks should be different in the SDL renderer context
                if (std::sqrt(distanceSq(XYPoint<int>({l[0], l[1]}), XYPoint<int>({lastSDL[0], lastSDL[1]}))) <= fillShortBlanks)
                    lineRGBA(renderer, lastSDL[0], lastSDL[1], (int)l[0], (int)l[1], red, green, blue, 255);
                else if (blankMoves)
                    lineRGBA(renderer, lastSDL[0], lastSDL[1], (int)l[0], (int)l[1], 0, 255, 255, 255); // blank move
                lineRGBA(renderer, (int)l[0], (int)l[1], (int)l[2], (int)l[3], red, green, blue, 255);
                // store the last SDL point
                lastSDL[0] = l[2];
                lastSDL[1] = l[3];
            }
        }

#ifdef LUMAX_OUTPUT
        Renderer::drawPoints(points, lumaxRenderer);
        Renderer::sendPointsToLumax(lumaxHandle, lumaxRenderer, 20000);
#endif
        
#ifdef MEASURETIME
        // measure time
        auto end_time = std::chrono::high_resolution_clock::now();
        auto time = end_time - start_time;
        std::cout << "Extracted " << houghLines.size() << " lines and ";
        std::cout << "generated " << points.size() << " points. ";
        std::cout << "Took " << time/std::chrono::milliseconds(1) << "ms to run.\n";
#endif

        // build text for displaying values
        std::string str = "FPS: " +  intToStr(1000.0f * frame / worldtime.getTicks());
        sdl::auxiliary::Utilities::renderText(str, font, textColor, renderer, 25, 25);
        str = "(w+, s-): General: fill shorts = " + intToStr(fillShortBlanks);
        sdl::auxiliary::Utilities::renderText(str, font, textColor, renderer, 25, 50);
        str = "(e+, d-): General: Light threshold = " + intToStr(lightThreshold);
        sdl::auxiliary::Utilities::renderText(str, font, textColor, renderer, 25, 75);
        str = "(r+, f-): Line detection: Intersection threshold = " + intToStr(interThreshold);
        sdl::auxiliary::Utilities::renderText(str, font, textColor, renderer, 25, 100);
        str = "(t+, g-): Line detection: Min line length = " + intToStr(minLineLength);
        sdl::auxiliary::Utilities::renderText(str, font, textColor, renderer, 25, 125);
        str = "(z+, h-): Line detection: Max line gap = " + intToStr(maxLineGap);
        sdl::auxiliary::Utilities::renderText(str, font, textColor, renderer, 25, 150);
        str = "(u+, j-): Blurring: Blur size = " + intToStr(blursize);
        sdl::auxiliary::Utilities::renderText(str, font, textColor, renderer, 25, 175);
        str = "(i+, k-): Edge Detection: Upper threshold = " + intToStr(upperThreshold);
        sdl::auxiliary::Utilities::renderText(str, font, textColor, renderer, 25, 200);
        str = "(o+, l-): Edge Detection: Lower threshold = " + intToStr(lowerThreshold);
        sdl::auxiliary::Utilities::renderText(str, font, textColor, renderer, 25, 225);

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
