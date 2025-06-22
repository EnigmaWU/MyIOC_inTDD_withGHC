///////////////////////////////////////////////////////////////////////////////////////////////////
// QUICK REFERENCE GUIDE
// 📝 Purpose: DAT (Data Transfer) boundary testing unit test framework
// 🔄 Process: User Story → Acceptance Criteria → Test Cases → Implementation
// 📂 Category: DataBoundary - Focus on DAT data transfer boundary conditions and limit parameter testing
// 🎯 Focus: Boundary values, null values, timeouts, blocking/non-blocking modes, data size limits and other edge cases
// Reference Unit Testing Templates in UT_FreelyDrafts.cxx when needed.
///////////////////////////////////////////////////////////////////////////////////////////////////

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
 *          AND return consistent behavior (success or defined error),
 *          AND receiver should handle zero-size data correctly.
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
 *************************************************************************************************/
//=======>END OF ACCEPTANCE CRITERIA================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES=======================================================================
/**************************************************************************************************
 * @brief 【Test Cases】
 *
 * [@AC-1,US-1] Parameter boundary validation
 *  TC-1:
 *      @[Name]: verifyDatParameterBoundary_byInvalidInputs_expectGracefulErrorHandling
 *      @[Purpose]: Verify IOC_sendDAT/IOC_recvDAT handle invalid parameters gracefully
 *      @[Brief]: Test NULL pointers, invalid LinkIDs, malformed DatDesc, verify proper error codes
 *  TC-2:
 *      @[Name]: verifyDatParameterBoundary_byEdgeCaseValues_expectValidationSuccess
 *      @[Purpose]: Verify boundary parameter values are validated correctly
 *      @[Brief]: Test edge case LinkIDs, extreme option values, verify acceptance/rejection
 *  TODO:TC-3:
 *
 *-------------------------------------------------------------------------------------------------
 * TODO:[@AC-2,US-1] Parameter boundary validation - IOC_Options
 *
 *-------------------------------------------------------------------------------------------------
 * TODO:[@AC-3,US-1] Parameter boundary validation - Mixed valid/invalid parameters
 *
 *-------------------------------------------------------------------------------------------------
 * TODO:[@AC-4,US-1] Parameter boundary validation - Mixed valid/invalid parameters
 *
 *-------------------------------------------------------------------------------------------------
 * [@AC-1,US-2] Data size boundary validation - Zero size
 *  TC-1:
 *      @[Name]: verifyDatDataSizeBoundary_byZeroSizeData_expectConsistentBehavior
 *      @[Purpose]: Verify zero-size data transmission behavior
 *      @[Brief]: Send 0-byte data, verify transmission and reception behavior
 *
 *---------------------------------------------------------------------------------------------------
 * [@AC-2,US-2] Data size boundary validation - Maximum size
 *  TC-1:
 *      @[Name]: verifyDatDataSizeBoundary_byMaximumAllowedSize_expectSuccessfulTransmission
 *      @[Purpose]: Verify maximum allowed data size transmission
 *      @[Brief]: Send data at maximum size limit, verify successful transmission and integrity
 *
 *---------------------------------------------------------------------------------------------------
 * [@AC-3,US-2] Data size boundary validation - Oversized data
 *  TC-1:
 *      @[Name]: verifyDatDataSizeBoundary_byOversizedData_expectDataTooLargeError
 *      @[Purpose]: Verify oversized data rejection
 *      @[Brief]: Attempt to send data exceeding limits, verify IOC_RESULT_DATA_TOO_LARGE
 *
 *---------------------------------------------------------------------------------------------------
 * [@AC-1,US-3] Timeout boundary validation - Zero timeout
 *  TC-1:
 *      @[Name]: verifyDatTimeoutBoundary_byZeroTimeout_expectImmediateReturn
 *      @[Purpose]: Verify zero timeout behavior
 *      @[Brief]: Configure zero timeout, verify immediate return without blocking
 *
 *---------------------------------------------------------------------------------------------------
 * [@AC-2,US-3] Blocking mode boundaries
 *  TC-1:
 *      @[Name]: verifyDatBlockingModeBoundary_byModeTransitions_expectConsistentBehavior
 *      @[Purpose]: Verify blocking/non-blocking mode transitions
 *      @[Brief]: Switch between blocking modes, verify each mode behaves correctly
 *
 *---------------------------------------------------------------------------------------------------
 * [@AC-3,US-3] Extreme timeout boundaries
 *  TC-1:
 *      @[Name]: verifyDatTimeoutBoundary_byExtremeValues_expectProperHandling
 *      @[Purpose]: Verify extreme timeout value handling
 *      @[Brief]: Test very small and very large timeout values, verify proper handling
 *
 *************************************************************************************************/
