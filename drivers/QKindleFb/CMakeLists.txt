ENABLE_QT()

INCLUDE_DIRECTORIES(.)

SET(SCREEN_PLUGIN_HDRS
  qkindlecursor.h
  qkindlefb.h)

SET(SCREEN_PLUGIN_SRCS
  ${SCREEN_PLUGIN_HDRS}
  qkindlecursor.cpp
  qkindlefb.cpp
  qkindlefbplugin.cpp)

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)
add_definitions(-DQT_SHARED)
add_definitions(-DQT_NO_DEBUG)

add_library(QKindleFb SHARED ${SCREEN_PLUGIN_SRCS})
target_link_libraries(QKindleFb device ${QT_LIBRARIES} ${ADD_LIB})

add_library(KindleFb ${SCREEN_PLUGIN_SRCS})
target_link_libraries(KindleFb device ${QT_LIBRARIES} ${ADD_LIB})

# Install the plugin to QT_PLUGINS_OUTPUT_PATH
SET_TARGET_PROPERTIES(QKindleFb PROPERTIES  RUNTIME_OUTPUT_DIRECTORY ${QT_PLUGINS_OUTPUT_PATH})
