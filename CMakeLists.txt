project(pvr.enigma2.stb)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(kodi REQUIRED)
find_package(kodiplatform REQUIRED)
find_package(p8-platform REQUIRED)
find_package(TinyXML REQUIRED)

# Grab the addon version info from the addon.xml file
FILE(READ ${PROJECT_NAME}/addon.xml.in E2STB_ADDONXML)
STRING(REGEX MATCH "<addon[^>]*version.?=.?.[0-9\\.]+" E2STB_VERSION_STRING ${E2STB_ADDONXML})
STRING(REGEX REPLACE ".*version=.([0-9\\.]+).*" "\\1" E2STB_VERSION ${E2STB_VERSION_STRING})

# Generate an E2STBVersion.h header with the version information
configure_file(src/E2STBVersion.h.in E2STBVersion.h)

include_directories(${kodiplatform_INCLUDE_DIRS}
                    ${p8-platform_INCLUDE_DIRS}
                    ${TINYXML_INCLUDE_DIR}
                    ${KODI_INCLUDE_DIR}
                    ${PROJECT_BINARY_DIR})

set(E2STB_SOURCES src/client.cpp
                  src/compat.h
                  src/E2STBBackendData.cpp
                  src/E2STBConnection.cpp
                  src/E2STBData.cpp
                  src/E2STBRecordings.cpp
                  src/E2STBTimeshift.cpp
                  src/E2STBUtils.cpp
                  src/E2STBVersion.h
                  src/E2STBXMLUtils.cpp)

set(DEPLIBS ${kodiplatform_LIBRARIES}
            ${p8-platform_LIBRARIES}
            ${TINYXML_LIBRARIES})

build_addon(pvr.enigma2.stb E2STB DEPLIBS)

include(CPack)
