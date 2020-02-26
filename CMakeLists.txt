cmake_minimum_required(VERSION 3.0)
PROJECT(indi_hic_agent C CXX)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")

include(GNUInstallDirs)

set (HICAGENT_VERSION_MAJOR 0)
set (HICAGENT_VERSION_MINOR 1)

find_package(CFITSIO REQUIRED)
find_package(INDI COMPONENTS client driver REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)
find_package(Nova REQUIRED)
find_package(GSL REQUIRED)
find_package(OpenCV REQUIRED )
find_package(CImg REQUIRED )


configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/indi_hic_agent.xml.cmake ${CMAKE_CURRENT_BINARY_DIR}/indi_hic_agent.xml )

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${INDI_INCLUDE_DIR})
include_directories(${CFITSIO_INCLUDE_DIR})
include_directories(${GSL_INCLUDE_DIRS})
include_directories(${CIMG_INCLUDE_DIRS})

include(CMakeCommon)

############# hic focus driver ###############
set(hic_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/indi_hicfocus_driver.cpp
   )

add_library(image ${CMAKE_CURRENT_SOURCE_DIR}/image.cpp)
add_library(polynomialfit ${CMAKE_CURRENT_SOURCE_DIR}/polynomialfit.cpp)
add_library(indi_hicfocus_client ${CMAKE_CURRENT_SOURCE_DIR}/indi_hicfocus_client.cpp)

add_executable(indi_hicfocus_agent ${hic_SRCS})
target_link_libraries(indi_hicfocus_agent indidriver indiclient image indi_hicfocus_client polynomialfit ${INDI_LIBRARIES} ${CFITSIO_LIBRARIES} ${ZLIB_LIBRARY}  ${NOVA_LIBRARIES} ${GSL_LIBRARIES} ${ZLIB_LIBRARY} ${CIMG_LIBRARIES} ${OpenCV_LIBS} z -pthread)

install(TARGETS indi_hicfocus_agent RUNTIME DESTINATION bin)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/indi_hicfocus_agent.xml DESTINATION ${INDI_DATA_DIR})

