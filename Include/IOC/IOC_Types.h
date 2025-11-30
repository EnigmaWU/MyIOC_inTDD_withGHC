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
    IOC_RESULT_TOO_MANY_CLIENTS = IOC_RESULT_TOO_MANY,
    IOC_RESULT_FULL_QUEUING_EVTDESC = IOC_RESULT_TOO_MANY,

    IOC_RESULT_CONFLICT = -504,
    IOC_RESULT_CONFLICT_EVENT_CONSUMER = IOC_RESULT_CONFLICT,
    IOC_RESULT_CONFLICT_SRVARGS = IOC_RESULT_CONFLICT,

    IOC_RESULT_NOT_EXIST = -505,
    IOC_RESULT_NOT_EXIST_LINK = IOC_RESULT_NOT_EXIST,
    IOC_RESULT_NOT_EXIST_SERVICE = IOC_RESULT_NOT_EXIST,
    IOC_RESULT_NO_EVENT_PENDING = IOC_RESULT_NOT_EXIST,
    IOC_RESULT_NO_CMD_PENDING = IOC_RESULT_NOT_EXIST,

    IOC_RESULT_TIMEOUT = -506,
    IOC_RESULT_BUSY = -507,
    IOC_RESULT_LINK_BROKEN = -508,
    IOC_RESULT_CMD_EXEC_FAILED = -509,
    IOC_RESULT_NO_CMD_EXECUTOR = -510,

    // DAT specific result codes
    IOC_RESULT_BUFFER_FULL = -511,
    IOC_RESULT_BUFFER_TOO_SMALL = -512,
    IOC_RESULT_DATA_TOO_LARGE = -515,
    IOC_RESULT_NO_DATA = -516,
    IOC_RESULT_ZERO_DATA = -516,  // Alias for when both PtrDataSize and EmdDataSize are zero
    IOC_RESULT_NOT_EXIST_STREAM = -517,
    IOC_RESULT_ACK_CMD_FAILED = -518,
    IOC_RESULT_INCOMPATIBLE_USAGE = -519,  // 🐛 TDD FIX: Added for UT_ServiceMisuse US-5/AC-1

    IOC_RESULT_EVTDESC_QUEUE_EMPTY = -520,
    IOC_RESULT_TOO_LONG_EMPTYING_EVTDESC_QUEUE = -521,
    IOC_RESULT_NOT_EMPTY_EVTDESC_QUEUE = IOC_RESULT_TOO_LONG_EMPTYING_EVTDESC_QUEUE,
    IOC_RESULT_INVALID_AUTO_LINK_ID = -522,
    IOC_RESULT_NOT_SUPPORT_MANUAL_ACCEPT = -523,  // 🐛 TDD FIX: Added for UT_ServiceMisuse US-4/AC-1

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

#define IOC_ID_INVALID ULONG_MAX  // Used for default variable value assignment, e.g. IOC_LinkID_T, IOC_SrvID_T, etc.

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

#define IOC_INVALID_LINK_ID IOC_ID_INVALID  // Used for default variable value assignment, e.g. IOC_LinkID_T

/**
 * @brief SrvID is a unique ID to identify a service in IOC.
 *
 */
typedef uint64_t IOC_SrvID_T;
typedef IOC_SrvID_T *IOC_SrvID_pT;

#define IOC_INVALID_SRV_ID IOC_ID_INVALID  // Used for default variable value assignment, e.g. IOC_SrvID_T

/**
 * @brief AutoLinkID is a unique ID to identify an automatic link in IOC.
 *    CONLES_MODE is used in ConlesMode, CONET_MODE is used in ConetMode.
 *    AutoLinkID is VALID without connectService or acceptClient.
 *      which means all other LinkID is INVALID by default before connectService or acceptClient.
 */
enum IOC_AutoLinkID_enum {
    IOC_CONLES_MODE_AUTO_LINK_ID_0 = 0U,
    IOC_CONLES_MODE_AUTO_LINK_ID = IOC_CONLES_MODE_AUTO_LINK_ID_0,  // Default

    // TODO: +More AutoLinkID_1/_2/...MAX
    IOC_CONLES_MODE_AUTO_LINK_ID_MAX = 1024U
};

