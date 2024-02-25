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
        target_link_libraries(${UT_EXE_NAME} ${LIB_IOC} ${LIBGTST} ${LIBGTST_MAIN})
        add_test(NAME ${UT_EXE_NAME} COMMAND ${UT_EXE_NAME})
    endforeach()
endfunction()

UT_addEachCXX_asTestExe()
