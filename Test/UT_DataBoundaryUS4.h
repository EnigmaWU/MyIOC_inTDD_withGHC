///////////////////////////////////////////////////////////////////////////////////////////////////
// UT_DataBoundaryUS4.h - DAT Boundary Testing: US-4 Error Code Coverage Validation
// 📝 Purpose: Header file for User Story 4 - Quality assurance engineer error code boundary testing
// 🔄 Focus: Comprehensive error code coverage, error consistency, boundary error path validation
// 🎯 Coverage: [@US-4] Error code coverage validation (comprehensive boundary error testing)
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef UT_DATABOUNDARYUS4_H
#define UT_DATABOUNDARYUS4_H

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief
 *  Validate IOC framework DAT (Data Transfer) error code coverage for boundary test scenarios,
 *  focusing on comprehensive error code validation, error consistency across configurations,
 *  and boundary error path verification.
 *
 *-------------------------------------------------------------------------------------------------
 *++DAT US-4 boundary testing validates comprehensive error code coverage of DAT data transfer
 *mechanism. This test file focuses on:
 *
 *  Test file scope:
 *  - Error code coverage validation: Comprehensive boundary error testing
 *  - Parameter boundary error codes: NULL pointers, invalid LinkID, malformed options
 *  - Data size boundary error codes: Zero-size, oversized data, extreme size values
 *  - Timeout/mode boundary error codes: Zero timeout, mode conflicts, extreme timeouts
 *  - Error precedence validation: Multiple error condition precedence order
 *  - Error code consistency: Cross-mode consistency, reproducible error codes
 *
 *  Reference documentation:
 *  - README_ArchDesign.md::MSG::DAT (error handling section)
 *  - README_RefAPIs.md::IOC_sendDAT/IOC_recvDAT (error codes documentation)
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING DESIGN==============================================================

/**************************************************************************************************
 * 📋 DAT US-4 ERROR CODE COVERAGE TEST FOCUS
 *
 * 🎯 DESIGN PRINCIPLE: Validate comprehensive DAT error code coverage under boundary conditions
 * 🔄 TESTING PRIORITY: Parameter errors → Data size errors → Timeout errors → Precedence validation
 *
 * ✅ ERROR CODE SCENARIOS COVERED:
 *    🔲 Parameter Error Coverage: NULL pointers, invalid LinkID, malformed DatDesc, option validation
 *    📏 Data Size Error Coverage: Zero-size data, oversized data, extreme size boundary validation
 *    ⏱️ Timeout Error Coverage: Zero timeout, extreme timeout, blocking mode conflicts
 *    🔄 Error Precedence Coverage: Multiple error conditions, validation order consistency
 *    🎯 Cross-Mode Consistency: Error code consistency across service/client + callback/poll modes
 *
 * ❌ EXCLUDED FROM US-4 ERROR CODE TESTING:
 *    ✅ Typical usage scenarios (covered by DataTypical)
 *    🚀 Performance testing and stress testing (covered by DataPerformance)
 *    🔄 Complex data transfer scenarios (covered by other DataBoundary US files)
 *    🛠️ Recovery scenarios and retry logic
 *    📊 Long-term stability testing
 *
 * 🎯 IMPLEMENTATION FOCUS:
 *    📋 Complete error code path coverage and validation consistency
 *    🔧 Error code reproducibility across different system configurations
 *    ⚡ Error precedence order validation and system stability
 *    🛡️ Boundary error isolation and cross-mode consistency verification
 *************************************************************************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================
/**************************************************************************************************
 * @brief 【User Story】
 *
 *  US-4: AS a quality assurance engineer,
 *    I WANT to validate comprehensive error code coverage for all boundary conditions,
 *   SO THAT I can ensure consistent error reporting across all boundary scenarios
 *      AND verify that error codes match their documented meanings,
 *      AND confirm that all boundary error paths are properly tested.
 *
 *************************************************************************************************/
