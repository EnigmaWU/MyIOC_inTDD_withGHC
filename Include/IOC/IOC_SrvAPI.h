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
 * This function connects to an IOC service and creates a communication link for data transfer.
 * The connection behavior can be configured through the pOption parameter.
 *
 * @param[out] pLinkID    Pointer to store the resulting link identifier upon successful connection.
 *                        🎯 **LINK USAGE**: The returned LinkID's capabilities are determined by
 *                        pConnArgs->Usage field:
 *                        - IOC_LinkUsageDatSender → LinkID can call IOC_sendDAT()
 *                        - IOC_LinkUsageDatReceiver → LinkID can call IOC_recvDAT()
 *                        - IOC_LinkUsageEvtConsumer → LinkID can subscribe to events
 *                        - IOC_LinkUsageEvtProducer → LinkID can publish events
 *                        This LinkID will be used for subsequent operations matching its usage.
 *
 * @param[in]  pConnArgs  Pointer to the connection arguments required for establishing the service.
 *                        🔑 **KEY FIELD**: pConnArgs->Usage determines what the resulting Link can do!
 *                        Contains service URI, usage type, and other connection parameters.
 *                        The service must have compatible UsageCapabilites for connection to succeed.
 *
 * @param[in]  pOption    (Optional) Pointer to additional options for configuring the connection.
 *                        ⚠️  **DEFAULT BEHAVIOR**: When pOption is NULL, the connection operates in
 *                        **SYNCHRONOUS MODE** by default, meaning:
 *                        - IOC_sendDAT() calls will BLOCK until data is transmitted
 *                        - IOC_recvDAT() calls will BLOCK until data is received
 *                        - Connection establishment will BLOCK until completed
 *
 *                        To enable ASYNCHRONOUS MODE, pass valid IOC_Options with appropriate flags.
 *
 * @return IOC_Result_T   The result of the connection attempt, indicating success or the type of failure.
 *                        - IOC_RESULT_SUCCESS: Connection established successfully, NEW Link created
 *                                              with capabilities determined by pConnArgs->Usage
 *                        - IOC_RESULT_INVALID_ARG: Invalid arguments provided
 *                        - IOC_RESULT_CONNECTION_FAILED: Unable to connect to service
 *                        - IOC_RESULT_INCOMPATIBLE_USAGE: Service doesn't support requested Usage
 *                        - IOC_RESULT_TIMEOUT: Connection attempt timed out (in sync mode)
 *                        - Other error codes as defined in IOC_Result_T
 *
 * @note 🎯 Link Usage: The resulting Link's usage capabilities are determined by pConnArgs->Usage
 * @note Default Connection Mode: SYNCHRONOUS (when pOption = NULL)
 * @note For performance testing: Consider async mode for high-throughput scenarios
 * @note Thread Safety: This function is thread-safe and can be called concurrently
 *
 * @example Link Usage Examples
 * @code
 * // Example 1: Create a SENDER link for data transmission
 * IOC_LinkID_T senderLinkID;
 * IOC_ConnArgs_T connArgs = {};
 * IOC_Helper_initConnArgs(&connArgs);
 * connArgs.SrvURI.pProtocol = IOC_SRV_PROTO_FIFO;
 * connArgs.SrvURI.pHost = IOC_SRV_HOST_LOCAL_PROCESS;
 * connArgs.SrvURI.pPath = "test/data/service";
 * connArgs.Usage = IOC_LinkUsageDatSender;  // 🎯 This determines Link capability
 *
 * // SYNC mode (default) - operations will block
 * IOC_Result_T result = IOC_connectService(&senderLinkID, &connArgs, NULL);
 * if (result == IOC_RESULT_SUCCESS) {
 *     // ✅ Now senderLinkID can SEND data (because Usage = DatSender)
 *     IOC_sendDAT(senderLinkID, &datDesc, NULL);
 *     // ❌ But senderLinkID CANNOT receive data
 *     // IOC_recvDAT(senderLinkID, &datDesc, NULL);  // This would fail
 * }
 *
 * // Example 2: Create a RECEIVER link for data reception
 * IOC_LinkID_T receiverLinkID;
 * connArgs.Usage = IOC_LinkUsageDatReceiver;  // 🎯 Different usage = different capability
 * result = IOC_connectService(&receiverLinkID, &connArgs, NULL);
 * if (result == IOC_RESULT_SUCCESS) {
 *     // ✅ Now receiverLinkID can RECEIVE data (because Usage = DatReceiver)
 *     IOC_recvDAT(receiverLinkID, &datDesc, NULL);
 *     // ❌ But receiverLinkID CANNOT send data
 *     // IOC_sendDAT(receiverLinkID, &datDesc, NULL);  // This would fail
 * }
 * @endcode
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
