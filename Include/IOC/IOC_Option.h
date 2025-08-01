#include "IOC_Types.h"

#ifndef __IOC_TYPEOPTION_H__
#define __IOC_TYPEOPTION_H__
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    IOC_OPTID_NONE = 0,
    IOC_OPTID_TIMEOUT = 1 << 0,    // set this IDs and Payload.TimeoutUS>=0, to set timeout for
                                   // execCMD,waitCMD,sendDAT,recvDAT,...
    IOC_OPTID_SYNC_MODE = 1 << 1,  // set this IDs and Payload.RZVD=0, to set SYNC mode for postEVT.
                                   // TODO(@W): +More...
} IOC_OptionsID_T;

#define IOC_TIMEOUT_INFINITE ULONG_MAX
#define IOC_TIMEOUT_NONBLOCK 0      // NonBlock means no timeout, which is equivalent to 0us
#define IOC_TIMEOUT_IMMEDIATE 1000  // 1ms = 1000us means immediate timeout
#ifdef CONFIG_BUILD_WITH_UNIT_TESTING
#define IOC_TIMEOUT_MAX 9000000  // 9-Seconds = 9*1000*1000us
#else
#define IOC_TIMEOUT_MAX 86400000000  // 24-Hours 24*60*60*1000ms*1000us
#endif                               // CONFIG_BUILD_WITH_UNIT_TESTING

typedef struct {
    IOC_OptionsID_T IDs;

    union {
        uint64_t RZVD[8];  // reserve for MAX payload size.
        ULONG_T TimeoutUS;
    } Payload;

} IOC_Options_T, *IOC_Options_pT;

static inline IOC_BoolResult_T IOC_Option_isAsyncMode(IOC_Options_pT pOption) {
    IOC_BoolResult_T IsAsyncMode = IOC_RESULT_YES;  // Default is AsyncMode
    if (pOption) {
        if (pOption->IDs & IOC_OPTID_SYNC_MODE) {
            IsAsyncMode = IOC_RESULT_NO;
        }
    }

    return IsAsyncMode;
}

static inline IOC_BoolResult_T IOC_Option_isSyncMode(IOC_Options_pT pOption) {
    IOC_BoolResult_T IsAsyncMode = IOC_Option_isAsyncMode(pOption);
    return (IsAsyncMode == IOC_RESULT_YES) ? IOC_RESULT_NO  // SyncMode is opposite to AsyncMode
                                           : IOC_RESULT_YES;
}

static inline IOC_BoolResult_T IOC_Option_isNonBlockMode(IOC_Options_pT pOption) {
    IOC_BoolResult_T IsNonBlockMode = IOC_RESULT_NO;  // Default is BlockMode
    if (pOption) {
        if (pOption->IDs & IOC_OPTID_TIMEOUT) {
            if (pOption->Payload.TimeoutUS == IOC_TIMEOUT_NONBLOCK) {
                IsNonBlockMode = IOC_RESULT_YES;
            }
        }
    }

    return IsNonBlockMode;
}

// NONBLOCK: ArgTimeoutUS MUST == 0, means NonBlock mode
#define IOC_Option_defineNonBlock(OptVarName) \
    IOC_Options_T OptVarName = {};            \
    OptVarName.IDs = IOC_OPTID_TIMEOUT;       \
    OptVarName.Payload.TimeoutUS = IOC_TIMEOUT_NONBLOCK;

#define IOC_Option_defineASyncNonBlock IOC_Option_defineNonBlock

// TIMEOUT: ArgTimeoutUS MUST > 0, ==0 means NonBlock mode
#define IOC_Option_defineTimeout(OptVarName, ArgTimeoutUS) \
    IOC_Options_T OptVarName = {};                         \
    OptVarName.IDs = IOC_OPTID_TIMEOUT;                    \
    OptVarName.Payload.TimeoutUS = ArgTimeoutUS;

#define IOC_Option_defineASyncTimeout IOC_Option_defineTimeout

#define IOC_Option_defineASyncMayBlock(OptVarName) IOC_Options_T OptVarName = {};
#define IOC_Option_defineASyncMode IOC_Option_defineASyncMayBlock

// NONBLOCK: ArgTimeoutUS MUST == 0, means NonBlock mode
#define IOC_Option_defineSyncNonBlock(OptVarName)                                \
    IOC_Options_T OptVarName = {};                                               \
    OptVarName.IDs = (IOC_OptionsID_T)(IOC_OPTID_SYNC_MODE | IOC_OPTID_TIMEOUT); \
    OptVarName.Payload.TimeoutUS = IOC_TIMEOUT_NONBLOCK;

#define IOC_Option_defineSyncTimeout(OptVarName, ArgTimeoutUS)                   \
    IOC_Options_T OptVarName = {};                                               \
    OptVarName.IDs = (IOC_OptionsID_T)(IOC_OPTID_SYNC_MODE | IOC_OPTID_TIMEOUT); \
    OptVarName.Payload.TimeoutUS = ArgTimeoutUS;

#define IOC_Option_defineSyncMayBlock(OptVarName) \
    IOC_Options_T OptVarName = {};                \
    OptVarName.IDs = IOC_OPTID_SYNC_MODE;

#define IOC_Option_defineSyncMode IOC_Option_defineSyncMayBlock

static inline ULONG_T IOC_Option_getTimeoutUS(IOC_Options_pT pOption) {
    ULONG_T TimeoutUS = IOC_TIMEOUT_INFINITE;  // Default is Infinite
    if (pOption) {
        if (pOption->IDs & IOC_OPTID_TIMEOUT) {
            TimeoutUS = pOption->Payload.TimeoutUS;
        }
    }

    return TimeoutUS;
}

static inline IOC_BoolResult_T IOC_Option_isTimeoutMode(IOC_Options_pT pOption) {
    ULONG_T TimeoutUS = IOC_Option_getTimeoutUS(pOption);
    if (0 < TimeoutUS && TimeoutUS < IOC_TIMEOUT_MAX) {
        return IOC_RESULT_YES;
    }
    return IOC_RESULT_NO;
}

static inline IOC_BoolResult_T IOC_Option_isMayBlockMode(IOC_Options_pT pOption) {
    IOC_BoolResult_T IsNonBlock = IOC_Option_isNonBlockMode(pOption);
    IOC_BoolResult_T IsTimeout = IOC_Option_isTimeoutMode(pOption);

    if (IsNonBlock == IOC_RESULT_NO && IsTimeout == IOC_RESULT_NO) {
        return IOC_RESULT_YES;  // MayBlock means not NonBlock and not Timeout
    }

    return IOC_RESULT_NO;
}

#ifdef __cplusplus
}
#endif
#endif  // __IOC_TYPEOPTION_H__