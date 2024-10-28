/**
 * @file _IOC_Logging.h
 * @brief This is internal header file used to define logging interface for internal use only.
 */

#include <assert.h>
#include <stdio.h>

#ifndef __INTER_OBJECT_COMMUNICATION_LOGGING_H__
#define __INTER_OBJECT_COMMUNICATION_LOGGING_H__
#ifdef __cplusplus
extern "C" {
#endif

#define _IOC_LogDebug(fmt, ...)  // printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#define _IOC_LogInfo(fmt, ...) printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define _IOC_LogWarn(fmt, ...) printf("[WARN] " fmt "\n", ##__VA_ARGS__)
#define _IOC_LogError(fmt, ...) printf("[ERROR] " fmt "\n", ##__VA_ARGS__)

// This macro is used to log the bug information and assert the program.
#define _IOC_LogBug(fmt, ...)                                            \
  do {                                                                   \
    printf("[BUG@ %s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    assert(0);                                                           \
  } while (0)

// This macro is used to assert the condexpr and log message if the condition is not satisfied same as assert().
#define _IOC_LogAssert(condexpr)                                     \
  do {                                                               \
    if (!(condexpr)) {                                               \
      printf("[ASSERT@ %s:%d] %s\n", __FILE__, __LINE__, #condexpr); \
      assert(0);                                                     \
    }                                                                \
  } while (0)

// This macro is used to mark the code path that is not tested yet.
#define _IOC_LogNotTested()                                                                \
  do {                                                                                     \
    printf("[NOT_TESTED@ %s:%d] This code path is not tested yet.\n", __FILE__, __LINE__); \
    assert(0);                                                                             \
  } while (0)

#ifdef __cplusplus
}
#endif
#endif  //__INTER_OBJECT_COMMUNICATION_LOGGING_H__