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
    // Parameter validation
    if (!pDatDesc) {
        return IOC_RESULT_INVALID_PARAM;
    }

    if (LinkID == IOC_ID_INVALID) {
        return IOC_RESULT_NOT_EXIST_LINK;
    }

    // Zero-size data validation - check if both PtrDataSize and EmdDataSize are zero
    if (pDatDesc->Payload.PtrDataSize == 0 && pDatDesc->Payload.EmdDataSize == 0) {
        return IOC_RESULT_ZERO_DATA;
    }

    // Get sender link object
    _IOC_LinkObject_pT pSenderLinkObj = _IOC_getLinkObjByLinkID(LinkID);
    if (!pSenderLinkObj) {
        return IOC_RESULT_NOT_EXIST_LINK;
    }

    printf("IOC_sendDAT: Sending %lu bytes on LinkID=%llu\n", pDatDesc->Payload.PtrDataSize, LinkID);

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
        return IOC_RESULT_NOT_SUPPORT;
    }

    IOC_Result_T Result = pMethods->OpSendData_F(pSenderLinkObj, pDatDesc, pOption);

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

    // 🚀 WHY SIMPLIFIED FLUSH: For ProtoFifo protocol, data transmission is immediate
    // and synchronous - when IOC_sendDAT() calls OpSendData_F(), the data is instantly
    // delivered to the receiver's callback. No buffering occurs, so flush is a no-op.
    //
    // 📋 PROTOCOL DIFFERENCES: Other protocols might implement different strategies:
    // - TCP: Could buffer data and flush() would force socket send
    // - UDP: Might batch packets and flush() would send the batch
    // - File: Could buffer writes and flush() would fsync() to disk
    //
    // 💡 DESIGN PRINCIPLE: Keep flush() simple for immediate protocols, let buffering
    // protocols override with their own flush implementation via OpFlushData_F (future).
    // Other protocols might buffer data and need actual flushing
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
    // Parameter validation
    if (!pDatDesc) {
        return IOC_RESULT_INVALID_PARAM;
    }

    if (LinkID == IOC_ID_INVALID) {
        return IOC_RESULT_NOT_EXIST_LINK;
    }

    // Get receiver link object
    _IOC_LinkObject_pT pReceiverLinkObj = _IOC_getLinkObjByLinkID(LinkID);
    if (!pReceiverLinkObj) {
        return IOC_RESULT_NOT_EXIST_LINK;
    }

    printf("IOC_recvDAT: Receiving data on LinkID=%llu\n", LinkID);

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