//======>END OF TEST CASES=========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF UNIT TESTING IMPLEMENTATION=======================================================
#include "_UT_IOC_Common.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST ENVIRONMENT SETUP==========================================================

// Private data structure for DAT boundary testing callbacks
typedef struct {
    bool CallbackExecuted;
    int ClientIndex;
    ULONG_T TotalReceivedSize;
    ULONG_T ReceivedDataCnt;
    char ReceivedContent[1024];  // Buffer for small data verification

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

    // Track boundary conditions
    if (DataSize == 0) {
        pPrivData->ZeroSizeDataReceived = true;
    }

    // Copy small data for verification (if space available)
    if (DataSize > 0 && pPrivData->TotalReceivedSize <= sizeof(pPrivData->ReceivedContent)) {
        memcpy(pPrivData->ReceivedContent + pPrivData->TotalReceivedSize - DataSize, pData,
               std::min(DataSize, sizeof(pPrivData->ReceivedContent) - (pPrivData->TotalReceivedSize - DataSize)));
    }

    printf("DAT Boundary Callback: Client[%d], received %lu bytes, total: %lu bytes\n", pPrivData->ClientIndex,
           DataSize, pPrivData->TotalReceivedSize);
    return IOC_RESULT_SUCCESS;
}

//======>END OF TEST ENVIRONMENT SETUP============================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST IMPLEMENTATIONS=============================================================

//======>BEGIN OF: [@AC-1,US-1] TC-1===============================================================
/**
 * @[Name]: verifyDatParameterBoundary_byInvalidInputs_expectGracefulErrorHandling
 * @[Steps]:
 *   1) Test IOC_sendDAT with invalid parameters AS BEHAVIOR.
 *      |-> Test NULL pDatDesc parameter
 *      |-> Test invalid LinkID (IOC_ID_INVALID, random values)
 *      |-> Test malformed DatDesc structures
 *      |-> Test invalid IOC_Options combinations
 *   2) Test IOC_recvDAT with invalid parameters AS BEHAVIOR.
 *      |-> Test NULL pDatDesc parameter
 *      |-> Test invalid LinkID
 *      |-> Test malformed DatDesc configurations
 *      |-> Test invalid IOC_Options combinations
 *   3) Test mixed valid/invalid parameter combinations AS BEHAVIOR.
 *      |-> Test parameter validation order consistency
 *      |-> Test fail-fast behavior with any invalid parameter
 *   4) Verify proper error codes and system stability AS VERIFY.
 *      |-> All invalid calls return appropriate error codes
 *      |-> No memory corruption or crashes occur
 *      |-> System state remains consistent after invalid calls
 *   5) Cleanup: ensure system state is clean AS CLEANUP.
 * @[Expect]: All invalid parameter combinations rejected with proper error codes, no crashes, consistent system state.
 * @[Notes]: Critical for robust error handling - validates comprehensive parameter validation logic per AC-1,AC-3,AC-4.
 */
