################################################################################
#Yume Engine MIT License (MIT)

# Copyright (c) 2015 arkenthera
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# File : CMakeLists.txt
# Date : 8.27.2015
# Comments :
################################################################################
set(LIB_NAME "Cef3D")

file(GLOB CEF3D_FILES *.cpp *.h)

source_group(Cef3D FILES ${CEF3D_FILES})

add_definitions(-DCEF3D_EXPORTS)

include_directories(${CMAKE_SOURCE_DIRECTORY}/Cef)

if(${CEF3D_SHARED_LIBS})
    ADD_LIBRARY(${LIB_NAME} SHARED ${CEF3D_FILES})
else()
    ADD_LIBRARY(${LIB_NAME} ${CEF3D_FILES})
endif()

target_link_libraries(${LIB_NAME} libcef_dll_wrapper ${CEF_STANDARD_LIBS})
