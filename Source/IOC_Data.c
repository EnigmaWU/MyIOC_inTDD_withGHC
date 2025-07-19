/**
 * @file IOC_Data.c
 * @brief Implementation of IOC Data Transfer APIs
 * @details Provides data streaming capabilities with NODROP guarantee
 */

#include "_IOC.h"

/**
 * @brief Send data chunk on the specified link
 * @param LinkID: the link ID to send data on
 * @param pDatDesc: pointer to data description with payload
 * @param pOption: optional parameters (can be NULL)
 * @return IOC_RESULT_SUCCESS: data sent successfully
 * @return IOC_RESULT_INVALID_PARAM: invalid parameters
 * @return IOC_RESULT_NOT_EXIST_LINK: LinkID does not exist
 */
IOC_Result_T IOC_sendDAT(IOC_LinkID_T LinkID, IOC_DatDesc_pT pDatDesc, IOC_Options_pT pOption) {
    // ════════════════════════════════════════════════════════════════════════════════════════
    // 🥇 PHASE 1: LinkID Validation (HIGHEST PRECEDENCE)
    // Rationale: Validate resource exists before processing any data or configuration
    // Security: Prevents data processing on invalid/unauthorized connections
    // Performance: Fail fast on connection issues
    // ════════════════════════════════════════════════════════════════════════════════════════

    // Check for invalid LinkID constant
    if (LinkID == IOC_ID_INVALID) {
        return IOC_RESULT_NOT_EXIST_LINK;
    }

    // Get sender link object (validates LinkID exists and is accessible)
    _IOC_LinkObject_pT pSenderLinkObj = _IOC_getLinkObjByLinkID(LinkID);
    if (!pSenderLinkObj) {
        return IOC_RESULT_NOT_EXIST_LINK;
    }

    // ════════════════════════════════════════════════════════════════════════════════════════
    // 🥈 PHASE 2: DatDesc Parameter Validation (SECOND PRECEDENCE)
    // Rationale: Validate data parameters after confirming connection exists
    // Security: Now safe to process data parameters on valid connection
    // Logic: Resource exists → validate what we want to send through it
    // ════════════════════════════════════════════════════════════════════════════════════════

    // Check DatDesc pointer validity
    if (!pDatDesc) {
        return IOC_RESULT_INVALID_PARAM;
    }

    // Data size validation - check for maximum allowed data size
    // 🎯 TDD REQUIREMENT: Implement IOC_RESULT_DATA_TOO_LARGE validation
    const size_t IOC_MAX_DATA_SIZE = 64 * 1024 * 1024;  // 64MB limit for single data chunk
    size_t totalDataSize = pDatDesc->Payload.PtrDataSize + pDatDesc->Payload.EmdDataLen;

    if (totalDataSize > IOC_MAX_DATA_SIZE) {
        return IOC_RESULT_DATA_TOO_LARGE;
    }

    // Zero-size data validation - check if both PtrDataSize and EmdDataLen are zero
    if (pDatDesc->Payload.PtrDataSize == 0 && pDatDesc->Payload.EmdDataLen == 0) {
        return IOC_RESULT_ZERO_DATA;
    }

    // ════════════════════════════════════════════════════════════════════════════════════════
    // 🥉 PHASE 3: Options Validation (LOWEST PRECEDENCE)
    // Rationale: Validate configuration after confirming connection and data are valid
    // Logic: Resource exists → Data is valid → Check how to send it
    // Note: Options validation is typically handled by protocol-specific implementation
    // ════════════════════════════════════════════════════════════════════════════════════════

    _IOC_LogDebug("IOC_sendDAT: Sending %lu bytes on LinkID=%llu\n", pDatDesc->Payload.PtrDataSize, LinkID);

    // 🎯 TDD IMPLEMENTATION: Track DAT sender state transitions for TDD GREEN
    // Set sender to "busy sending" before operation, restore to "ready" after
    pthread_mutex_lock(&pSenderLinkObj->DatState.SubStateMutex);
    pSenderLinkObj->DatState.IsSending = true;
    pSenderLinkObj->DatState.CurrentSubState = IOC_LinkSubStateDatSenderBusySendDat;
    pSenderLinkObj->DatState.LastOperationTime = time(NULL);
    pthread_mutex_unlock(&pSenderLinkObj->DatState.SubStateMutex);

    // 🔄 TDD GREEN: Update ConlesEvent SubState for IOC_getLinkState() compatibility
    // Bridge the gap between Service-mode DAT state and ConlesMode state reporting
    _IOC_updateConlesEventSubState(LinkID, IOC_LinkSubStateDatSenderBusySendDat);

    // 🔄 WHY ARCHITECTURE CHANGE: The original implementation used global variables
    // (_gTDD_PendingData) to store data, completely bypassing the protocol layer.
    // This violated the layered architecture where:
    // - Application Layer (IOC_sendDAT) should call Service Layer
    // - Service Layer should delegate to Protocol Layer (OpSendData_F)
    // - Protocol Layer handles actual transmission (FIFO callbacks, TCP sockets, etc.)
    //
    // 🚫 PROBLEM WITH OLD APPROACH: Global variables meant only one protocol could work,
    // no cross-protocol communication, and data never reached the receiver because
    // IOC_flushDAT() only printed logs without triggering protocol transmission.
    //
    // ✅ NEW APPROACH: Delegate to protocol-specific implementation via method table.
    // Each protocol (FIFO, TCP, UDP) can implement its own optimal transmission strategy.
    _IOC_SrvProtoMethods_pT pMethods = pSenderLinkObj->pMethods;
    if (!pMethods || !pMethods->OpSendData_F) {
        // Restore sender state on error
        pthread_mutex_lock(&pSenderLinkObj->DatState.SubStateMutex);
        pSenderLinkObj->DatState.IsSending = false;
        pthread_mutex_unlock(&pSenderLinkObj->DatState.SubStateMutex);
        return IOC_RESULT_NOT_SUPPORT;
    }

    IOC_Result_T Result = pMethods->OpSendData_F(pSenderLinkObj, pDatDesc, pOption);

    // 🎯 TDD IMPLEMENTATION: Restore sender state after operation completes
    pthread_mutex_lock(&pSenderLinkObj->DatState.SubStateMutex);
    pSenderLinkObj->DatState.IsSending = false;
    pSenderLinkObj->DatState.CurrentSubState = IOC_LinkSubStateDatSenderReady;
    pSenderLinkObj->DatState.LastOperationTime = time(NULL);
    pthread_mutex_unlock(&pSenderLinkObj->DatState.SubStateMutex);

    // 🔄 TDD GREEN: Update ConlesEvent SubState back to Ready
    _IOC_updateConlesEventSubState(LinkID, IOC_LinkSubStateDatSenderReady);

    return Result;
}