/**
 * @brief Link connection lifecycle state enumeration
 *        Tracks the TRANSPORT-LAYER connection state (TCP/UDP/FIFO establishment)
 *
 * 🔑 KEY DIFFERENCE: IOC_LinkConnState_T vs IOC_LinkState_T
 *
 * ┌────────────────────────────────────────────────────────────────────────────┐
 * │ IOC_LinkConnState_T (THIS ENUM)   │ IOC_LinkState_T                       │
 * ├───────────────────────────────────┼───────────────────────────────────────┤
 * │ Transport/Network Layer           │ Application/Operation Layer           │
 * │ Tracks connection establishment   │ Tracks operational readiness          │
 * │ ConetMode ONLY                    │ Both ConetMode & ConlesMode           │
 * │ Connect → Connected → Disconnect  │ Ready ↔ Busy (during operations)     │
 * │ Example: Connecting, Broken       │ Example: Ready, BusyCbProcEvt        │
 * └───────────────────────────────────┴───────────────────────────────────────┘
 *
 * USAGE SCOPE:
 *  - ConetMode:   BOTH IOC_LinkConnState_T (connection) + IOC_LinkState_T (operation)
 *  - ConlesMode:  ONLY IOC_LinkState_T (no connection phase, uses AUTO_LINK_ID)
 *
 * RELATIONSHIP:
 *  Connection State (this) → Operation State → SubState (CMD/DAT/EVT details)
 *  Example: Connected → Ready → CmdInitiatorBusyExecCmd
 *
 * RefMore: README_ArchDesign-State.md::Link State Machine
 */
typedef enum {
    IOC_LinkConnStateDisconnected = 0,  ///< No connection, no LinkID assigned
    IOC_LinkConnStateConnecting,        ///< IOC_connectService() in progress, establishing connection
    IOC_LinkConnStateConnected,         ///< TCP/UDP/FIFO connection established, handshake complete
    IOC_LinkConnStateDisconnecting,     ///< IOC_disconnectService() in progress, closing connection
    IOC_LinkConnStateBroken,            ///< Connection error detected, requires cleanup
} IOC_LinkConnState_T,
    *IOC_LinkConnState_pT;

/**
 * @brief Query the current connection state of a link
 *
 * @param LinkID: Link ID to query
 * @param pState: Pointer to receive the current connection state
 *
 * @return IOC_RESULT_SUCCESS: State retrieved successfully
 * @return IOC_RESULT_INVALID_PARAM: Invalid LinkID or NULL pState
 * @return IOC_RESULT_NOT_EXIST_LINK: LinkID does not exist
 */
IOC_Result_T IOC_getLinkConnState(IOC_LinkID_T LinkID, IOC_LinkConnState_T *pState);

