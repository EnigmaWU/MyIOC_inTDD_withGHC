/**
 * @file _IOC.h
 * @brief This is internal header file, which is included by internal source file.
 * @attention
 *   ALL internal function or type's name should be prefixed with "_", such as _IOC_doXYZ() or _IOC_DataType_T.
 */

#include <IOC/IOC.h>  //Module IOC's public header file
#include <pthread.h>
#include <semaphore.h>
// #include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef __INTER_OBJECT_COMMUNICATION_INTERNAL_H__
#define __INTER_OBJECT_COMMUNICATION_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

// make sure this file is included only once
#include "_IOC_ConetEvent.h"
#include "_IOC_ConlesEvent.h"
#include "_IOC_Logging.h"
#include "_IOC_Types.h"

#define _MAX_IOC_SRV_OBJ_NUM 2
#define _MAX_IOC_CLI_OBJ_NUM_PER_SRV 3

// 🎯 TDD GREEN: ConlesEvent SubState bridge function for DAT operations
void _IOC_updateConlesEventSubState(IOC_LinkID_T linkID, IOC_LinkSubState_T subState);

// 🎯 TDD GREEN: Role negotiation helper for multi-role service support (US-3)
// Computes complementary link role: Client=Executor → Service=Initiator on that link
IOC_LinkUsage_T _IOC_negotiateLinkRole(IOC_LinkUsage_T ServiceCapabilities, IOC_LinkUsage_T ClientRequestedUsage);

// 🧪 TEST HOOKS: Fault injection and diagnostics (only available in test builds)
#ifdef CONFIG_BUILD_WITH_UNIT_TESTING
void IOC_test_setFailNextAlloc(int count);
uint16_t IOC_getServiceCount(void);
uint16_t IOC_getLinkCount(void);
#endif

#ifdef __cplusplus
}
#endif
#endif  //__INTER_OBJECT_COMMUNICATION_INTERNAL_H__