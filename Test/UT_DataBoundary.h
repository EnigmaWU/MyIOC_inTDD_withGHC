///////////////////////////////////////////////////////////////////////////////////////////////////
// QUICK REFERENCE GUIDE
// 📝 Purpose: DAT (Data Transfer) boundary testing unit test framework
// 🔄 Process: User Story → Acceptance Criteria → Test Cases → Implementation
// 📂 Category: DataBoundary - Focus on DAT data transfer boundary conditions and limit parameter testing
// 🎯 Focus: Boundary values, null values, timeouts, blocking/non-blocking modes, data size limits and other edge cases
// Reference Unit Testing Templates in UT_FreelyDrafts.cxx when needed.
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef UT_DATABOUNDARY_H
#define UT_DATABOUNDARY_H

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief
 *  Validate IOC framework DAT (Data Transfer) boundary test scenarios, focusing on boundary conditions,
 *  limit parameters, exceptional inputs and error handling verification.
 *
 *-------------------------------------------------------------------------------------------------
 *++DAT boundary testing validates boundary conditions of DAT data transfer mechanism. This test file
 *complements other test files in the test suite:
 *
 *  Test file scope differentiation:
 *  - DataTypical: validates typical usage scenarios and common data types
 *  - DataCapability: validates system capability limits and capacity testing
 *  - DataBoundary: validates boundary conditions, exceptional inputs and error handling
 *  - DataState: validates connection and state boundary behaviors
 *  - DataPerformance: validates performance characteristics and optimization scenarios
 *
 *  Reference documentation:
 *  - README_ArchDesign.md::MSG::DAT (boundary conditions section)
 *  - README_RefAPIs.md::IOC_sendDAT/IOC_recvDAT (error codes)
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING DESIGN==============================================================

/**************************************************************************************************
 * 📋 DAT BOUNDARY TEST FOCUS
 *
 * 🎯 DESIGN PRINCIPLE: Validate DAT behavior under boundary conditions and error handling capabilities
 * 🔄 TESTING PRIORITY: Parameter boundaries → Data size boundaries → Timeout boundaries → Mode boundaries
 *
 * ✅ BOUNDARY SCENARIOS COVERED:
 *    🔲 Parameter Boundaries: NULL pointers, invalid LinkID, malformed DatDesc, edge case values
 *    📏 Data Size Boundaries: 0 bytes, minimum/maximum data, oversized data (exceeding limits)
 *    ⏱️ Timeout Boundaries: 0 timeout, extremely short/long timeout, timeout behavior validation
 *    🔄 Mode Boundaries: blocking/non-blocking/timeout mode boundary switching
 *
 * ❌ EXCLUDED FROM BOUNDARY TESTING:
 *    ✅ Typical usage scenarios (covered by DataTypical)
 *    🚀 Performance testing and stress testing (covered by DataPerformance)
 *    🔄 Complex concurrency scenarios
 *    🛠️ Failure recovery scenarios
 *    📊 Long-term stability testing
 *
 * 🎯 IMPLEMENTATION FOCUS:
 *    📋 Error code validation and system stability under edge conditions
 *    🔧 System protection against invalid inputs and edge case attacks
 *    ⚡ Deterministic behavior verification at boundary conditions
 *    🛡️ Memory safety and crash prevention with malformed inputs
 *************************************************************************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================
/**************************************************************************************************
 * @brief 【User Story】
 *
 *  US-1: AS a DAT application developer,
 *    I WANT to understand how IOC_sendDAT/IOC_recvDAT behave with boundary parameters,
 *   SO THAT I can handle edge cases properly in my application
 *      AND avoid unexpected crashes or data corruption,
 *      AND implement proper error handling for boundary conditions.
 *
 *  US-2: AS a system integrator,
 *    I WANT to verify DAT handles data size boundaries correctly,
 *   SO THAT I can ensure system stability with minimal/maximal data sizes
 *      AND understand the behavior when data exceeds system limits,
 *      AND plan appropriate data chunking strategies.
 *
 *  US-3: AS a real-time application developer,
 *    I WANT to test DAT timeout and blocking mode boundaries,
 *   SO THAT I can implement proper timeout handling in time-critical scenarios
 *      AND understand the precise behavior of blocking/non-blocking modes,
 *      AND ensure deterministic behavior at timeout boundaries.
 *
 *  US-4: AS a quality assurance engineer,
 *    I WANT to validate comprehensive error code coverage for all boundary conditions,
 *   SO THAT I can ensure consistent error reporting across all boundary scenarios
 *      AND verify that error codes match their documented meanings,
 *      AND confirm that all boundary error paths are properly tested.
 *
 *  US-5: AS a stream processing developer,
 *    I WANT to verify DAT stream granularity behavior across different send/receive patterns,
 *   SO THAT I can ensure data integrity when sending and receiving at different granularities
 *      AND understand how IOC handles byte-by-byte vs block-by-block streaming scenarios,
 *      AND verify stream reconstruction works correctly across granularity boundaries.
 *
 *************************************************************************************************/
