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

add_subdirectory(_ltplib)
add_subdirectory(_default)

#https://stackoverflow.com/questions/6241922/how-to-properly-set-cmake-install-prefix-from-the-command-line
#https://cmake.org/cmake/help/latest/command/install.html
#https://www.baeldung.com/linux/cmake-cmake_install_prefix
#https://runebook.dev/en/articles/cmake/variable/cmake_install_prefix
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/../examples
		CACHE PATH "install prefix" FORCE
	)
endif()
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")


install(TARGETS _ltplib _default
	LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}
)

# cmake -S src -B build && cmake --build build --target install
