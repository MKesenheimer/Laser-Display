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
#include <libconfig.h++>

#include "SDLTools/Utilities.h"
#include "SDLTools/Timer.h"
#include "SDLTools/CommandLineParser.h"
#include "GameLibrary/Point.h"
#include "GameLibrary/Renderer.h"
#include "GameLibrary/Algorithms.h"
#include "GameLibrary/Sort.h"
#include "GameLibrary/vector.h"
#include "GameLibrary/matrix.h"
#include "GameLibrary/operators.h"
#include "GameLibrary/Fit.h"

#define OCVSTEP 0
#define MEASURETIME

// InputType structure 
enum InputType {
    image, video, camera
};

// all relevant paramters
struct Parameters {
    // input handling
    std::array<int, 4> crop = {0, 0, 0, 0};
    InputType inputtype = InputType::image;
    std::string inputFile;

    // renderer options
    int width = 900;
    int height = 600;

    // opencv specific
    int fillShortBlanks = 10;
    int lightThreshold = 50;
    int interThreshold = 10;
    int minLineLength = 0;
    int maxLineGap = 4;
    int blursize = 17;
    int upperThreshold = 30;
    int lowerThreshold = 10;
    int rResolution = 1;
    float thetaResolution = 10 * CV_PI / 180; // 10°
    bool blankMoves = false;
    bool doColorCorrection = false;
    bool colorBoost = true;

    // SDL specific
    int maxFramesPerSecond = 20;
};

template<typename T> 
T distanceSq(XYPoint<T> a, XYPoint<T> b) {
    T deltaX = a.first - b.first;
    T deltaY = a.second - b.second;
    return (deltaX * deltaX + deltaY * deltaY);
}

void usage(char* argv[]) {
    std::cout << "Usage:" << std::endl << argv[0] << " -i <path/filename> [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "-h                                                   display help message" << std::endl;
    std::cout << "-i <path/filename>                                   input file to render" << std::endl;
    std::cout << "-x <width>                                           display width" << std::endl;
    std::cout << "-y <height>                                          display height" << std::endl;
    std::cout << "-c <crop-left>,<crop-up>,<crop-right>,<crop-down>    crop dimensions" << std::endl;

    std::exit(-1);
}

cv::Mat readInputSource(const std::string& input, cv::VideoCapture& capture, InputType inputtype, std::array<int, 4> cropDim) {
    cv::Mat img;
    if (inputtype == InputType::image) {
        // read image from file
        img = cv::imread(input);
    } else if (inputtype == InputType::video || inputtype == InputType::camera) {
        // get a new frame from camera
        capture >> img;
    
        if (cropDim[0] != 0 || cropDim[1] != 0 || cropDim[2] != 0 || cropDim[3] != 0) {
            cropDim[0] = Algorithms::constrain(cropDim[0], 0, img.cols / 2);
            cropDim[1] = Algorithms::constrain(cropDim[1], 0, img.rows / 2);
            cropDim[2] = Algorithms::constrain(cropDim[2], 0, img.cols / 2);
            cropDim[3] = Algorithms::constrain(cropDim[3], 0, img.rows / 2);
            // Crop the full image to that image contained by the rectangle crop
            cv::Rect crop(cropDim[0], cropDim[1], img.cols - cropDim[0] - cropDim[2], img.rows - cropDim[1] - cropDim[3]);
            img = img(crop);
        }
    }
    return img;
}

