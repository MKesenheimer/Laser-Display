########################################################################
#                          -*- Makefile -*-                            #
########################################################################
COMPILER = g++

########################################################################
## Flags
FLAGS   = -g -std=c++17 -D LUMAX_OUTPUT
#FLAGS   = -g -std=c++17
## find shared libraries during runtime: set rpath:
LDFLAGS = -rpath @executable_path/libs
PREPRO  =
##verbose level 1
#DEBUG   = -D DEBUGV1
##verbose level 2
#DEBUG  += -D DEBUGV2
##verbose level 3
#DEBUG  += -D DEBUGV3
OPT     = -O2
WARN    = -Wall -Wno-missing-braces

### generate directory obj, if not yet existing
$(shell mkdir -p build)

########################################################################
## Paths, modify if necessary
WORKINGDIR = $(shell pwd)
PARENTDIR  = $(WORKINGDIR)/..
LIBS       = $(WORKINGDIR)/libs
EIGEN      = $(LIBS)/eigen
GTEST      = $(LIBS)/googletest
LIBLUMAX   = $(LIBS)/lumax
LIBGTEST   = $(LIBS)/googletest/build/lib

########################################################################
### DO NOT MODIFY BELOW THIS LINE ######################################
########################################################################

########################################################################
## search for the files and set paths
vpath %.cpp $(WORKINGDIR) $(WORKINGDIR)/GameLibrary $(WORKINGDIR)/unittests
vpath %.m $(WORKINGDIR)
vpath %.a $(WORKINGDIR)/build
vpath %.o $(WORKINGDIR)/build

########################################################################
## Includes
CXX  = $(COMPILER) $(FLAGS) $(OPT) $(WARN) $(DEBUG) $(PREPRO) -I$(WORKINGDIR) -I$(LIBS) -I$(EIGEN) -I$(GTEST)/googletest/include
INCLUDE = $(wildcard *.h $(UINCLUDE)/*.h)

########################################################################
## libraries
### SDL
CXX += $(shell sdl2-config --cflags)
LDFLAGS += $(shell sdl2-config --static-libs) -lSDL2_gfx -lSDL2_image -lSDL2_ttf

### OpenCV
CXX += $(shell pkg-config --cflags opencv4)
LDFLAGS += $(shell pkg-config --libs opencv4)

### Lumax
LDFLAGS += -L$(LIBLUMAX) -llumax 

# Frameworks
# -framework SDL_gfx 
#FRM = -framework Cocoa

### Unittests
LDFLAGS_U = $(LDFLAGS)
LDFLAGS_U += -L$(LIBGTEST) -lgtest

########################################################################
## Build rules
%.a: %.cpp $(INCLUDE)
	$(CXX) -c -o build/$@ $<

%.a: %.m $(INCLUDE)
	$(CXX) -c -o build/$@ $<

########################################################################
## BUILD Files
BUILD = Main.a Renderer.a Algorithms.a Sort.a Collision.a Object.a Solver.a 

## BUILD files for unittests
BUILD_U = Renderer.a Algorithms.a Sort.a Collision.a Object.a Solver.a
BUILD_U += UnitTests.a Main.a


########################################################################
## Rules
## type make -j4 [rule] to speed up the compilation
laser-display: $(BUILD)
	  $(CXX) $(patsubst %,build/%,$(BUILD)) $(LDFLAGS) $(FRM) -o $@

gtest: $(BUILD_U)
	$(CXX) $(patsubst %,build/%,$(BUILD_U)) $(LDFLAGS_U) -o $@

# TODO: rule to build google unittest library
# TODO: rule to copy the built shared libraries to ./libs

clean:
	rm -f build/*.a laser-display gtest

do:
	make && ./laser-display

########################################################################
#                       -*- End of Makefile -*-                        #
########################################################################