//======>END OF USER STORY=========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//=======>BEGIN OF ACCEPTANCE CRITERIA=============================================================
/**************************************************************************************************
 * @brief 【Acceptance Criteria】
 *
 * 🎯 Focus on DAT BOUNDARY testing - validate system behavior and error handling under boundary conditions
 *
 * [@US-1] Parameter boundary validation
 *  AC-1: GIVEN invalid parameters (NULL pointers, invalid LinkID, malformed DatDesc),
 *         WHEN calling IOC_sendDAT or IOC_recvDAT,
 *         THEN system should return appropriate error codes (IOC_RESULT_INVALID_PARAM, IOC_RESULT_NOT_EXIST_LINK)
 *          AND not crash or corrupt memory,
 *          AND handle each invalid parameter combination gracefully,
 *          AND maintain system state consistency after invalid calls.
 *
 *  AC-2: GIVEN boundary parameter values (edge case LinkIDs, extreme option values),
 *         WHEN performing DAT operations,
 *         THEN system should validate parameters properly
 *          AND reject invalid boundary values with IOC_RESULT_INVALID_PARAM,
 *          AND accept valid boundary values with IOC_RESULT_SUCCESS or appropriate status,
 *          AND provide consistent validation behavior across all parameter types.
 *
 *  AC-3: GIVEN invalid IOC_Options parameter combinations,
 *         WHEN calling IOC_sendDAT or IOC_recvDAT with malformed options,
 *         THEN system should return IOC_RESULT_INVALID_PARAM
 *          AND not attempt the operation with invalid options,
 *          AND validate options before processing other parameters.
 *
 *  AC-4: GIVEN mixed valid/invalid parameter combinations,
 *         WHEN calling DAT functions with some valid and some invalid parameters,
 *         THEN system should prioritize parameter validation order consistently
 *          AND return the most appropriate error code for the first invalid parameter detected,
 *          AND not process any operation when any parameter is invalid.
 *
 *-------------------------------------------------------------------------------------------------
 * [@US-2] Data size boundary validation
 *  AC-1: GIVEN zero-size data (0 bytes),
 *         WHEN calling IOC_sendDAT with empty payload,
 *         THEN system should handle empty data appropriately
 *          AND return consistent behavior (success, defined error, or IOC_RESULT_ZERO_DATA),
 *          AND receiver should handle zero-size data correctly when applicable.
 *
 *  AC-2: GIVEN maximum allowed data size,
 *         WHEN sending data at the size limit,
 *         THEN transmission should succeed
 *          AND data integrity should be maintained,
 *          AND performance should remain reasonable.
 *
 *  AC-3: GIVEN data exceeding maximum allowed size,
 *         WHEN calling IOC_sendDAT with oversized payload,
 *         THEN system should return IOC_RESULT_DATA_TOO_LARGE
 *          AND not attempt transmission,
 *          AND not cause memory issues or system instability.
 *
 *-------------------------------------------------------------------------------------------------
 * [@US-3] Timeout and blocking mode boundaries
 *  AC-1: GIVEN zero timeout configuration,
 *         WHEN performing DAT operations with immediate timeout,
 *         THEN system should return immediately (IOC_RESULT_TIMEOUT or IOC_RESULT_SUCCESS)
 *          AND not block indefinitely,
 *          AND provide consistent timing behavior.
 *
 *  AC-2: GIVEN blocking vs non-blocking mode switches,
 *         WHEN transitioning between different blocking modes,
 *         THEN each mode should behave according to specification
 *          AND mode transitions should be clean and predictable,
 *          AND no data should be lost during mode changes.
 *
 *  AC-3: GIVEN extreme timeout values (very small, very large),
 *         WHEN configuring timeout boundaries,
 *         THEN system should handle timeout edge cases properly
 *          AND respect timeout constraints accurately,
 *          AND not overflow or underflow time calculations.
 *
 *-------------------------------------------------------------------------------------------------
 * [@US-4] Error code coverage validation
 *  AC-1: GIVEN all boundary error scenarios (invalid params, oversized data, timeouts, mode conflicts),
 *         WHEN performing comprehensive boundary testing across all DAT operations,
 *         THEN every boundary condition should return specific, documented error codes
 *          AND error codes should be consistent across similar boundary scenarios,
 *          AND no boundary condition should result in undefined behavior or missing error reporting,
 *          AND error code coverage should include all documented IOC_RESULT_* values for DAT operations.
 *
 *-------------------------------------------------------------------------------------------------
 * [@US-5] Stream granularity boundary validation
 *  AC-1: GIVEN DAT stream with byte-by-byte sending and block-by-block receiving,
 *         WHEN sender calls IOC_sendDAT with 1-byte chunks repeatedly,
 *         THEN receiver should reconstruct data correctly via IOC_recvDAT or callback
 *          AND data integrity should be maintained across granularity boundaries,
 *          AND stream ordering should be preserved regardless of receive granularity,
 *          AND no data should be lost or duplicated during granularity conversion.
 *
 *  AC-2: GIVEN DAT stream with block-by-block sending and byte-by-byte receiving,
 *         WHEN sender calls IOC_sendDAT with large chunks (1KB+),
 *         THEN receiver should be able to receive data in smaller fragments
 *          AND partial reception should work correctly with IOC_recvDAT,
 *          AND callback reception should handle large chunks appropriately,
 *          AND stream boundaries should not cause data corruption or loss.
 *
 *  AC-3: GIVEN DAT stream with variable granularity patterns,
 *         WHEN alternating between different send/receive chunk sizes during transmission,
 *         THEN stream consistency should be maintained throughout granularity changes
 *          AND data reconstruction should work correctly for mixed-size patterns,
 *          AND system should handle rapid granularity switching without buffer issues,
 *          AND end-to-end data integrity should be verifiable across all granularity combinations.
 *************************************************************************************************/
