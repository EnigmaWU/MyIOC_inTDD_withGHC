// Description: This file is the header file for the MyIOC module.

#include <stdint.h>

#ifndef __INTER_OBJECT_COMMUNICATION_H__
#define __INTER_OBJECT_COMMUNICATION_H__
#ifdef __cplusplus
extern "C" {
#endif

typedef enum 
{
    IOC_RESULT_SUCCESS = 0,
    IOC_RESULT_FAILURE = 1,
} IOC_Result_T;

IOC_Result_T IOC_helloAPI(void);

#ifdef __cplusplus
}
#endif
#endif//__INTER_OBJECT_COMMUNICATION_H__
