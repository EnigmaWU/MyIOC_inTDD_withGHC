// Description: This file is the header file for the MyIOC module.

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
 *    RefMore: IOC_CapabiltyDescription_T in IOC_Types.h
 * @return IOC_RESULT_SUCCESS: get capability successfully.
 * @return IOC_RESULT_NOT_SUPPORT: the CapID is not supported.
 *
 * @attention
 *  IOC's capability is static or update once when call IOC_initModule.
 */
IOC_Result_T IOC_getCapabilty(
    /*ARG_INOUT*/ IOC_CapabiltyDescription_pT pCapDesc);

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
