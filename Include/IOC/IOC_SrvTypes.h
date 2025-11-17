/**
 * @file IOC_SrvTypes.h
 * @brief IOC's Service Types defined here.
 *
 */

#include "IOC_CmdDesc.h"
#include "IOC_CmdID.h"
#include "IOC_DatDesc.h"
#include "IOC_EvtAPI.h"
#include "IOC_EvtDesc.h"
#include "IOC_Types.h"

#ifndef __IOC_TYPESRV_H__
#define __IOC_TYPESRV_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Command execution callback function type
 *        This callback is invoked when a command needs to be executed in callback mode
 *
 * @param LinkID: the link ID where the command was received
 * @param pCmdDesc: pointer to command description containing command details and payload
 * @param pCbPriv: callback private context data
 *
 * @return IOC_RESULT_SUCCESS: command executed successfully
 * @return Other IOC_Result_T values: command execution failed with specific error
 */
typedef IOC_Result_T (*IOC_CbExecCmd_F)(IOC_LinkID_T LinkID, IOC_CmdDesc_pT pCmdDesc, void *pCbPriv);

/**
 * @brief Data reception callback function type
 *        This callback is invoked when data is received in callback mode
 *
 * @param LinkID: the link ID where the data was received
 * @param pDataDesc: pointer to data description containing data details and payload
 * @param pCbPriv: callback private context data
 *
 * @return IOC_RESULT_SUCCESS: data processed successfully
 * @return Other IOC_Result_T values: data processing failed with specific error
 */
typedef IOC_Result_T (*IOC_CbRecvDat_F)(IOC_LinkID_T LinkID, IOC_DatDesc_pT pDataDesc, void *pCbPriv);

/**
 * @brief Service-level callback for new auto-accepted client links
 *        Invoked when IOC_SRVFLAG_AUTO_ACCEPT is enabled and a new client is accepted.
 *        Called asynchronously from the auto-accept daemon thread; keep it non-blocking.
 */
typedef void (*IOC_CbOnAutoAccepted_F)(IOC_SrvID_T SrvID, IOC_LinkID_T NewLinkID, void *pSrvPriv);

/**
 * @brief Event usage arguments for IOC framework
 *        Contains all event-related parameters and configurations
 *        Used in both IOC_SrvArgs_T and IOC_ConnArgs_T for event capabilities
 */
typedef struct {
    // Event callback configuration
    IOC_CbProcEvt_F CbProcEvt_F;  // Callback function for processing events
    void *pCbPrivData;            // Callback private context data

    // Event subscription configuration
    ULONG_T EvtNum;        // Number of EvtIDs to subscribe/produce
    IOC_EvtID_T *pEvtIDs;  // Array of EvtIDs to subscribe/produce

    // TODO:Reserved;
} IOC_EvtUsageArgs_T, *IOC_EvtUsageArgs_pT;

/**
 * @brief Command usage arguments for IOC framework
 *        Contains all command-related parameters and configurations
 *        Used in both IOC_SrvArgs_T and IOC_ConnArgs_T for command capabilities
 */
typedef struct {
    // Command execution callback configuration
    IOC_CbExecCmd_F CbExecCmd_F;  // Callback function for executing commands
    void *pCbPrivData;            // Callback private context data

    // Command capability configuration
    ULONG_T CmdNum;        // Number of CmdIDs that this executor handles
    IOC_CmdID_T *pCmdIDs;  // Array of CmdIDs that this executor handles

    // TODO: Reserved;
} IOC_CmdUsageArgs_T, *IOC_CmdUsageArgs_pT;

/**
 * @brief Data usage arguments for IOC framework
 *        Contains all data transfer-related parameters and configurations
 *        Used in both IOC_SrvArgs_T and IOC_ConnArgs_T for data transfer capabilities
 */
typedef struct {
    // Data receiver configuration
    IOC_CbRecvDat_F CbRecvDat_F;  // Callback function for receiving data
    void *pCbPrivData;            // Receiver callback private context data

    // TODO: Reserved;

} IOC_DatUsageArgs_T, *IOC_DatUsageArgs_pT;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define IOC_SRV_URI_PROTOCOL_MAX_LEN 16
#define IOC_SRV_URI_HOST_MAX_LEN 64
#define IOC_SRV_URI_PATH_MAX_LEN 128
//===>URI: Uniform Resource Identifier, defined in RFC 3986.
// Standard URI: scheme:[//[user:password@]host[:port]][/]path[?query][#fragment]
// Service use URI to identify what is onlined.
// Client use URI to connect to the service.
typedef struct {
    union {
        const char *pScheme;
        const char *pProtocol;  // RefMacro: IOC_SRV_PROTO_*
    };

    // TODO: char *pUser;
    // TODO: char *pPwd;

    union {
        const char *pHost;  // RefMacro: IOC_SRV_HOST_*
        const char *pDomain;
    };

    union {
        const char *pPath;
        const char *pSrvName;
        const char *pTopic;
    };

    uint16_t Port;  // IF protocol=udp/tcp/http/... THEN Port is required

    // TODO: char *pQuery;
    // TODO: char *pFragment;
} IOC_SrvURI_T, *IOC_SrvURI_pT;

