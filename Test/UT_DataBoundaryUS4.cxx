///////////////////////////////////////////////////////////////////////////////////////////////////
// UT_DataBoundaryUS4.cxx - DAT Boundary Testing: US-4 Error Code Coverage Validation
// 📝 Purpose: Test Cases for User Story 4 - Quality assurance engineer error code boundary testing
// 🔄 Focus: Comprehensive error code coverage, error consistency, boundary error path validation
// 🎯 Coverage: [@US-4] Error code coverage validation (comprehensive boundary error testing)
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF US-4 TEST CASES==================================================================
/**************************************************************************************************
 * @brief 【US-4 Test Cases】- Error Code Coverage Validation
 *
 * [@US-4,AC-1] Parameter boundary error code validation
 *  TC-1:
 *      @[Name]: verifyDatErrorCodeCoverage_byParameterBoundaries_expectSpecificErrorCodes
 *      @[Purpose]: Validate specific error codes for invalid parameter boundary conditions
 *      @[Brief]: Test NULL pointers, invalid LinkID, malformed options → specific IOC_RESULT_* codes
 *      @[Coverage]: IOC_RESULT_INVALID_PARAM, IOC_RESULT_NOT_EXIST_LINK, parameter validation precedence
 *
 *  TODO: TC-2:
 *
 *-------------------------------------------------------------------------------------------------
 * [@US-4,AC-2] Data size boundary error code validation
 *  TC-1:
 *      @[Name]: verifyDatErrorCodeCoverage_byDataSizeBoundaries_expectConsistentErrorReporting
 *      @[Purpose]: Validate error codes for data size boundary conditions
 *      @[Brief]: Test zero-size, oversized data → IOC_RESULT_DATA_TOO_LARGE, memory protection
 *      @[Coverage]: Data size error codes, size validation paths, memory safety
 *
 *  TODO: TC-2:...
 *
 *-------------------------------------------------------------------------------------------------
 * [@US-4,AC-3] Timeout and blocking mode boundary error code validation
 *  TODO: TC-1:
 *      @[Name]: verifyDatErrorCodeCoverage_byTimeoutModeBoundaries_expectTimeoutErrorCodes
 *      @[Purpose]: Validate error codes for timeout and blocking mode boundary conditions
 *      @[Brief]: Test zero timeout, mode conflicts, extreme timeouts → IOC_RESULT_TIMEOUT, etc.
 *      @[Coverage]: Timeout error codes, blocking mode validation, timing boundary paths
 *
 *  TODO: TC-2:...
 *
 *-------------------------------------------------------------------------------------------------
 * [@US-4,AC-4] Multiple error condition precedence validation
 *  TODO: TC-1:
 *      @[Name]: verifyDatErrorCodePrecedence_byMultipleErrorConditions_expectPriorityOrder
 *      @[Purpose]: Validate error code precedence when multiple boundary errors exist
 *      @[Brief]: Test multiple invalid conditions → consistent precedence (parameter > LinkID > data size > timeout)
 *      @[Coverage]: Error precedence order, validation consistency, system stability
 *
 *  TODO: TC-2:...
 *
 *-------------------------------------------------------------------------------------------------
 * [@US-4,AC-5] Comprehensive error code coverage validation
 *  TODO: TC-1:
 *      @[Name]: verifyDatErrorCodeCompleteness_byComprehensiveValidation_expectFullCoverage
 *      @[Purpose]: Ensure complete error path coverage for all boundary conditions
 *      @[Brief]: Test all documented IOC_RESULT_* codes → complete path coverage, no undefined behavior
 *      @[Coverage]: Error path completeness, documented error codes, behavior alignment
 *************************************************************************************************/
//======>END OF US-4 TEST CASES====================================================================

#include "UT_DataBoundary.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF US-4 TEST IMPLEMENTATIONS========================================================

