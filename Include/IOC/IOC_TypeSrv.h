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

    // TODO: char *pPath;
    // TODO: char *pQuery;
    // TODO: char *pFragment;
} IOC_SrvURI_T, *IOC_SrvURI_pT;

// TODO: #define IOC_SRV_PROTO_AUTO "auto"
// TODO: #define IOC_SRV_PROTO_TCP "tcp"
#define IOC_SRV_PROTO_UDP "udp"
// TODO: #define IOC_SRV_PROTO_HTTP "http"

#define IOC_SRV_HOST_LOOPBACK "Loopback"

typedef struct {
    IOC_SrvURI_T SrvURI;
} IOC_SrvArgs_T, *IOC_SrvArgs_pT;

typedef struct {
    IOC_SrvURI_T SrvURI;
} IOC_ConnArgs_T, *IOC_ConnArgs_pT;

#ifdef __cplusplus
}
#endif
#endif  //__IOC_TYPESRV_H__