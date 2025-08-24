#include "IOC/IOC_CmdAPI.h"
#include "_IOC.h"

//=================================================================================================
// Command Processing Queue and Threading
//=================================================================================================

#define _MAX_PENDING_COMMANDS_PER_LINK 16
#define _COMMAND_QUEUE_TIMEOUT_MS 5000

// Command execution context for threading
typedef struct {
    IOC_CmdDesc_T CmdDesc;
    _IOC_LinkObject_pT pSrcLink;   // Source link (command sender)
    _IOC_LinkObject_pT pDestLink;  // Destination link (command executor)
    pthread_cond_t CompletionCond;
    pthread_mutex_t CompletionMutex;
    bool IsCompleted;
    IOC_Result_T ExecutionResult;
} _IOC_CommandContext_T, *_IOC_CommandContext_pT;

// Simple command queue per link
typedef struct {
    _IOC_CommandContext_T PendingCommands[_MAX_PENDING_COMMANDS_PER_LINK];
    int QueueHead;
    int QueueTail;
    int QueueCount;
    pthread_mutex_t QueueMutex;
    pthread_cond_t QueueCond;
} _IOC_CommandQueue_T, *_IOC_CommandQueue_pT;

// Global command queue table (indexed by LinkID)
static _IOC_CommandQueue_T _mIOC_CmdQueues[256] = {};  // Simplified fixed-size array
static bool _mIOC_CmdQueuesInitialized = false;
static pthread_mutex_t _mIOC_CmdQueuesMutex = PTHREAD_MUTEX_INITIALIZER;

//=================================================================================================
// Command Queue Management
//=================================================================================================

static void __IOC_initCommandQueues() {
    if (_mIOC_CmdQueuesInitialized) return;

    pthread_mutex_lock(&_mIOC_CmdQueuesMutex);
    if (!_mIOC_CmdQueuesInitialized) {
        for (int i = 0; i < 256; i++) {
            pthread_mutex_init(&_mIOC_CmdQueues[i].QueueMutex, NULL);
            pthread_cond_init(&_mIOC_CmdQueues[i].QueueCond, NULL);
            _mIOC_CmdQueues[i].QueueHead = 0;
            _mIOC_CmdQueues[i].QueueTail = 0;
            _mIOC_CmdQueues[i].QueueCount = 0;
        }
        _mIOC_CmdQueuesInitialized = true;
    }
    pthread_mutex_unlock(&_mIOC_CmdQueuesMutex);
}

static _IOC_CommandQueue_pT __IOC_getCommandQueue(IOC_LinkID_T LinkID) {
    __IOC_initCommandQueues();
    return &_mIOC_CmdQueues[LinkID % 256];  // Simple hash for demo
}

//=================================================================================================
// Command Execution Logic
//=================================================================================================

static IOC_Result_T __IOC_executeCommandViaCallback(_IOC_LinkObject_pT pDestLink, IOC_CmdDesc_pT pCmdDesc,
                                                    _IOC_CommandContext_pT pCmdContext) {
    // Find the command executor callback in the destination link's usage args
    IOC_CmdUsageArgs_pT pCmdUsageArgs = NULL;

    if (pDestLink->Args.Usage == IOC_LinkUsageCmdExecutor && pDestLink->Args.UsageArgs.pCmd != NULL) {
        pCmdUsageArgs = pDestLink->Args.UsageArgs.pCmd;
    }

    if (!pCmdUsageArgs || !pCmdUsageArgs->CbExecCmd_F) {
        return IOC_RESULT_NO_CMD_EXECUTOR;
    }

    // Check if this command ID is supported
    bool CmdSupported = false;
    for (int i = 0; i < pCmdUsageArgs->CmdNum; i++) {
        if (pCmdUsageArgs->pCmdIDs[i] == pCmdDesc->CmdID) {
            CmdSupported = true;
            break;
        }
    }

    if (!CmdSupported) {
        return IOC_RESULT_NOT_SUPPORT;
    }

    // Execute the command via callback
    pCmdDesc->Status = IOC_CMD_STATUS_PROCESSING;
    IOC_Result_T CallbackResult = pCmdUsageArgs->CbExecCmd_F(pDestLink->ID, pCmdDesc, pCmdUsageArgs->pCbPrivData);

    return CallbackResult;
}

