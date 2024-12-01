/**
 * @file IOC_SrvAPI.h
 * @brief IOC's Service APIs defined here.
 *
 */

#include "IOC_EvtDesc.h"
#include "IOC_Option.h"
#include "IOC_SrvTypes.h"

#ifndef __IOC_SRV_API_H__
#define __IOC_SRV_API_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initiates the IOC online service.
 *
 * This function starts the online service for the IOC module.
 *
 * @param[out] pSrvID Pointer to store the service ID.
 * @param[in] pSrvArgs Pointer to the service arguments.
 * @return IOC_Result_T Status of the operation.
 */
IOC_Result_T IOC_onlineService(
    /*ARG_OUT */ IOC_SrvID_pT pSrvID,
    /*ARG_IN*/ const IOC_SrvArgs_pT pSrvArgs);
/**
 * @brief Takes the specified IOC service offline.
 *
 * This function initiates the process to bring the IOC service identified
 * by the provided service ID offline. It ensures that all necessary
 * shutdown procedures are performed to safely disable the service.
 *
 * @param SrvID The identifier of the IOC service to be taken offline.
 * @return IOC_Result_T The result of the offline operation, indicating success or failure.
 */
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

/**
 * @brief Accepts a client connection for a specified service.
 *
 * @param[in] SrvID The identifier of the service to accept the client for.
 * @param[out] pLinkID Pointer to store the resulting link identifier.
 * @param[in, optional] pOption Optional parameters for accepting the client.
 *
 * @return IOC_Result_T Indicates success or failure of the operation.
 */
IOC_Result_T IOC_acceptClient(
    /*ARG_IN*/ IOC_SrvID_T SrvID,
    /*ARG_OUT*/ IOC_LinkID_pT pLinkID,
    /*ARG_IN_OPTIONAL*/ const IOC_Options_pT pOption);

/**
 * @brief Establishes a connection to the IOC service.
 *
 * @param[out] pLinkID    Pointer to store the resulting link identifier upon successful connection.
 * @param[in]  pConnArgs  Pointer to the connection arguments required for establishing the service.
 * @param[in]  pOption    (Optional) Pointer to additional options for configuring the connection.
 *
 * @return IOC_Result_T   The result of the connection attempt, indicating success or the type of failure.
 */
IOC_Result_T IOC_connectService(
    /*ARG_OUT*/ IOC_LinkID_pT pLinkID,
    /*ARG_IN*/ const IOC_ConnArgs_pT pConnArgs,
    /*ARG_IN_OPTIONAL*/ const IOC_Options_pT pOption);

/**
 * @brief Closes the specified IOC link.
 *
 * @param LinkID The identifier of the link to be closed.
 * @return The result of the close operation.
 */
IOC_Result_T IOC_closeLink(
    /*ARG_IN*/ IOC_LinkID_T LinkID);

#if 0
//TODO: IF Service onlined with IOC_SRVFLAG_BROADCAST
// THEN broadcast to SrvID means broadcast to all accpeted Links of this Service.
IOC_Result_T IOC_broadcastEVT(
    /*ARG_IN*/ IOC_SrvID_T SrvID,
    /*ARG_IN*/ const IOC_EvtDesc_pT pEvtDesc,
    /*ARG_IN_OPTIONAL*/ const IOC_Options_pT pOption);
#endif

#ifdef __cplusplus
}
#endif
#endif  //__IOC_SRV_API_H__
