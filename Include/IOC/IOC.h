// Description: This file is the header file for the MyIOC module.

#include "IOC_CmdAPI.h"
#include "IOC_DatAPI.h"
#include "IOC_EvtAPI.h"
#include "IOC_Helpers.h"
#include "IOC_SrvAPI.h"
#include "IOC_Types.h"

#ifndef __INTER_OBJECT_COMMUNICATION_H__
#define __INTER_OBJECT_COMMUNICATION_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief get IOC's capability by CapID in pCapDesc.
 * @param pCapDesc: the capability description.
 *    RefMore: IOC_CapabilityDescription_T in IOC_Types.h
 * @return IOC_RESULT_SUCCESS: get capability successfully.
 * @return IOC_RESULT_NOT_SUPPORT: the CapID is not supported.
 *
 * @attention
 *  IOC's capability is static or update once when call IOC_initModule.
 */
IOC_Result_T IOC_getCapability(
    /*ARG_INOUT*/ IOC_CapabilityDescription_pT pCapDesc);

//===>Helper APIs for UnitTesting && Debugging=========================================================================
/**
 * @brief get LinkID's MainState and SubState.
 *      This is a helper API mainly used for UnitTesting && Debugging.
 *    RefMore: IOC_LinkState_T in IOC_Types.h
 *    RefMore: README_ArchDesign::State
 * @param LinkID: the link ID to get the state.
 * @param pLinkMainState: to store the main state of the LinkID.
 * @param pLinkSubState: to store the sub state of the LinkID.
 * @return IOC_RESULT_SUCCESS: get the state successfully.
 * @return IOC_RESULT_NOT_EXIST_LINK: the LinkID is not exist.
 */
IOC_Result_T IOC_getLinkState(
    /*ARG_IN*/ IOC_LinkID_T LinkID,
    /*ARG_OUT*/ IOC_LinkState_pT pLinkMainState,
    /*ARG_OUT_OPTIONAL*/ IOC_LinkSubState_pT pLinkSubState);

/**
 * @brief Get service-side LinkIDs for state inspection and management.
 *      This enables querying receiver-side states and comprehensive service monitoring.
 * @param SrvID: the service ID to get connected LinkIDs from.
 * @param pLinkIDs: array to store the LinkIDs (caller allocated).
 * @param MaxLinks: maximum number of LinkIDs the array can hold.
 * @param pActualCount: actual number of LinkIDs returned.
 * @return IOC_RESULT_SUCCESS: LinkIDs retrieved successfully.
 * @return IOC_RESULT_NOT_EXIST_SERVICE: the SrvID does not exist.
 * @return IOC_RESULT_BUFFER_TOO_SMALL: provided array is too small.
 */
IOC_Result_T IOC_getServiceLinkIDs(
    /*ARG_IN*/ IOC_SrvID_T SrvID,
    /*ARG_OUT*/ IOC_LinkID_T* pLinkIDs,
    /*ARG_IN*/ uint16_t MaxLinks,
    /*ARG_OUT*/ uint16_t* pActualCount);

/**
 * @brief Get comprehensive service state including all connected links.
 *      This provides complete service monitoring capability.
 * @param SrvID: the service ID to get state information.
 * @param pServiceState: service state information (reserved for future use).
 * @param pConnectedLinks: number of currently connected links.
 * @return IOC_RESULT_SUCCESS: service state retrieved successfully.
 * @return IOC_RESULT_NOT_EXIST_SERVICE: the SrvID does not exist.
 */
IOC_Result_T IOC_getServiceState(
    /*ARG_IN*/ IOC_SrvID_T SrvID,
    /*ARG_OUT_OPTIONAL*/ void* pServiceState,  // Reserved for future service state structure
    /*ARG_OUT_OPTIONAL*/ uint16_t* pConnectedLinks);

#define IOC_BugAbort()                                       \
    do {                                                     \
        fprintf(stderr, "BUG: %s:%d\n", __FILE__, __LINE__); \
        abort();                                             \
    } while (0)
//<===Helper APIs for UnitTesting && Debugging==========================================================================

#ifdef __cplusplus
}
#endif
#endif  //__INTER_OBJECT_COMMUNICATION_H__
