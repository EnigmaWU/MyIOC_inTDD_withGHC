/**
 * @file IOC_TypeSrv.h
 * @brief IOC's Service Types defined here.
 *
 */

#include "IOC_Types.h"

#ifndef __IOC_TYPESRV_H__
#define __IOC_TYPESRV_H__
#ifdef __cplusplus
extern "C" {
#endif

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
} IOC_SrvArgs_T, *IOC_SrvArgs_pT;

typedef struct {
    IOC_SrvURI_T SrvURI;
    /**
     * @brief what this link is used for, which means what the link can do.
     *  SUCH AS:
     *      IF: Usage = IOC_LINK_USAGE_EVT_CONSUMER;
     *      THEN: Link can only subscribe events.
     *      AND: Service must have IOC_LINK_USAGE_EVT_PRODUCER usage capability.
     */
    IOC_LinkUsage_T Usage;
} IOC_ConnArgs_T, *IOC_ConnArgs_pT;

#ifdef __cplusplus
}
#endif
#endif  //__IOC_TYPESRV_H__