static IOC_Result_T __IOC_findDestinationLink(IOC_LinkID_T SrcLinkID, _IOC_LinkObject_pT *ppDestLink) {
    // For this simple implementation, we need to find the "other end" of the connection
    // In a real implementation, this would involve protocol-specific link mapping

    _IOC_LinkObject_pT pSrcLink = _IOC_getLinkObjByLinkID(SrcLinkID);
    if (!pSrcLink) {
        return IOC_RESULT_NOT_EXIST_LINK;
    }

    // DEBUG: Print source link information
    printf("[DEBUG] Finding dest link for SrcLinkID=%llu, Usage=%d, Path='%s'\n", (unsigned long long)SrcLinkID,
           pSrcLink->Args.Usage, pSrcLink->Args.SrvURI.pPath);

    // DEBUG: Scan ALL links to see what exists
    printf("[DEBUG] Scanning all possible links:\n");
    for (IOC_LinkID_T scanID = 1; scanID < 1100; scanID++) {
        _IOC_LinkObject_pT pScanLink = _IOC_getLinkObjByLinkID(scanID);
        if (pScanLink) {
            printf("[DEBUG]   LinkID=%llu, Usage=%d, Path='%s'\n", (unsigned long long)scanID, pScanLink->Args.Usage,
                   pScanLink->Args.SrvURI.pPath);
        }
    }

    // Simple approach: scan all possible link IDs to find the connected peer
    // In practice, this should be maintained by the service/connection infrastructure
    for (IOC_LinkID_T candidateID = 1; candidateID < 1100; candidateID++) {
        if (candidateID == SrcLinkID) continue;

        _IOC_LinkObject_pT pCandidateLink = _IOC_getLinkObjByLinkID(candidateID);
        if (!pCandidateLink) continue;

        printf("[DEBUG] Checking candidate LinkID=%llu, Usage=%d, Path='%s'\n", (unsigned long long)candidateID,
               pCandidateLink->Args.Usage, pCandidateLink->Args.SrvURI.pPath);

        // Check if this link has complementary usage (CmdInitiator↔CmdExecutor)
        bool IsValidPeer = false;

        if (pSrcLink->Args.Usage == IOC_LinkUsageCmdInitiator &&
            pCandidateLink->Args.Usage == IOC_LinkUsageCmdExecutor) {
            IsValidPeer = true;
        } else if (pSrcLink->Args.Usage == IOC_LinkUsageCmdExecutor &&
                   pCandidateLink->Args.Usage == IOC_LinkUsageCmdInitiator) {
            IsValidPeer = true;
        }

        if (IsValidPeer) {
            // Additional check: ensure they're connected to the same service endpoint
            // by comparing the SrvURI paths (simplified check)
            if (strcmp(pSrcLink->Args.SrvURI.pPath, pCandidateLink->Args.SrvURI.pPath) == 0) {
                printf("[DEBUG] Found valid peer: LinkID=%llu\n", (unsigned long long)candidateID);
                *ppDestLink = pCandidateLink;
                return IOC_RESULT_SUCCESS;
            }
        }
    }

    printf("[DEBUG] No valid peer found for SrcLinkID=%llu\n", (unsigned long long)SrcLinkID);
    return IOC_RESULT_NO_CMD_EXECUTOR;
}

//=================================================================================================
// Legacy Command Implementation (Fallback)
//=================================================================================================

/**
 * @brief Legacy command execution implementation (bypass protocol layer)
 * @param LinkID Source link ID (CmdInitiator)
 * @param pCmdDesc Command descriptor
 * @param pOption Execution options
 * @return IOC_RESULT_SUCCESS on successful execution
 *
 * @note This is the original implementation that bypasses the protocol layer.
 *       Used as fallback when protocol doesn't implement OpExecCmd_F.
 */
static IOC_Result_T __IOC_execCMD_legacy(IOC_LinkID_T LinkID, IOC_CmdDesc_pT pCmdDesc, IOC_Options_pT pOption) {
    // Get source link
    _IOC_LinkObject_pT pSrcLink = _IOC_getLinkObjByLinkID(LinkID);
    if (!pSrcLink) {
        return IOC_RESULT_NOT_EXIST_LINK;
    }

    // Find destination link (the command executor)
    _IOC_LinkObject_pT pDestLink = NULL;
    IOC_Result_T FindResult = __IOC_findDestinationLink(LinkID, &pDestLink);
    if (FindResult != IOC_RESULT_SUCCESS) {
        return FindResult;
    }

    // Create command execution context
    _IOC_CommandContext_T CmdContext = {};
    memcpy(&CmdContext.CmdDesc, pCmdDesc, sizeof(IOC_CmdDesc_T));
    CmdContext.pSrcLink = pSrcLink;
    CmdContext.pDestLink = pDestLink;
    CmdContext.IsCompleted = false;
    CmdContext.ExecutionResult = IOC_RESULT_BUG;

    pthread_mutex_init(&CmdContext.CompletionMutex, NULL);
    pthread_cond_init(&CmdContext.CompletionCond, NULL);

    // Execute command synchronously via callback
    IOC_Result_T ExecResult = __IOC_executeCommandViaCallback(pDestLink, &CmdContext.CmdDesc, &CmdContext);

    // Copy results back to caller's CmdDesc
    if (ExecResult == IOC_RESULT_SUCCESS) {
        // Copy the modified command descriptor back
        memcpy(pCmdDesc, &CmdContext.CmdDesc, sizeof(IOC_CmdDesc_T));
    }

    // Cleanup
    pthread_mutex_destroy(&CmdContext.CompletionMutex);
    pthread_cond_destroy(&CmdContext.CompletionCond);

    return ExecResult;
}