#if LUMAX_OUTPUT
// TODO: move to seperate file
void colorCorrection(void* lumaxHandle, Renderer::LumaxRenderer& ren, SDL_Renderer* renderer, TTF_Font* font) {
    sdl::auxiliary::Timer fps;
    SDL_Event e;
    bool quit = false;
    bool done = false;
    std::string message = std::string();
    size_t maxSteps = 7;
    math::vector<math::vector<double>> pointsRed(4, 2);
    math::vector<math::vector<double>> pointsGre(4, 2);
    math::vector<math::vector<double>> pointsBlu(4, 2);

    for (int step = 0; step < maxSteps; ++step) {
        int r = 0, g = 0, b = 0;
        switch(step) {
            case 0:
                r = 0;
                g = 0;
                b = 0;
                done = false;
                message = "Increase red until it is bareley visible";
                break;
            case 1:
                r = 0;
                g = 0;
                b = 0;
                message = "Increase green until it is bareley visible";
                done = false;
                break;
            case 2:
                r = 0;
                g = 0;
                b = 0;
                message = "Increase blue until it is bareley visible";
                done = false;
                break;
            case 3:
                r = 255;
                g = 255;
                b = 255;
                done = false;
                message = "Decrease red, green and blue until the beam is white";
                break;
            case 4:
                r = 170;
                g = 170;
                b = 170;
                done = false;
                message = "Vary red and green until the beam is white. Keep blue constant.";
                break;
            case 5:
                r = 170;
                g = 170;
                b = 170;
                done = false;
                message = "Vary green and blue until the beam is white. Keep red constant.";
                break;
            case 6:
                r = 170;
                g = 170;
                b = 170;
                done = false;
                message = "Vary blue and red until the beam is white. Keep green constant.";
                break;
        }
        
        while (!done && !quit) {
            fps.start();
            // read any events that occured, for now we'll just quit if any event occurs
            while (SDL_PollEvent(&e)) {
                // if user closes the window
                if (e.type == SDL_QUIT)
                    quit = true;
                // if user presses any key
                else if (e.type == SDL_KEYDOWN) {
                    if (e.key.keysym.sym == SDLK_SPACE) {
                        done = true;
                        switch(step) {
                            case 0:
                                pointsRed[0][0] = 0; // soll
                                pointsRed[0][1] = r; // ist
                                break;
                            case 1:
                                pointsGre[0][0] = 0; // soll
                                pointsGre[0][1] = g; // ist
                                break;
                            case 2:
                                pointsBlu[0][0] = 0; // soll
                                pointsBlu[0][1] = b; // ist
                                break;
                            case 3:
                                pointsRed[1][0] = 255; // soll
                                pointsRed[1][1] = r; // ist
                                pointsGre[1][0] = 255; // soll
                                pointsGre[1][1] = g; // ist
                                pointsBlu[1][0] = 255; // soll
                                pointsBlu[1][1] = b; // ist
                                break;
                            case 4:
                                pointsRed[2][0] = 170; // soll
                                pointsRed[2][1] = r; // ist
                                pointsGre[2][0] = 170; // soll
                                pointsGre[2][1] = g; // ist
                                break;
                            case 5:
                                pointsGre[3][0] = 170; // soll
                                pointsGre[3][1] = g; // ist
                                pointsBlu[2][0] = 170; // soll
                                pointsBlu[2][1] = b; // ist
                                break;
                            case 6:
                                pointsBlu[3][0] = 170; // soll
                                pointsBlu[3][1] = b; // ist
                                pointsRed[3][0] = 170; // soll
                                pointsRed[3][1] = r; // ist
                                break;
                        }
                    }
                }
            }

            const uint8_t* keystate = SDL_GetKeyboardState(NULL);
            if (keystate[SDL_SCANCODE_Q]) {
                r = Algorithms::constrain<int>(r + 1, 0, 255);
            }
            if (keystate[SDL_SCANCODE_A]) {
                r = Algorithms::constrain<int>(r - 1, 0, 255);
            }
            if (keystate[SDL_SCANCODE_W]) {
                g = Algorithms::constrain<int>(g + 1, 0, 255);
            }
            if (keystate[SDL_SCANCODE_S]) {
                g = Algorithms::constrain<int>(g - 1, 0, 255);
            }
            if (keystate[SDL_SCANCODE_E]) {
                b = Algorithms::constrain<int>(b + 1, 0, 255);
            }
            if (keystate[SDL_SCANCODE_D]) {
                b = Algorithms::constrain<int>(b - 1, 0, 255);
            }

            // Draw the background black
            SDL_RenderClear(renderer);        
            boxRGBA(renderer, 0, 0, Renderer::screen_width, Renderer::screen_height, 10, 10, 10, 255);

            // build text for displaying values
            SDL_Color textColor = {0, 255, 0};
            sdl::auxiliary::Utilities::renderText(message, font, textColor, renderer, 25, 25);
            std::string str = "(q+, a-): increase/decrease red " + Algorithms::typeToStr<int>(r);
            sdl::auxiliary::Utilities::renderText(str, font, textColor, renderer, 25, 50);
            str = "(w+, s-): increase/decrease green " + Algorithms::typeToStr<int>(g);
            sdl::auxiliary::Utilities::renderText(str, font, textColor, renderer, 25, 75);
            str = "(e+, d-): increase/decrease blue " + Algorithms::typeToStr<int>(b);
            sdl::auxiliary::Utilities::renderText(str, font, textColor, renderer, 25, 100);

            // apply the renderer to the screen
            SDL_RenderPresent(renderer);

            std::vector<Point<float>> points;
            points.push_back({ 100,  100,   0,   0,   0, 255, false});
            points.push_back({ 100,  100, r, g, b, 255, false});
            points.push_back({-100,  100, r, g, b, 255, false});
            points.push_back({-100, -100, r, g, b, 255, false});
            points.push_back({ 100, -100, r, g, b, 255, false});
            points.push_back({ 100,  100, r, g, b, 255, false});
            Renderer::drawPoints(points, ren);
            Renderer::sendPointsToLumax(lumaxHandle, ren, 200);

            // apply the fps cap
            if (fps.getTicks() < 1000 / 10) {
                SDL_Delay((1000 / 10) - fps.getTicks() );
            }
        }
    }

    // Summary
    std::cout << "Summary:" << std::endl;
    math::matrix<double> matrixRed(pointsRed);
    math::matrix<double> matrixGre(pointsGre);
    math::matrix<double> matrixBlu(pointsBlu);

    for (size_t i = 0; i < matrixRed.rows(); ++i)
        std::cout << "Pred_" << i << " = (" << matrixRed(i, 0) << ", " << matrixRed(i, 1) << ")" << std::endl;
    for (size_t i = 0; i < matrixGre.rows(); ++i)
        std::cout << "Pgre_" << i << " = (" << matrixGre(i, 0) << ", " << matrixGre(i, 1) << ")" << std::endl;
    for (size_t i = 0; i < matrixBlu.rows(); ++i)
        std::cout << "Pblu_" << i << " = (" << matrixBlu(i, 0) << ", " << matrixBlu(i, 1) << ")" << std::endl;

    math::vector<double> coeffRed = math::utilities::Fit::polyFit<double>(matrixRed, 2);
    math::vector<double> coeffGre = math::utilities::Fit::polyFit<double>(matrixGre, 2);
    math::vector<double> coeffBlu = math::utilities::Fit::polyFit<double>(matrixBlu, 2);

    std::cout << "Color correction polynomials:" << std::endl;
    std::cout << "Pol_red(x) = " << coeffRed[0] << " + " << coeffRed[1] << " * x + " << coeffRed[2] << " * x^2" << std::endl; 
    std::cout << "Pol_gre(x) = " << coeffGre[0] << " + " << coeffGre[1] << " * x + " << coeffGre[2] << " * x^2" << std::endl; 
    std::cout << "Pol_blu(x) = " << coeffBlu[0] << " + " << coeffBlu[1] << " * x + " << coeffBlu[2] << " * x^2" << std::endl; 

    // store the coefficients
    ren.parameters.colorCorr.ar = static_cast<float>(coeffRed[2]);
    ren.parameters.colorCorr.br = static_cast<float>(coeffRed[1]);
    ren.parameters.colorCorr.cr = static_cast<float>(coeffRed[0]);
    ren.parameters.colorCorr.ag = static_cast<float>(coeffGre[2]);
    ren.parameters.colorCorr.bg = static_cast<float>(coeffGre[1]);
    ren.parameters.colorCorr.cg = static_cast<float>(coeffGre[0]);
    ren.parameters.colorCorr.ab = static_cast<float>(coeffBlu[2]);
    ren.parameters.colorCorr.bb = static_cast<float>(coeffBlu[1]);
    ren.parameters.colorCorr.cb = static_cast<float>(coeffBlu[0]);
}
#endif

