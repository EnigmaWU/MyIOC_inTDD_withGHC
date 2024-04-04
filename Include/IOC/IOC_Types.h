#include <stdint.h>
#include <stdio.h>
#include <sys/errno.h>
#include <sys/time.h>

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
  IOC_RESULT_POSIX_ENOMEM = -ENOMEM,

  // IOC's Result
  IOC_RESULT_NOT_IMPLEMENTED          = -500,
  IOC_RESULT_NOT_SUPPORT              = -501,
  IOC_RESULT_NO_EVTCOSMER             = -502,
  IOC_RESULT_TOO_MANY_EVTCOSMER       = -503,
  IOC_RESULT_CONFLICT_EVTCOSMER       = -504,
  IOC_RESULT_TOO_MANY_QUEUING_EVTDESC = -505,

  IOC_RESULT_BUG = -999,
} IOC_Result_T;

/**
 * @brief LinkID is a unique ID to identify a link between two objects in communication.
 *---------------------------------------------------------------------------------------------------------------------
 * Design::FSM
 *    RefReadme::MSG::EVT::FSM::Conles/Conet
 *    RefType::IOC_LinkState_T
 */
typedef uint64_t IOC_LinkID_T;
#define IOC_CONLES_MODE_AUTO_LINK_ID 0
// TODO: IOC_CONLES_MODE_AUTO_LINK_ID_0<DFT>/_1/_2/...

// RefReadme::MSG::EVT::FSM::Conles/Conet
typedef enum {
  IOC_LinkStateReady = 0,  // Link is ready to use.
  IOC_LinkStateBusy  = 1,  // Link is busy, can't be used.
  // IOC_LinkStateError = 2,  // Link is error, can't be used.
} IOC_LinkState_T;

typedef enum {
  IOC_LinkSubState_ReadyIdle   = 0,  // Link is ready to use.
  IOC_LinkSubState_ReadyLocked = 1,  // Link is ready but locked.

  IOC_LinkSubState_BusyProcing        = 0,  //  Link is busy of processing.
  IOC_LinkSubState_BusyProcingPosting = 1,  //  Link is busy of processing&posting.
} IOC_LinkSubState_T;

typedef enum {
  IOC_OPTID_TIMEOUT = 1 << 0,    // set this IDs and Payload.TimeoutUS>=0, to set timeout for execCMD,waitCMD,sendDAT,recvDAT,...
  IOC_OPTID_SYNC_MODE = 1 << 1,  // set this IDs and Payload.RZVD=0, to set SYNC mode for postEVT.
  // TODO(@W): +More...
} IOC_OptionsID_T;

typedef struct 
{
    IOC_OptionsID_T IDs;

    union {
      uint64_t RZVD[8];  // reserve for MAX payload size.
      uint32_t TimeoutUS;
    } Payload;

} IOC_Options_T, *IOC_Options_pT;

//---------------------------------------------------------------------------------------------------------------------
#if 0
typedef enum {
  IOC_MSGFLAG_MAYDROP = 1 << 0,  // set this flag to allow drop this message if no enough resource.
} IOC_MsgFlags_T;
#endif

//MSG is Common Head of Evt and Cmd
typedef struct
{
    ULONG_T RZVD;

    //TODO(@W): +More...
    //ULONG_T SeqID;
    //IOC_MsgFlags_T Flags;
} IOC_MsgDesc_T, *IOC_MsgDesc_pT;


typedef     uint64_t      IOC_EvtID_T;
typedef struct 
{
    //MsgCommon
    IOC_MsgDesc_T MsgDesc;

    //EvtSpecific
    IOC_EvtID_T EvtID;
    //TOOD(@W): +More...
} IOC_EvtDesc_T, *IOC_EvtDesc_pT;

#define IOC_calcArrayElmtCnt(array) (sizeof(array) / sizeof(array[0]))
typedef IOC_Result_T (*IOC_CbProcEvt_F)(IOC_EvtDesc_pT pEvtDesc, void *pCbPriv);
typedef struct 
{
    IOC_CbProcEvt_F CbProcEvt_F;
    void *pCbPrivData;  // Callback private context data
    ULONG_T EvtNum;  // number of EvtIDs, IOC_calcArrayElmtCnt(SubEvtIDs)
    IOC_EvtID_T *pEvtIDs;
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
    uint16_t MaxEvtCosmer;  // How many EvtCosmer can be subEVT in ConlesMode.
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