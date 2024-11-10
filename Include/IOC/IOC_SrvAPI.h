/**
 * @file IOC_SrvAPI.h
 * @brief IOC's Service APIs defined here.
 *
 */

#include "IOC_SrvTypes.h"
#include "IOC_TypeOption.h"

#ifndef __IOC_SRV_API_H__
#define __IOC_SRV_API_H__
#ifdef __cplusplus
extern "C" {
#endif

IOC_Result_T IOC_onlineService(
    /*ARG_OUT */ IOC_SrvID_pT pSrvID,
    /*ARG_IN*/ const IOC_SrvArgs_pT pSrvArgs);
IOC_Result_T IOC_offlineService(
    /*ARG_IN*/ IOC_SrvID_T SrvID);

#if 0
IOC_Result_T IOC_setSrvParam(
    /*ARG_IN*/ IOC_SrvID_T SrvID,
    /*ARG_IN*/ const IOC_SrvParam_pT pSrvParam);
IOC_Result_T IOC_getSrvParam(
    /*ARG_IN*/ IOC_SrvID_T SrvID,
    /*ARG_OUT*/ IOC_SrvParam_pT pSrvParam);
#endif

IOC_Result_T IOC_acceptClient(
    /*ARG_IN*/ IOC_SrvID_T SrvID,
    /*ARG_OUT*/ IOC_LinkID_pT pLinkID,
    /*ARG_IN_OPTIONAL*/ const IOC_Options_pT pOption);

IOC_Result_T IOC_connectService(
    /*ARG_OUT*/ IOC_LinkID_pT pLinkID,
    /*ARG_IN*/ const IOC_ConnArgs_pT pConnArgs,
    /*ARG_IN_OPTIONAL*/ const IOC_Options_pT pOption);

IOC_Result_T IOC_closeLink(
    /*ARG_IN*/ IOC_LinkID_T LinkID);

#ifdef __cplusplus
}
#endif
#endif  //__IOC_SRV_API_H__
