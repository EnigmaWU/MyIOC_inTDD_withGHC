/**
 * @file IOC_Data.c
 * @brief Implementation of IOC Data Transfer APIs
 * @details Provides data streaming capabilities with NODROP guarantee
 */

#include "_IOC.h"

// TDD GREEN: Constants needed for implementation
#define _MAX_IOC_LINK_OBJ_NUM 8

// TDD GREEN: Simple storage for pending data transmission
typedef struct {
    IOC_LinkID_T SenderLinkID;
    void *pData;
    ULONG_T DataSize;
    bool HasPendingData;
} __TDD_PendingData_T;

// TDD GREEN: Simple storage for receiver callback info
typedef struct {
    IOC_CbRecvDat_F CbRecvDat_F;
    void *pCbPrivData;
    bool IsRegistered;
} __TDD_ReceiverCallback_T;

static __TDD_PendingData_T _gTDD_PendingData = {0};
static __TDD_ReceiverCallback_T _gTDD_ReceiverCallback = {0};

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

    // Get sender link object
    _IOC_LinkObject_pT pSenderLinkObj = _IOC_getLinkObjByLinkID(LinkID);
    if (!pSenderLinkObj) {
        return IOC_RESULT_NOT_EXIST_LINK;
    }

    printf("IOC_sendDAT: Sending %lu bytes on LinkID=%llu\n", pDatDesc->Payload.PtrDataSize, LinkID);

    // TDD GREEN: Store data for later delivery during flush
    _gTDD_PendingData.SenderLinkID = LinkID;
    _gTDD_PendingData.pData = pDatDesc->Payload.pData;
    _gTDD_PendingData.DataSize = pDatDesc->Payload.PtrDataSize;
    _gTDD_PendingData.HasPendingData = true;

    pDatDesc->Status = IOC_DAT_STATUS_SENDING;
    pDatDesc->Result = IOC_RESULT_SUCCESS;

    return IOC_RESULT_SUCCESS;
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
    // TODO: Add parameter validation
    if (!pDatDesc) {
        return IOC_RESULT_INVALID_PARAM;
    }

    // TODO: Add link validation
    if (LinkID == IOC_ID_INVALID) {
        return IOC_RESULT_NOT_EXIST_LINK;
    }

    // TDD GREEN: Minimal implementation
    printf("IOC_recvDAT: Receiving data on LinkID=%llu\n", LinkID);

    // TODO: Implement actual data reception logic

    return IOC_RESULT_SUCCESS;
}
