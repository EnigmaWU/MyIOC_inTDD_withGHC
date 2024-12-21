#include <assert.h>
#include <inttypes.h>
#include <limits.h>
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

    // POSIX's Result (errno.h)
    IOC_RESULT_POSIX_ENOMEM = -ENOMEM,
    IOC_RESULT_POSIX_EINVAL = -EINVAL,
    IOC_RESULT_POSIX_EAGAIN = -EAGAIN,
    IOC_RESULT_POSIX_EPERM = -EPERM,
    IOC_RESULT_INVALID_PARAM = IOC_RESULT_POSIX_EINVAL,

    // IOC's Result
    IOC_RESULT_NOT_IMPLEMENTED = -500,

    IOC_RESULT_NOT_SUPPORT = -501,
    IOC_RESULT_NOT_SUPPORT_BROADCAST_EVENT = IOC_RESULT_NOT_SUPPORT,

    IOC_RESULT_NO_EVENT_CONSUMER = -502,

    IOC_RESULT_TOO_MANY = -503,
    IOC_RESULT_TOO_MANY_EVENT_CONSUMER = IOC_RESULT_TOO_MANY,
    IOC_RESULT_TOO_MANY_QUEUING_EVTDESC = IOC_RESULT_TOO_MANY,
    IOC_RESULT_TOO_MANY_SERVICES = IOC_RESULT_TOO_MANY,
    IOC_RESULT_TOO_MANY_LINKS = IOC_RESULT_TOO_MANY,
    IOC_RESULT_FULL_QUEUING_EVTDESC = IOC_RESULT_TOO_MANY,

    IOC_RESULT_CONFLICT = -504,
    IOC_RESULT_CONFLICT_EVENT_CONSUMER = IOC_RESULT_CONFLICT,
    IOC_RESULT_CONFLICT_SRVARGS = IOC_RESULT_CONFLICT,

    IOC_RESULT_NOT_EXIST = -505,
    IOC_RESULT_NOT_EXIST_LINK = IOC_RESULT_NOT_EXIST,
    IOC_RESULT_NOT_EXIST_SERVICE = IOC_RESULT_NOT_EXIST,

    IOC_RESULT_EVTDESC_QUEUE_EMPTY = -507,
    IOC_RESULT_TOO_LONG_EMPTYING_EVTDESC_QUEUE = -508,
    IOC_RESULT_NOT_EMPTY_EVTDESC_QUEUE = IOC_RESULT_TOO_LONG_EMPTYING_EVTDESC_QUEUE,
    IOC_RESULT_INVALID_AUTO_LINK_ID = -509,

    IOC_RESULT_BUG = -999,
} IOC_Result_T;

const char *IOC_getResultStr(IOC_Result_T Result);

typedef enum {
    IOC_TRUE = true,
    IOC_FALSE = false,

    IOC_YES = true,
    IOC_NO = false,

    IOC_RESULT_TRUE = true,
    IOC_RESULT_FALSE = false,

    IOC_RESULT_YES = true,
    IOC_RESULT_NO = false,
} IOC_BoolResult_T,
    IOC_Bool_T;

#define IOC_ID_INVALID ULONG_MAX

/**
 * @brief LinkID is a unique ID to identify a link between two objects in communication.
 *     In ConlesMode, LinkID is predefined as IOC_CONLES_MODE_AUTO_LINK_ID.
 *     In ConetMode, LinkID is dynamically extablished,
 *      which get from connect API in ClientSide, or accept API in ServerSide.
 *---------------------------------------------------------------------------------------------------------------------
 * Design::FSM
 *    RefReadme::MSG::EVT::Conles/Conet
 *    RefType::IOC_LinkState_T
 */
typedef uint64_t IOC_LinkID_T;
typedef IOC_LinkID_T *IOC_LinkID_pT;

#define IOC_INVALID_LINK_ID IOC_ID_INVALID

/**
 * @brief SrvID is a unique ID to identify a service in IOC.
 *
 */
typedef uint64_t IOC_SrvID_T;
typedef IOC_SrvID_T *IOC_SrvID_pT;

#define IOC_INVALID_SRV_ID IOC_ID_INVALID

enum IOC_AutoLinkID_enum {
    IOC_CONLES_MODE_AUTO_LINK_ID_0 = 0U,
    IOC_CONLES_MODE_AUTO_LINK_ID = IOC_CONLES_MODE_AUTO_LINK_ID_0,  // Default

    // TODO: +More AutoLinkID_1/_2/...MAX
    IOC_CONLES_MODE_AUTO_LINK_ID_MAX = 1024U
};

/**
 * RefMore README_ArchDesign::State
 *    |-> EVT::Conet
 *    |-> EVT::Conles
 */
typedef enum {
    IOC_LinkStateUndefined = 0,
    IOC_LinkStateReady = 1,

    // RefMore ConlesMode subEVT/unsubEVT/cbProcEVT
    IOC_LinkStateBusyCbProcEvt,
    IOC_LinkStateBusySubEvt,
    IOC_LinkStateBusyUnsubEvt,
} IOC_LinkState_T,
    *IOC_LinkState_pT;

typedef enum {
    IOC_LinkSubStateDefault = 0,
    IOC_LinkSubStateIdle = IOC_LinkSubStateDefault,

} IOC_LinkSubState_T,
    *IOC_LinkSubState_pT;

/**
 * @brief LinkUsage is a bit mask to identify the usage of a link, or the capability of a service.
 *  As a Service, it MAY have multiple usage, e.g. <EvtProducer and CmdExecutor and ...>.
 *  As a Link, it MAY ONLY have a single pair of usage, e.g. <EvtProducer vs EvtConsumer>,
 *      or <CmdInitiator vs CmdExecutor>, or <DatSender vs DatReceiver>,
 *      AND a single usage at each side, e.g. <EvtProducer or EvtConsumer>.
 */
typedef enum {
    IOC_LinkUsageUndefined = 0,

    IOC_LinkUsageEvtProducer = (1U << 0),
    IOC_LinkUsageEvtConsumer = (1U << 1),
    IOC_LinkUsageCmdInitiator = (1U << 2),
    IOC_LinkUsageCmdExecutor = (1U << 3),
    IOC_LinkUsageDatSender = (1U << 4),
    IOC_LinkUsageDatReceiver = (1U << 5),

    IOC_LinkUsageMask = IOC_LinkUsageEvtProducer | IOC_LinkUsageEvtConsumer | IOC_LinkUsageCmdInitiator |
                        IOC_LinkUsageCmdExecutor | IOC_LinkUsageDatSender | IOC_LinkUsageDatReceiver,
} IOC_LinkUsage_T;

//---------------------------------------------------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//===>Capabilty

// RefAPI: IOC_getCapabilty
typedef enum {
    IOC_CAPID_CONLES_MODE_EVENT = 1,  // RefDT: IOC_ConlesModeEventCapabilty_T
                                      // TODO(@W): +More...
} IOC_CapabiltyID_T;

typedef struct {
    uint16_t MaxEvtConsumer;     // How many EvtConsumer can be subEVT in ConlesMode.
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
#endif  //__IOC_TYPES_H__