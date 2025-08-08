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
// TODO: #define IOC_SRV_PROTO_TCP "tcp"
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
     * @brief AUTO_ACCEPT flag for automatic connection acceptance
     *  WHEN service is onlined with AUTO_ACCEPT flag,
     *      incoming client connections are automatically accepted without manual IOC_acceptClient() calls.
     *  This is useful for DAT services and other service types that want to automatically handle connections.
     *
     *  BEHAVIOR:
     *    - Automatically starts a daemon thread to accept incoming connections
     *    - Works with any service type (DAT, Event, Command, etc.)
     *    - No manual IOC_acceptClient() required, but callback in each UsageArgs must be set, such as:
     *        - IOC_DatUsageArgs_T.CbRecvDat_F for service as DatReceiver
     *        - IOC_EvtUsageArgs_T.CbProcEvt_F for service as EvtConsumer
     *        - IOC_CmdUsageArgs_T.CbExecCmd_F for service as CmdExecutor
     *    - Connections are accepted in the order they arrive
     *
     *  RESTRITIONS:
     *.   When AutoAcceptFlagOn, service MUST be as DatReciver|EvtConsumer|CmdExecutor in callback mode.
     */
    IOC_SRVFLAG_AUTO_ACCEPT = 1 << 1,
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