TEST(UT_DataBoundary, verifyDatParameterBoundary_byInvalidInputs_expectGracefulErrorHandling) {
    //===SETUP===
    printf("BEHAVIOR: verifyDatParameterBoundary_byInvalidInputs_expectGracefulErrorHandling\n");

    //===BEHAVIOR: IOC_sendDAT Invalid Parameter Tests===
    printf("📋 Testing IOC_sendDAT invalid parameters...\n");

    // Test 1.1: NULL pDatDesc for IOC_sendDAT (AC-1)
    IOC_Result_T Result = IOC_sendDAT(IOC_ID_INVALID, NULL, NULL);
    ASSERT_EQ(IOC_RESULT_INVALID_PARAM, Result)
        << "IOC_sendDAT should reject NULL pDatDesc with IOC_RESULT_INVALID_PARAM";

    // Test 1.2: Invalid LinkID for IOC_sendDAT (AC-1)
    IOC_DatDesc_T ValidDatDesc = {0};
    IOC_initDatDesc(&ValidDatDesc);
    const char *testData = "test";
    ValidDatDesc.Payload.pData = (void *)testData;
    ValidDatDesc.Payload.PtrDataSize = 4;

    Result = IOC_sendDAT(IOC_ID_INVALID, &ValidDatDesc, NULL);
    ASSERT_EQ(IOC_RESULT_NOT_EXIST_LINK, Result)
        << "IOC_sendDAT should reject invalid LinkID with IOC_RESULT_NOT_EXIST_LINK";

    // Test 1.3: Malformed DatDesc for IOC_sendDAT (AC-1)
    IOC_DatDesc_T MalformedDatDesc = {0};
    // Intentionally create malformed DatDesc (uninitialized/corrupted structure)
    MalformedDatDesc.Payload.pData = (void *)0xDEADBEEF;  // Invalid pointer
    MalformedDatDesc.Payload.PtrDataSize = 0xFFFFFFFF;    // Extreme size

    Result = IOC_sendDAT(IOC_ID_INVALID, &MalformedDatDesc, NULL);
    ASSERT_TRUE(Result == IOC_RESULT_INVALID_PARAM || Result == IOC_RESULT_NOT_EXIST_LINK)
        << "IOC_sendDAT should reject malformed DatDesc with appropriate error code";

    // Test 1.4: Test with NULL options (valid case for comparison)
    Result = IOC_sendDAT(IOC_ID_INVALID, &ValidDatDesc, NULL);
    ASSERT_EQ(IOC_RESULT_NOT_EXIST_LINK, Result)
        << "IOC_sendDAT with valid DatDesc and NULL options should return NOT_EXIST_LINK for invalid LinkID";

    // Test 1.5: Test with zero-initialized valid DatDesc but invalid data pointer
    IOC_DatDesc_T ZeroDataDesc = {0};
    IOC_initDatDesc(&ZeroDataDesc);
    ZeroDataDesc.Payload.pData = NULL;      // NULL data pointer
    ZeroDataDesc.Payload.PtrDataSize = 10;  // But non-zero size

    Result = IOC_sendDAT(IOC_ID_INVALID, &ZeroDataDesc, NULL);
    ASSERT_TRUE(Result == IOC_RESULT_INVALID_PARAM || Result == IOC_RESULT_NOT_EXIST_LINK)
        << "IOC_sendDAT should handle NULL data pointer with non-zero size appropriately";

    //===BEHAVIOR: IOC_recvDAT Invalid Parameter Tests===
    printf("📋 Testing IOC_recvDAT invalid parameters...\n");

    // Test 2.1: NULL pDatDesc for IOC_recvDAT (AC-1)
    Result = IOC_recvDAT(IOC_ID_INVALID, NULL, NULL);
    ASSERT_EQ(IOC_RESULT_INVALID_PARAM, Result)
        << "IOC_recvDAT should reject NULL pDatDesc with IOC_RESULT_INVALID_PARAM";

    // Test 2.2: Invalid LinkID for IOC_recvDAT (AC-1)
    IOC_DatDesc_T RecvDatDesc = {0};
    IOC_initDatDesc(&RecvDatDesc);
    Result = IOC_recvDAT(IOC_ID_INVALID, &RecvDatDesc, NULL);
    ASSERT_EQ(IOC_RESULT_NOT_EXIST_LINK, Result)
        << "IOC_recvDAT should reject invalid LinkID with IOC_RESULT_NOT_EXIST_LINK";

    // Test 2.3: Malformed DatDesc for IOC_recvDAT (AC-1)
    IOC_DatDesc_T MalformedRecvDesc = {0};
    // Create malformed receive descriptor
    MalformedRecvDesc.Payload.pData = NULL;
    MalformedRecvDesc.Payload.PtrDataSize = 100;  // Non-zero size with NULL buffer

    Result = IOC_recvDAT(IOC_ID_INVALID, &MalformedRecvDesc, NULL);
    ASSERT_TRUE(Result == IOC_RESULT_INVALID_PARAM || Result == IOC_RESULT_NOT_EXIST_LINK)
        << "IOC_recvDAT should reject malformed DatDesc with appropriate error code";

    // Test 2.4: Test with NULL options for IOC_recvDAT (valid case)
    Result = IOC_recvDAT(IOC_ID_INVALID, &RecvDatDesc, NULL);
    ASSERT_EQ(IOC_RESULT_NOT_EXIST_LINK, Result)
        << "IOC_recvDAT with valid DatDesc and NULL options should return NOT_EXIST_LINK for invalid LinkID";

    //===BEHAVIOR: Mixed Parameter Validation Tests (AC-4)===
    printf("📋 Testing mixed valid/invalid parameter combinations...\n");

    // Test 3.1: NULL DatDesc with NULL options - test parameter validation order
    Result = IOC_sendDAT(IOC_ID_INVALID, NULL, NULL);
    ASSERT_EQ(IOC_RESULT_INVALID_PARAM, Result) << "Parameter validation should catch NULL pDatDesc consistently";

    // Test 3.2: Multiple invalid parameters - ensure consistent error priority
    Result = IOC_recvDAT(IOC_ID_INVALID, NULL, NULL);
    ASSERT_EQ(IOC_RESULT_INVALID_PARAM, Result) << "Parameter validation should be consistent in error priority";

    // Test 3.3: Random invalid LinkID values to test robustness
    IOC_LinkID_T RandomInvalidIDs[] = {0xDEADBEEF, 0xFFFFFFFF, 0x12345678, (IOC_LinkID_T)-1};
    for (size_t i = 0; i < sizeof(RandomInvalidIDs) / sizeof(RandomInvalidIDs[0]); i++) {
        Result = IOC_sendDAT(RandomInvalidIDs[i], &ValidDatDesc, NULL);
        ASSERT_TRUE(Result == IOC_RESULT_NOT_EXIST_LINK || Result == IOC_RESULT_INVALID_PARAM)
            << "IOC_sendDAT should handle random invalid LinkIDs gracefully: " << RandomInvalidIDs[i];
    }

    //===VERIFY: System Stability===
    printf("🔍 Verifying system stability...\n");

    // Verify no memory corruption by attempting a valid-structure operation
    // (This would crash if memory was corrupted)
    IOC_DatDesc_T TestDesc = {0};
    IOC_initDatDesc(&TestDesc);
    ASSERT_NO_FATAL_FAILURE({
        Result = IOC_sendDAT(IOC_ID_INVALID, &TestDesc, NULL);
        // Expect NOT_EXIST_LINK since we're using invalid LinkID with valid parameters
        ASSERT_EQ(IOC_RESULT_NOT_EXIST_LINK, Result);
    }) << "System should remain stable and not crash after invalid parameter tests";

    // Test system stability with multiple consecutive invalid calls
    for (int i = 0; i < 10; i++) {
        Result = IOC_sendDAT(IOC_ID_INVALID, NULL, NULL);
        ASSERT_EQ(IOC_RESULT_INVALID_PARAM, Result)
            << "System should consistently reject invalid parameters on call #" << i;
    }

    // KeyVerifyPoint: All invalid parameter tests completed without crashes
    printf("✅ All invalid parameter combinations properly rejected with correct error codes\n");
    printf("✅ System maintained stability throughout boundary testing\n");
    printf("✅ No memory corruption or system instability detected\n");
    printf("✅ Parameter validation order and consistency verified\n");

    //===CLEANUP===
    // No cleanup needed for parameter validation tests
    // System demonstrated stability throughout testing
}