void handleKeyPress(Parameters& parameters) {
    // handle keyboard inputs (no lags and delays!)
    const uint8_t* keystate = SDL_GetKeyboardState(NULL);
    if (keystate[SDL_SCANCODE_B]) {
        parameters.blankMoves = !parameters.blankMoves;
    }
    if (keystate[SDL_SCANCODE_W]) {
        parameters.fillShortBlanks++;
    }
    if (keystate[SDL_SCANCODE_S]) {
        parameters.fillShortBlanks = (parameters.fillShortBlanks >= 2 ? parameters.fillShortBlanks - 1 : parameters.fillShortBlanks);
    }
    if (keystate[SDL_SCANCODE_E]) {
        parameters.lightThreshold++;
    }
    if (keystate[SDL_SCANCODE_D]) {
        parameters.lightThreshold = (parameters.lightThreshold >= 2 ? parameters.lightThreshold - 1 : parameters.lightThreshold);
    }
    if (keystate[SDL_SCANCODE_R]) {
        parameters.interThreshold++;
    }
    if (keystate[SDL_SCANCODE_F]) {
        parameters.interThreshold = (parameters.interThreshold >= 1 ? parameters.interThreshold - 1 : parameters.interThreshold);
    }
    if (keystate[SDL_SCANCODE_T]) {
        parameters.minLineLength++;
    }
    if (keystate[SDL_SCANCODE_G]) {
        parameters.minLineLength = (parameters.minLineLength >= 1 ? parameters.minLineLength - 1 : parameters.minLineLength);   
    }
    if (keystate[SDL_SCANCODE_Y]) {
        parameters.maxLineGap++;
    }
    if (keystate[SDL_SCANCODE_H]) {
        parameters.maxLineGap = (parameters.maxLineGap >= 1 ? parameters.maxLineGap - 1 : parameters.maxLineGap);
    }
    if (keystate[SDL_SCANCODE_U]) {
        parameters.blursize += 2;
    }
    if (keystate[SDL_SCANCODE_J]) {
        parameters.blursize = (parameters.blursize >= 3 ? parameters.blursize - 2 : parameters.blursize);
    }
    if (keystate[SDL_SCANCODE_I]) {
        parameters.upperThreshold++;
    }
    if (keystate[SDL_SCANCODE_K]) {
        parameters.upperThreshold = (parameters.upperThreshold >= 1 ? parameters.upperThreshold - 1 : parameters.upperThreshold);
    }
    if (keystate[SDL_SCANCODE_O]) {
        parameters.lowerThreshold = (parameters.lowerThreshold < parameters.upperThreshold ? parameters.lowerThreshold + 1 : parameters.upperThreshold - 1);
    }
    if (keystate[SDL_SCANCODE_L]) {
        parameters.lowerThreshold = (parameters.lowerThreshold >= 1 ? parameters.lowerThreshold - 1 : parameters.lowerThreshold);
    }
}

