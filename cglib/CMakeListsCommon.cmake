include(${CGLIB_DIR}/CMakeExerciseConfig.cmake)

set(${CGLIB_DIR} ${CMAKE_CURRENT_LIST_DIR})

set(CGLIB_BINARY_DIR ${CGLIB_DIR}/build)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CGLIB_DIR}/ext/cmake_helpers)

include(ChangeWorkingDirectory)

SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Debug CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
ENDIF()

if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	add_definitions(-DNOMINMAX -D_CRT_SECURE_NO_WARNINGS)
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -Wall -Wundef -Wshadow -Werror=undef -Wfatal-errors ")
endif()


set(CMAKE_USE_RELATIVE_PATHS TRUE)

add_definitions(-DGLM_ENABLE_EXPERIMENTAL -D_USE_MATH_DEFINES -DCGLIB_DIR=\"${CGLIB_DIR}\")

#ogl
set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
if (OPENGL_FOUND)
  include_directories(${OPENGL_INCLUDE_DIR})
else (OPENGL_FOUND)
  MESSAGE(ERROR "OpenGL environment missing")
endif (OPENGL_FOUND)


set(GLFW_BINARY_DIR ${CGLIB_BINARY_DIR}/glfw)

set(GLFW_LIBRARY_DIR ${GLFW_BINARY_DIR}/src)

find_package(GLFW)
if(GLFW_FOUND)
	message(STATUS "GLFW FOUND")
	set(GLFW_LIBRARIES ${GLFW_LIBRARY})
	include_directories(${GLFW_INCLUDE_DIR})
	if(UNIX AND NOT APPLE)
		set(GLFW_LIBRARIES ${GLFW_LIBRARIES} Xxf86vm Xrandr Xi X11)
    else (APPLE)
		set(GLFW_LIBRARIES ${GLFW_LIBRARIES})
	endif()
else()
	message(STATUS "could not find system wide GFLW install, using our own version of GLFW instead")
	include_directories(${CGLIB_DIR}/lib/glfw/include)
	
	set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "fwrwere")
	set(GLFW_BUILD_TESTS OFF CACHE BOOL "aegegherh")
	set(GLFW_INSTALL OFF CACHE BOOL "fasdfa")

	add_subdirectory(${CGLIB_DIR}/lib/glfw ${GLFW_BINARY_DIR})
	set(GLFW_LIBRARIES glfw ${GLFW_LIBRARIES})
endif()

message(STATUS "GLFW_LIBRARIES: ${GLFW_LIBRARIES}")

find_package(GLEW)
if(GLEW_FOUND)
	message(STATUS "GLEW FOUND")
	include_directories(${GLEW_INCLUDE_DIR})
else()
	#make this global, so we can access it from subproject
	set(GLEW_BUILD_INFO OFF CACHE BOOL "")
	include_directories(${CGLIB_DIR}/lib/glew/include)
	add_subdirectory(${CGLIB_DIR}/lib/glew ${CGLIB_BINARY_DIR}/glew)
if (WIN32)
  set(GLEW_LIBRARY ${GLEW_LIBRARY}s)
else()
  set(GLEW_LIBRARY ${GLEW_LIBRARY}s)
endif()
endif()

add_subdirectory (${CGLIB_DIR} ${CGLIB_BINARY_DIR}/cglib) 

if (APPLE)
include_directories(
${CGLIB_DIR}/src
${CGLIB_DIR}/lib/glm
${CGLIB_DIR}/include
)
else()
include_directories(SYSTEM
	${CGLIB_DIR}/lib/glm
)

include_directories(
    ${CGLIB_DIR}/src
    ${CGLIB_DIR}/include
)
endif()

#if (WIN32)
#	set(GLFW_LIB ${CGLIB_DIR}/lib/glfw/lib/win)
#	if (CMAKE_CL_64)
#		set(GLFW_LIB ${GLFW_LIB}64)
#	else()
#		set(GLFW_LIB ${GLFW_LIB}32)
#	endif()
#else()
#	set(GLFW_LIB ${CGLIB_DIR}/lib/glfw/lib/x11)
#endif()
#

#if (WIN32)
#	set(GLEW_LIB ${CGLIB_DIR}/lib/glew/lib/glew)
#	if (CMAKE_CL_64)
#		message( FATAL_ERROR "On windows glew doesn't support 64 bit" )
#	else()
#		set(GLEW_LIB ${GLEW_LIB}32)
#	endif()
#	set(GLEW_LIB ${GLEW_LIB}s)
#else()
#	set(GLEW_LIB ${CGLIB_DIR}/lib/glew/lib/libGLEW.so.1.7.0)
#endif()

if (NOT APPLE)
    set(CGLIB_LINK_LIBRARIES_cg cglib_cg_${EXERCISE_NUMBER} ${GLEW_LIBRARY} ${GLFW_LIBRARIES} ${OPENGL_LIBRARIES})
    set(CGLIB_LINK_LIBRARIES_cg_solution cglib_cg_solution_${EXERCISE_NUMBER} ${GLEW_LIBRARY} ${GLFW_LIBRARIES} ${OPENGL_LIBRARIES})

    set(CGLIB_LINK_LIBRARIES_cg_test cglib_TestFramework cglib_cg_test_${EXERCISE_NUMBER} ${GLEW_LIBRARY} ${GLFW_LIBRARIES} ${OPENGL_LIBRARIES})
    set(CGLIB_LINK_LIBRARIES_cg_test_solution cglib_TestFramework cglib_cg_test_solution_${EXERCISE_NUMBER} ${GLEW_LIBRARY} ${GLFW_LIBRARIES} ${OPENGL_LIBRARIES})
else()
    set(CGLIB_LINK_LIBRARIES_cg cglib_cg_${EXERCISE_NUMBER} ${GLEW_LIBRARY} ${GLFW_LIBRARIES} ${OPENGL_LIBRARIES})
    set(CGLIB_LINK_LIBRARIES_cg_solution cglib_cg_solution_${EXERCISE_NUMBER} ${GLEW_LIBRARY} ${GLFW_LIBRARIES} ${OPENGL_LIBRARIES})

    set(CGLIB_LINK_LIBRARIES_cg_test cglib_TestFramework cglib_cg_test_${EXERCISE_NUMBER} ${GLEW_LIBRARY} ${GLFW_LIBRARIES} ${OPENGL_LIBRARIES})
    set(CGLIB_LINK_LIBRARIES_cg_test_solution cglib_TestFramework cglib_cg_test_solution_${EXERCISE_NUMBER} ${GLEW_LIBRARY} ${GLFW_LIBRARIES} ${OPENGL_LIBRARIES})
endif()

foreach (target ${targets})
	add_executable(${target} ${EXERCISE_SOURCE} ${EXERCISE_SOURCE_SHADERS})
	source_group( "Shaders" FILES ${EXERCISE_SOURCE_SHADERS} )
  
	set_target_properties(${target} PROPERTIES COMPILE_FLAGS ${CXXFLAGS_${target}})

	target_link_libraries(${target} ${CGLIB_LINK_LIBRARIES_${target}})

	set_target_properties(${target} PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}
		RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR})
endforeach(target)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT cg)


if(WIN32)
	configure_file(${CGLIB_DIR}/application.vcxproj.user-template ${PROJECT_NAME}.vcxproj.user @ONLY)
endif()
