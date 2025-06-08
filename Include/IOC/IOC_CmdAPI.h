#include "IOC_CmdDesc.h"
#include "IOC_Option.h"
#include "IOC_SrvTypes.h"

#ifndef __IOC_COMMAND_API_H__
#define __IOC_COMMAND_API_H__
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
 * @brief Command executor callback arguments
 *    WHEN: onlineService with UsageCapabilities::CmdExecutor,
 *      THEN: IOC_CmdExecArgs_T in IOC_SrvArgs_T::pCmdExecArgs which indicates autoAcceptCmdInitiatorClient
 *    WHEN: connectService with Usage::CmdExecutor,
 *      THEN: IOC_CmdExecArgs_T in IOC_ConnArgs_T::pCmdExecArgs
 */
typedef struct {
    IOC_CbExecCmd_F CbExecCmd_F;  // Callback function for executing commands
    void *pCbPrivData;            // Callback private context data

    ULONG_T CmdNum;        // Number of CmdIDs, IOC_calcArrayElmtCnt(pCmdIDs)
    IOC_CmdID_T *pCmdIDs;  // Array of CmdIDs that this executor handles

} IOC_CmdExecArgs_T, *IOC_CmdExecArgs_pT;

/**
 * @brief Execute a command on the specified link
 *        CmdInitiator calls this API to send a command to CmdExecutor and get result synchronously
 *
 * @param LinkID: the link ID between CmdInitiator and CmdExecutor
 *     RefMore: README_ArchDesign::Object::Link
 * @param pCmdDesc: the command description. IOC will COPY-IN command parameters and COPY-OUT results
 *     RefMore: README_ArchDesign::Concept::MSG::CMD
 * @param pOption: the options for this execCMD
 *     Supported options: IOC_OPTID_TIMEOUT, IOC_OPTID_BLOCKING_MODE, IOC_OPTID_RELIABILITY_MODE
 *
 * @return IOC_RESULT_SUCCESS: command executed successfully
 * @return IOC_RESULT_NO_CMD_EXECUTOR: no CmdExecutor registered for this command
 * @return IOC_RESULT_BUSY: CmdExecutor is busy (when immediate NONBLOCK mode)
 * @return IOC_RESULT_TIMEOUT: command execution timeout
 * @return IOC_RESULT_CMD_EXEC_FAILED: CmdExecutor returned failure
 * @return IOC_RESULT_LINK_BROKEN: communication link is broken during execution
 * @return IOC_RESULT_INVALID_PARAM: invalid parameters
 * @return IOC_RESULT_NOT_EXIST_LINK: LinkID does not exist or already closed
 *
 * RefUT: UT_ConetCmdXXX
 */
IOC_Result_T IOC_execCMD(IOC_LinkID_T LinkID, IOC_CmdDesc_pT pCmdDesc, IOC_Options_pT pOption);

/**
 * @brief Wait for a command on the specified link (polling mode)
 *        CmdExecutor calls this API to actively wait for commands when using polling mode
 *
 * @param LinkID: the link ID to wait for commands
 * @param pCmdDesc: pointer to command description buffer to receive command details
 * @param pOption: the options for this waitCMD
 *     Supported options: IOC_OPTID_TIMEOUT
 *
 * @return IOC_RESULT_SUCCESS: command received successfully
 * @return IOC_RESULT_TIMEOUT: wait timeout (when timeout configured)
 * @return IOC_RESULT_LINK_BROKEN: communication link is broken
 * @return IOC_RESULT_INVALID_PARAM: invalid parameters
 * @return IOC_RESULT_NOT_EXIST_LINK: LinkID does not exist or already closed
 *
 * RefUT: UT_ConetCmdPollingXXX
 */
IOC_Result_T IOC_waitCMD(IOC_LinkID_T LinkID, IOC_CmdDesc_pT pCmdDesc, IOC_Options_pT pOption);

/**
 * @brief Acknowledge a command execution (explicit response mode)
 *        CmdExecutor calls this API to send command response separately from callback
 *
 * @param LinkID: the link ID where the command was received
 * @param pCmdDesc: the command description with execution results
 * @param pOption: the options for this ackCMD
 *     Supported options: IOC_OPTID_TIMEOUT
 *
 * @return IOC_RESULT_SUCCESS: command response sent successfully
 * @return IOC_RESULT_ACK_CMD_FAILED: failed to send command response
 * @return IOC_RESULT_TIMEOUT: ack timeout
 * @return IOC_RESULT_LINK_BROKEN: communication link is broken
 * @return IOC_RESULT_INVALID_PARAM: invalid parameters
 * @return IOC_RESULT_NOT_EXIST_LINK: LinkID does not exist or already closed
 *
 * RefUT: UT_ConetCmdExplicitAckXXX
 */
IOC_Result_T IOC_ackCMD(IOC_LinkID_T LinkID, IOC_CmdDesc_pT pCmdDesc, IOC_Options_pT pOption);

// TODO: IOC_cancelCMD(IOC_LinkID_T LinkID, ...)
// TODO: IOC_queryCMD(IOC_LinkID_T LinkID, ...)
// TODO: IOC_setCmdLinkParams(IOC_LinkID_T LinkID, ...)

#ifdef __cplusplus
}
#endif
#endif  // __IOC_COMMAND_API_H__