int getParameters(int argc, char* argv[], Parameters& parameters) {
    // Check if all necessary command line arguments were provided
    if (argc < 2 || sdl::auxiliary::CommandLineParser::cmdOptionExists(argv, argv + argc, "-h"))
        usage(argv);

    if (sdl::auxiliary::CommandLineParser::cmdOptionExists(argv, argv + argc, "-q"))
        parameters.doColorCorrection = true;

    // read input file
    parameters.inputFile = sdl::auxiliary::CommandLineParser::readCmdNormalized(argv, argv + argc, "-i");
    if (parameters.inputFile == std::string()) {
        std::cerr << "Error: no input given." << std::endl;
        usage(argv);
    }
    std::cout << "Input file: " << parameters.inputFile << std::endl;

    // determine input type
    parameters.inputtype = InputType::image;
    if (parameters.inputFile == "camera") {
        parameters.inputtype = InputType::camera;
    } else {
        std::string suffix = parameters.inputFile.substr(parameters.inputFile.size() - 3);
        std::transform(suffix.begin(), suffix.end(), suffix.begin(), ::tolower);
        if (suffix == "png" || suffix == "jpg" || suffix == "jpeg") {
            parameters.inputtype = InputType::image;
            std::cout << "Input type: image" << std::endl;
        } else if (suffix == "mp4") {
            parameters.inputtype = InputType::video;
            std::cout << "Input type: video" << std::endl;
        } else {
            std::cerr << "Error: Data type unknown." << std::endl;
            usage(argv);
        }
    }

    // read max FPS
    parameters.maxFramesPerSecond = sdl::auxiliary::CommandLineParser::readCmdOption<int>(argv, argv + argc, "-f", 1, 100);
    std::cout << "max FPS: " << parameters.maxFramesPerSecond << std::endl;

    // screen dimensions
    parameters.width = sdl::auxiliary::CommandLineParser::readCmdOption<int>(argv, argv + argc, "-x", 0, INT_MAX);
    parameters.height = sdl::auxiliary::CommandLineParser::readCmdOption<int>(argv, argv + argc, "-y", 0, INT_MAX);
    if (parameters.width == 0 && parameters.height == 0) {
        std::cout << "Using original dimensions of input source." << std::endl;
    } else {
        std::cout << "Screen width: " << parameters.width << std::endl;
        std::cout << "Screen height: " << parameters.height << std::endl;
    }

    // read in crop size:
    std::vector<int> cropv = sdl::auxiliary::CommandLineParser::readCmdOptionList<int>(argv, argv + argc, "-c", 0, INT_MAX);
    if (cropv.size() == 4) {
        for (size_t i = 0; i < cropv.size(); ++i)
            parameters.crop[i] = cropv[i];
        std::cout << "Crop size: (" << parameters.crop[0] << ", " << parameters.crop[1] << ", " << parameters.crop[2] << ", " << parameters.crop[3] << ")" << std::endl;
    }

    return 0;
}