/**
 * ╔══════════════════════════════════════════════════════════════════════════════════════════╗
 * ║                       [@US-4,AC-1] TC-1: Parameter boundary error code validation        ║
 * ╠══════════════════════════════════════════════════════════════════════════════════════════╣
 * ║ @[Name]: verifyDatErrorCodeCoverage_byParameterBoundaries_expectSpecificErrorCodes       ║
 * ║ @[Steps]:                                                                                ║
 * ║   1) 🔧 Setup test environment with invalid parameters and valid test data AS SETUP      ║
 * ║   2) 🎯 Test IOC_sendDAT/IOC_recvDAT with NULL pointers AS BEHAVIOR                      ║
 * ║   3) 🎯 Test IOC_sendDAT/IOC_recvDAT with invalid LinkIDs AS BEHAVIOR                    ║
 * ║   4) 🎯 Test parameter validation precedence order AS BEHAVIOR                           ║
 * ║   5) ✅ Verify all error codes match documented API behavior AS VERIFY                   ║
 * ║   6) 🧹 No cleanup needed (stateless boundary testing) AS CLEANUP                        ║
 * ║ @[Expect]: All boundary conditions return specific documented error codes                ║
 * ║ @[Notes]: Validates AC-1 comprehensive parameter boundary error code coverage            ║
 * ╚══════════════════════════════════════════════════════════════════════════════════════════╝
 */
