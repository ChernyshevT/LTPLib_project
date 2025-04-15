cmake_minimum_required(VERSION 3.15)
project(_ltplib)
project(_default)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if(NOT WIN32)
	set(CMAKE_CXX_VISIBILITY_PRESET hidden)
	set(CMAKE_CXX_VISIBILITY_INLINES_HIDDEN YES)
endif()

include(FetchContent)
find_package(Python3 COMPONENTS Interpreter Development)

add_subdirectory(src/_ltplib)
add_subdirectory(src/_default)

install(TARGETS _ltplib _default
	LIBRARY DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/_ltplib
)

# cd build
# cmake ..
# cmake --build . --target install
# cmake --build . --target clean && rm CMakeCache.txt