#ifdef LUMAX_OUTPUT
int getLumaxParameters(int argc, char* argv[], Renderer::LumaxParameters& parameters) {
    // read config from file
    libconfig::Config cfg;
    // Read the file. If there is an error, report it and exit.
    try {
        cfg.readFile("config.cfg");
    } catch(const libconfig::FileIOException &fioex) {
        std::cerr << "I/O error while reading file." << std::endl;
        return -1;
    } catch(const libconfig::ParseException &pex) {
        std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
                  << " - " << pex.getError() << std::endl;
        return -1;
    }

    // Get the version
    try {
        std::string version = cfg.lookup("version");
        std::cout << "Version: " << version << std::endl << std::endl;
    } catch(const libconfig::SettingNotFoundException &nfex) {
        std::cerr << "No 'version' setting in configuration file." << std::endl;
        return -1;
    }

    const libconfig::Setting& root = cfg.getRoot();
    try {
        const libconfig::Setting &colorCorrection = root["application"]["color-correction"];
        colorCorrection.lookupValue("ar", parameters.colorCorr.ar);
        colorCorrection.lookupValue("br", parameters.colorCorr.br);
        colorCorrection.lookupValue("cr", parameters.colorCorr.cr);
        colorCorrection.lookupValue("ag", parameters.colorCorr.ag);
        colorCorrection.lookupValue("bg", parameters.colorCorr.bg);
        colorCorrection.lookupValue("cg", parameters.colorCorr.cg);
        colorCorrection.lookupValue("ab", parameters.colorCorr.ab);
        colorCorrection.lookupValue("bb", parameters.colorCorr.bb);
        colorCorrection.lookupValue("cb", parameters.colorCorr.cb);
        std::cout << "ar = " << parameters.colorCorr.ar;
        std::cout << ", br = " << parameters.colorCorr.br;
        std::cout << ", cr = " << parameters.colorCorr.cr << std::endl;
        std::cout << "ag = " << parameters.colorCorr.ag;
        std::cout << ", bg = " << parameters.colorCorr.bg;
        std::cout << ", cg = " << parameters.colorCorr.cg << std::endl;
        std::cout << "ab = " << parameters.colorCorr.ab;
        std::cout << ", bb = " << parameters.colorCorr.bb;
        std::cout <<  ", cb = " << parameters.colorCorr.cb << std::endl;
    } catch(const libconfig::SettingNotFoundException &nfex) {} // Ignore


    parameters.mirrorFactX = -1;
    parameters.mirrorFactY = 1;
    parameters.scalingX = 0.2;
    parameters.scalingY = 0.15;
    parameters.swapXY = 0;

    return 0;
}
#endif