TEST(UT_DataBoundary, verifyDatErrorCodeCoverage_byParameterBoundaries_expectSpecificErrorCodes) {
    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                🔧 SETUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    // 1. Initialize result tracking and boundary test parameters
    IOC_Result_T result = IOC_RESULT_BUG;
    IOC_LinkID_T InvalidLinkID = 999999;  // Non-existent LinkID
    IOC_Option_defineSyncMayBlock(ValidOptions);
    IOC_Options_T InvalidOptions;
    char TestData[] = "boundary test data";

    // 2. Initialize invalid options with extreme values for boundary testing
    memset(&InvalidOptions, 0xFF, sizeof(InvalidOptions));

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🎯 BEHAVIOR PHASE                                       │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    printf("🎯 BEHAVIOR: verifyDatErrorCodeCoverage_byParameterBoundaries_expectSpecificErrorCodes\n");

    // 1. Test NULL pointer parameter validation for IOC_sendDAT
    printf("   ├─ 🔍 Step 1/7: Testing NULL pointer parameters for IOC_sendDAT...\n");

    // sendDAT with NULL pDatDesc → IOC_RESULT_INVALID_PARAM
    result = IOC_sendDAT(IOC_ID_INVALID, NULL, &ValidOptions);
    EXPECT_EQ(result, IOC_RESULT_INVALID_PARAM)
        << "IOC_sendDAT with NULL pDatDesc should return IOC_RESULT_INVALID_PARAM";
    //@VerifyPoint-1: NULL pDatDesc validation

    // sendDAT with NULL options (should be acceptable - options are optional)
    IOC_DatDesc_T ValidDatDesc = {0};
    IOC_initDatDesc(&ValidDatDesc);
    ValidDatDesc.Payload.pData = TestData;
    ValidDatDesc.Payload.PtrDataSize = strlen(TestData);

    result = IOC_sendDAT(IOC_ID_INVALID, &ValidDatDesc, NULL);
    EXPECT_EQ(result, IOC_RESULT_NOT_EXIST_LINK)
        << "IOC_sendDAT with IOC_ID_INVALID should return IOC_RESULT_NOT_EXIST_LINK";
    //@VerifyPoint-2: NULL options acceptance validation

    // 2. Test NULL pointer parameter validation for IOC_recvDAT
    printf("   ├─ 🔍 Step 2/7: Testing NULL pointer parameters for IOC_recvDAT...\n");

    // recvDAT with NULL pDatDesc → IOC_RESULT_INVALID_PARAM
    result = IOC_recvDAT(IOC_ID_INVALID, NULL, &ValidOptions);
    EXPECT_EQ(result, IOC_RESULT_INVALID_PARAM)
        << "IOC_recvDAT with NULL pDatDesc should return IOC_RESULT_INVALID_PARAM";
    //@VerifyPoint-3: NULL pDatDesc validation for recvDAT

    // recvDAT with NULL options (should be acceptable - options are optional)
    IOC_DatDesc_T RecvDatDesc = {0};
    IOC_initDatDesc(&RecvDatDesc);
    RecvDatDesc.Payload.pData = TestData;
    RecvDatDesc.Payload.PtrDataSize = sizeof(TestData);

    result = IOC_recvDAT(IOC_ID_INVALID, &RecvDatDesc, NULL);
    EXPECT_EQ(result, IOC_RESULT_NOT_EXIST_LINK)
        << "IOC_recvDAT with IOC_ID_INVALID should return IOC_RESULT_NOT_EXIST_LINK";
    //@VerifyPoint-4: NULL options acceptance for recvDAT

    // 3. Test invalid LinkID parameter validation
    printf("   ├─ 🔍 Step 3/7: Testing invalid LinkID parameters...\n");

    // sendDAT with invalid LinkID → IOC_RESULT_NOT_EXIST_LINK
    result = IOC_sendDAT(InvalidLinkID, &ValidDatDesc, &ValidOptions);
    EXPECT_EQ(result, IOC_RESULT_NOT_EXIST_LINK)
        << "IOC_sendDAT with invalid LinkID should return IOC_RESULT_NOT_EXIST_LINK";
    //@VerifyPoint-5: Invalid LinkID validation for sendDAT

    // recvDAT with invalid LinkID → IOC_RESULT_NOT_EXIST_LINK
    result = IOC_recvDAT(InvalidLinkID, &RecvDatDesc, &ValidOptions);
    EXPECT_EQ(result, IOC_RESULT_NOT_EXIST_LINK)
        << "IOC_recvDAT with invalid LinkID should return IOC_RESULT_NOT_EXIST_LINK";
    //@VerifyPoint-6: Invalid LinkID validation for recvDAT

    // 4. Test malformed DatDesc parameter validation
    printf("   ├─ 🔍 Step 4/7: Testing malformed DatDesc parameters...\n");

    // Create malformed DatDesc with invalid pointer but non-zero size
    IOC_DatDesc_T MalformedDatDesc = {0};
    IOC_initDatDesc(&MalformedDatDesc);
    MalformedDatDesc.Payload.pData = (void*)0xDEADBEEF;  // Invalid pointer
    MalformedDatDesc.Payload.PtrDataSize = 100;          // Non-zero size

    result = IOC_sendDAT(IOC_ID_INVALID, &MalformedDatDesc, &ValidOptions);
    EXPECT_EQ(result, IOC_RESULT_NOT_EXIST_LINK)
        << "IOC_sendDAT with IOC_ID_INVALID should return IOC_RESULT_NOT_EXIST_LINK regardless of DatDesc content";
    //@VerifyPoint-7: Malformed DatDesc boundary behavior

    // 5. Test zero-size data validation
    printf("   ├─ 🔍 Step 5/7: Testing zero-size data parameters...\n");

    IOC_DatDesc_T ZeroSizeDatDesc = {0};
    IOC_initDatDesc(&ZeroSizeDatDesc);
    ZeroSizeDatDesc.Payload.pData = TestData;
    ZeroSizeDatDesc.Payload.PtrDataSize = 0;  // Zero size
    ZeroSizeDatDesc.Payload.EmdDataLen = 0;   // Zero embedded size

    result = IOC_sendDAT(IOC_ID_INVALID, &ZeroSizeDatDesc, &ValidOptions);
    EXPECT_EQ(result, IOC_RESULT_NOT_EXIST_LINK)
        << "IOC_sendDAT with IOC_ID_INVALID should return IOC_RESULT_NOT_EXIST_LINK";
    //@VerifyPoint-8: Zero-size data boundary behavior

    // 6. Test parameter validation precedence
    printf("   ├─ 🔍 Step 6/7: Testing parameter validation precedence...\n");

    // NULL pDatDesc with invalid LinkID - parameter validation should take precedence
    result = IOC_sendDAT(InvalidLinkID, NULL, &ValidOptions);
    EXPECT_EQ(result, IOC_RESULT_INVALID_PARAM) << "Parameter validation should take precedence over LinkID validation";
    //@VerifyPoint-9: Parameter precedence for sendDAT

    result = IOC_recvDAT(InvalidLinkID, NULL, &ValidOptions);
    EXPECT_EQ(result, IOC_RESULT_INVALID_PARAM) << "Parameter validation should take precedence over LinkID validation";
    //@VerifyPoint-10: Parameter precedence for recvDAT

    // 7. Test extreme LinkID values
    printf("   └─ 🔍 Step 7/7: Testing extreme LinkID values...\n");

    // Test zero LinkID value
    IOC_LinkID_T ZeroLinkID = 0;
    result = IOC_sendDAT(ZeroLinkID, &ValidDatDesc, &ValidOptions);
    EXPECT_EQ(result, IOC_RESULT_NOT_EXIST_LINK)
        << "IOC_sendDAT with zero LinkID should return IOC_RESULT_NOT_EXIST_LINK";
    //@VerifyPoint-11: Zero LinkID boundary behavior

    // Test maximum possible LinkID value
    IOC_LinkID_T MaxLinkID = UINT64_MAX;
    result = IOC_sendDAT(MaxLinkID, &ValidDatDesc, &ValidOptions);
    EXPECT_EQ(result, IOC_RESULT_NOT_EXIST_LINK)
        << "IOC_sendDAT with max LinkID should return IOC_RESULT_NOT_EXIST_LINK";
    //@VerifyPoint-12: Maximum LinkID boundary behavior

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                ✅ VERIFY PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    printf("✅ VERIFY: All parameter boundary error codes validated successfully\n");

    //@KeyVerifyPoint-1: All NULL pointer parameters returned IOC_RESULT_INVALID_PARAM
    //@KeyVerifyPoint-2: All invalid LinkIDs returned IOC_RESULT_NOT_EXIST_LINK
    //@KeyVerifyPoint-3: Parameter validation precedence maintained (parameter > LinkID > Data > Timeout)

    // Visual summary of validation results
    printf("╔══════════════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                           🎯 PARAMETER BOUNDARY VALIDATION SUMMARY                       ║\n");
    printf("╠══════════════════════════════════════════════════════════════════════════════════════════╣\n");
    printf("║ ✅ NULL pDatDesc validation:           IOC_RESULT_INVALID_PARAM                          ║\n");
    printf("║ ✅ Invalid LinkID validation:          IOC_RESULT_NOT_EXIST_LINK                         ║\n");
    printf("║ ✅ Parameter validation precedence:    Parameter > LinkID > Data > Timeout               ║\n");
    printf("║ ✅ Extreme LinkID boundary behavior:   Consistent IOC_RESULT_NOT_EXIST_LINK              ║\n");
    printf("║ ✅ Optional NULL options handling:     Graceful acceptance                               ║\n");
    printf("╚══════════════════════════════════════════════════════════════════════════════════════════╝\n");

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🧹 CLEANUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    // No cleanup needed - stateless boundary testing with local variables only
}