//======>END OF USER STORY=========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//=======>BEGIN OF ACCEPTANCE CRITERIA=============================================================
/**************************************************************************************************
 * @brief 【Acceptance Criteria】
 *
 * 🎯 Focus on DAT US-4 ERROR CODE COVERAGE testing - validate comprehensive error code coverage
 *
 * [@US-4,AC-1] Parameter boundary error code validation
 *  AC-1: GIVEN invalid parameter boundary conditions (NULL pointers, invalid LinkID, malformed options),
 *         WHEN calling IOC_sendDAT or IOC_recvDAT with boundary parameter combinations,
 *         THEN system should return specific documented error codes (IOC_RESULT_INVALID_PARAM,
 *IOC_RESULT_NOT_EXIST_LINK) AND error codes should be consistent between sendDAT and recvDAT for identical invalid
 *parameters, AND parameter validation should follow documented precedence order, AND no boundary parameter condition
 *should result in undefined behavior or system crash.
 *
 * [@US-4,AC-2] Data size boundary error code validation
 *  AC-2: GIVEN data size boundary error conditions (zero size, oversized data, extreme size values),
 *         WHEN performing DAT operations with boundary data sizes,
 *         THEN system should return appropriate size-related error codes (IOC_RESULT_DATA_TOO_LARGE, etc.)
 *          AND error codes should be consistent across similar data size boundary scenarios,
 *          AND data size validation should occur after parameter validation,
 *          AND memory protection should be maintained for all data size boundary conditions.
 *
 * [@US-4,AC-3] Timeout and blocking mode boundary error code validation
 *  AC-3: GIVEN timeout and blocking mode boundary error conditions (zero timeout, mode conflicts, extreme timeouts),
 *         WHEN configuring boundary timeout and blocking mode combinations,
 *         THEN system should return specific timeout/mode error codes (IOC_RESULT_TIMEOUT, IOC_RESULT_INVALID_PARAM)
 *          AND timeout error behavior should be consistent across sendDAT and recvDAT,
 *          AND mode validation should occur during parameter validation phase,
 *          AND extreme timeout values should be handled gracefully without overflow/underflow.
 *
 * [@US-4,AC-4] Multiple error condition precedence validation
 *  AC-4: GIVEN multiple simultaneous boundary error conditions,
 *         WHEN calling DAT functions with multiple invalid parameters or boundary violations,
 *         THEN system should return error codes following documented validation precedence
 *          AND error precedence should be consistent across all boundary scenarios,
 *          AND first detected boundary error should be reported (parameter > LinkID > data size > timeout),
 *          AND multiple boundary errors should not cause system instability or undefined behavior.
 *
 * [@US-4,AC-5] Comprehensive error code coverage validation
 *  AC-5: GIVEN comprehensive boundary error scenarios across all DAT operations,
 *         WHEN testing complete error path coverage for boundary conditions,
 *         THEN all documented IOC_RESULT_* error codes should be reachable through boundary testing
 *          AND error code meanings should match documented behavior exactly,
 *          AND no boundary condition should result in undocumented or undefined error codes,
 *          AND error handling should provide complete path coverage for all boundary scenarios.
 *
 *************************************************************************************************/