int main(int argc, char* argv[]) {
    Parameters parameters;
    int ret = getParameters(argc, argv, parameters);    

#ifdef LUMAX_OUTPUT
    // open Lumax device
    int NumOfCards = Lumax_GetPhysicalDevices();
    void* lumaxHandle = NULL;
    std::cout << "Number of MiniLumax devices: " <<  NumOfCards << std::endl;
    if (NumOfCards > 0) {
        lumaxHandle = Lumax_OpenDevice(1, 0);
        std::cout << "Lumax_OpenDevice returned handle: 0x" << std::hex << (unsigned long)lumaxHandle << std::endl;
        if (lumaxHandle == NULL) {
            std::cerr << "I/O Error while opening the Lumax device." << std::endl;
            SDL_Quit();
            return 1;
        }
    }

    // declare the lumax renderer
    Renderer::LumaxRenderer lumaxRenderer;
    ret = getLumaxParameters(argc, argv, lumaxRenderer.parameters);
#endif

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
        std::cerr << "Error in TTF_Init: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Start up SDL and make sure it went ok
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "Error in SDL_Init: " << SDL_GetError() << std::endl;
        return -1;
    }
    
    // initialize video or camera input
    cv::VideoCapture capture = cv::VideoCapture();
    if (parameters.inputtype == camera) {
        // open the default camera
        capture.open(0);
    } else if (parameters.inputtype == video) {
        // open video file
        capture.open(parameters.inputFile);
    }
    if (parameters.inputtype == video || parameters.inputtype == camera) {
        if (!capture.isOpened()) {
            std::cerr << "Error while opening the video device." << std::endl;
            SDL_Quit();
            return 1;
        }
    }

    // read image for the first time to get its dimensions
    cv::Mat img = readInputSource(parameters.inputFile, capture, parameters.inputtype, parameters.crop);
    // sets the global variabls Renderer::screen_width and Renderer::screen_height
    if (parameters.width == 0 && parameters.height == 0) {
        // use the original dimensions
        Renderer::setDimensions(img.cols, img.rows);
    } else {
        // use fixed dimensions
        Renderer::setDimensions(parameters.width, parameters.height);
    }

    // Set up our window and renderer, this time let's put our window in the center of the screen
    SDL_Window *window = SDL_CreateWindow("Laser-Display", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, Renderer::screen_width, Renderer::screen_height, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        std::cerr << "Error in SDL_CreateWindow: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    // load the SDL renderer and set the screen dimensions
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
        std::cerr << "Error in SDL_CreateRenderer: " << SDL_GetError() << std::endl;
        sdl::auxiliary::Utilities::cleanup(window);
        SDL_Quit();
        return -1;
    }

    // create a texture for displaying images
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR24, SDL_TEXTUREACCESS_STREAMING, img.cols, img.rows);
    if (renderer == NULL) {
        std::cerr << "Error in SDL_CreateTexture: " << SDL_GetError() << std::endl;
        sdl::auxiliary::Utilities::cleanup(window, renderer);
        SDL_Quit();
        return -1;
    }

    // setup text rendering
    TTF_Font* font = TTF_OpenFont("fonts/lazy.ttf", 16);
    if (font == NULL) {
        std::cerr << "Error in TTF_OpenFont: " << SDL_GetError() << std::endl;
        sdl::auxiliary::Utilities::cleanup(window, renderer, texture);
        SDL_Quit();
        return -1;
    }
    SDL_Color textColor = {0, 255, 0};

#ifdef LUMAX_OUTPUT
    // before we start: do the color calibration routine
    if (parameters.doColorCorrection == true)
        colorCorrection(lumaxHandle, lumaxRenderer, renderer, font);