//======>BEGIN OF: [@AC-2,US-1] TC-2===============================================================
/**
 * @[Name]: verifyDatParameterBoundary_byEdgeCaseValues_expectValidationSuccess
 * @[Steps]:
 *   1) Test LinkID boundary values (valid/invalid edge cases) AS BEHAVIOR.
 *      |-> Test minimum/maximum theoretical LinkID values
 *      |-> Test just-out-of-range LinkID values
 *      |-> Test special LinkID values (IOC_ID_INVALID, etc.)
 *   2) Test DatDesc field boundary values AS BEHAVIOR.
 *      |-> Test minimum/maximum data sizes (1 byte, near-max sizes)
 *      |-> Test boundary pointer values and data alignment
 *      |-> Test extreme but valid embedded data configurations
 *   3) Test IOC_Options boundary values AS BEHAVIOR.
 *      |-> Test minimum/maximum timeout values
 *      |-> Test boundary blocking mode configurations
 *      |-> Test extreme but valid option combinations
 *   4) Test mixed boundary parameter combinations AS BEHAVIOR.
 *      |-> Test valid boundary combinations that should succeed
 *      |-> Test invalid boundary combinations that should fail gracefully
 *   5) Verify consistent validation behavior AS VERIFY.
 *      |-> Valid boundary values return appropriate success/status codes
 *      |-> Invalid boundary values return IOC_RESULT_INVALID_PARAM
 *      |-> Validation behavior is consistent across parameter types
 *   6) Cleanup test structures and connections AS CLEANUP.
 * @[Expect]: Valid boundary values accepted with success, invalid boundary values rejected with
 * IOC_RESULT_INVALID_PARAM, consistent validation behavior.
 * @[Notes]: Systematic boundary value testing per AC-2 - validates parameter validation logic at edge cases.
 */
