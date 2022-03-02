cmake_minimum_required(VERSION 2.9)

project(shader-toy-webgpu)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-nonportable-include-path -fno-exceptions -fno-rtti")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3 -D_DEBUG=1 -Wno-unused -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g0 -DNDEBUG=1 -flto -O3")

file(GLOB sources src/*.cpp)
file(GLOB_RECURSE headers src/*.h)

if (EMSCRIPTEN)
	file(GLOB_RECURSE platform_sources src/ems/*.cpp)
	set(CMAKE_EXECUTABLE_SUFFIX ".js")
	
	# TODO: all the debug flags are link only

	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}  -s DISABLE_EXCEPTION_CATCHING=1 ")
	# --closure 1

	set(CMAKE_EXE_LINKER_FLAGS "-s ASSERTIONS=0  -s USE_WEBGPU=1 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1  -s EXIT_RUNTIME=1  -s FILESYSTEM=1 -s USE_SDL=2  --use-preload-plugins  -s USE_LIBJPEG=1 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='[\"bmp\",\"png\",\"jpg\"]'")

	# Linker flags to optimize for smallest output code size
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --no-heap-copy --preload-file happytree.jpg")
elseif (WIN32)
	file(GLOB_RECURSE platform_sources src/win/*)
elseif (APPLE)
	file(GLOB_RECURSE platform_sources src/mac/*)
endif()

add_executable(index main.cpp ${sources} ${platform_sources} ${headers})

target_include_directories(index PRIVATE "${CMAKE_CURRENT_LIST_DIR}/inc")
target_include_directories(index PRIVATE "${CMAKE_CURRENT_LIST_DIR}/lib/glm")