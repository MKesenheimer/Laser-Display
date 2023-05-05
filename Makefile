########################################################################
#                          -*- Makefile -*-                            #
########################################################################
# Force rebuild on these rules
.PHONY: all libs clean clean-libs
.DEFAULT_GOAL := laser-display

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
SYSTEMINC  = /opt/local/include
LIBS       = $(WORKINGDIR)/libs
EIGEN      = $(LIBS)/eigen
GTEST      = $(LIBS)/googletest
LIBLUMAX   = $(LIBS)/lumax/libs
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
CXX  = $(COMPILER) $(FLAGS) $(OPT) $(WARN) $(DEBUG) $(PREPRO) -I$(SYSTEMINC) -I$(WORKINGDIR) -I$(LIBS) -I$(EIGEN) -I$(GTEST)/googletest/include
INCLUDE = $(wildcard *.h $(UINCLUDE)/*.h)

########################################################################
## libraries
### SDL
CXX += $(shell sdl2-config --cflags)
LDFLAGS += $(shell sdl2-config --libs) -lSDL2_gfx -lSDL2_image -lSDL2_ttf

### OpenCV
CXX += $(shell pkg-config --cflags opencv4)
LDFLAGS += $(shell pkg-config --libs opencv4)

### Lumax
# TODO: make this OS dependent
LDFLAGS += -L$(LIBLUMAX) -llumax_darwin 

### libconfig
LDFLAGS += -lconfig++

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
BUILD = main.a renderer.a algorithms.a sort.a collision.a object.a solver.a 

## BUILD files for unittests
BUILD_U = renderer.a algorithms.a sort.a collision.a object.a solver.a
BUILD_U += unitTests.a gtest.a


########################################################################
## Rules
## type make -j4 [rule] to speed up the compilation
all: libs laser-display gtest

laser-display: $(BUILD)
	$(CXX) $(patsubst %,build/%,$(BUILD)) $(LDFLAGS) $(FRM) -o $@

gtest: $(BUILD_U)
	$(CXX) $(patsubst %,build/%,$(BUILD_U)) $(LDFLAGS_U) -o $@

libs:
	cd $(GTEST) && mkdir -p $(GTEST)/build && cd $(GTEST)/build && \
	cmake -DBUILD_SHARED_LIBS=ON .. && make
	cp $(LIBGTEST)/* $(LIBS)
	cd $(LIBLUMAX) && ./make.sh
	cp $(LIBLUMAX)/*.dylib $(LIBS)

clean-all: clean clean-libs

clean:
	rm -f build/*.a laser-display gtest

clean-libs:
	cd $(GTEST) && rm -rf build 
	cd $(LIBLUMAX) && rm -f *.dylib *.so
	rm -f $(LIBS)/*.dylib $(LIBS)/*.so

do:
	make && ./laser-display

########################################################################
#                       -*- End of Makefile -*-                        #
########################################################################
