// This is a common header file for all UTs of IOC from API caller's perspective,
//   which means the UTs focus on IOC's behavior from user viewpoint, but its internal implementation.

//===>RefMore: TEMPLATE OF UT CASE in UT_FreelyDrafts.cxx

#include <fcntl.h>

#include <cstddef>
#include <thread>

#include "../Source/_IOC.h"
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define GTEST_HAS_PTHREAD 1
#include <gtest/gtest.h>