//=======>END OF ACCEPTANCE CRITERIA================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES ORGANIZATION==========================================================
/**************************************************************************************************
 * @brief 【Test Cases Organization】
 *
 * Test Cases are organized by User Story and implemented in separate files:
 *
 * 📂 UT_DataBoundaryUS1.cxx - [@US-1] Parameter boundary validation
 *    └── [@AC-1,US-1] TC-1: verifyDatParameterBoundary_byInvalidInputs_expectGracefulErrorHandling
 *    └── [@AC-2,US-1] TC-2: verifyDatParameterBoundary_byEdgeCaseValues_expectValidationSuccess
 *    └── TODO: [@AC-3,US-1] IOC_Options boundary validation
 *    └── TODO: [@AC-4,US-1] Mixed valid/invalid parameter combinations
 *
 * 📂 UT_DataBoundaryUS2.cxx - [@US-2] Data size boundary validation
 *    └── [@AC-1,US-2] TC-1: verifyDatDataSizeBoundary_byZeroSizeData_expectConsistentBehavior
 *    └── [@AC-1,US-2] TC-2: verifyDatDataSizeBoundary_byZeroSizeEdgeCases_expectRobustHandling
 *    └── TODO: [@AC-2,US-2] Maximum data size boundary validation
 *    └── TODO: [@AC-3,US-2] Oversized data boundary validation
 *
 * 📂 UT_DataBoundaryUS3.cxx - [@US-3] Timeout and blocking mode boundaries (TODO)
 *    └── TODO: [@AC-1,US-3] Zero timeout boundary validation
 *    └── TODO: [@AC-2,US-3] Blocking mode boundaries
 *    └── TODO: [@AC-3,US-3] Extreme timeout boundaries
 *
 * 📂 UT_DataBoundaryUS4.cxx - [@US-4] Error code coverage validation (TODO)
 *    └── TODO: Comprehensive error code boundary testing
 *
 * 📂 UT_DataBoundaryUS5.cxx - [@US-5] Stream granularity boundary validation
 *    └── [@AC-1,US-5] TC-1: verifyDatStreamGranularity_byByteToBlockPattern_expectDataIntegrity
 *    └── [@AC-1,US-5] TC-2: verifyDatStreamGranularity_byBurstThenPausePattern_expectBatchingBehavior
 *    └── [@AC-2,US-5] TC-1: verifyDatStreamGranularity_byBlockToBytePattern_expectFragmentationSupport
 *    └── [@AC-3,US-5] TC-1: verifyDatStreamGranularity_byVariablePatterns_expectConsistentBehavior
 *************************************************************************************************/