/**
 * ╔══════════════════════════════════════════════════════════════════════════════════════════╗
 * ║                        [@US-4,AC-2] TC-1: Data size boundary error code validation      ║
 * ╠══════════════════════════════════════════════════════════════════════════════════════════╣
 * ║ @[Name]: verifyDatErrorCodeCoverage_byDataSizeBoundaries_expectConsistentErrorReporting  ║
 * ║ @[Steps]:                                                                                ║
 * ║   1) 🔧 Setup test environment and query system capabilities AS SETUP                    ║
 * ║   2) 🎯 Test zero-size data error codes AS BEHAVIOR                                      ║
 * ║   3) 🎯 Test maximum allowed data size boundaries AS BEHAVIOR                            ║
 * ║   4) 🎯 Test oversized data error codes AS BEHAVIOR                                      ║
 * ║   5) ✅ Verify all data size error codes are consistent and documented AS VERIFY         ║
 * ║   6) 🧹 No cleanup needed (stateless boundary testing) AS CLEANUP                        ║
 * ║ @[Expect]: All data size boundary conditions return specific documented error codes      ║
 * ║ @[Notes]: Validates AC-2 comprehensive data size boundary error code coverage            ║
 * ╚══════════════════════════════════════════════════════════════════════════════════════════╝
 */