#endif

    // the event structure
    bool quit = false;
    bool pause = false;
    SDL_Event e;
    while (!quit) {
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

        handleKeyPress(parameters);

#ifdef MEASURETIME
        // measure time
        auto start_time = std::chrono::high_resolution_clock::now();
#endif

        if (!pause) {
            img = readInputSource(parameters.inputFile, capture, parameters.inputtype, parameters.crop);
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
        cv::GaussianBlur(img, img_blur, cv::Size(parameters.blursize, parameters.blursize), 0);
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
        cv::Canny(img_gray, edges, parameters.lowerThreshold, parameters.upperThreshold, 3, false);
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
        HoughLinesP(edges, houghLines, parameters.rResolution, parameters.thetaResolution, parameters.interThreshold, parameters.minLineLength, parameters.maxLineGap);
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
            cv::Vec3b intensity1 = lines.at<cv::Vec3b>(cv::Point(Algorithms::constrain<int>(l[0], 0, lines.cols - 1), Algorithms::constrain<int>(l[1], 0, lines.rows - 1)));
            cv::Vec3b intensity2 = lines.at<cv::Vec3b>(cv::Point(Algorithms::constrain<int>(l[2], 0, lines.cols - 1), Algorithms::constrain<int>(l[3], 0, lines.rows - 1)));
            
            int blue  = Algorithms::constrain<int>((intensity1.val[0] + intensity2.val[0]) / 2, 0, 255);
            int green = Algorithms::constrain<int>((intensity1.val[1] + intensity2.val[1]) / 2, 0, 255);
            int red   = Algorithms::constrain<int>((intensity1.val[2] + intensity2.val[2]) / 2, 0, 255);

            // sort out dark lines
            if ((blue + green + red) >= parameters.lightThreshold) {
                // color boost
                if (parameters.colorBoost) {
                    float colorFactor = 255 / std::max(blue, std::max(green, red));
                    blue  *= colorFactor;
                    green *= colorFactor;
                    red   *= colorFactor;
                }

                // Points for Laser output
                if (std::sqrt(distanceSq(XYPoint<int>({l[0], l[1]}), XYPoint<int>({lastLaser[0], lastLaser[1]}))) > parameters.fillShortBlanks) {
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
                if (std::sqrt(distanceSq(XYPoint<int>({l[0], l[1]}), XYPoint<int>({lastSDL[0], lastSDL[1]}))) <= parameters.fillShortBlanks)
                    lineRGBA(renderer, lastSDL[0], lastSDL[1], (int)l[0], (int)l[1], red, green, blue, 255);
                else if (parameters.blankMoves)
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
        std::string str = "FPS: " +  Algorithms::typeToStr<int>(1000.0f * frame / worldtime.getTicks());
        sdl::auxiliary::Utilities::renderText(str, font, textColor, renderer, 25, 25);
        str = "(w+, s-): General: fill shorts = " + Algorithms::typeToStr<int>(parameters.fillShortBlanks);
        sdl::auxiliary::Utilities::renderText(str, font, textColor, renderer, 25, 50);
        str = "(e+, d-): General: Light threshold = " + Algorithms::typeToStr<int>(parameters.lightThreshold);
        sdl::auxiliary::Utilities::renderText(str, font, textColor, renderer, 25, 75);
        str = "(r+, f-): Line detection: Intersection threshold = " + Algorithms::typeToStr<int>(parameters.interThreshold);
        sdl::auxiliary::Utilities::renderText(str, font, textColor, renderer, 25, 100);
        str = "(t+, g-): Line detection: Min line length = " + Algorithms::typeToStr<int>(parameters.minLineLength);
        sdl::auxiliary::Utilities::renderText(str, font, textColor, renderer, 25, 125);
        str = "(z+, h-): Line detection: Max line gap = " + Algorithms::typeToStr<int>(parameters.maxLineGap);
        sdl::auxiliary::Utilities::renderText(str, font, textColor, renderer, 25, 150);
        str = "(u+, j-): Blurring: Blur size = " + Algorithms::typeToStr<int>(parameters.blursize);
        sdl::auxiliary::Utilities::renderText(str, font, textColor, renderer, 25, 175);
        str = "(i+, k-): Edge Detection: Upper threshold = " + Algorithms::typeToStr<int>(parameters.upperThreshold);
        sdl::auxiliary::Utilities::renderText(str, font, textColor, renderer, 25, 200);
        str = "(o+, l-): Edge Detection: Lower threshold = " + Algorithms::typeToStr<int>(parameters.lowerThreshold);
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
        if ((cap == true) && (fps.getTicks() < 1000 / parameters.maxFramesPerSecond) ) {
            SDL_Delay((1000 / parameters.maxFramesPerSecond) - fps.getTicks() );
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
