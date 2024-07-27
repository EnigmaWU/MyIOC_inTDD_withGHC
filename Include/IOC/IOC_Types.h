#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

  IOC_RESULT_YES = 1,
  IOC_RESULT_NO  = 0,

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

#include "IOC_Types_EvtID.h"

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
  IOC_OPTID_TIMEOUT =
      1 << 0,  // set this IDs and Payload.TimeoutUS>=0, to set timeout for execCMD,waitCMD,sendDAT,recvDAT,...
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

static inline bool IOC_Option_isAsyncMode(IOC_Options_pT pOption) {
    bool IsAsyncMode = true;  // Default is AsyncMode
    if (pOption) {
      if (pOption->IDs & IOC_OPTID_SYNC_MODE) {
        IsAsyncMode = false;
      }
    }

    return IsAsyncMode;
}

static inline bool IOC_Option_isNonBlockMode(IOC_Options_pT pOption) {
    bool IsNonBlockMode = false;  // Default is BlockMode
    if (pOption) {
      if (pOption->IDs & IOC_OPTID_TIMEOUT) {
        if (pOption->Payload.TimeoutUS == 0) {
          IsNonBlockMode = true;
        }
      }
    }

    return IsNonBlockMode;
}

static inline ULONG_T IOC_Option_getTimeoutUS(IOC_Options_pT pOption) {
    ULONG_T TimeoutUS = 0xFFFFFFFFFFFFFFFFULL;  // Default is Infinite
    if (pOption) {
      if (pOption->IDs & IOC_OPTID_TIMEOUT) {
        TimeoutUS = pOption->Payload.TimeoutUS;
      }
    }

    return TimeoutUS;
}

#define IOC_Option_defineNonBlock(OptVarName)       \
  IOC_Options_T OptVarName     = {};                \
  OptVarName.IDs               = IOC_OPTID_TIMEOUT; \
  OptVarName.Payload.TimeoutUS = 0;

//---------------------------------------------------------------------------------------------------------------------
#if 0
typedef enum {
  IOC_MSGFLAG_MAYDROP = 1 << 0,  // set this flag to allow drop this message if no enough resource.
} IOC_MsgFlags_T;
#endif

//MSG is Common Head of Evt and Cmd
typedef struct
{
    // TODO: ULONG_T MagicID;  // MagicID is used to identify the message type.
    ULONG_T SeqID;
    struct timespec TimeStamp;  // when call IOC_postEVT set value from IOC_getCurrentTimeSpec

    // IOC_MsgFlags_T Flags;
} IOC_MsgDesc_T, *IOC_MsgDesc_pT;

typedef struct 
{
    //MsgCommon
    IOC_MsgDesc_T MsgDesc;

    //EvtSpecific
    IOC_EvtID_T EvtID;
    ULONG_T EvtValue;

    // TOOD(@W): +More..., such as EvtPayload
} IOC_EvtDesc_T, *IOC_EvtDesc_pT;

typedef IOC_Result_T (*IOC_CbProcEvt_F)(IOC_EvtDesc_pT pEvtDesc, void *pCbPriv);
typedef struct 
{
    /**
     * @brief CbProcEvt_F + pCbPrivData is used to IDENTIFY one EvtConsumer.
     *    RefMore: IOC_UnsubEvtArgs_T
     */
    IOC_CbProcEvt_F CbProcEvt_F;  // Callback function for processing the event
    void *pCbPrivData;  // Callback private context data

    ULONG_T EvtNum;  // number of EvtIDs, IOC_calcArrayElmtCnt(SubEvtIDs)
    IOC_EvtID_T *pEvtIDs;  // EvtIDs to subscribe

    // TODO: AutoLinkID
} IOC_SubEvtArgs_T, *IOC_SubEvtArgs_pT;

typedef struct {
    IOC_CbProcEvt_F CbProcEvt_F;
    union {
      void *pCbPrivData;
      void *pCbPriv;  // Deprecated
    };
} IOC_UnsubEvtArgs_T, *IOC_UnsubEvtArgs_pT;

#include "IOC_Types_EvtID.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//===>Capabilty

// RefAPI: IOC_getCapabilty
typedef enum {
  IOC_CAPID_CONLES_MODE_EVENT = 1,  // RefDT: IOC_ConlesModeEventCapabilty_T
  // TODO(@W): +More...
} IOC_CapabiltyID_T;

typedef struct {
    uint16_t MaxEvtConsumer;  // How many EvtConsumer can be subEVT in ConlesMode.
} IOC_ConlesModeEventCapabilty_T, *IOC_ConlesModeEventCapabilty_pT;

typedef struct {
    IOC_CapabiltyID_T CapID;
    union {
      ULONG_T RZVD[8];  // reserve for MAX payload size.
      IOC_ConlesModeEventCapabilty_T ConlesModeEvent;
    };
} IOC_CapabiltyDescription_T, *IOC_CapabiltyDescription_pT;

#ifdef __cplusplus
}
#endif
#endif//__IOC_TYPES_H__