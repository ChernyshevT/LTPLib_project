cmake_minimum_required(VERSION 3.15)  # Ensure CMake is recent enough
set(CMAKE_SHARED_LIBRARY_PREFIX "")

################################################################################
# Download pybind11 from GitHub
FetchContent_Declare(
	pybind11
	GIT_REPOSITORY https://github.com/pybind/pybind11.git
	GIT_TAG        master
	GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(pybind11)

################################################################################
# Download fmt from GitHub
FetchContent_Declare(
	fmt
	GIT_REPOSITORY https://github.com/fmtlib/fmt.git
	GIT_TAG        master
	GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(fmt)

################################################################################
#~ file(GLOB SOURCES *.cpp)
add_library(_ltplib SHARED 
	_ltplib.cxx
	def_grid_cfg.cxx
	def_grid.cxx
	def_pstore.cxx
	def_vcache.cxx
	def_csection_set_cfg.cxx
	def_csection_set.cxx
	def_err_codes.cxx
	def_mcsim_funcs.cxx
	#def_order_funcs.cxx
	def_ppost_funcs.cxx
	def_ppush_funcs.cxx
	def_remap_funcs.cxx
)

set_target_properties(_ltplib PROPERTIES LIBRARY_OUTPUT_NAME
	"_ltplib"
)

# Ensure _ltplib is compiled with Position Independent Code (PIC)
set_target_properties(_ltplib PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Link pybind11 and fmt directly
target_link_libraries(_ltplib PRIVATE pybind11::module fmt::fmt-header-only)

# Include pybind11 and fmt headers
target_include_directories(_ltplib PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/..
	${pybind11_INCLUDE_DIRS}
	${fmt_SOURCE_DIR}/include
)

# Set compiler flags for _ltplib
target_compile_options(_ltplib PRIVATE
	-Wall
	-Wpedantic
	-fvisibility=hidden
)
if(DEFINED OPTS)
	target_compile_options(_ltplib PRIVATE ${OPTS})
endif()