static inline const char *IOC_Helper_printSingleLineSrvURI(IOC_SrvURI_pT pSrvURI, char *pLineBuf, size_t LineBufSiz) {
    static char _mSingleLineBuf[128];  // Use static buffer if LineBuf is NULL, for easy use but not thread-safe.
    //---------------------------------------------------------------------------------------------
    if (!pLineBuf) {
        pLineBuf = &_mSingleLineBuf[0];
        LineBufSiz = sizeof(_mSingleLineBuf);
    }

    snprintf(pLineBuf, LineBufSiz, "%s://%s:%d/%s", pSrvURI->pProtocol, pSrvURI->pHost, pSrvURI->Port, pSrvURI->pPath);
    return pLineBuf;
}

static inline IOC_BoolResult_T IOC_Helper_isEqualSrvURI(const IOC_SrvURI_pT pSrvURI1, const IOC_SrvURI_pT pSrvURI2) {
    if (!strcmp(pSrvURI1->pProtocol, pSrvURI2->pProtocol) && !strcmp(pSrvURI1->pHost, pSrvURI2->pHost) &&
        !strcmp(pSrvURI1->pPath, pSrvURI2->pPath) && pSrvURI1->Port == pSrvURI2->Port) {
        return IOC_RESULT_YES;
    } else {
        return IOC_RESULT_NO;
    }
}

#define IOC_SRV_PROTO_AUTO "auto"  // transport protocol is auto selected by IOC
#define IOC_SRV_PROTO_FIFO "fifo"  // intraprocess/interthread FIFO queue communication protocol
#define IOC_SRV_PROTO_TCP "tcp"    // TCP socket-based network communication protocol
// TODO: #define IOC_SRV_PROTO_UDP "udp"
// TODO: #define IOC_SRV_PROTO_HTTP "http"

#define IOC_SRV_HOST_LOCAL_PROCESS "localprocess"  //=inter-thread communication
#define IOC_SRV_HOST_LOCAL_HOST "localhost"        //=inter-process communication
#define IOC_SRV_HOST_IPV4_ANY "0.0.0.0"            //=inter-host communication