/**
 * @brief Flush pending data on the specified link
 * @param LinkID: the link ID to flush data on
 * @param pOption: optional parameters (can be NULL)
 * @return IOC_RESULT_SUCCESS: data flushed successfully
 */
IOC_Result_T IOC_flushDAT(IOC_LinkID_T LinkID, IOC_Options_pT pOption) {
    if (LinkID == IOC_ID_INVALID) {
        return IOC_RESULT_NOT_EXIST_LINK;
    }

    printf("IOC_flushDAT: Flushing data on LinkID=%llu\n", LinkID);

    // Get link object to determine protocol and flush appropriately
    _IOC_LinkObject_pT pLinkObj = _IOC_getLinkObjByLinkID(LinkID);
    if (!pLinkObj) {
        return IOC_RESULT_NOT_EXIST_LINK;
    }

    // 🚀 MICRO-BATCHING SUPPORT: For ProtoFifo protocol, flush any accumulated batch data
    // The time-window batching feature requires explicit flushing to deliver queued data
    // when the application signals completion of a burst or end of data stream.
    _IOC_SrvProtoMethods_pT pMethods = pLinkObj->pMethods;
    if (pMethods && pMethods->pProtocol && strcmp(pMethods->pProtocol, IOC_SRV_PROTO_FIFO) == 0) {
        // ProtoFifo protocol: flush any accumulated batch data
        // This ensures that data queued during batching windows is delivered to receivers
        extern IOC_Result_T __IOC_flushData_ofProtoFifo(_IOC_LinkObject_pT pLinkObj, const IOC_Options_pT pOption);
        return __IOC_flushData_ofProtoFifo(pLinkObj, pOption);
    }

    // 📋 PROTOCOL DIFFERENCES: Other protocols might implement different strategies:
    // - TCP: Could buffer data and flush() would force socket send
    // - UDP: Might batch packets and flush() would send the batch
    // - File: Could buffer writes and flush() would fsync() to disk
    //
    // 💡 DESIGN PRINCIPLE: Keep flush() simple for immediate protocols, let buffering
    // protocols override with their own flush implementation via OpFlushData_F (future).
    return IOC_RESULT_SUCCESS;
}

