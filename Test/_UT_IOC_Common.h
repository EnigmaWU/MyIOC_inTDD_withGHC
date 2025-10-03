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

///////////////////////////////////////////////////////////////////////////////////////////////////
// 🔍 KEY VERIFY POINT MACROS - For Enhanced Test Readability and Documentation
//
// Purpose: Mark critical verification points in tests following FreelyDrafts template
//          Template guideline: "each case SHOULD has less than 3 key ASSERT_XYZ verify points"
//
// Usage:
//   //@KeyVerifyPoint-1: Brief description of what this verifies
//   VERIFY_KEYPOINT_EQ(actual, expected, "Critical verification description");
//
// Benefits:
//   - Clear marking of KEY verification points (vs regular assertions)
//   - Better test documentation and readability
//   - Enhanced failure messages with context
//   - Easier to identify critical test failures
///////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief VERIFY_KEYPOINT_EQ - Mark a KEY equality verification point
 * @param actual The actual value being tested
 * @param expected The expected value
 * @param description Brief description of what this KEY point verifies
 *
 * Example:
 *   VERIFY_KEYPOINT_EQ(subState2, IOC_LinkSubStateCmdExecutorReady,
 *                      "Link2 must show Executor role (multi-role service architecture)");
 */
#define VERIFY_KEYPOINT_EQ(actual, expected, description)                                                            \
    do {                                                                                                             \
        printf("🔑 [KEY VERIFY POINT] %s\n", description);                                                           \
        ASSERT_EQ(expected, actual) << "⚠️ KEY VERIFICATION FAILED: " << description << "\n   Expected: " << expected \
                                    << "\n   Actual:   " << actual;                                                  \
    } while (0)

/**
 * @brief VERIFY_KEYPOINT_NE - Mark a KEY inequality verification point
 * @param val1 First value
 * @param val2 Second value
 * @param description Brief description of what this KEY point verifies
 *
 * Example:
 *   VERIFY_KEYPOINT_NE(subState1, subState2,
 *                      "Links must have independent single-role states");
 */
#define VERIFY_KEYPOINT_NE(val1, val2, description)                                                      \
    do {                                                                                                 \
        printf("🔑 [KEY VERIFY POINT] %s\n", description);                                               \
        ASSERT_NE(val1, val2) << "⚠️ KEY VERIFICATION FAILED: " << description << "\n   Value1: " << val1 \
                              << "\n   Value2: " << val2 << "\n   (Values must be different)";           \
    } while (0)

/**
 * @brief VERIFY_KEYPOINT_TRUE - Mark a KEY boolean true verification point
 * @param condition The condition that must be true
 * @param description Brief description of what this KEY point verifies
 *
 * Example:
 *   VERIFY_KEYPOINT_TRUE(linkID != IOC_ID_INVALID,
 *                        "Service must accept client connection");
 */
#define VERIFY_KEYPOINT_TRUE(condition, description)                                                                   \
    do {                                                                                                               \
        printf("🔑 [KEY VERIFY POINT] %s\n", description);                                                             \
        ASSERT_TRUE(condition) << "⚠️ KEY VERIFICATION FAILED: " << description << "\n   Condition evaluated to FALSE"; \
    } while (0)

/**
 * @brief VERIFY_KEYPOINT_FALSE - Mark a KEY boolean false verification point
 * @param condition The condition that must be false
 * @param description Brief description of what this KEY point verifies
 *
 * Example:
 *   VERIFY_KEYPOINT_FALSE(errorOccurred,
 *                         "No errors should occur during normal operation");
 */
#define VERIFY_KEYPOINT_FALSE(condition, description)                                                                  \
    do {                                                                                                               \
        printf("🔑 [KEY VERIFY POINT] %s\n", description);                                                             \
        ASSERT_FALSE(condition) << "⚠️ KEY VERIFICATION FAILED: " << description << "\n   Condition evaluated to TRUE"; \
    } while (0)

/**
 * @brief VERIFY_KEYPOINT_NULL - Mark a KEY null pointer verification point
 * @param ptr The pointer that must be NULL
 * @param description Brief description of what this KEY point verifies
 */
#define VERIFY_KEYPOINT_NULL(ptr, description)                                                                 \
    do {                                                                                                       \
        printf("🔑 [KEY VERIFY POINT] %s\n", description);                                                     \
        ASSERT_EQ(nullptr, ptr) << "⚠️ KEY VERIFICATION FAILED: " << description << "\n   Pointer is NOT NULL"; \
    } while (0)

/**
 * @brief VERIFY_KEYPOINT_NOT_NULL - Mark a KEY non-null pointer verification point
 * @param ptr The pointer that must NOT be NULL
 * @param description Brief description of what this KEY point verifies
 */