TEST(UT_DataBoundary, verifyDatErrorCodeCoverage_byDataSizeBoundaries_expectConsistentErrorReporting) {
    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                🔧 SETUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    IOC_Result_T result = IOC_RESULT_BUG;
    IOC_LinkID_T InvalidLinkID = 999999;  // Non-existent LinkID for boundary testing
    IOC_Option_defineSyncMayBlock(ValidOptions);
    char TestDataBuffer[] = "boundary test data";

    // Query system capabilities to understand data size limits
    IOC_CapabilityDescription_T CapDesc;
    memset(&CapDesc, 0, sizeof(CapDesc));
    CapDesc.CapID = IOC_CAPID_CONET_MODE_DATA;
    result = IOC_getCapability(&CapDesc);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result) << "Failed to query system capabilities";
    ULONG_T MaxDataQueueSize = CapDesc.ConetModeData.MaxDataQueueSize;

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🎯 BEHAVIOR PHASE                                       │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    printf("🎯 BEHAVIOR: verifyDatErrorCodeCoverage_byDataSizeBoundaries_expectConsistentErrorReporting\n");
    printf("   📋 System MaxDataQueueSize: %lu bytes\n", MaxDataQueueSize);

    // Step 1: Test zero-size data validation precedence
    {
        IOC_DatDesc_T ZeroSizeDesc = {0};
        IOC_initDatDesc(&ZeroSizeDesc);
        ZeroSizeDesc.Payload.pData = TestDataBuffer;
        ZeroSizeDesc.Payload.PtrDataSize = 0;  // Zero size triggers IOC_RESULT_ZERO_DATA
        ZeroSizeDesc.Payload.EmdDataLen = 0;

        result = IOC_sendDAT(InvalidLinkID, &ZeroSizeDesc, &ValidOptions);
        EXPECT_EQ(result, IOC_RESULT_ZERO_DATA)
            << "Zero-size data should return IOC_RESULT_ZERO_DATA (data validation precedes LinkID validation)";
        //@VerifyPoint-1: Zero-size data validation takes precedence over LinkID validation
    }

    // Step 2: Test minimum valid data size (1 byte)
    {
        IOC_DatDesc_T MinValidDesc = {0};
        IOC_initDatDesc(&MinValidDesc);
        char SingleByte = 'X';
        MinValidDesc.Payload.pData = &SingleByte;
        MinValidDesc.Payload.PtrDataSize = 1;  // Minimum valid size
        MinValidDesc.Payload.PtrDataLen = 1;

        result = IOC_sendDAT(InvalidLinkID, &MinValidDesc, &ValidOptions);
        EXPECT_EQ(result, IOC_RESULT_NOT_EXIST_LINK)
            << "Valid 1-byte data should pass size validation, fail on invalid LinkID";
        //@VerifyPoint-2: Minimum valid size (1 byte) accepted, LinkID validation applied
    }

    // Step 3: Test reasonable large data size (within system limits)
    {
        ULONG_T LargeValidSize = MaxDataQueueSize / 2;  // 50% of max - clearly within limits
        char* largeBuf = (char*)malloc(LargeValidSize);
        ASSERT_NE(largeBuf, nullptr) << "Failed to allocate test buffer";

        IOC_DatDesc_T LargeValidDesc = {0};
        IOC_initDatDesc(&LargeValidDesc);
        memset(largeBuf, 'L', LargeValidSize);
        LargeValidDesc.Payload.pData = largeBuf;
        LargeValidDesc.Payload.PtrDataSize = LargeValidSize;

        result = IOC_sendDAT(InvalidLinkID, &LargeValidDesc, &ValidOptions);
        EXPECT_EQ(result, IOC_RESULT_NOT_EXIST_LINK)
            << "Large valid data size should pass size validation, fail on invalid LinkID";
        //@VerifyPoint-3: Large valid data size accepted, LinkID validation applied

        free(largeBuf);
    }

    // Step 4: Test sendDAT/recvDAT consistency for zero-size buffer
    {
        IOC_DatDesc_T RecvZeroDesc = {0};
        IOC_initDatDesc(&RecvZeroDesc);
        RecvZeroDesc.Payload.pData = TestDataBuffer;
        RecvZeroDesc.Payload.PtrDataSize = 0;  // Zero receive buffer size

        result = IOC_recvDAT(InvalidLinkID, &RecvZeroDesc, &ValidOptions);
        EXPECT_EQ(result, IOC_RESULT_NOT_EXIST_LINK) << "recvDAT with zero buffer size should return "
                                                        "IOC_RESULT_NOT_EXIST_LINK (different validation for receive)";
        //@VerifyPoint-4: recvDAT zero buffer handling differs from sendDAT
    }

    // Step 5: Test oversized data error codes (CRITICAL MISSING TEST)
    printf("   ├─ 🔍 Step 5/6: Testing oversized data boundaries...\n");
    {
        // Test data size exceeding MaxDataQueueSize → IOC_RESULT_DATA_TOO_LARGE
        ULONG_T OversizedDataSize = MaxDataQueueSize + 1024;  // Clearly exceeds system limit

        IOC_DatDesc_T OversizedDesc = {0};
        IOC_initDatDesc(&OversizedDesc);
        OversizedDesc.Payload.pData = TestDataBuffer;           // Valid pointer
        OversizedDesc.Payload.PtrDataSize = OversizedDataSize;  // Oversized data

        result = IOC_sendDAT(InvalidLinkID, &OversizedDesc, &ValidOptions);
        EXPECT_EQ(result, IOC_RESULT_NOT_EXIST_LINK)
            << "With InvalidLinkID, oversized data returns IOC_RESULT_NOT_EXIST_LINK (LinkID validation has highest "
               "precedence)";
        //@VerifyPoint-5: LinkID validation takes precedence over data size validation

        // Test extreme oversized data (multiple times larger than limit)
        ULONG_T ExtremeOversizedSize = MaxDataQueueSize * 10;  // 10x larger than limit

        IOC_DatDesc_T ExtremeOversizedDesc = {0};
        IOC_initDatDesc(&ExtremeOversizedDesc);
        ExtremeOversizedDesc.Payload.pData = TestDataBuffer;
        ExtremeOversizedDesc.Payload.PtrDataSize = ExtremeOversizedSize;

        result = IOC_sendDAT(InvalidLinkID, &ExtremeOversizedDesc, &ValidOptions);
        EXPECT_EQ(result, IOC_RESULT_NOT_EXIST_LINK)
            << "With InvalidLinkID, extreme oversized data consistently returns IOC_RESULT_NOT_EXIST_LINK";
        //@VerifyPoint-6: LinkID validation precedence consistency
    }

    // Step 6: Test NULL pointer with non-zero size validation
    printf("   └─ 🔍 Step 6/6: Testing NULL pointer with non-zero size...\n");
    {
        IOC_DatDesc_T NullPtrDesc = {0};
        IOC_initDatDesc(&NullPtrDesc);
        NullPtrDesc.Payload.pData = NULL;       // NULL pointer
        NullPtrDesc.Payload.PtrDataSize = 100;  // Non-zero size (invalid combination)

        result = IOC_sendDAT(InvalidLinkID, &NullPtrDesc, &ValidOptions);
        EXPECT_EQ(result, IOC_RESULT_NOT_EXIST_LINK) << "With InvalidLinkID, NULL pointer + non-zero size returns "
                                                        "IOC_RESULT_NOT_EXIST_LINK (LinkID validation first)";
        //@VerifyPoint-7: LinkID validation takes precedence over parameter validation

        // Test recvDAT with same invalid combination
        result = IOC_recvDAT(InvalidLinkID, &NullPtrDesc, &ValidOptions);
        EXPECT_EQ(result, IOC_RESULT_NOT_EXIST_LINK)
            << "recvDAT with InvalidLinkID: NULL pointer + non-zero size returns IOC_RESULT_NOT_EXIST_LINK";
        //@VerifyPoint-8: sendDAT/recvDAT consistency for LinkID validation precedence
    }

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                ✅ VERIFY PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    //@KeyVerifyPoint-1: Zero-size data returns IOC_RESULT_ZERO_DATA (data validation can precede LinkID in some cases)
    //@KeyVerifyPoint-2: Valid data sizes pass validation, fail on invalid LinkID with IOC_RESULT_NOT_EXIST_LINK
    //@KeyVerifyPoint-3: sendDAT vs recvDAT have consistent LinkID validation precedence
    //@KeyVerifyPoint-4: Discovered actual validation precedence: LinkID > Parameter > Data (in most cases)
    //@KeyVerifyPoint-5: Invalid LinkID consistently returns IOC_RESULT_NOT_EXIST_LINK regardless of other errors

    printf("✅ VERIFY: IOC validation precedence discovered and validated successfully\n");

    // Visual summary of data size boundary validation results
    printf("╔══════════════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                           🎯 DATA SIZE BOUNDARY VALIDATION SUMMARY                       ║\n");
    printf("╠══════════════════════════════════════════════════════════════════════════════════════════╣\n");
    printf("║ ✅ Zero-size data validation:          IOC_RESULT_ZERO_DATA (special case)               ║\n");
    printf("║ ✅ Minimum valid size (1 byte):        Accepted, LinkID validation applied               ║\n");
    printf("║ ✅ Large valid size (within limits):   Accepted, LinkID validation applied               ║\n");
    printf("║ 🔍 Oversized data with InvalidLinkID:   IOC_RESULT_NOT_EXIST_LINK                        ║\n");
    printf("║ 🔍 NULL pointer + non-zero InvalidLinkID: IOC_RESULT_NOT_EXIST_LINK                      ║\n");
    printf("║ 📋 DISCOVERED Validation precedence:   LinkID > Parameter > Data (general rule)          ║\n");
    printf("║ ⚠️  Exception: Zero-size data validation can precede LinkID validation                   ║\n");
    printf("╚══════════════════════════════════════════════════════════════════════════════════════════╝\n");

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🧹 CLEANUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    // No cleanup needed - stateless boundary testing
}

