#include "IOC_DatDesc.h"
#include "IOC_Option.h"
#include "IOC_SrvTypes.h"

#ifndef __IOC_DATA_API_H__
#define __IOC_DATA_API_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Data reception callback function type is defined in IOC_SrvTypes.h
 */

/**
 * @brief Data receiver callback arguments are defined in IOC_SrvTypes.h
 *    WHEN: onlineService with UsageCapabilities::DatReceiver,
 *      THEN: IOC_DatUsageArgs_T in IOC_SrvArgs_T::UsageArgs.pDat
 *    WHEN: connectService with Usage::DatReceiver,
 *      THEN: IOC_DatUsageArgs_T in IOC_ConnArgs_T::UsageArgs.pDat
 */

/**
 * @brief Send data chunk on the specified link
 *        DataSender calls this API to send data chunk to DataReceiver asynchronously
 *        First call automatically initializes the stream, subsequent calls continue the stream
 *
 * @param LinkID: the link ID between DataSender and DataReceiver
 *     RefMore: README_ArchDesign::Object::Link
 * @param pDatDesc: the data description. IOC will COPY-IN data payload for reliability
 *     RefMore: README_ArchDesign::Concept::MSG::DAT
 * @param pOption: the options for this sendDAT
 *     Supported options: IOC_OPTID_TIMEOUT, IOC_OPTID_BLOCKING_MODE
 *     Note: RELIABILITY_MODE is always NODROP (immutable for stream consistency)
 *
 * @return IOC_RESULT_SUCCESS: data chunk queued for transmission successfully
 * @return IOC_RESULT_BUFFER_FULL: IOC buffer is full (when immediate NONBLOCK mode)
 * @return IOC_RESULT_TIMEOUT: data transmission timeout (when NONBLOCK mode with timeout)
 * @return IOC_RESULT_STREAM_CLOSED: data stream was closed by peer or due to error
 * @return IOC_RESULT_LINK_BROKEN: communication link is broken during transmission
 * @return IOC_RESULT_INVALID_PARAM: invalid parameters
 * @return IOC_RESULT_NOT_EXIST_LINK: LinkID does not exist or already closed
 * @return IOC_RESULT_DATA_TOO_LARGE: data chunk exceeds maximum allowed size
 *
 * RefUT: UT_ConetDatSendXXX
 */
IOC_Result_T IOC_sendDAT(IOC_LinkID_T LinkID, IOC_DatDesc_pT pDatDesc, IOC_Options_pT pOption);

/**
 * @brief Receive data chunk on the specified link (polling mode)
 *        DataReceiver calls this API to actively wait for data chunks when using polling mode
 *
 * @param LinkID: the link ID to receive data from
 * @param pDatDesc: pointer to data description buffer to receive data details
 * @param pOption: the options for this recvDAT
 *     Supported options: IOC_OPTID_TIMEOUT, IOC_OPTID_BLOCKING_MODE
 *
 * @return IOC_RESULT_SUCCESS: data chunk received successfully
 * @return IOC_RESULT_TIMEOUT: receive timeout (when timeout configured)
 * @return IOC_RESULT_LINK_BROKEN: communication link is broken
 * @return IOC_RESULT_INVALID_PARAM: invalid parameters
 * @return IOC_RESULT_NOT_EXIST_LINK: LinkID does not exist or already closed
 * @return IOC_RESULT_NO_DATA: no data available (when immediate NONBLOCK mode)
 * @return IOC_RESULT_DATA_CORRUPTED: data integrity check failed
 *
 * RefUT: UT_ConetDatRecvPollingXXX
 */
IOC_Result_T IOC_recvDAT(IOC_LinkID_T LinkID, IOC_DatDesc_pT pDatDesc, IOC_Options_pT pOption);

/**
 * @brief Force transmission of buffered data on the specified link
 *        DataSender calls this API to ensure immediate delivery of buffered data chunks
 *        This is the ONLY explicit control operation needed for data streaming
 *
 * @param LinkID: the link ID to flush buffered data
 * @param pOption: the options for this flushDAT
 *     Supported options: IOC_OPTID_TIMEOUT
 *
 * @return IOC_RESULT_SUCCESS: buffered data flushed successfully
 * @return IOC_RESULT_TIMEOUT: flush timeout
 * @return IOC_RESULT_STREAM_CLOSED: data stream is closed
 * @return IOC_RESULT_LINK_BROKEN: communication link is broken
 * @return IOC_RESULT_INVALID_PARAM: invalid parameters
 * @return IOC_RESULT_NOT_EXIST_LINK: LinkID does not exist or already closed
 *
 * RefUT: UT_ConetDatFlushXXX
 */
IOC_Result_T IOC_flushDAT(IOC_LinkID_T LinkID, IOC_Options_pT pOption);

// TODO: IOC_cancelDAT(IOC_LinkID_T LinkID, ...)

#ifdef __cplusplus
}
#endif
#endif  // __IOC_DATA_API_H__