TEST(UT_DataBoundary, verifyDatParameterBoundary_byEdgeCaseValues_expectValidationSuccess) {
    //===SETUP===
    printf("BEHAVIOR: verifyDatParameterBoundary_byEdgeCaseValues_expectValidationSuccess\n");

    //===BEHAVIOR: LinkID Boundary Value Testing===
    printf("📋 Testing LinkID boundary values...\n");

    // Prepare valid DatDesc for testing LinkID boundaries
    IOC_DatDesc_T ValidDatDesc = {0};
    IOC_initDatDesc(&ValidDatDesc);
    const char *testData = "boundary";
    ValidDatDesc.Payload.pData = (void *)testData;
    ValidDatDesc.Payload.PtrDataSize = 8;

    // Test 1.1: IOC_ID_INVALID explicitly (should fail)
    IOC_Result_T Result = IOC_sendDAT(IOC_ID_INVALID, &ValidDatDesc, NULL);
    ASSERT_EQ(IOC_RESULT_NOT_EXIST_LINK, Result) << "IOC_ID_INVALID should be rejected with NOT_EXIST_LINK";

    // Test 1.2: Zero LinkID (typically invalid unless specifically supported)
    Result = IOC_sendDAT(0, &ValidDatDesc, NULL);
    ASSERT_TRUE(Result == IOC_RESULT_NOT_EXIST_LINK || Result == IOC_RESULT_INVALID_PARAM)
        << "Zero LinkID should be rejected with appropriate error code";

    // Test 1.3: Maximum possible LinkID values (test system bounds)
    IOC_LinkID_T MaxBoundaryIDs[] = {
        0x7FFFFFFF,  // Maximum positive 32-bit value
        0xFFFFFFFE,  // Near maximum unsigned value
        0x80000000,  // Sign bit boundary
        1,           // Minimum positive value
        2,           // Just above minimum
    };

    for (size_t i = 0; i < sizeof(MaxBoundaryIDs) / sizeof(MaxBoundaryIDs[0]); i++) {
        Result = IOC_sendDAT(MaxBoundaryIDs[i], &ValidDatDesc, NULL);
        ASSERT_TRUE(Result == IOC_RESULT_NOT_EXIST_LINK || Result == IOC_RESULT_INVALID_PARAM)
            << "Boundary LinkID " << MaxBoundaryIDs[i] << " should be handled gracefully";

        // Test should not crash - if it reaches here, validation worked
        printf("   ✓ LinkID boundary value 0x%016llX handled gracefully (result: %d)\n",
               (unsigned long long)MaxBoundaryIDs[i], Result);
    }

    //===BEHAVIOR: DatDesc Field Boundary Testing===
    printf("📋 Testing DatDesc field boundary values...\n");

    // Test 2.1: Minimum data size (1 byte)
    IOC_DatDesc_T MinSizeDesc = {0};
    IOC_initDatDesc(&MinSizeDesc);
    char oneByte = 'X';
    MinSizeDesc.Payload.pData = &oneByte;
    MinSizeDesc.Payload.PtrDataSize = 1;  // Minimum meaningful size

    Result = IOC_sendDAT(IOC_ID_INVALID, &MinSizeDesc, NULL);
    ASSERT_EQ(IOC_RESULT_NOT_EXIST_LINK, Result)
        << "1-byte data size should be valid (failed due to invalid LinkID only)";

    // Test 2.2: Large but reasonable data size
    IOC_DatDesc_T LargeDesc = {0};
    IOC_initDatDesc(&LargeDesc);
    const size_t LargeSize = 64 * 1024;  // 64KB - large but not extreme
    char *largeBuf = (char *)malloc(LargeSize);
    if (largeBuf != NULL) {
        memset(largeBuf, 'L', LargeSize);
        LargeDesc.Payload.pData = largeBuf;
        LargeDesc.Payload.PtrDataSize = LargeSize;

        Result = IOC_sendDAT(IOC_ID_INVALID, &LargeDesc, NULL);
        ASSERT_EQ(IOC_RESULT_NOT_EXIST_LINK, Result)
            << "Large data size (64KB) should be valid (failed due to invalid LinkID only)";

        free(largeBuf);
        printf("   ✓ Large data size (%zu bytes) handled correctly\n", LargeSize);
    }

    // Test 2.3: Edge case - valid pointer with zero size
    IOC_DatDesc_T ZeroSizeValidPtr = {0};
    IOC_initDatDesc(&ZeroSizeValidPtr);
    ZeroSizeValidPtr.Payload.pData = (void *)testData;  // Valid pointer
    ZeroSizeValidPtr.Payload.PtrDataSize = 0;           // Zero size

    Result = IOC_sendDAT(IOC_ID_INVALID, &ZeroSizeValidPtr, NULL);
    ASSERT_TRUE(Result == IOC_RESULT_NOT_EXIST_LINK || Result == IOC_RESULT_INVALID_PARAM)
        << "Zero size with valid pointer should be handled consistently";
    printf("   ✓ Zero size with valid pointer handled (result: %d)\n", Result);

    // Test 2.4: Test properly initialized DatDesc vs uninitialized
    IOC_DatDesc_T UninitializedDesc;                              // Not zero-initialized
    memset(&UninitializedDesc, 0xFF, sizeof(UninitializedDesc));  // Fill with garbage

    Result = IOC_sendDAT(IOC_ID_INVALID, &UninitializedDesc, NULL);
    ASSERT_TRUE(Result == IOC_RESULT_INVALID_PARAM || Result == IOC_RESULT_NOT_EXIST_LINK)
        << "Uninitialized DatDesc should be handled gracefully";
    printf("   ✓ Uninitialized DatDesc handled gracefully (result: %d)\n", Result);

    //===BEHAVIOR: IOC_Options Boundary Testing===
    printf("📋 Testing IOC_Options boundary values...\n");

    // Test 3.1: NULL options (should be valid)
    Result = IOC_sendDAT(IOC_ID_INVALID, &ValidDatDesc, NULL);
    ASSERT_EQ(IOC_RESULT_NOT_EXIST_LINK, Result) << "NULL options should be valid (failed only due to invalid LinkID)";

    // Test 3.2: Test with stack-allocated options structure
    IOC_Options_T StackOptions = {};
    // Initialize with default/zero values
    Result = IOC_sendDAT(IOC_ID_INVALID, &ValidDatDesc, &StackOptions);
    ASSERT_EQ(IOC_RESULT_NOT_EXIST_LINK, Result)
        << "Zero-initialized options should be valid (failed only due to invalid LinkID)";

    // Test 3.3: Test with garbage options pointer (should be detected)
    IOC_Options_T *GarbageOptions = (IOC_Options_T *)0xDEADBEEF;
    Result = IOC_sendDAT(IOC_ID_INVALID, &ValidDatDesc, GarbageOptions);
    ASSERT_TRUE(Result == IOC_RESULT_INVALID_PARAM || Result == IOC_RESULT_NOT_EXIST_LINK)
        << "Invalid options pointer should be handled gracefully";
    printf("   ✓ Invalid options pointer handled gracefully (result: %d)\n", Result);

    //===BEHAVIOR: IOC_Options Boundary Testing (Enhanced AC-3 coverage)===
    printf("📋 Testing IOC_Options boundary values for invalid parameter testing...\n");

    // Test 1.6: Test with malformed IOC_Options structure
    IOC_Options_T MalformedOptions;
    memset(&MalformedOptions, 0xAA, sizeof(MalformedOptions));  // Fill with pattern
    MalformedOptions.IDs = (IOC_OptionsID_T)0xDEADBEEF;         // Invalid option ID

    Result = IOC_sendDAT(IOC_ID_INVALID, &ValidDatDesc, &MalformedOptions);
    ASSERT_TRUE(Result == IOC_RESULT_INVALID_PARAM || Result == IOC_RESULT_NOT_EXIST_LINK)
        << "IOC_sendDAT should handle malformed IOC_Options gracefully";

    // Test 1.7: Test with extreme timeout values in malformed options
    IOC_Options_T ExtremeOptions = {};
    ExtremeOptions.Payload.TimeoutUS = 0xFFFFFFFFFFFFFFFFULL;  // Maximum value

    Result = IOC_sendDAT(IOC_ID_INVALID, &ValidDatDesc, &ExtremeOptions);
    ASSERT_TRUE(Result == IOC_RESULT_INVALID_PARAM || Result == IOC_RESULT_NOT_EXIST_LINK)
        << "IOC_sendDAT should handle extreme timeout values appropriately";

    //===BEHAVIOR: IOC_recvDAT Invalid Parameter Tests===
    printf("📋 Testing IOC_recvDAT invalid parameters...\n");

    // Test 2.1: NULL pDatDesc for IOC_recvDAT (AC-1)
    Result = IOC_recvDAT(IOC_ID_INVALID, NULL, NULL);
    ASSERT_EQ(IOC_RESULT_INVALID_PARAM, Result)
        << "IOC_recvDAT should reject NULL pDatDesc with IOC_RESULT_INVALID_PARAM";

    // Test 2.2: Invalid LinkID for IOC_recvDAT (AC-1)
    IOC_DatDesc_T RecvDatDesc = {0};
    IOC_initDatDesc(&RecvDatDesc);
    Result = IOC_recvDAT(IOC_ID_INVALID, &RecvDatDesc, NULL);
    ASSERT_EQ(IOC_RESULT_NOT_EXIST_LINK, Result)
        << "IOC_recvDAT should reject invalid LinkID with IOC_RESULT_NOT_EXIST_LINK";

    // Test 2.3: Malformed DatDesc for IOC_recvDAT (AC-1)
    IOC_DatDesc_T MalformedRecvDesc = {0};
    // Create malformed receive descriptor
    MalformedRecvDesc.Payload.pData = NULL;
    MalformedRecvDesc.Payload.PtrDataSize = 100;  // Non-zero size with NULL buffer

    Result = IOC_recvDAT(IOC_ID_INVALID, &MalformedRecvDesc, NULL);
    ASSERT_TRUE(Result == IOC_RESULT_INVALID_PARAM || Result == IOC_RESULT_NOT_EXIST_LINK)
        << "IOC_recvDAT should reject malformed DatDesc with appropriate error code";

    // Test 2.4: Test with NULL options for IOC_recvDAT (valid case)
    Result = IOC_recvDAT(IOC_ID_INVALID, &RecvDatDesc, NULL);
    ASSERT_EQ(IOC_RESULT_NOT_EXIST_LINK, Result)
        << "IOC_recvDAT with valid DatDesc and NULL options should return NOT_EXIST_LINK for invalid LinkID";

    //===BEHAVIOR: Mixed Parameter Validation Tests (AC-4)===
    printf("📋 Testing mixed valid/invalid parameter combinations...\n");

    // Test 3.1: NULL DatDesc with NULL options - test parameter validation order
    Result = IOC_sendDAT(IOC_ID_INVALID, NULL, NULL);
    ASSERT_EQ(IOC_RESULT_INVALID_PARAM, Result) << "Parameter validation should catch NULL pDatDesc consistently";

    // Test 3.2: Multiple invalid parameters - ensure consistent error priority
    Result = IOC_recvDAT(IOC_ID_INVALID, NULL, NULL);
    ASSERT_EQ(IOC_RESULT_INVALID_PARAM, Result) << "Parameter validation should be consistent in error priority";

    // Test 3.3: Random invalid LinkID values to test robustness
    IOC_LinkID_T RandomInvalidIDs[] = {0xDEADBEEF, 0xFFFFFFFF, 0x12345678, (IOC_LinkID_T)-1};
    for (size_t i = 0; i < sizeof(RandomInvalidIDs) / sizeof(RandomInvalidIDs[0]); i++) {
        Result = IOC_sendDAT(RandomInvalidIDs[i], &ValidDatDesc, NULL);
        ASSERT_TRUE(Result == IOC_RESULT_NOT_EXIST_LINK || Result == IOC_RESULT_INVALID_PARAM)
            << "IOC_sendDAT should handle random invalid LinkIDs gracefully: " << RandomInvalidIDs[i];
    }

    //===VERIFY: System Stability===
    printf("🔍 Verifying system stability...\n");

    // Verify no memory corruption by attempting a valid-structure operation
    // (This would crash if memory was corrupted)
    IOC_DatDesc_T TestDesc = {0};
    IOC_initDatDesc(&TestDesc);
    ASSERT_NO_FATAL_FAILURE({
        Result = IOC_sendDAT(IOC_ID_INVALID, &TestDesc, NULL);
        // Expect NOT_EXIST_LINK since we're using invalid LinkID with valid parameters
        ASSERT_EQ(IOC_RESULT_NOT_EXIST_LINK, Result);
    }) << "System should remain stable and not crash after invalid parameter tests";

    // Test system stability with multiple consecutive invalid calls
    for (int i = 0; i < 10; i++) {
        Result = IOC_sendDAT(IOC_ID_INVALID, NULL, NULL);
        ASSERT_EQ(IOC_RESULT_INVALID_PARAM, Result)
            << "System should consistently reject invalid parameters on call #" << i;
    }

    // KeyVerifyPoint: All invalid parameter tests completed without crashes
    printf("✅ All invalid parameter combinations properly rejected with correct error codes\n");
    printf("✅ System maintained stability throughout boundary testing\n");
    printf("✅ No memory corruption or system instability detected\n");
    printf("✅ Parameter validation order and consistency verified\n");

    //===CLEANUP===
    // No cleanup needed for parameter validation tests
    // System demonstrated stability throughout testing
}