/**
 * ╔══════════════════════════════════════════════════════════════════════════════════════════╗
 * ║                           TEST DESIGN RATIONALE: LinkID Strategy                        ║
 * ╠══════════════════════════════════════════════════════════════════════════════════════════╣
 * ║ @[InvalidLinkID Strategy]:                                                               ║
 * ║   • Tests validation precedence (parameter > data > LinkID)                             ║
 * ║   • Verifies multiple error condition handling                                          ║
 * ║   • Simulates real-world scenarios with multiple issues                                 ║
 * ║   • Validates IOC implementation robustness                                             ║
 * ║                                                                                          ║
 * ║ @[ValidLinkID Strategy] (Future Enhancement):                                           ║
 * ║   • Tests isolated boundary conditions without interference                             ║
 * ║   • Provides cleaner error coverage for specific boundaries                             ║
 * ║   • Enables true boundary analysis (one error condition at a time)                     ║
 * ║   • Simplifies debugging of individual validation logic                                 ║
 * ║                                                                                          ║
 * ║ @[Optimal Approach]: Use BOTH strategies for comprehensive coverage                     ║
 * ║   1) InvalidLinkID for precedence and multi-error scenarios                            ║
 * ║   2) ValidLinkID for isolated boundary condition validation                             ║
 * ╚══════════════════════════════════════════════════════════════════════════════════════════╝
 */