#define VERIFY_KEYPOINT_NOT_NULL(ptr, description)                                                         \
    do {                                                                                                   \
        printf("🔑 [KEY VERIFY POINT] %s\n", description);                                                 \
        ASSERT_NE(nullptr, ptr) << "⚠️ KEY VERIFICATION FAILED: " << description << "\n   Pointer is NULL"; \
    } while (0)

/**
 * @brief VERIFY_KEYPOINT_LT - Mark a KEY less-than verification point
 * @param val1 First value (should be less than val2)
 * @param val2 Second value
 * @param description Brief description of what this KEY point verifies
 */
#define VERIFY_KEYPOINT_LT(val1, val2, description)                                                      \
    do {                                                                                                 \
        printf("🔑 [KEY VERIFY POINT] %s\n", description);                                               \
        ASSERT_LT(val1, val2) << "⚠️ KEY VERIFICATION FAILED: " << description << "\n   Value1: " << val1 \
                              << "\n   Value2: " << val2 << "\n   (Value1 must be < Value2)";            \
    } while (0)

/**
 * @brief VERIFY_KEYPOINT_LE - Mark a KEY less-than-or-equal verification point
 * @param val1 First value (should be <= val2)
 * @param val2 Second value
 * @param description Brief description of what this KEY point verifies
 */
#define VERIFY_KEYPOINT_LE(val1, val2, description)                                                      \
    do {                                                                                                 \
        printf("🔑 [KEY VERIFY POINT] %s\n", description);                                               \
        ASSERT_LE(val1, val2) << "⚠️ KEY VERIFICATION FAILED: " << description << "\n   Value1: " << val1 \
                              << "\n   Value2: " << val2 << "\n   (Value1 must be <= Value2)";           \
    } while (0)

/**
 * @brief VERIFY_KEYPOINT_GT - Mark a KEY greater-than verification point
 * @param val1 First value (should be greater than val2)
 * @param val2 Second value
 * @param description Brief description of what this KEY point verifies
 */
#define VERIFY_KEYPOINT_GT(val1, val2, description)                                                      \
    do {                                                                                                 \
        printf("🔑 [KEY VERIFY POINT] %s\n", description);                                               \
        ASSERT_GT(val1, val2) << "⚠️ KEY VERIFICATION FAILED: " << description << "\n   Value1: " << val1 \
                              << "\n   Value2: " << val2 << "\n   (Value1 must be > Value2)";            \
    } while (0)

/**
 * @brief VERIFY_KEYPOINT_GE - Mark a KEY greater-than-or-equal verification point
 * @param val1 First value (should be >= val2)
 * @param val2 Second value
 * @param description Brief description of what this KEY point verifies
 */
#define VERIFY_KEYPOINT_GE(val1, val2, description)                                                      \
    do {                                                                                                 \
        printf("🔑 [KEY VERIFY POINT] %s\n", description);                                               \
        ASSERT_GE(val1, val2) << "⚠️ KEY VERIFICATION FAILED: " << description << "\n   Value1: " << val1 \
                              << "\n   Value2: " << val2 << "\n   (Value1 must be >= Value2)";           \
    } while (0)

/**
 * @brief VERIFY_KEYPOINT_STREQ - Mark a KEY string equality verification point
 * @param str1 First string
 * @param str2 Second string (expected)
 * @param description Brief description of what this KEY point verifies
 */
#define VERIFY_KEYPOINT_STREQ(str1, str2, description)                                                          \
    do {                                                                                                        \
        printf("🔑 [KEY VERIFY POINT] %s\n", description);                                                      \
        ASSERT_STREQ(str1, str2) << "⚠️ KEY VERIFICATION FAILED: " << description << "\n   Expected: \"" << str2 \
                                 << "\"\n   Actual:   \"" << str1 << "\"";                                      \
    } while (0)

/**
 * @brief VERIFY_KEYPOINT_STRNE - Mark a KEY string inequality verification point
 * @param str1 First string
 * @param str2 Second string
 * @param description Brief description of what this KEY point verifies
 */
#define VERIFY_KEYPOINT_STRNE(str1, str2, description)                                                         \
    do {                                                                                                       \
        printf("🔑 [KEY VERIFY POINT] %s\n", description);                                                     \
        ASSERT_STRNE(str1, str2) << "⚠️ KEY VERIFICATION FAILED: " << description << "\n   String1: \"" << str1 \
                                 << "\"\n   String2: \"" << str2 << "\""                                       \
                                 << "\n   (Strings must be different)";                                        \
    } while (0)

///////////////////////////////////////////////////////////////////////////////////////////////////
// End of KEY VERIFY POINT MACROS
///////////////////////////////////////////////////////////////////////////////////////////////////