//======>END OF TEST CASES ORGANIZATION============================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF SHARED TEST ENVIRONMENT SETUP===================================================

#include <chrono>
#include <vector>

#include "_UT_IOC_Common.h"

// Private data structure for DAT boundary testing callbacks
typedef struct {
    bool CallbackExecuted;
    int ClientIndex;
    ULONG_T TotalReceivedSize;
    ULONG_T ReceivedDataCnt;
    char ReceivedContent[2048];       // Buffer for data verification (increased for granularity tests)
    ULONG_T ReceivedContentWritePos;  // Current write position in ReceivedContent buffer

    // Boundary-specific tracking
    bool ZeroSizeDataReceived;
    bool MaxSizeDataReceived;
    bool ErrorOccurred;
    IOC_Result_T LastErrorCode;

    // Additional boundary tracking
    ULONG_T MaxDataSizeReceived;
    bool TimeoutOccurred;
    bool BlockingModeChanged;
    ULONG_T CallbackExecutionTime;  // For timeout testing

    // Timing tracking for batching behavior analysis
    std::chrono::high_resolution_clock::time_point FirstCallbackTime;
    std::chrono::high_resolution_clock::time_point LastCallbackTime;
    bool FirstCallbackRecorded;
    ULONG_T LargestSingleCallback;       // Track largest single callback size
    std::vector<ULONG_T> CallbackSizes;  // Track all callback sizes for analysis
} __DatBoundaryPrivData_T;

// Callback function for DAT boundary testing
static IOC_Result_T __CbRecvDat_Boundary_F(IOC_LinkID_T LinkID, IOC_DatDesc_pT pDatDesc, void *pCbPriv) {
    __DatBoundaryPrivData_T *pPrivData = (__DatBoundaryPrivData_T *)pCbPriv;

    // Extract data from DatDesc
    void *pData;
    ULONG_T DataSize;
    IOC_Result_T result = IOC_getDatPayload(pDatDesc, &pData, &DataSize);
    if (result != IOC_RESULT_SUCCESS) {
        pPrivData->ErrorOccurred = true;
        pPrivData->LastErrorCode = result;
        return result;
    }

    pPrivData->ReceivedDataCnt++;
    pPrivData->CallbackExecuted = true;
    pPrivData->TotalReceivedSize += DataSize;

    // Record timing for batching analysis
    auto currentTime = std::chrono::high_resolution_clock::now();
    if (!pPrivData->FirstCallbackRecorded) {
        pPrivData->FirstCallbackTime = currentTime;
        pPrivData->FirstCallbackRecorded = true;
    }
    pPrivData->LastCallbackTime = currentTime;

    // Track callback sizes for batching analysis
    pPrivData->CallbackSizes.push_back(DataSize);
    if (DataSize > pPrivData->LargestSingleCallback) {
        pPrivData->LargestSingleCallback = DataSize;
    }

    // Track boundary conditions
    if (DataSize == 0) {
        pPrivData->ZeroSizeDataReceived = true;
    }

    // Copy small data for verification (if space available)
    if (DataSize > 0 && (pPrivData->ReceivedContentWritePos + DataSize) <= sizeof(pPrivData->ReceivedContent)) {
        memcpy(pPrivData->ReceivedContent + pPrivData->ReceivedContentWritePos, pData, DataSize);
        pPrivData->ReceivedContentWritePos += DataSize;
    }

    printf("DAT Boundary Callback: Client[%d], received %lu bytes, total: %lu bytes\n", pPrivData->ClientIndex,
           DataSize, pPrivData->TotalReceivedSize);
    return IOC_RESULT_SUCCESS;
}

//======>END OF SHARED TEST ENVIRONMENT SETUP=====================================================

#endif  // UT_DATABOUNDARY_H
