// sudo port install opencv4 eigen
// add 
//    export PKG_CONFIG_PATH="/opt/local/lib/opencv4/pkgconfig":$PKG_CONFIG_PATH
// to your shell profile.
//
// compile: 
//    g++ -std=c++17 $(pkg-config --cflags --libs opencv4) laser-display.cpp -o laser-display


#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <sstream>
#include <stdio.h>

#include <SDL.h>
#include <SDL_image.h>
#include "SDLTools/Utilities.h"
#include "SDLTools/Timer.h"
#include "GameLibrary/Renderer.h"

const int FRAMES_PER_SECOND = 20; // Fps auf 20 festlegen
const std::string image("test.jpg");

std::string intToStr(int a) {
    std::stringstream ss;
    ss << a;
    return ss.str();
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

    //Start up SDL and make sure it went ok
    if (SDL_Init(SDL_INIT_VIDEO) != 0){
        sdl::auxiliary::Utilities::logSDLError(std::cout, "SDL_Init");
        return -1;
    }

    // Set up our window and renderer, this time let's put our window in the center of the screen
    SDL_Window *window = SDL_CreateWindow("Laser-Display", SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL){
        sdl::auxiliary::Utilities::logSDLError(std::cout, "CreateWindow");
        SDL_Quit();
        return -1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL){
        sdl::auxiliary::Utilities::logSDLError(std::cout, "CreateRenderer");
        sdl::auxiliary::Utilities::cleanup(window);
        SDL_Quit();
        return -1;
    }

    // read image for the first time to get its dimensions
    cv::Mat img = cv::imread(image);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR24, SDL_TEXTUREACCESS_STREAMING, img.cols, img.rows);
    if (renderer == NULL){
        sdl::auxiliary::Utilities::logSDLError(std::cout, "CreateTexture");
        sdl::auxiliary::Utilities::cleanup(window, renderer);
        SDL_Quit();
        return -1;
    }

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
#if 0
            sdl::auxiliary::Utilities::cleanup(window, renderer);
#endif
            SDL_Quit();
            return 1;
        }
    }

    LumaxRenderer lumaxRenderer;
    lumaxRenderer.mirrorFactX = -1;
    lumaxRenderer.mirrorFactY = 1;
    // scaling of the laser output in respect to the SDL screen
    lumaxRenderer.scalingX = 0.3;
    lumaxRenderer.scalingY = 0.3;
#endif

    // logic
    bool quit = false;
    bool reset = false;

    // opencv specific
    int upperThreshold = 300;
    int lowerThreshold = 100;
    int blursize = 5;

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
            upperThreshold--;
        }
        if (keystate[SDL_SCANCODE_O]) {
            lowerThreshold++;
        }
        if (keystate[SDL_SCANCODE_L]) {
            lowerThreshold--;
        }
        
        

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
        cv::cvtColor(edges, edges, cv::COLOR_GRAY2RGB);
        // use edges as mask and multiply original image with mask
        cv::Mat dst;
        cv::bitwise_and(img, edges, edges);

        // build text for displaying values
        std::string str = "Upper threshold = " + intToStr(upperThreshold);
        cv::putText(edges, str, cv::Point(50, 50), cv::FONT_HERSHEY_DUPLEX, 1, cv::Scalar(0, 255, 0), 1, false);
        str = "Lower threshold = " + intToStr(lowerThreshold);
        cv::putText(edges, str, cv::Point(50, 100), cv::FONT_HERSHEY_DUPLEX, 1, cv::Scalar(0, 255, 0), 1, false);
        str = "Blur = " + intToStr(blursize);
        cv::putText(edges, str, cv::Point(50, 150), cv::FONT_HERSHEY_DUPLEX, 1, cv::Scalar(0, 255, 0), 1, false);
        // update the window caption
        str = "FPS: " +  intToStr(1000.0f * frame / worldtime.getTicks());
        cv::putText(edges, str, cv::Point(50, 200), cv::FONT_HERSHEY_DUPLEX, 1, cv::Scalar(0, 255, 0), 1, false);
        if (worldtime.getTicks() > 1000 ) {
            worldtime.start();
            frame = 0;
        }

        // copy the image to an SDL texture
        SDL_UpdateTexture(texture, NULL, (void*)edges.data, edges.step1());

        // render the texture
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
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
    IMG_Quit();
    SDL_Quit();

    return 0;
}
