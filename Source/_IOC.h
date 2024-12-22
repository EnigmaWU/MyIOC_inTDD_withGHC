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

#ifdef __cplusplus
}
#endif
#endif  //__INTER_OBJECT_COMMUNICATION_INTERNAL_H__