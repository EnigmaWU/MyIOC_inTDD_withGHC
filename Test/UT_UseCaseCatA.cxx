// Following UTs are User Acceptance Tests(a.k.a UAT) of Use Case Category-A(a.k.a UseCaseCatA) in file README_UseCase.md

// IOC's API to support UseCaseCatA is defined in IOC_${doXYZ}_inConlesMode style in file IOC.h, such as:
//  'post event' is IOC_postEVT_inConlesMode
//  'subscribe event' is IOC_subEVT_inConlesMode
//  'unsubscribe event' is IOC_unsubEVT_inConlesMode

// Include _UT_IOC_Common.h as it includes all the necessary headers including gtest as UTFWK and IOC headers
#include "_UT_IOC_Common.h"

// ALL UT must use TEMPLATE defined UT_FreelyDrafts.cxx and reference exist UT codes in UT_ConsleEvent*.cxx
//=====================================================================================================================

/**
 * @brief According to the Catetory-A's Use Case-01/02/... and corresponding scenarios in README_UseCase.md.
 *  [Use Case-01::Scenario-01]: verifyCallbackSuccess_byObjBSubEvtFirst_thenObjAPostEvt
 *  [Use Case-01::Scenario-01.a]: verifyPostFailFirst_thenVerifyCallbackSuccess_byObjAPostEvtBeforeAndAfterObjBSubEvt
 *  [Use Case-01::Scenario-02]:
 *  [Use Case-01::Scenario-02.a]:
 *  [Use Case-01::Scenario-02.b]:
 *  [Use Case-01::Scenario-02.c]:
 *
 *  [Use Case-02::Scenario-01]:
 *
 *  [Use Case-03::Scenario-01]:
 *  [Use Case-03::Scenario-02]:
 *
 *  [Use Case-04::Scenario-01]:
 */

// #####################################################################################################################
