# Make sure you already have cmake 2.6 installed. For cross compiling.
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
# Additional cmake modules
SET(CMAKE_FIND_STATIC FIRST) 
SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules")

# Boox project.
Project(Duokan CXX C)
ENABLE_TESTING()


# Global definitions
SET(ADD_LIB "")
IF(UNIX OR BUILD_FOR_ARM)
    ADD_DEFINITIONS(-DSHARE_ROOT="/usr/share")
    ADD_DEFINITIONS(-DUNIX_PLATFORM)
ELSE (UNIX OR BUILD_FOR_ARM)
    ADD_DEFINITIONS(-DSHARE_ROOT="")
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
ENDIF (UNIX OR BUILD_FOR_ARM)

ADD_DEFINITIONS(-DUSE_BUILT_IN_DRIVER)

#CMAKE_BUILD_TYPE could be Debug/Release/RelWithDebINfo/MinSizeRel
IF(UNIX)
SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")
ENDIF (UNIX)

# Check to use arm toolchain or not
IF(BUILD_FOR_ARM)
    INCLUDE(arm_toolchain)
    USE_ARM_TOOLCHAIN()
    #include(FindTsLib)
 #include(FindDBus)
ELSE(BUILD_FOR_ARM)
link_directories("/usr/lib")
ENDIF(BUILD_FOR_ARM)

# Find Thrift & Boost
include(FindThrift)
include(enable_boost)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${THRIFT_INCLUDE_DIR})
LINK_DIRECTORIES(${Boost_LIBRARY_DIR})

# Project include directories.
INCLUDE_DIRECTORIES(BEFORE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/libs
    ${CMAKE_SOURCE_DIR}/drivers
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_SOURCE_DIR}/libs/DkReader/inc
	${CMAKE_SOURCE_DIR}/framework/inc
)

# Find thread library for the current platform
include(enable_qt)
#include(misc)
include(strict_warning)
include(tc)

LINK_DIRECTORIES(${QT_LIBRARY_DIR})

# Output directories.
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/libs)
SET(TEST_OUTPUT_PATH ${PROJECT_BINARY_DIR}/unittests)
SET(QT_PLUGINS_OUTPUT_PATH ${PROJECT_BINARY_DIR}/plugins)

add_subdirectory(system_manager)
add_subdirectory(web_browser)
add_subdirectory(evernote_explorer)
add_subdirectory(third_party)
add_subdirectory(libs)
#IF(BUILD_FOR_ARM)
add_subdirectory(drivers/QKindleFb)
add_subdirectory(drivers/transformation)
#ENDIF(BUILD_FOR_ARM)
add_subdirectory(drivers/KindleTS)
add_subdirectory(drivers/KindleFiveWay)
add_subdirectory(drivers/KindleKeyboard)
add_subdirectory(ui)
add_subdirectory(framework)
add_subdirectory(common)
IF(UNIX OR BUILD_FOR_ARM)
add_subdirectory(ime)
ENDIF(UNIX OR BUILD_FOR_ARM)