typedef enum {
    IOC_SRVFLAG_NONE = 0,

    /**
     * @brief BROADCAST_EVENT vs P2P==Link2Link
     *  P2P means Point to Point, which is a direct link between two objects.
     *  WHEN service online on server side, it has a SrvID.
     *  WHEN client connect, it has a pair of LinkID both in client and server side,
     *      we name this pair LinkIDs as ConnLinkID and AcptLinkID, or LinkID_atCli and LinkID_atSrv.
     *  ByDefault: Srvice is P2P,
     *      we use AcptLinkID and ConnLinkID to communicate.
     *  If service is onlined with BROADCAST flag on, SrvID is aliased to SrvLinkID,
     *      we use SrvLinkID and ConnLinkIDs to communicate.
     *
     *  Which means:
     *    (DFT/P2P) AcptLinkID <--> ConnLinkID
     *          e.g. postEVT(AcptLinkID) --> ONLY ConnLinkID will CbProcEvt
     *          e.g. postEVT(ConnLinkID) --> ONLY AcptLinkID will CbProcEvt
     *    (BROADCAST_EVENT) SrvLinkID <--> ConnLinkIDs
     *          e.g. postEVT(SrvLinkID) --> ALL ConnLinkIDs will CbProcEvt
     */
    IOC_SRVFLAG_BROADCAST_EVENT = 1 << 0,

    /**
     * @brief AUTO_ACCEPT — automatically accept clients
     *
     * Core behavior:
     *  - Starts a background accept loop; no manual IOC_acceptClient() is needed.
     *  - Stores accepted links inside the service; discover via IOC_getServiceLinkIDs().
     *  - Keeps callbacks where they belong (DAT recv on receiver, CMD exec on executor, EVT consume on consumer).
     *
     * Immediate notification vs polling:
     *  - Polling: Periodically call IOC_getServiceLinkIDs() to find new links.
     *  - Immediate: Provide OnAutoAccepted_F(srv, link, priv) to act right after acceptance (non-blocking).
     *
     * Clear examples by capability
     *  1) DAT — US-1 (Service=DatReceiver, Client=DatSender)
     *     - Service:
     *         Flags include IOC_SRVFLAG_AUTO_ACCEPT;
     *         UsageCapabilites |= IOC_LinkUsageDatReceiver;
     *         SrvArgs.UsageArgs.pDat->CbRecvDat_F = MyServiceRecvCb; // data arrives here
     *         // Optional: OnAutoAccepted_F(srv, link) to init per-link tracking.
     *     - Client:
     *         Usage = IOC_LinkUsageDatSender; use IOC_sendDAT()/IOC_flushDAT() to push data.
     *
     *  2) DAT — US-2 (Service=DatSender, Client=DatReceiver)
     *     - Service:
     *         Flags include IOC_SRVFLAG_AUTO_ACCEPT;
     *         UsageCapabilites |= IOC_LinkUsageDatSender;
     *         OnAutoAccepted_F(srv, link): start/enable sending to 'link' using IOC_sendDAT()/IOC_flushDAT().
     *         // Or poll IOC_getServiceLinkIDs() and send later.
     *     - Client:
     *         Usage = IOC_LinkUsageDatReceiver;
     *         ConnArgs.UsageArgs.pDat->CbRecvDat_F = MyCliRecvCb; // receives
     *
     *  3) CMD — request/response
     *     - Service as CmdExecutor (client initiates):
     *         Flags include IOC_SRVFLAG_AUTO_ACCEPT;
     *         UsageCapabilites |= IOC_LinkUsageCmdExecutor;
     *         SrvArgs.UsageArgs.pCmd->CbExecCmd_F = MyExecCb; // executes incoming commands
     *         OnAutoAccepted_F optional: attach per-link executor context/state.
     *       Client: Usage = IOC_LinkUsageCmdInitiator; issue commands via the command API.
     *
     *     - Service as CmdInitiator (service initiates):
     *         Flags include IOC_SRVFLAG_AUTO_ACCEPT;
     *         UsageCapabilites |= IOC_LinkUsageCmdInitiator;
     *         OnAutoAccepted_F(srv, link): optionally issue commands to 'link' immediately.
     *       Client: Usage = IOC_LinkUsageCmdExecutor; set ConnArgs.UsageArgs.pCmd->CbExecCmd_F.
     *
     *  4) EVT — publish/subscribe
     *     - Service as EvtProducer (service publishes):
     *         Use IOC_SRVFLAG_BROADCAST_EVENT to turn SrvID into a broadcast sender to all ConnLinkIDs
     *          IF OnAutoAccepted_F is set NULL, ELSE OnAutoAccepted_F(srv, link) will be called.
     *         AUTO_ACCEPT can still run for the main service links if you also expose DAT/CMD.
     *       Client: Usage = IOC_LinkUsageEvtConsumer; set ConnArgs.UsageArgs.pEvt->CbProcEvt_F to receive.
     *
     *     - Service as EvtConsumer (clients publish, service consumes):
     *         Flags include IOC_SRVFLAG_AUTO_ACCEPT;
     *         UsageCapabilites |= IOC_LinkUsageEvtConsumer;
     *         SrvArgs.UsageArgs.pEvt->CbProcEvt_F = MySrvEvtCb; // receive events from producers
     *         OnAutoAccepted_F optional: track producer link(s).
     *       Client: Usage = IOC_LinkUsageEvtProducer; publish events via the event API.
     */
    IOC_SRVFLAG_AUTO_ACCEPT = 1 << 1,

    /**
     * @brief KEEP_ACCEPTED_LINK — control accepted link lifecycle during service shutdown
     *
     * Default behavior (flag NOT set):
     *  - When IOC_offlineService() is called, ALL accepted links (SrvLinkIDs) are automatically closed
     *  - This prevents resource leaks and ensures clean shutdown
     *  - Callbacks registered on those links will no longer be invoked
     *  - Client-side links (CliLinkIDs) may become invalid/disconnected
     *
     * With IOC_SRVFLAG_KEEP_ACCEPTED_LINK:
     *  - Accepted links survive service shutdown
     *  - Links remain valid and functional even after service goes offline
     *  - Developer is responsible for manual link cleanup via IOC_closeLink()
     *  - Useful for advanced scenarios like service restart while preserving connections
     *
     * Example usage:
     *  - Without flag: Automatic cleanup, safe by default
     *    SrvArgs.Flags = IOC_SRVFLAG_AUTO_ACCEPT;
     *  - With flag: Manual cleanup required, advanced control
     *    SrvArgs.Flags = IOC_SRVFLAG_AUTO_ACCEPT | IOC_SRVFLAG_KEEP_ACCEPTED_LINK;
     */
    IOC_SRVFLAG_KEEP_ACCEPTED_LINK = 1 << 2,
} IOC_SrvFlags_T;