/**
 * @brief Receive data chunk on the specified link (polling mode)
 * @param LinkID: the link ID to receive data from
 * @param pDatDesc: pointer to data description buffer
 * @param pOption: optional parameters (can be NULL)
 * @return IOC_RESULT_SUCCESS: data received successfully
 */
IOC_Result_T IOC_recvDAT(IOC_LinkID_T LinkID, IOC_DatDesc_pT pDatDesc, IOC_Options_pT pOption) {
    // ════════════════════════════════════════════════════════════════════════════════════════
    // 🥇 PHASE 1: LinkID Validation (HIGHEST PRECEDENCE - IDENTICAL to sendDAT)
    // Rationale: Validate resource exists before processing any data or configuration
    // Security: Prevents data processing on invalid/unauthorized connections
    // Performance: Fail fast on connection issues
    // ════════════════════════════════════════════════════════════════════════════════════════

    // Check for invalid LinkID constant
    if (LinkID == IOC_ID_INVALID) {
        return IOC_RESULT_NOT_EXIST_LINK;
    }

    // Get receiver link object (validates LinkID exists and is accessible)
    _IOC_LinkObject_pT pReceiverLinkObj = _IOC_getLinkObjByLinkID(LinkID);
    if (!pReceiverLinkObj) {
        return IOC_RESULT_NOT_EXIST_LINK;
    }

    // ════════════════════════════════════════════════════════════════════════════════════════
    // 🥈 PHASE 2: DatDesc Parameter Validation (SECOND PRECEDENCE - IDENTICAL to sendDAT)
    // Rationale: Validate data parameters after confirming connection exists
    // Security: Now safe to process data parameters on valid connection
    // Logic: Resource exists → validate what we want to receive through it
    // ════════════════════════════════════════════════════════════════════════════════════════

    // Check DatDesc pointer validity
    if (!pDatDesc) {
        return IOC_RESULT_INVALID_PARAM;
    }

    // ════════════════════════════════════════════════════════════════════════════════════════
    // 🥉 PHASE 3: Options Validation (LOWEST PRECEDENCE - IDENTICAL to sendDAT)
    // Rationale: Validate configuration after confirming connection and data are valid
    // Logic: Resource exists → Data buffer is valid → Check how to receive it
    // Note: Options validation is typically handled by protocol-specific implementation
    // ════════════════════════════════════════════════════════════════════════════════════════

    _IOC_LogDebug("IOC_recvDAT: Receiving data on LinkID=%llu", LinkID);

    // 🔄 ARCHITECTURE IMPROVEMENT: Delegate to protocol-specific implementation
    // Each protocol (FIFO, TCP, UDP) can implement its own optimal reception strategy:
    // - FIFO: Polling buffer for intra-process communication
    // - TCP: Socket receive operations
    // - UDP: Packet reception
    _IOC_SrvProtoMethods_pT pMethods = pReceiverLinkObj->pMethods;
    if (!pMethods || !pMethods->OpRecvData_F) {
        return IOC_RESULT_NOT_SUPPORT;
    }

    IOC_Result_T Result = pMethods->OpRecvData_F(pReceiverLinkObj, pDatDesc, pOption);

    return Result;
}
