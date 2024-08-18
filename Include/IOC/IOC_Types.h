#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/_types/_null.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#ifndef __IOC_TYPES_H__
#define __IOC_TYPES_H__
#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long ULONG_T;

typedef enum {
  IOC_RESULT_SUCCESS = 0,
  IOC_RESULT_FAILURE = -1,

  // POSIX's Result (errno.h)
  IOC_RESULT_POSIX_ENOMEM  = -ENOMEM,
  IOC_RESULT_INVALID_PARAM = -EINVAL,

  // IOC's Result
  IOC_RESULT_NOT_IMPLEMENTED                 = -500,
  IOC_RESULT_NOT_SUPPORT                     = -501,
  IOC_RESULT_NO_EVENT_CONSUMER               = -502,
  IOC_RESULT_TOO_MANY_EVENT_CONSUMER         = -503,
  IOC_RESULT_CONFLICT_EVENT_CONSUMER         = -504,
  IOC_RESULT_TOO_MANY_QUEUING_EVTDESC        = -505,
  IOC_RESULT_NOT_EXIST_LINK                  = -506,
  IOC_RESULT_EVTDESC_QUEUE_EMPTY             = -507,
  IOC_RESULT_TOO_LONG_EMPTYING_EVTDESC_QUEUE = -508,
  IOC_RESULT_INVALID_AUTO_LINK_ID            = -509,

  IOC_RESULT_BUG = -999,
} IOC_Result_T;

const char *IOC_getResultStr(IOC_Result_T Result);

typedef enum {
  IOC_RESULT_NO  = 0,  // False
  IOC_RESULT_YES = 1,  // True
} IOC_BoolResult_T;

/**
 * @brief LinkID is a unique ID to identify a link between two objects in communication.
 *     In ConlesMode, LinkID is predefined as IOC_CONLES_MODE_AUTO_LINK_ID.
 *     In ConetMode, LinkID is dynamically extablished,
 *      then get it from connect API in ClientSide, or accept API in ServerSide.
 *---------------------------------------------------------------------------------------------------------------------
 * Design::FSM
 *    RefReadme::MSG::EVT::Conles/Conet
 *    RefType::IOC_LinkState_T
 */
typedef uint64_t IOC_LinkID_T;
#define IOC_CONLES_MODE_AUTO_LINK_ID 0UL
// TODO: IOC_CONLES_MODE_AUTO_LINK_ID_0<DFT>/_1/_2/...

/**
 * RefMore README_ArchDesign::State
 *    |-> EVT::Conet
 *    |-> EVT::Conles
 */
typedef enum {
  IOC_LinkStateUndefined = 0,
  IOC_LinkStateReady     = 1,

  // RefMore ConlesMode subEVT/unsubEVT/cbProcEVT
  IOC_LinkStateBusyCbProcEvt,
  IOC_LinkStateBusySubEvt,
  IOC_LinkStateBusyUnsubEvt,
} IOC_LinkState_T,
    *IOC_LinkState_pT;

typedef enum {
  IOC_LinkSubStateDefault = 0,
  IOC_LinkSubStateIdle    = IOC_LinkSubStateDefault,

} IOC_LinkSubState_T,
    *IOC_LinkSubState_pT;

typedef enum {
  IOC_OPTID_NONE    = 0,
  IOC_OPTID_TIMEOUT = 1 << 0,    // set this IDs and Payload.TimeoutUS>=0, to set timeout for
                                 // execCMD,waitCMD,sendDAT,recvDAT,...
  IOC_OPTID_SYNC_MODE = 1 << 1,  // set this IDs and Payload.RZVD=0, to set SYNC mode for postEVT.
  // TODO(@W): +More...
} IOC_OptionsID_T;

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
        if (pOption->Payload.TimeoutUS == 0) {
          IsNonBlockMode = IOC_RESULT_YES;
        }
      }
    }

    return IsNonBlockMode;
}

