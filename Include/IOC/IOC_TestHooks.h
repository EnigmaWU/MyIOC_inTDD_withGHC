/**
 * @file IOC_TestHooks.h
 * @brief Test-only hooks for fault injection and diagnostics.
 * 
 * ⚠️ WARNING: This header is FOR UNIT TESTING ONLY!
 *    Do NOT include this in production code.
 *    These APIs are NOT part of the stable public interface.
 * 
 * @note Only available when CONFIG_BUILD_WITH_UNIT_TESTING is defined.
 */

#ifndef __IOC_TEST_HOOKS_H__
#define __IOC_TEST_HOOKS_H__

#ifdef CONFIG_BUILD_WITH_UNIT_TESTING

#include "IOC_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inject allocation failure for the next N attempts (for fault injection tests).
 * 
 * @param count Number of upcoming allocation attempts to fail.
 *              Each failed allocation decrements this counter.
 *              When count reaches 0, normal allocation resumes.
 * 
 * @example
 *   IOC_test_setFailNextAlloc(1);  // Next calloc will fail
 *   IOC_onlineService(...);        // This will return POSIX_ENOMEM
 *   IOC_onlineService(...);        // This succeeds (counter expired)
 */
void IOC_test_setFailNextAlloc(int count);

/**
 * @brief Get current count of onlined services (for leak detection tests).
 * @return Number of active service objects currently registered.
 */
uint16_t IOC_getServiceCount(void);

/**
 * @brief Get current count of allocated link objects (for leak detection tests).
 * @return Number of active link objects currently allocated.
 */
uint16_t IOC_getLinkCount(void);

#ifdef __cplusplus
}
#endif

#endif  // CONFIG_BUILD_WITH_UNIT_TESTING

#endif  // __IOC_TEST_HOOKS_H__