typedef struct {
    IOC_SrvURI_T SrvURI;
    IOC_SrvFlags_T Flags;

    /**
     * @brief what usage capabilities the service has, which means what new links can be accepted.
     *  SUCH AS:
     *      IF: UsageCapabilites = IOC_LINK_USAGE_EVT_PRODUCER | IOC_LINK_USAGE_CMD_EXECUTOR;
     *      THEN: Service can accept a new link with usage IOC_LINK_USAGE_EVT_CONSUMER or IOC_LINK_USAGE_CMD_INITIATOR.
     */
    IOC_LinkUsage_T UsageCapabilites;

    /**
     * @brief Usage-specific arguments for different service capabilities
     *        Since UsageCapabilites can be a combination of multiple capabilities,
     *        we use struct to allow providing arguments for each capability:
     *        - For IOC_LinkUsageEvtProducer: use EvtUsageArgs for event-related arguments
     *        - For IOC_LinkUsageCmdExecutor: use CmdUsageArgs for command-related arguments
     *        - For IOC_LinkUsageDatReceiver/DatSender: use DatUsageArgs for data transfer arguments
     *        - Set unused capability arguments to their default/NULL values
     */
    struct {
        IOC_EvtUsageArgs_pT pEvt;  // Event usage arguments (producer/consumer)
        IOC_CmdUsageArgs_pT pCmd;  // Command usage arguments (executor/initiator)
        IOC_DatUsageArgs_pT pDat;  // Data usage arguments (sender/receiver)
        void *pGeneric;            // Generic pointer for future extensions
    } UsageArgs;

    // IF: AUTO_ACCEPT flag is on and OnAutoAccepted_F is set,
    // WHEN: after a new client is accepted,
    // THEN: OnAutoAccepted_F will be called with the new LinkID
    IOC_CbOnAutoAccepted_F OnAutoAccepted_F;
    void *pSrvPriv;
} IOC_SrvArgs_T, *IOC_SrvArgs_pT;

static inline void IOC_Helper_initSrvArgs(IOC_SrvArgs_pT pSrvArgs) {
    if (pSrvArgs) {
        memset(pSrvArgs, 0, sizeof(IOC_SrvArgs_T));
    }
}

typedef struct {
    IOC_SrvURI_T SrvURI;
    /**
     * @brief Determines the usage capability of the Link created by IOC_connectService()
     *
     * ⚠️  **CRITICAL**: This Usage field directly determines what operations the resulting
     *                  LinkID can perform after IOC_connectService() succeeds:
     *
     *        - IOC_LinkUsageDatSender: Link can call IOC_sendDAT() to send data
     *        - IOC_LinkUsageDatReceiver: Link can call IOC_recvDAT() to receive data
     *        - IOC_LinkUsageEvtConsumer: Link can subscribe to events
     *        - IOC_LinkUsageEvtProducer: Link can publish events
     *        - IOC_LinkUsageCmdInitiator: Link can initiate commands
     *        - IOC_LinkUsageCmdExecutor: Link can execute commands
     *
     *  COMPATIBILITY REQUIREMENT:
     *      The service's UsageCapabilites must be compatible with this Usage:
     *      IF: Usage = IOC_LinkUsageDatSender;
     *      THEN: Service must have IOC_LinkUsageDatReceiver capability.
     *      IF: Usage = IOC_LinkUsageEvtConsumer;
     *      THEN: Service must have IOC_LinkUsageEvtProducer capability.
     *      AND so on for other usage types.
     *
     *  EXAMPLE:
     *      connArgs.Usage = IOC_LinkUsageDatSender;
     *      IOC_connectService(&linkID, &connArgs, NULL);
     *      // Now linkID can be used with IOC_sendDAT(linkID, ...)
     */
    IOC_LinkUsage_T Usage;

    /**
     * @brief Usage-specific arguments for different link usages
     *        The union member to use depends on the Usage flag:
     *        - For IOC_LinkUsageEvtConsumer/EvtProducer: use pEvtUsageArgs
     *        - For IOC_LinkUsageCmdExecutor/CmdInitiator: use pCmdUsageArgs
     *        - For IOC_LinkUsageDatReceiver/DatSender: use pDatUsageArgs
     */
    union {
        IOC_EvtUsageArgs_pT pEvt;  // Event usage arguments (consumer/producer)
        IOC_CmdUsageArgs_pT pCmd;  // Command usage arguments (executor/initiator)
        IOC_DatUsageArgs_pT pDat;  // Data usage arguments (receiver/sender)
        void *pGeneric;            // Generic pointer for future extensions
    } UsageArgs;
} IOC_ConnArgs_T, *IOC_ConnArgs_pT;

static inline void IOC_Helper_initConnArgs(IOC_ConnArgs_pT pConnArgs) {
    if (pConnArgs) {
        memset(pConnArgs, 0, sizeof(IOC_ConnArgs_T));
    }
}

#ifdef __cplusplus
}
#endif
#endif  //__IOC_TYPESRV_H__