//=======>END OF ACCEPTANCE CRITERIA================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES ORGANIZATION==========================================================
/**************************************************************************************************
 * @brief 【Test Cases Organization】
 *
 * Test Cases are organized by Acceptance Criteria and implemented in separate files:
 *
 * 📂 UT_DataBoundaryUS4AC1.cxx - [@US-4,AC-1] Parameter boundary error code validation
 *    └── [@AC-1,US-4] TC-1: verifyDatErrorCodeCoverage_byParameterBoundaries_expectSpecificErrorCodes
 *    └── [@AC-1,US-4] TC-2: verifyDatErrorCodeCoverage_byParameterConsistency_expectReproducibleErrorCodes
 *    └── TODO: [@AC-1,US-4] IOC_Options boundary validation
 *    └── TODO: [@AC-1,US-4] Mixed valid/invalid parameter combinations
 *
 * 📂 UT_DataBoundaryUS4AC2.cxx - [@US-4,AC-2] Data size boundary error code validation
 *    └── [@AC-2,US-4] TC-1: verifyDatErrorCodeCoverage_byDataSizeBoundaries_expectConsistentErrorReporting
 *    └── [@AC-2,US-4] TC-2: verifyDatErrorCodeCoverage_byDataSizeConsistency_expectIsolatedDataValidation
 *    └── TODO: [@AC-2,US-4] Maximum data size boundary validation
 *    └── TODO: [@AC-2,US-4] Oversized data boundary validation
 *
 * 📂 UT_DataBoundaryUS4AC3.cxx - [@US-4,AC-3] Timeout and blocking mode boundary error code validation
 *    └── [@AC-3,US-4] TC-1: verifyDatErrorCodeCoverage_byTimeoutModeBoundaries_expectTimeoutErrorCodes
 *    └── [@AC-3,US-4] TC-2: verifyDatErrorCodeCoverage_byTimeoutModeConsistency_expectIsolatedTimeoutValidation
 *    └── TODO: [@AC-3,US-4] Extreme timeout boundary validation
 *    └── TODO: [@AC-3,US-4] Mode conflict boundary validation
 *
 * 📂 UT_DataBoundaryUS4AC4.cxx - [@US-4,AC-4] Multiple error condition precedence validation
 *    └── [@AC-4,US-4] TC-1: verifyDatErrorCodePrecedence_byMultipleErrorConditions_expectPriorityOrder
 *    └── [@AC-4,US-4] TC-2: verifyDatErrorCodePrecedence_byConsistencyValidation_expectReproducibleBehavior
 *    └── TODO: [@AC-4,US-4] Error precedence with ValidLinkID scenarios
 *    └── TODO: [@AC-4,US-4] Cross-mode error precedence validation
 *
 * 📂 UT_DataBoundaryUS4AC5.cxx - [@US-4,AC-5] Comprehensive error code coverage validation
 *    └── TODO: [@AC-5,US-4] TC-1: verifyDatErrorCodeCompleteness_byComprehensiveValidation_expectFullCoverage
 *    └── TODO: [@AC-5,US-4] Complete error path coverage validation
 *************************************************************************************************/
//======>END OF TEST CASES ORGANIZATION============================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF SHARED TEST ENVIRONMENT SETUP===================================================

#include "UT_DataBoundary.h"

// US-4 specific data structures for error code validation testing
typedef struct {
    const char* ConfigName;
    IOC_LinkID_T LinkID;
    IOC_SrvID_T ServiceID;
    IOC_CapabilityID_T CapID;
    bool IsService;    // true for service, false for client
    bool UseCallback;  // true for callback mode, false for polling mode

    // Error tracking
    IOC_Result_T LastSendResult;
    IOC_Result_T LastRecvResult;
    bool ErrorCodeConsistent;

    // Validation tracking
    int ParameterValidationCount;
    int DataSizeValidationCount;
    int TimeoutValidationCount;
} __DatErrorCodeTestConfig_T;

// Shared test configuration for ValidLinkID scenarios
typedef struct {
    std::vector<__DatErrorCodeTestConfig_T> TestConfigs;
    IOC_SrvID_T ServiceID1;
    IOC_SrvID_T ServiceID2;
    bool SystemInitialized;

    // Error code tracking
    std::map<IOC_Result_T, int> ErrorCodeCounts;
    std::vector<IOC_Result_T> ObservedErrorCodes;

    // Validation consistency tracking
    bool CrossModeConsistency;
    bool ParameterPrecedenceValidated;
    bool DataSizePrecedenceValidated;
    bool TimeoutPrecedenceValidated;
} __DatErrorCodeSharedTestData_T;

// Global test configuration for US-4 error code coverage testing
extern __DatErrorCodeSharedTestData_T g_US4_SharedTestData;

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF US-4 SPECIFIC UTILITY FUNCTIONS=================================================

/**
 * @brief Document and track error precedence discoveries for US-4 testing
 * @param testName Name of the test case documenting the precedence
 * @param scenario Description of the error scenario
 * @param expectedResult Expected error code
 * @param actualResult Actual error code returned by system
 * @param precedenceLevel Precedence level (1=highest, 2=medium, 3=lowest)
 */
