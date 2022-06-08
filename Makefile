########################################################################
#                          -*- Makefile -*-                            #
########################################################################

COMPILER = g++

########################################################################
## Flags
FLAGS   = -g -std=c++17 -D LUMAX_OUTPUT
#FLAGS   = -g -std=c++17
LDFLAGS = 
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
## Paths

WORKINGDIR = $(shell pwd)
PARENTDIR  = $(WORKINGDIR)/..

########################################################################
## search for the files and set paths

vpath %.cpp $(WORKINGDIR) $(WORKINGDIR)/GameLibrary $(WORKINGDIR)/unittests
vpath %.m $(WORKINGDIR)
vpath %.a $(WORKINGDIR)/build
vpath %.o $(WORKINGDIR)/build

########################################################################
## Includes
CXX  = $(COMPILER) $(FLAGS) $(OPT) $(WARN) $(DEBUG) $(PREPRO) -I$(WORKINGDIR)
INCLUDE = $(wildcard *.h $(UINCLUDE)/*.h)

########################################################################
## SDL
CXX += $(shell sdl2-config --cflags)
LDFLAGS += $(shell sdl2-config --static-libs) -lSDL2_gfx -lSDL2_image -lSDL2_ttf

########################################################################
## OpenCV
CXX += $(shell pkg-config --cflags opencv4)
LDFLAGS += $(shell pkg-config --libs opencv4)

########################################################################

%.a: %.cpp $(INCLUDE)
	$(CXX) -c -o build/$@ $<

%.a: %.m $(INCLUDE)
	$(CXX) -c -o build/$@ $<

# Libraries
LIB = -L./GameLibrary -L./unittests -llumax

# Frameworks
# -framework SDL_gfx 
FRM = -framework Cocoa

########################################################################
## Linker files

### USER Files ###
USER = Main.a Renderer.a Algorithms.a
### TODO: not necessary, remove!
USER += Collision.a Object.a Solver.a UnitTests.a


########################################################################
## Rules
## type make -j4 [rule] to speed up the compilation

BUILD = $(USER)

laser-display: $(BUILD)
	  $(CXX) $(patsubst %,build/%,$(BUILD)) $(LDFLAGS) $(LIB) $(FRM) -o $@

clean:
	rm -f build/*.a laser-display

do:
	make && cp GameLibrary/liblumax.so . && ./laser-display

########################################################################
#                       -*- End of Makefile -*-                        #
########################################################################
