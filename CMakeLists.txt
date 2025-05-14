cmake_minimum_required(VERSION 3.16)

project(MyCraftCodes LANGUAGES C CXX)

#IF CMAKE_C/CXX_COMPILER is not set, and on macOS, set default compiler to clang
if(NOT CMAKE_C_COMPILER AND NOT CMAKE_CXX_COMPILER)
    if(APPLE)
        set(CMAKE_C_COMPILER /opt/homebrew/opt/llvm/bin/clang)
        set(CMAKE_CXX_COMPILER /opt/homebrew/opt/llvm/bin/clang++)
    else()
        set(CMAKE_C_COMPILER /usr/bin/gcc)
        set(CMAKE_CXX_COMPILER /usr/bin/g++)
    endif()
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)

set(CMAKE_VERBOSE_MAKEFILE ON)

option(CONFIG_BUILD_UNIT_TESTING "Build unit test" ON)

# 抑制 macOS 系统头文件 availability 的 warning
add_compile_options(-Wno-availability)

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

    if(APPLE)
        include_directories(/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1)
        include_directories(/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include)
    endif()

    # Find the googletest headers
    find_path(INCGTST gtest/gtest.h)
    # Find the googletest library
    find_library(LIBGTST gtest)
    find_library(LIBGTST_MAIN gtest_main)

    # Each *.cxx in current directory will be a standalone executable test case 
    #   to verify LIB_IOC use gtest as framework.
    function(UT_addEachCXX_asTestExe)
        file(GLOB UT_CXX_FILES *.cxx)
        foreach(UT_CXX_FILE ${UT_CXX_FILES})
            get_filename_component(UT_EXE_NAME ${UT_CXX_FILE} NAME_WE)
            add_executable(${UT_EXE_NAME} ${UT_CXX_FILE})
            target_include_directories(${UT_EXE_NAME} PRIVATE ${INCGTST})
            target_link_libraries(${UT_EXE_NAME} ${LIB_IOC} ${LIBGTST} ${LIBGTST_MAIN} pthread)
            add_test(NAME ${UT_EXE_NAME} COMMAND ${UT_EXE_NAME})
        endforeach()
    endfunction()

    target_compile_definitions(${LIB_IOC} PRIVATE CONFIG_BUILD_WITH_UNIT_TESTING)
    add_subdirectory(Test)
    #add_subdirectory(Source)

endif()
