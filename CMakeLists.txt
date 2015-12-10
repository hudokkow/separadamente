project(pvr.enigma2.stb)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(kodi REQUIRED)
find_package(kodiplatform REQUIRED)
find_package(platform REQUIRED)
find_package(TinyXML REQUIRED)

include_directories(${kodiplatform_INCLUDE_DIRS}
                    ${platform_INCLUDE_DIRS}
                    ${TINYXML_INCLUDE_DIR}
                    ${KODI_INCLUDE_DIR})

set(ENIGMA2_SOURCES src/client.cpp
                    src/compat.h
                    src/E2STBConnection.cpp
                    src/E2STBData.cpp
                    src/E2STBTimeshift.cpp
                    src/E2STBUtils.cpp
                    src/E2STBXMLUtils.cpp)

set(DEPLIBS ${kodiplatform_LIBRARIES}
            ${platform_LIBRARIES}
            ${TINYXML_LIBRARIES})

build_addon(pvr.enigma2.stb ENIGMA2 DEPLIBS)

include(CPack)