//=================================================================================================
// Public Command API Implementation
//=================================================================================================

IOC_Result_T IOC_execCMD(IOC_LinkID_T LinkID, IOC_CmdDesc_pT pCmdDesc, IOC_Options_pT pOption) {
    if (!pCmdDesc) {
        return IOC_RESULT_INVALID_PARAM;
    }

    // Get source link
    _IOC_LinkObject_pT pSrcLink = _IOC_getLinkObjByLinkID(LinkID);
    if (!pSrcLink) {
        return IOC_RESULT_NOT_EXIST_LINK;
    }

    // Verify that source link can initiate commands
    if (pSrcLink->Args.Usage != IOC_LinkUsageCmdInitiator) {
        return IOC_RESULT_INVALID_PARAM;
    }

    // 🚀 PROPER ARCHITECTURE: Delegate to protocol-specific implementation
    // This is how the layered architecture should work:
    // IOC_execCMD() → pLink->pMethods->OpExecCmd_F() → __IOC_execCmd_ofProtoFifo()
    //
    // 🔧 BENEFITS:
    // - Protocol abstraction: Different protocols can implement commands differently
    // - Extensibility: New protocols just implement the OpExecCmd_F method
    // - Consistency: All IOC APIs follow the same delegation pattern
    if (!pSrcLink->pMethods || !pSrcLink->pMethods->OpExecCmd_F) {
        // Fallback: Protocol doesn't implement commands, use legacy direct approach
        return __IOC_execCMD_legacy(LinkID, pCmdDesc, pOption);
    }

    // Use protocol-specific command execution
    return pSrcLink->pMethods->OpExecCmd_F(pSrcLink, pCmdDesc, pOption);
}

IOC_Result_T IOC_waitCMD(IOC_LinkID_T LinkID, IOC_CmdDesc_pT pCmdDesc, IOC_Options_pT pOption) {
    if (!pCmdDesc) {
        return IOC_RESULT_INVALID_PARAM;
    }

    _IOC_LinkObject_pT pLink = _IOC_getLinkObjByLinkID(LinkID);
    if (!pLink) {
        return IOC_RESULT_NOT_EXIST_LINK;
    }

    if (pLink->Args.Usage != IOC_LinkUsageCmdExecutor) {
        return IOC_RESULT_INVALID_PARAM;
    }

    // 🚀 PROPER ARCHITECTURE: Delegate to protocol-specific implementation
    if (!pLink->pMethods || !pLink->pMethods->OpWaitCmd_F) {
        // Protocol doesn't implement command waiting - return NOT_SUPPORT
        return IOC_RESULT_NOT_SUPPORT;
    }

    // Use protocol-specific command waiting
    return pLink->pMethods->OpWaitCmd_F(pLink, pCmdDesc, pOption);
}

IOC_Result_T IOC_ackCMD(IOC_LinkID_T LinkID, IOC_CmdDesc_pT pCmdDesc, IOC_Options_pT pOption) {
    if (!pCmdDesc) {
        return IOC_RESULT_INVALID_PARAM;
    }

    _IOC_LinkObject_pT pLink = _IOC_getLinkObjByLinkID(LinkID);
    if (!pLink) {
        return IOC_RESULT_NOT_EXIST_LINK;
    }

    // 🚀 PROPER ARCHITECTURE: Delegate to protocol-specific implementation
    if (!pLink->pMethods || !pLink->pMethods->OpAckCmd_F) {
        // Protocol doesn't implement command acknowledgment - return NOT_SUPPORT
        return IOC_RESULT_NOT_SUPPORT;
    }

    // Use protocol-specific command acknowledgment
    return pLink->pMethods->OpAckCmd_F(pLink, pCmdDesc, pOption);
}
