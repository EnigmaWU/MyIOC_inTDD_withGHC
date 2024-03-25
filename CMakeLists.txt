cmake_minimum_required(VERSION 3.20)

project(MyCraftCodes LANGUAGES C CXX)

set(CMAKE_C_COMPILER /opt/homebrew/opt/llvm/bin/clang)
set(CMAKE_CXX_COMPILER /opt/homebrew/opt/llvm/bin/clang++)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)

set(CMAKE_VERBOSE_MAKEFILE ON)

option(CONFIG_BUILD_UNIT_TESTING "Build unit test" ON)

if(CMAKE_BUILD_TYPE STREQUAL "DiagASAN")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fsanitize=address -fno-omit-frame-pointer -Wno-deprecated-declarations")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
elseif(CMAKE_BUILD_TYPE STREQUAL "DiagTSAN")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fsanitize=thread -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fsanitize=thread -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread")
elseif(CMAKE_BUILD_TYPE STREQUAL "DiagUBSAN")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fsanitize=undefined -fno-omit-frame-pointer")
    #TODO(@W): CXX_FLAGS
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined")
elseif(CMAKE_BUILD_TYPE STREQUAL "DiagMSAN")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fsanitize=memory -fno-omit-frame-pointer")
    #TODO(@W): CXX_FLAGS
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=memory")
elseif(CMAKE_BUILD_TYPE STREQUAL "DiagLSAN")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fsanitize=leak -fno-omit-frame-pointer")
    #TODO(@W): CXX_FLAGS
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=leak")
endif()


###############################################################################
###===> libMyIOC
set(LIB_IOC MyIOC)
file(GLOB LIB_IOC_SRCFILES Source/*.c)
add_library(${LIB_IOC} STATIC ${LIB_IOC_SRCFILES})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Include)

if(CONFIG_BUILD_UNIT_TESTING)
    enable_testing()

    target_compile_definitions(${LIB_IOC} PRIVATE CONFIG_BUILD_WITH_UNIT_TESTING)
    add_subdirectory(Test)

endif()