#define IOC_Option_defineNonBlock(OptVarName)       \
  IOC_Options_T OptVarName     = {};                \
  OptVarName.IDs               = IOC_OPTID_TIMEOUT; \
  OptVarName.Payload.TimeoutUS = 0;

#define IOC_Option_defineASyncNonBlock IOC_Option_defineNonBlock

#define IOC_Option_defineTimeout(OptVarName, TimeoutUS) \
  IOC_Options_T OptVarName     = {};                    \
  OptVarName.IDs               = IOC_OPTID_TIMEOUT;     \
  OptVarName.Payload.TimeoutUS = TimeoutUS;

#define IOC_Option_defineASyncTimeout IOC_Option_defineTimeout

#define IOC_Option_defineSyncNonBlock(OptVarName)                                            \
  IOC_Options_T OptVarName     = {};                                                         \
  OptVarName.IDs               = (IOC_OptionsID_T)(IOC_OPTID_SYNC_MODE | IOC_OPTID_TIMEOUT); \
  OptVarName.Payload.TimeoutUS = 0;

#define IOC_Option_defineSyncTimeout(OptVarName, TimeoutUS)                                  \
  IOC_Options_T OptVarName     = {};                                                         \
  OptVarName.IDs               = (IOC_OptionsID_T)(IOC_OPTID_SYNC_MODE | IOC_OPTID_TIMEOUT); \
  OptVarName.Payload.TimeoutUS = TimeoutUS;

static inline ULONG_T IOC_Option_getTimeoutUS(IOC_Options_pT pOption) {
    ULONG_T TimeoutUS = ULONG_MAX;  // Default is Infinite
    if (pOption) {
      if (pOption->IDs & IOC_OPTID_TIMEOUT) {
        TimeoutUS = pOption->Payload.TimeoutUS;
      }
    }

    return TimeoutUS;
}

static inline IOC_BoolResult_T IOC_Option_isTimeoutMode(IOC_Options_pT pOption) {
    ULONG_T TimeoutUS = IOC_Option_getTimeoutUS(pOption);
    if (0 < TimeoutUS && TimeoutUS < ULONG_MAX) {
      return IOC_RESULT_YES;
    }
    return IOC_RESULT_NO;
}

static inline IOC_BoolResult_T IOC_Option_isMayBlockMode(IOC_Options_pT pOption) {
    IOC_BoolResult_T IsNonBlock = IOC_Option_isNonBlockMode(pOption);
    IOC_BoolResult_T IsTimeout  = IOC_Option_isTimeoutMode(pOption);

    if (IsNonBlock == IOC_RESULT_NO && IsTimeout == IOC_RESULT_NO) {
      return IOC_RESULT_YES;  // MayBlock means not NonBlock and not Timeout
    }

    return IOC_RESULT_NO;
}

//---------------------------------------------------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//===>Capabilty

// RefAPI: IOC_getCapabilty
typedef enum {
  IOC_CAPID_CONLES_MODE_EVENT = 1,  // RefDT: IOC_ConlesModeEventCapabilty_T
  // TODO(@W): +More...
} IOC_CapabiltyID_T;

typedef struct {
    uint16_t MaxEvtConsumer;  // How many EvtConsumer can be subEVT in ConlesMode.
    uint16_t DepthEvtDescQueue;  // How many EvtDesc can be queued in IOC's EvtDescQueue.
} IOC_ConlesModeEventCapabilty_T, *IOC_ConlesModeEventCapabilty_pT;

typedef struct {
    // set this CapID and get the capability description.
    IOC_CapabiltyID_T CapID;  // RefType: IOC_CapabiltyID_T

    union {
      ULONG_T RZVD[8];  // reserve for MAX payload size.
      IOC_ConlesModeEventCapabilty_T ConlesModeEvent;
    };
} IOC_CapabiltyDescription_T, *IOC_CapabiltyDescription_pT;

#ifdef __cplusplus
}
#endif
#endif//__IOC_TYPES_H__