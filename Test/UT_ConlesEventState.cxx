/**
 * @file UT_ConlesEventState.cxx
 * @brief This file contains the UTs to verify State of Event in ConlesMode.
 * @RefMore:
 *     README_ArchDesign @ PRJROOT
 *         |-> Concept
 *             |-> Conet vs Conles
 *             |-> MSG::EVT
 *         |-> State
 *             |-> EVT::Conles
 */

/**
 * @section DesignOfUT ConlesEventState
 * Based on the Concept of Conles and the state of EVT,
 *  by defination of IOC_LinkState_T in IOC_Types.h,
 *  by defination of IOC_getLinkState in IOC.h,
 *  by considering UT design aspect in _UT_IOC_Common.h.
 * Design UTs to verify every State and SubState of Event in ConlesMode,
 *  by design combination behaviors of IOC_[sub/unsub/post]EVT.
 *
 * List of UTs in summary:
 *  - Case01_verify...
 *
 */

#include "_UT_IOC_Common.h"