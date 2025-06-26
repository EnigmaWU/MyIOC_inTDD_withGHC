#include "IOC_MsgDesc.h"
#include "IOC_Types.h"

#ifndef __IOC_TYPES_DATDESC_H__
#define __IOC_TYPES_DATDESC_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Data payload structure for stream data chunks
 *     IF EmbDataSize > 0, then EmbData[] is used to store the data.
 *     IF EmbDataSize == 0, then pData is used to store the data in heap memory.
 *     EmbData[] is used for small data chunks to avoid heap allocation.
 *     pData is used for larger data chunks that exceed the size of EmbData[].
 */
typedef struct {
    void *pData;  // Pointer to data chunk
    // asDatSender: prepares this before sending and recycle after send returns.
    // asDatReceiver: prepares this before receiving and recycle after receive returns.

    ULONG_T PtrDataSize;  // asDatSender: size of pData (bytes)
                          // asDatReceiver: size of pData (bytes) may be received

    ULONG_T PtrDataLen;  // asDatSender: length of data in pData (bytes) to send
                         // asDatReceiver: length of data received in pData (bytes)

    ULONG_T EmdDataLen;   // Actual length of data in EmbData (bytes)
    ULONG_T EmdData[16];  // Embedded data array for small chunks (64 bytes on 64-bit systems)
} IOC_DatPayload_T, *IOC_DatPayload_pT;

/**
 * @brief Data description structure for stream-based data transfer
 *        Contains all information about a data chunk including metadata and payload
 */
typedef struct {
    // MsgCommon - inherited from IOC_MsgDesc_T
    IOC_MsgDesc_T MsgDesc;

    // Data payload
    IOC_DatPayload_T Payload;  // Data chunk payload

} IOC_DatDesc_T, *IOC_DatDesc_pT;

/**
 * @brief Initialize data description structure with default values
 *
 * @param pDatDesc: pointer to data description to initialize
 * @param streamID: stream identifier (auto-generated if 0)
 * @param chunkSize: size of data chunk to transfer
 */
static inline void IOC_initDatDesc(IOC_DatDesc_pT pDatDesc) {
    if (pDatDesc) {
        memset(pDatDesc, 0, sizeof(IOC_DatDesc_T));
        // Note: No timestamp for simple version
    }
}

/**
 * @brief Get data payload pointer and size
 *
 * @param pDatDesc: pointer to data description
 * @param ppData: output pointer to data
 * @param pDataSize: output size of data
 * @return IOC_RESULT_SUCCESS on success, error code on failure
 */
static inline IOC_Result_T IOC_getDatPayload(const IOC_DatDesc_pT pDatDesc, void **ppData, ULONG_T *pDataSize) {
    if (!pDatDesc || !ppData || !pDataSize) {
        return IOC_RESULT_INVALID_PARAM;
    }

    if (pDatDesc->Payload.EmdDataLen > 0) {
        *ppData = (void *)pDatDesc->Payload.EmdData;
        *pDataSize = pDatDesc->Payload.EmdDataLen;
    } else if (pDatDesc->Payload.pData && pDatDesc->Payload.PtrDataSize > 0) {
        *ppData = pDatDesc->Payload.pData;
        *pDataSize = pDatDesc->Payload.PtrDataSize;
    } else {
        *ppData = NULL;
        *pDataSize = 0;
        return IOC_RESULT_NO_DATA;
    }

    return IOC_RESULT_SUCCESS;
}

#ifdef __cplusplus
}
#endif
#endif  // __IOC_TYPES_DATDESC_H__