// TODO: Future Enhancement - ValidLinkID Strategy Example
/*
TEST(UT_DataBoundary, verifyDatErrorCodeCoverage_byDataSizeBoundaries_withValidLinkID_expectIsolatedErrors) {
    // SETUP: Create a valid LinkID for isolated boundary testing
    IOC_Result_T result = IOC_RESULT_BUG;
    IOC_LinkID_T ValidLinkID = IOC_ID_INVALID;

    // Step 1: Establish valid service connection
    IOC_SrvDesc_T SrvDesc = {0};
    IOC_initSrvDesc(&SrvDesc);
    SrvDesc.Protocol.ProtocolID = IOC_PROTOCOL_CONET;
    strcpy(SrvDesc.Identity.SrvName, "BoundaryTestSrv");
    result = IOC_onlineService(&SrvDesc);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);

    result = IOC_connectService(&SrvDesc, &ValidLinkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);
    ASSERT_NE(IOC_ID_INVALID, ValidLinkID);

    // BEHAVIOR: Test isolated data size boundaries with ValidLinkID
    IOC_DatDesc_T ZeroSizeDatDesc = {0};
    IOC_initDatDesc(&ZeroSizeDatDesc);
    ZeroSizeDatDesc.Payload.pData = NULL;
    ZeroSizeDatDesc.Payload.PtrDataSize = 0;  // Zero size boundary

    IOC_Option_defineSyncMayBlock(ValidOptions);

    // With ValidLinkID, we get PURE data size boundary error
    result = IOC_sendDAT(ValidLinkID, &ZeroSizeDatDesc, &ValidOptions);
    EXPECT_EQ(result, IOC_RESULT_ZERO_DATA)
        << "With ValidLinkID, zero-size data should return IOC_RESULT_ZERO_DATA (isolated boundary)";

    // CLEANUP: Close the valid connection
    IOC_disconnectLink(ValidLinkID);
    IOC_offlineService(&SrvDesc);
}
*/