inline void US4_DocumentErrorPrecedence(const char* testName, const char* scenario, IOC_Result_T expectedResult,
                                        IOC_Result_T actualResult, int precedenceLevel) {
    printf("   🔍 [%s] Scenario: %s\n", testName, scenario);
    printf("        Expected: %d, Actual: %d, Precedence Level: %d\n", expectedResult, actualResult, precedenceLevel);

    // Track error code observations
    g_US4_SharedTestData.ErrorCodeCounts[actualResult]++;
    g_US4_SharedTestData.ObservedErrorCodes.push_back(actualResult);
}

/**
 * @brief Validate that error codes match US-4 acceptance criteria expectations
 * @param errorCode The error code to validate
 * @param errorContext Description of when this error occurred
 * @return true if error code is expected for US-4 boundary testing
 */
inline bool US4_IsExpectedBoundaryErrorCode(IOC_Result_T errorCode, const char* errorContext) {
    // US-4 focuses on comprehensive boundary error coverage
    bool isExpected = (errorCode == IOC_RESULT_INVALID_PARAM) || (errorCode == IOC_RESULT_NOT_EXIST_LINK) ||
                      (errorCode == IOC_RESULT_ZERO_DATA) || (errorCode == IOC_RESULT_DATA_TOO_LARGE) ||
                      (errorCode == IOC_RESULT_TIMEOUT) || (errorCode == IOC_RESULT_BUFFER_FULL) ||
                      (errorCode == IOC_RESULT_BUFFER_TOO_SMALL);

    if (!isExpected) {
        printf("   ⚠️  Unexpected error code %d in context: %s\n", errorCode, errorContext);
    }

    return isExpected;
}

/**
 * @brief Initialize US-4 shared test data
 */
inline void US4_InitializeSharedTestData() {
    g_US4_SharedTestData.ErrorCodeCounts.clear();
    g_US4_SharedTestData.ObservedErrorCodes.clear();
    g_US4_SharedTestData.CrossModeConsistency = true;
    g_US4_SharedTestData.ParameterPrecedenceValidated = false;
    g_US4_SharedTestData.DataSizePrecedenceValidated = false;
    g_US4_SharedTestData.TimeoutPrecedenceValidated = false;
}

//======>END OF US-4 SPECIFIC UTILITY FUNCTIONS===================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF HIERARCHICAL INCLUDE STRUCTURE DOCUMENTATION====================================

/**
 * @brief Hierarchical Include Structure for US-4 Test Files
 *
 * The UT_DataBoundaryUS4 test files follow a hierarchical include structure:
 *
 * ┌─── UT_DataBoundaryUS4ACn.cxx (AC-specific test implementations)
 * │    └── #include "UT_DataBoundaryUS4.h"
 * │        └── #include "UT_DataBoundary.h" (base boundary testing framework)
 * │            └── System IOC headers, GoogleTest, etc.
 * │
 * ├─── Shared US-4 Utilities Available in All AC Files:
 * │    ├── US4_DocumentErrorPrecedence() - Document error precedence discoveries
 * │    ├── US4_IsExpectedBoundaryErrorCode() - Validate expected boundary error codes
 * │    ├── US4_InitializeSharedTestData() - Initialize shared test data
 * │    └── g_US4_SharedTestData - Global shared test data for error tracking
 * │
 * ├─── Benefits of This Structure:
 * │    ├── Reduced duplication: US-4 common code in one place
 * │    ├── Better organization: Clear hierarchy UT_DataBoundaryUS4ACn → US4 → DataBoundary
 * │    ├── Cleaner dependencies: Each level builds on the previous
 * │    ├── Better maintainability: Changes to US-4 utilities affect all AC files
 * │    └── Shared state: Error tracking and validation across all AC test cases
 * │
 * └─── File Responsibilities:
 *      ├── UT_DataBoundary.h: Base boundary testing framework, common test utilities
 *      ├── UT_DataBoundaryUS4.h: US-4 specific types, utilities, shared test data
 *      └── UT_DataBoundaryUS4ACn.cxx: Individual acceptance criteria implementations
 */

//======>END OF HIERARCHICAL INCLUDE STRUCTURE DOCUMENTATION======================================

#endif  // UT_DATABOUNDARYUS4_H