/**
 * @brief Link operational state enumeration
 *        Tracks the APPLICATION-LAYER operational readiness for CMD/EVT/DAT operations
 *
 * 🔑 KEY DIFFERENCE: IOC_LinkState_T (THIS ENUM) vs IOC_LinkConnState_T
 *
 * ┌────────────────────────────────────────────────────────────────────────────┐
 * │ IOC_LinkState_T (THIS ENUM)       │ IOC_LinkConnState_T                   │
 * ├───────────────────────────────────┼───────────────────────────────────────┤
 * │ Application/Operation Layer       │ Transport/Network Layer               │
 * │ Tracks operational readiness      │ Tracks connection establishment       │
 * │ Both ConetMode & ConlesMode       │ ConetMode ONLY                        │
 * │ Ready ↔ Busy (during operations)  │ Connect → Connected → Disconnect      │
 * │ Example: Ready, BusyCbProcEvt     │ Example: Connecting, Broken           │
 * └───────────────────────────────────┴───────────────────────────────────────┘
 *
 * USAGE SCOPE:
 *  - ConetMode:   IOC_LinkConnState_T (connection) + THIS ENUM (operation)
 *  - ConlesMode:  THIS ENUM ONLY (no connection phase, uses AUTO_LINK_ID)
 *
 * STATE HIERARCHY:
 *  IOC_LinkConnState_T → IOC_LinkState_T (THIS) → IOC_LinkSubState_T
 *  Example: Connected → Ready → CmdInitiatorBusyExecCmd
 *
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

    // DAT Service SubStates
    IOC_LinkSubStateDatSenderReady,
    IOC_LinkSubStateDatSenderBusySendDat,
    IOC_LinkSubStateDatReceiverReady,
    IOC_LinkSubStateDatReceiverBusyRecvDat,    // polling mode
    IOC_LinkSubStateDatReceiverBusyCbRecvDat,  // callback mode

    // CMD Service SubStates
    IOC_LinkSubStateCmdInitiatorReady,        // Command initiator ready to send commands
    IOC_LinkSubStateCmdInitiatorBusyExecCmd,  // Command initiator busy executing command (waiting for response)
    IOC_LinkSubStateCmdExecutorReady,         // Command executor ready to receive commands
    IOC_LinkSubStateCmdExecutorBusyExecCmd,   // Command executor busy processing command (callback mode)
    IOC_LinkSubStateCmdExecutorBusyWaitCmd,   // Command executor busy waiting for command (polling mode)

    // ❓ WHY NO EVT SubStates?
    //
    // EVT operations are FIRE-AND-FORGET (stateless, non-blocking):
    //  - postEVT(): Queue event → Done immediately (no waiting for subscribers)
    //  - subEVT()/unsubEVT(): Register/unregister callback → Done immediately
    //  - Event delivery: Asynchronous via callback (tracked by IOC_LinkState_T::BusyCbProcEvt)
    //
    // CMD/DAT are STATEFUL (blocking or flow-controlled):
    //  - CMD: Request → WAIT for response → Complete (needs "BusyExecCmd" state)
    //  - DAT: Send → Track buffer/flow control → Continue (needs "BusySendDat" state)
    //
    // EVT uses IOC_LinkState_T main states instead:
    //  - IOC_LinkStateReady: Ready for postEVT/subEVT/unsubEVT
    //  - IOC_LinkStateBusyCbProcEvt: Processing event in callback (ConlesMode only)
    //  - IOC_LinkStateBusySubEvt/BusyUnsubEvt: Managing subscriptions (ConlesMode only)
    //
    // RefMore: README_ArchDesign-State.md::Event State Machine::Why No EVT SubStates

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

// RefAPI: IOC_getCapability
typedef enum {
    IOC_CAPID_CONLES_MODE_EVENT = 1,  // RefDT: IOC_ConlesModeEventCapability_T
    IOC_CAPID_CONET_MODE_EVENT,       // RefDT: IOC_ConetModeEventCapability_T
    IOC_CAPID_CONET_MODE_DATA,        // RefDT: IOC_ConetModeDataCapability_T
    IOC_CAPID_CONET_MODE_COMMAND,     // RefDT: IOC_ConetModeCommandCapability_T
                                      // TODO(@W): +More...
} IOC_CapabilityID_T;

typedef struct {
    uint16_t MaxEvtConsumer;     // How many EvtConsumer can be subEVT in ConlesMode.
    uint16_t DepthEvtDescQueue;  // How many EvtDesc can be queued in IOC's EvtDescQueue.
} IOC_ConlesModeEventCapability_T, *IOC_ConlesModeEventCapability_pT;

typedef struct {
    uint16_t MaxSrvNum;  // How many services can be onlined in ConetMode.
    uint16_t MaxCliNum;  // How many clients can be connected to a service in ConetMode.
} IOC_ConetModeCommonCapability_T, *IOC_ConetModeCommonCapability_pT;

typedef struct {
    IOC_ConetModeCommonCapability_T Common;  // RefType: IOC_ConetModeCommonCapability_T
    // TODO(@W): +More event capability fields...
} IOC_ConetModeEventCapability_T, *IOC_ConetModeEventCapability_pT;

typedef struct {
    IOC_ConetModeCommonCapability_T Common;  // RefType: IOC_ConetModeCommonCapability_T
    // TODO(@W): +More command capability fields...
} IOC_ConetModeCommandCapability_T, *IOC_ConetModeCommandCapability_pT;

typedef struct {
    IOC_ConetModeCommonCapability_T Common;  // RefType: IOC_ConetModeCommonCapability_T
    // TODO(@W): +More data capability fields...
    ULONG_T MaxDataQueueSize;  // Maximum size of the data queue in ConetMode.
                               // which means how many data chunks can be buffered in the queue.
} IOC_ConetModeDataCapability_T, *IOC_ConetModeDataCapability_pT;

typedef struct {
    // set this CapID and get the capability description.
    IOC_CapabilityID_T CapID;  // RefType: IOC_CapabilityID_T

    union {
        ULONG_T RZVD[8];                                    // reserve for MAX payload size.
        IOC_ConlesModeEventCapability_T ConlesModeEvent;    // CapID: IOC_CAPID_CONLES_MODE_EVENT
        IOC_ConetModeEventCapability_T ConetModeEvent;      // CapID: IOC_CAPID_CONET_MODE_EVENT
        IOC_ConetModeCommandCapability_T ConetModeCommand;  // CapID: IOC_CAPID_CONET_MODE_COMMAND
        IOC_ConetModeDataCapability_T ConetModeData;        // CapID: IOC_CAPID_CONET_MODE_DATA
        // TODO(@W): +More capability fields...
    };
} IOC_CapabilityDescription_T, *IOC_CapabilityDescription_pT;

#ifdef __cplusplus
}
#endif
#endif  //__IOC_TYPES_H__