CC = emcc
CXX = em++
INC_DIR = inc
SRC_DIR = src
WEB_DIR = out
EXE=out/index.js
EMS_DIR = $(SRC_DIR)/ems
SOURCES = main.cpp $(SRC_DIR)/Camera.cpp $(SRC_DIR)/CameraController.cpp $(EMS_DIR)/glue.cpp $(EMS_DIR)/webgpu.cpp $(EMS_DIR)/window.cpp
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))

##---------------------------------------------------------------------
## EMSCRIPTEN OPTIONS
##---------------------------------------------------------------------

EMS += -s USE_WEBGPU=1 -s WASM=1
EMS += -s ALLOW_MEMORY_GROWTH=1
EMS += -s DISABLE_EXCEPTION_CATCHING=1 -s EXIT_RUNTIME=1
EMS += -s ASSERTIONS=0 -s FILESYSTEM=1 -s --use-preload-plugins
EMS += -s USE_SDL=2 -s USE_LIBJPEG=1 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='["bmp","png","jpg"]'
LDFLAGS = --no-heap-copy --preload-file happytree.jpg

##---------------------------------------------------------------------
## FINAL BUILD FLAGS
##---------------------------------------------------------------------


CPPFLAGS = -I$(INC_DIR) -Ilib/glm 
CPPFLAGS += -Wall -Wformat -Os
CPPFLAGS += $(EMS)
LIBS = $(EMS)

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

%.o:$(SRC_DIR)/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS)  -o $@ -c $<

%.o:$(EMS_DIR)/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@  -c $<

all: $(EXE)
	@echo Build complete for $(EXE)

$(WEB_DIR):
	mkdir $@
	
serve: all
	python -m SimpleHTTPServer

$(EXE): $(OBJS) $(WEB_DIR)
	$(CXX) -o $@ $(OBJS) $(LIBS) $(LDFLAGS)

clean:
	rm -f $(EXE) $(OBJS) $(WEB_DIR)/*.js $(WEB_DIR)/*.wasm $(WEB_DIR)/*.wasm.pre
	find . -name ‘.DS_Store’ -type f -delete