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
 * @brief Establishes a client connection to an IOC service.
 *
 * This function creates a communication link between a client and an IOC service, enabling
 * data transfer, event messaging, or command execution based on the specified usage type.
 * The resulting link's capabilities are strictly determined by the pConnArgs->Usage field.
 *
 * @param[out] pLinkID    Pointer to store the resulting link identifier upon successful connection.
 *                        ⚠️  **CRITICAL**: The returned LinkID can ONLY be used for operations
 *                        matching its Usage capability:
 *                        - IOC_LinkUsageDatSender    → Can call IOC_sendDAT() only
 *                        - IOC_LinkUsageDatReceiver  → Can call IOC_recvDAT() only
 *                        - IOC_LinkUsageEvtConsumer  → Can subscribe to events via IOC_subEVT()
 *                        - IOC_LinkUsageEvtProducer  → Can publish events via IOC_postEVT()
 *                        - IOC_LinkUsageCmdInitiator → Can initiate commands via IOC_execCMD()
 *                        - IOC_LinkUsageCmdExecutor  → Can execute commands via callback
 *
 * @param[in]  pConnArgs  Connection arguments specifying service details and link usage.
 *                        **REQUIRED FIELDS**:
 *                        - SrvURI: Service address (protocol, host, path)
 *                        - Usage: Determines link capabilities (see pLinkID description)
 *
 *                        **OPTIONAL FIELDS**:
 *                        - UsageArgs: Usage-specific configuration
 *                          - .pEvt: Event callback/subscription config (for event usage)
 *                          - .pCmd: Command execution config (for command usage)
 *                          - .pDat: Data transfer config (for data usage)
 *
 *                        **SERVICE COMPATIBILITY**: The target service must support the
 *                        complementary usage capability:
 *                        | Client Usage             | Required Service Capability    |
 *                        |-------------------------|-------------------------------|
 *                        | IOC_LinkUsageDatSender   | IOC_LinkUsageDatReceiver      |
 *                        | IOC_LinkUsageDatReceiver | IOC_LinkUsageDatSender        |
 *                        | IOC_LinkUsageEvtConsumer | IOC_LinkUsageEvtProducer      |
 *                        | IOC_LinkUsageEvtProducer | IOC_LinkUsageEvtConsumer      |
 *                        | IOC_LinkUsageCmdInitiator| IOC_LinkUsageCmdExecutor      |
 *                        | IOC_LinkUsageCmdExecutor | IOC_LinkUsageCmdInitiator     |
 *
 *                        🚩 **FUTURE FEATURE - AUTO-SUBSCRIBE**: When Usage == IOC_LinkUsageEvtConsumer
 *                        and UsageArgs.pEvt is provided, the function may automatically subscribe
 *                        to specified events after connection. See UT_EventTypicalAutoSubscribe.cxx.
 *
 * @param[in]  pOption    Optional connection configuration. Pass NULL for default behavior.
 *                        **DEFAULT (pOption = NULL)**: SYNCHRONOUS mode where:
 *                        - Connection establishment blocks until completed
 *                        - Subsequent IOC_sendDAT()/IOC_recvDAT() calls block until completion
 *                        - Timeout behavior follows system defaults
 *
 *                        **CUSTOM OPTIONS**: Set IOC_Options to configure:
 *                        - Timeout values (IOC_OPTID_TIMEOUT)
 *                        - Asynchronous operation modes
 *                        - Protocol-specific parameters
 *
 * @retval IOC_RESULT_SUCCESS          Connection established successfully. Link created with
 *                                     capabilities matching pConnArgs->Usage.
 * @retval IOC_RESULT_INVALID_PARAM    Invalid arguments (NULL pointers, malformed URI, etc.)
 * @retval IOC_RESULT_CONNECTION_FAILED Unable to reach service or establish connection
 * @retval IOC_RESULT_INCOMPATIBLE_USAGE Service doesn't support the requested Usage type
 * @retval IOC_RESULT_POSIX_ENOMEM     Insufficient memory to create link object
 * @retval IOC_RESULT_TIMEOUT          Connection attempt timed out (in synchronous mode)
 * @retval IOC_RESULT_TOO_MANY_CLIENTS Service has reached maximum client limit
 *
 * @note **Thread Safety**: This function is thread-safe and supports concurrent connections.
 * @note **Resource Management**: Use IOC_closeLink() to properly release the connection.
 * @note **Performance**: For high-throughput scenarios, consider asynchronous mode via pOption.
 * @note **Debugging**: Use IOC_getLinkState() to verify connection state after successful connection.
 *
 * @warning **Protocol Dependency**: Connection behavior varies by protocol (FIFO, TCP, etc.).
 *          Some protocols may require the service to be online before connection attempts.
 *
 * @see IOC_onlineService() - Create a service that clients can connect to
 * @see IOC_acceptClient() - Server-side function to accept incoming connections
 * @see IOC_closeLink() - Close an established connection
 * @see IOC_getLinkState() - Check connection status and sub-states
 * @see IOC_SrvTypes.h - Data types and constants used in connection arguments
 *
 * @example Data Transfer Client (Sender)
 * @code
 * // Create a client that SENDS data to a service
 * IOC_LinkID_T senderLinkID;
 * IOC_ConnArgs_T connArgs = {};
 * IOC_Helper_initConnArgs(&connArgs);
 *
 * // Configure service location
 * connArgs.SrvURI.pProtocol = IOC_SRV_PROTO_FIFO;
 * connArgs.SrvURI.pHost = IOC_SRV_HOST_LOCAL_PROCESS;
 * connArgs.SrvURI.pPath = "data/processing/service";
 *
 * // Set usage: this link will SEND data
 * connArgs.Usage = IOC_LinkUsageDatSender;
 *
 * // Connect in default SYNC mode
 * IOC_Result_T result = IOC_connectService(&senderLinkID, &connArgs, NULL);
 * if (result == IOC_RESULT_SUCCESS) {
 *     // ✅ Now senderLinkID can SEND data
 *     IOC_DatDesc_T dataDesc = { .DatLen = strlen("Hello"), .pDat = "Hello" };
 *     IOC_sendDAT(senderLinkID, &dataDesc, NULL);  // This works
 *
 *     // ❌ senderLinkID CANNOT receive data
 *     // IOC_recvDAT(senderLinkID, &dataDesc, NULL);  // Would return error
 *
 *     IOC_closeLink(senderLinkID);  // Clean up
 * }
 * @endcode
 *
 * @example Event Consumer Client with Custom Callback
 * @code
 * // Create a client that CONSUMES events from a service
 * IOC_LinkID_T consumerLinkID;
 * IOC_ConnArgs_T connArgs = {};
 * IOC_Helper_initConnArgs(&connArgs);
 *
 * // Service that produces events
 * connArgs.SrvURI.pProtocol = IOC_SRV_PROTO_FIFO;
 * connArgs.SrvURI.pHost = IOC_SRV_HOST_LOCAL_PROCESS;
 * connArgs.SrvURI.pPath = "monitoring/alerts";
 * connArgs.Usage = IOC_LinkUsageEvtConsumer;  // This link will CONSUME events
 *
 * // Connect and then manually subscribe to specific events
 * IOC_Result_T result = IOC_connectService(&consumerLinkID, &connArgs, NULL);
 * if (result == IOC_RESULT_SUCCESS) {
 *     // Setup event subscription
 *     IOC_EvtID_T events[] = { IOC_EVTID_TEST_KEEPALIVE, IOC_EVTID_TEST_ERROR };
 *     IOC_SubEvtArgs_T subArgs = {
 *         .CbProcEvt_F = MyEventCallback,
 *         .pCbPrivData = &myPrivateData,
 *         .EvtNum = 2,
 *         .pEvtIDs = events
 *     };
 *
 *     IOC_subEVT(consumerLinkID, &subArgs);  // Subscribe to events
 *
 *     // Events will now be delivered to MyEventCallback()
 *     // ... application logic ...
 *
 *     IOC_closeLink(consumerLinkID);
 * }
 * @endcode
 *
 * @example Connection with Timeout Configuration
 * @code
 * // Connect with custom timeout settings
 * IOC_Options_T options = {};
 * options.IDs = IOC_OPTID_TIMEOUT;
 * options.Payload.TimeoutUS = 5000000;  // 5 second timeout
 *
 * IOC_ConnArgs_T connArgs = {};
 * IOC_Helper_initConnArgs(&connArgs);
 * connArgs.SrvURI.pProtocol = IOC_SRV_PROTO_FIFO;
 * connArgs.SrvURI.pHost = IOC_SRV_HOST_LOCAL_PROCESS;
 * connArgs.SrvURI.pPath = "slow/service";
 * connArgs.Usage = IOC_LinkUsageDatSender;
 *
 * IOC_LinkID_T linkID;
 * IOC_Result_T result = IOC_connectService(&linkID, &connArgs, &options);
 * if (result == IOC_RESULT_TIMEOUT) {
 *     printf("Service took too long to respond\n");
 * } else if (result == IOC_RESULT_SUCCESS) {
 *     printf("Connected successfully within timeout\n");
 *     IOC_closeLink(linkID);
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
