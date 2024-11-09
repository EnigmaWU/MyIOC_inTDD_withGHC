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

typedef uint64_t IOC_SrvID_T;
typedef IOC_SrvID_T *IOC_SrvID_pT;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//===>URI: Uniform Resource Identifier, defined in RFC 3986.
// Standard URI: scheme:[//[user:password@]host[:port]][/]path[?query][#fragment]
// Service use URI to identify what is onlined.
// Client use URI to connect to the service.
typedef struct {
    union {
        char *pScheme;
        char *pProtocol;  // e.g. "auto", "tcp", "udp", "http", "https", "ftp"
    };

    // TODO: char *pUser;
    // TODO: char *pPwd;

    char *pHost;
    uint16_t Port;
    char *pPath;

    // TODO: char *pQuery;
    // TODO: char *pFragment;
} IOC_SrvURI_T, *IOC_SrvURI_pT;

#define IOC_SRV_PROTO_AUTO "auto"  // transport protocol is auto selected by IOC
// TODO: #define IOC_SRV_PROTO_TCP "tcp"
// TODO: #define IOC_SRV_PROTO_UDP "udp"
// TODO: #define IOC_SRV_PROTO_HTTP "http"

#define IOC_SRV_HOST_LOOPBACK "Loopback"

typedef enum {
    IOC_SRVFLAG_NONE = 0,

    /**
     * @brief BROADCAST vs P2P==Link2Link
     *  P2P means Point to Point, which is a direct link between two objects.
     *  WHEN service online on server side, it has a SrvID, and MAY get LinkID from this SrvID,
     *      we name this LinkID as SrvLinkID.
     *    WHEN client connect, it has a pair of LinkID both in client and server side,
     *      we name these LinkIDs as ConnLinkID and AcptLinkID.
     *  ByDefault: Srvice is P2P,
     *      we use AcptLinkID and ConnLinkID to communicate.
     *  If service is onlined with BROADCAST flag on,
     *      we use SrvLinkID and ConnLinkID to communicate.
     *
     *  Which means:
     *    <DFT/P2P> AcptLinkID <--> ConnLinkID
     *          e.g. postEVT(AcptLinkID) --> ONLY ConnLinkID will CbProcEvt
     *          e.g. postEVT(ConnLinkID) --> ONLY AcptLinkID will CbProcEvt
     *    <BROADCAST> SrvLinkID <--> ConnLinkIDs
     *          e.g. postEVT(SrvLinkID) --> ALL ConnLinkIDs will CbProcEvt
     *          e.g. postEVT(ConnLinkID) --> SrvLinkID and ALL OTHER ConnLinkIDs will CbProcEvt
     */
    // TODO: IOC_SRVFLAG_BROADCAST = 1 << 0,
} IOC_SrvFlags_T;

typedef struct {
    IOC_SrvURI_T SrvURI;
    IOC_SrvFlags_T Flags;
} IOC_SrvArgs_T, *IOC_SrvArgs_pT;

typedef struct {
    IOC_SrvURI_T SrvURI;
} IOC_ConnArgs_T, *IOC_ConnArgs_pT;

#ifdef __cplusplus
}
#endif
#endif  //__IOC_TYPESRV_H__