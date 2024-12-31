cmake_minimum_required(VERSION 3.15)
set(CMAKE_SHARED_LIBRARY_PREFIX "")

#~ set(PY_FILES
	#~ ${CMAKE_CURRENT_SOURCE_DIR}/run_ppost_fns.template.py
	#~ ${CMAKE_CURRENT_SOURCE_DIR}/run_ppush_fns.template.py
	#~ ${CMAKE_CURRENT_SOURCE_DIR}/run_remap_fns.template.py
#~ )
#~ set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
#~ file(MAKE_DIRECTORY ${OUTPUT_DIR})

#~ # Add custom command to generate code for each input file
#~ foreach(PY_FILE ${PY_FILES})
    #~ get_filename_component(FILE_NAME ${PY_FILE} NAME_WE)
    #~ set(OUTPUT_FILE ${OUTPUT_DIR}/generated_${FILE_NAME}.cpp)

    #~ add_custom_command(
        #~ OUTPUT ${OUTPUT_FILE}
        #~ COMMAND ${CMAKE_COMMAND} -E env python3 ${CMAKE_CURRENT_SOURCE_DIR}/gen_fns.py ${PY_FILES} ${OUTPUT_DIR}
        #~ DEPENDS ${PY_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/gen_fns.py
        #~ COMMENT "Generating code for ${PY_FILE} using Python"
    #~ )

    #~ list(APPEND GENERATED_FILES ${OUTPUT_FILE})
#~ endforeach()

################################################################################
add_library(_default SHARED
	_default.cxx
	run_ppush.cxx
	run_ppost.cxx
	run_mcsim.cxx
	run_remap.cxx
	run_order.cxx
)

set_target_properties(_default PROPERTIES LIBRARY_OUTPUT_NAME
	"_default"
)

# Force static linking of the C++ standard library for GCC/Clang (Linux/macOS)
if(UNIX AND NOT APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++")
elseif(MSVC) # For MSVC (Windows), use /MT for static runtime library linking
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MT")
endif()

# Find OpenMP (this will automatically set up the correct compiler flags)
find_package(OpenMP REQUIRED)
target_link_libraries(_default PRIVATE OpenMP::OpenMP_CXX)

target_include_directories(_default PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_compile_options(_default PRIVATE
	-Wall
	-Wpedantic
	-Werror
	-fno-sanitize=all
	-O3
)
if(DEFINED OPTS)
	target_compile_options(_default PRIVATE ${OPTS})
endif()