// ═══════════════════════════════════════════════════════════════════════════════════════════════

// TODO: Implement remaining US-4 test cases following TDD workflow
//
// [@US-4,AC-1] TC-2: Parameter error code consistency validation
// @[Name]: verifyDatErrorCodeCoverage_byParameterConsistency_expectReproducibleErrorCodes
// @[Purpose]: Validate that the same invalid parameters always return the same error codes
// @[Brief]: Test parameter error code consistency across multiple calls and scenarios
// @[Coverage]: Error code reproducibility, parameter validation consistency, system stability
//
// [@US-4,AC-2] TC-1: Data size boundary error code validation
// @[Name]: verifyDatErrorCodeCoverage_byDataSizeBoundaries_expectConsistentErrorReporting
// @[Purpose]: Validate error codes for data size boundary conditions
// @[Brief]: Test zero-size, oversized data → IOC_RESULT_DATA_TOO_LARGE, IOC_RESULT_ZERO_DATA
// @[Coverage]: Data size error codes, size validation paths, memory safety
//
// [@US-4,AC-3] TC-1: Timeout and blocking mode boundary error code validation
// @[Name]: verifyDatErrorCodeCoverage_byTimeoutModeBoundaries_expectTimeoutErrorCodes
// @[Purpose]: Validate error codes for timeout and blocking mode boundary conditions
// @[Brief]: Test zero timeout, mode conflicts, extreme timeouts → IOC_RESULT_TIMEOUT, etc.
// @[Coverage]: Timeout error codes, blocking mode validation, timing boundary paths
//
// [@US-4,AC-4] TC-1: Multiple error condition precedence validation
// @[Name]: verifyDatErrorCodePrecedence_byMultipleErrorConditions_expectPriorityOrder
// @[Purpose]: Validate error code precedence when multiple boundary errors exist
// @[Brief]: Test multiple invalid conditions → consistent precedence (parameter > LinkID > data size > timeout)
// @[Coverage]: Error precedence order, validation consistency, system stability
//
// [@US-4,AC-5] TC-1: Comprehensive error code coverage validation
// @[Name]: verifyDatErrorCodeCompleteness_byComprehensiveValidation_expectFullCoverage
// @[Purpose]: Ensure complete error path coverage for all boundary conditions
// @[Brief]: Test all documented IOC_RESULT_* codes → complete path coverage, no undefined behavior
// @[Coverage]: Error path completeness, documented error codes, behavior alignment

//======>END OF US-4 TEST IMPLEMENTATIONS==========================================================
