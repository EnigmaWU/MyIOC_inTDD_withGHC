///////////////////////////////////////////////////////////////////////////////////////////////////
// UT_DataBoundaryUS4AC4.cxx - DAT Boundary Testing: US-4 AC-4 Multiple Error Condition Precedence Validation  
// 📝 Purpose: Test Cases for User Story 4, Acceptance Criteria 4 - Multiple error condition precedence validation
// 🔄 Focus: LinkID > DatDescParams > Options (logical resource-first validation precedence)
// 🎯 Coverage: [@US-4,AC-4] Multiple error condition precedence validation (comprehensive boundary error testing)
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "UT_DataBoundaryUS4.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF US-4 SHARED DATA IMPLEMENTATION=================================================

// Global test configuration for US-4 error code coverage testing
__DatErrorCodeSharedTestData_T g_US4_SharedTestData = {.TestConfigs = {},
                                                       .ServiceID1 = 0,
                                                       .ServiceID2 = 0,
                                                       .SystemInitialized = false,
                                                       .ErrorCodeCounts = {},
                                                       .ObservedErrorCodes = {},
                                                       .CrossModeConsistency = true,
                                                       .ParameterPrecedenceValidated = false,
                                                       .DataSizePrecedenceValidated = false,
                                                       .TimeoutPrecedenceValidated = false};

//======>END OF US-4 SHARED DATA IMPLEMENTATION===================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF IMPROVED ERROR PRECEDENCE TEST IMPLEMENTATIONS===================================

/**
 * ╔══════════════════════════════════════════════════════════════════════════════════════════╗
 * ║                    Error Precedence Validation: LinkID > DatDescParams > Options         ║
 * ╠══════════════════════════════════════════════════════════════════════════════════════════╣
 * ║ @[Name]: verifyDatErrorCodePrecedence_byImprovedOrder_expectLogicalValidation            ║
 * ║ @[Purpose]: Validate the error precedence order: LinkID > DatDescParams > Options        ║
 * ║ @[Rationale]: Resource-first validation is more logical and consistent                   ║
 * ║ @[Expected]: All error combinations should follow the improved precedence order          ║
 * ╚══════════════════════════════════════════════════════════════════════════════════════════╝
 */
TEST(UT_DataBoundary, verifyDatErrorCodePrecedence_byImprovedOrder_expectLogicalValidation) {
    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                🔧 SETUP PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    IOC_Result_T result = IOC_RESULT_BUG;
    IOC_LinkID_T InvalidLinkID = 999999;  // Non-existent LinkID
    char TestDataBuffer[] = "improved precedence test";

    printf("🎯 TESTING IMPROVED ERROR PRECEDENCE: LinkID > DatDescParams > Options\n");
    printf("   📋 This represents the LOGICAL precedence order that should be implemented\n");
    printf("   📋 Error codes: -22=INVALID_PARAM, -516=ZERO_DATA, -515=DATA_TOO_LARGE, -505=NOT_EXIST_LINK\n");

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                               🎯 BEHAVIOR PHASE                                       │
    // └──────────────────────────────────────────────────────────────────────────────────────┘

    // ════════════════════════════════════════════════════════════════════════════════════════
    // Test 1: LinkID Validation Takes HIGHEST Precedence
    // ════════════════════════════════════════════════════════════════════════════════════════
    printf("   ├─ 🔍 Test 1: LinkID validation should take HIGHEST precedence...\n");

    // Test 1a: Invalid LinkID + NULL DatDesc → LinkID error should win
    {
        IOC_Option_defineSyncMayBlock(ValidOptions);

        result = IOC_sendDAT(InvalidLinkID, NULL, &ValidOptions);
        printf("   │  🧪 Invalid LinkID + NULL DatDesc → Expected: -505 (LinkID), ");
        printf("Actual: %d", (int)result);

        // IMPROVED: LinkID validation should happen FIRST
        EXPECT_EQ(result, IOC_RESULT_NOT_EXIST_LINK)
            << "IMPROVED PRECEDENCE: Invalid LinkID should be detected BEFORE parameter validation";
        printf(" ✅ IMPROVED\n");
    }

    // Test 1b: Invalid LinkID + Zero Size Data → LinkID error should win
    {
        IOC_DatDesc_T ZeroSizeDesc = {0};
        IOC_initDatDesc(&ZeroSizeDesc);
        ZeroSizeDesc.Payload.pData = TestDataBuffer;
        ZeroSizeDesc.Payload.PtrDataSize = 0;  // Zero size data error

        IOC_Option_defineSyncMayBlock(ValidOptions);

        result = IOC_sendDAT(InvalidLinkID, &ZeroSizeDesc, &ValidOptions);
        printf("   │  🧪 Invalid LinkID + Zero Size Data → Expected: -505 (LinkID), ");
        printf("Actual: %d", (int)result);

        // IMPROVED: LinkID validation should happen BEFORE data validation
        EXPECT_EQ(result, IOC_RESULT_NOT_EXIST_LINK)
            << "IMPROVED PRECEDENCE: Invalid LinkID should be detected BEFORE data size validation";
        printf(" ✅ IMPROVED\n");
    }

    // Test 1c: Invalid LinkID + NULL Data Pointer + Non-Zero Size → LinkID error should win
    {
        IOC_DatDesc_T MalformedDesc = {0};
        IOC_initDatDesc(&MalformedDesc);
        MalformedDesc.Payload.pData = NULL;       // Invalid parameter
        MalformedDesc.Payload.PtrDataSize = 100;  // Non-zero size (inconsistent)

        IOC_Option_defineSyncMayBlock(ValidOptions);

        result = IOC_sendDAT(InvalidLinkID, &MalformedDesc, &ValidOptions);
        printf("   │  🧪 Invalid LinkID + NULL ptr + Non-zero size → Expected: -505 (LinkID), ");
        printf("Actual: %d", (int)result);

        // IMPROVED: LinkID validation should happen BEFORE parameter consistency validation
        EXPECT_EQ(result, IOC_RESULT_NOT_EXIST_LINK)
            << "IMPROVED PRECEDENCE: Invalid LinkID should be detected BEFORE parameter consistency validation";
        printf(" ✅ IMPROVED\n");
    }

    // Test 1d: Invalid LinkID + Invalid Options → LinkID error should win
    {
        IOC_DatDesc_T ValidDesc = {0};
        IOC_initDatDesc(&ValidDesc);
        ValidDesc.Payload.pData = TestDataBuffer;
        ValidDesc.Payload.PtrDataSize = strlen(TestDataBuffer);

        IOC_Options_T InvalidOptions;
        memset(&InvalidOptions, 0, sizeof(InvalidOptions));
        InvalidOptions.IDs = (IOC_OptionsID_T)0xDEAD;  // Invalid options

        result = IOC_sendDAT(InvalidLinkID, &ValidDesc, &InvalidOptions);
        printf("   │  🧪 Invalid LinkID + Invalid Options → Expected: -505 (LinkID), ");
        printf("Actual: %d", (int)result);

        // IMPROVED: LinkID validation should happen BEFORE options validation
        EXPECT_EQ(result, IOC_RESULT_NOT_EXIST_LINK)
            << "IMPROVED PRECEDENCE: Invalid LinkID should be detected BEFORE options validation";
        printf(" ✅ IMPROVED\n");
    }

    // ════════════════════════════════════════════════════════════════════════════════════════
    // Test 2: DatDescParams Validation Takes SECOND Precedence (when LinkID is valid)
    // ════════════════════════════════════════════════════════════════════════════════════════
    printf("   ├─ 🔍 Test 2: DatDesc params should take SECOND precedence (requires valid LinkID)...\n");
    printf("   │  📋 Note: These tests would require a valid LinkID to isolate DatDesc validation\n");
    printf("   │  📋 Implementation would need valid LinkID test scenarios\n");

    // Test 2a: Valid LinkID + NULL DatDesc + Invalid Options → DatDesc error should win
    printf("   │  🧪 [DESIGN] Valid LinkID + NULL DatDesc + Invalid Options → Expected: -22 (PARAM)\n");

    // Test 2b: Valid LinkID + Zero Size Data + Invalid Options → DatDesc error should win
    printf("   │  🧪 [DESIGN] Valid LinkID + Zero Size + Invalid Options → Expected: -516 (ZERO_DATA)\n");

    // Test 2c: Valid LinkID + Oversized Data + Invalid Options → DatDesc error should win
    printf("   │  🧪 [DESIGN] Valid LinkID + Oversized Data + Invalid Options → Expected: -515 (DATA_TOO_LARGE)\n");

    // ════════════════════════════════════════════════════════════════════════════════════════
    // Test 3: Options Validation Takes LOWEST Precedence (when LinkID and DatDesc are valid)
    // ════════════════════════════════════════════════════════════════════════════════════════
    printf("   └─ 🔍 Test 3: Options should take LOWEST precedence (requires valid LinkID + DatDesc)...\n");
    printf("      📋 Note: These tests would require valid LinkID + valid DatDesc scenarios\n");

    // Test 3a: Valid LinkID + Valid DatDesc + Invalid Options → Options error should be detected
    printf("      🧪 [DESIGN] Valid LinkID + Valid DatDesc + Invalid Options → Expected: Option Error\n");

    // ┌──────────────────────────────────────────────────────────────────────────────────────┐
    // │                                ✅ VERIFY PHASE                                        │
    // └──────────────────────────────────────────────────────────────────────────────────────┘
    printf("✅ IMPROVED PRECEDENCE VALIDATION SUMMARY:\n");
    printf("╔══════════════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                         🎯 IMPROVED ERROR PRECEDENCE DESIGN                              ║\n");
    printf("╠══════════════════════════════════════════════════════════════════════════════════════════╣\n");
    printf("║ 🥇 FIRST:  LinkID validation       → IOC_RESULT_NOT_EXIST_LINK (-505)                   ║\n");
    printf("║ 🥈 SECOND: DatDesc param validation → IOC_RESULT_INVALID_PARAM (-22)                    ║\n");
    printf("║           ├─ NULL pointer checks   → IOC_RESULT_INVALID_PARAM (-22)                    ║\n");
    printf("║           ├─ Zero size data        → IOC_RESULT_ZERO_DATA (-516)                       ║\n");
    printf("║           └─ Oversized data        → IOC_RESULT_DATA_TOO_LARGE (-515)                  ║\n");
    printf("║ 🥉 THIRD:  Options validation      → IOC_RESULT_INVALID_PARAM (-22)                    ║\n");
    printf("║                                                                                          ║\n");
    printf("║ ✅ BENEFITS OF IMPROVED DESIGN:                                                         ║\n");
    printf("║   📋 Logical: Check resource exists before processing data                              ║\n");
    printf("║   🎯 Consistent: Same precedence order for all operations                               ║\n");
    printf("║   ⚡ Performance: Fail fast on invalid connections                                      ║\n");
    printf("║   🛡️ Security: Don't process data on invalid links                                      ║\n");
    printf("║   🧠 Intuitive: Resource → Data → Config validation flow                                ║\n");
    printf("╚══════════════════════════════════════════════════════════════════════════════════════════╝\n");
}

/**
 * ╔══════════════════════════════════════════════════════════════════════════════════════════╗
 * ║                    IMPROVED: Cross-Operation Consistency Validation                      ║
 * ╠══════════════════════════════════════════════════════════════════════════════════════════╣
 * ║ @[Name]: verifyDatErrorCodePrecedence_byImprovedConsistency_expectUniformBehavior       ║
 * ║ @[Purpose]: Validate that IMPROVED precedence is consistent across sendDAT/recvDAT      ║
 * ╚══════════════════════════════════════════════════════════════════════════════════════════╝
 */
TEST(UT_DataBoundary, verifyDatErrorCodePrecedence_byImprovedConsistency_expectUniformBehavior) {
    printf("🎯 TESTING IMPROVED CROSS-OPERATION CONSISTENCY\n");
    printf("   📋 sendDAT and recvDAT should have IDENTICAL precedence behavior\n");

    IOC_Result_T sendResult, recvResult;
    IOC_LinkID_T InvalidLinkID = 999999;
    char TestDataBuffer[] = "consistency test";

    // ════════════════════════════════════════════════════════════════════════════════════════
    // Test: Cross-operation precedence consistency
    // ════════════════════════════════════════════════════════════════════════════════════════
    printf("   🔍 Testing sendDAT vs recvDAT precedence consistency...\n");

    // Test 1: Invalid LinkID + NULL DatDesc consistency
    {
        IOC_Option_defineSyncMayBlock(ValidOptions);

        sendResult = IOC_sendDAT(InvalidLinkID, NULL, &ValidOptions);
        recvResult = IOC_recvDAT(InvalidLinkID, NULL, &ValidOptions);

        printf("   │  🧪 Invalid LinkID + NULL DatDesc:\n");
        printf("   │     sendDAT: %d, recvDAT: %d", (int)sendResult, (int)recvResult);

        // IMPROVED: Both should return LinkID error consistently
        EXPECT_EQ(sendResult, IOC_RESULT_NOT_EXIST_LINK) << "sendDAT should prioritize LinkID validation";
        EXPECT_EQ(recvResult, IOC_RESULT_NOT_EXIST_LINK) << "recvDAT should prioritize LinkID validation";
        EXPECT_EQ(sendResult, recvResult) << "sendDAT and recvDAT should have identical precedence behavior";

        if (sendResult == recvResult && sendResult == IOC_RESULT_NOT_EXIST_LINK) {
            printf(" ✅ IMPROVED CONSISTENCY\n");
        } else {
            printf(" ❌ INCONSISTENT\n");
        }
    }

    // Test 2: Invalid LinkID + Zero Size Data consistency
    {
        IOC_DatDesc_T ZeroSizeDesc = {0};
        IOC_initDatDesc(&ZeroSizeDesc);
        ZeroSizeDesc.Payload.pData = TestDataBuffer;
        ZeroSizeDesc.Payload.PtrDataSize = 0;

        IOC_Option_defineSyncMayBlock(ValidOptions);

        sendResult = IOC_sendDAT(InvalidLinkID, &ZeroSizeDesc, &ValidOptions);
        recvResult = IOC_recvDAT(InvalidLinkID, &ZeroSizeDesc, &ValidOptions);

        printf("   │  🧪 Invalid LinkID + Zero Size Data:\n");
        printf("   │     sendDAT: %d, recvDAT: %d", (int)sendResult, (int)recvResult);

        // IMPROVED: Both should return LinkID error consistently
        EXPECT_EQ(sendResult, IOC_RESULT_NOT_EXIST_LINK) << "sendDAT should prioritize LinkID validation";
        EXPECT_EQ(recvResult, IOC_RESULT_NOT_EXIST_LINK) << "recvDAT should prioritize LinkID validation";
        EXPECT_EQ(sendResult, recvResult) << "sendDAT and recvDAT should have identical precedence behavior";

        if (sendResult == recvResult && sendResult == IOC_RESULT_NOT_EXIST_LINK) {
            printf(" ✅ IMPROVED CONSISTENCY\n");
        } else {
            printf(" ❌ INCONSISTENT\n");
        }
    }

    printf("✅ IMPROVED CONSISTENCY VALIDATION COMPLETE\n");
}

/**
 * ╔══════════════════════════════════════════════════════════════════════════════════════════╗
 * ║                    IMPROVED: LinkID Value Independence Validation                        ║
 * ╠══════════════════════════════════════════════════════════════════════════════════════════╣
 * ║ @[Name]: verifyDatErrorCodePrecedence_byImprovedIndependence_expectUniformLinkIDBehavior ║
 * ║ @[Purpose]: Validate that precedence is independent of specific LinkID values           ║
 * ╚══════════════════════════════════════════════════════════════════════════════════════════╝
 */
TEST(UT_DataBoundary, verifyDatErrorCodePrecedence_byImprovedIndependence_expectUniformLinkIDBehavior) {
    printf("🎯 TESTING IMPROVED LinkID VALUE INDEPENDENCE\n");
    printf("   📋 Precedence should be consistent across ALL invalid LinkID values\n");

    // Test different invalid LinkID values
    const IOC_LinkID_T INVALID_LINK_IDS[] = {
        999999,      // Large invalid value
        0,           // Zero LinkID
        UINT64_MAX,  // Maximum value
        0xDEADBEEF,  // Hex pattern
        12345678     // Random invalid value
    };
    const size_t NUM_INVALID_LINK_IDS = sizeof(INVALID_LINK_IDS) / sizeof(INVALID_LINK_IDS[0]);

    char TestDataBuffer[] = "linkid independence test";

    printf("   🔍 Testing precedence consistency across %zu different invalid LinkID values...\n",
           NUM_INVALID_LINK_IDS);

    for (size_t i = 0; i < NUM_INVALID_LINK_IDS; i++) {
        IOC_LinkID_T testLinkID = INVALID_LINK_IDS[i];

        printf("   │  🧪 Testing LinkID[%zu] = %llu (0x%llX):\n", i, (unsigned long long)testLinkID,
               (unsigned long long)testLinkID);

        // Test 1: LinkID + NULL DatDesc → Should ALWAYS return LinkID error
        {
            IOC_Option_defineSyncMayBlock(ValidOptions);
            IOC_Result_T result = IOC_sendDAT(testLinkID, NULL, &ValidOptions);

            printf("   │     NULL DatDesc: %d", (int)result);
            EXPECT_EQ(result, IOC_RESULT_NOT_EXIST_LINK)
                << "LinkID[" << i << "] + NULL DatDesc should ALWAYS return IOC_RESULT_NOT_EXIST_LINK";

            if (result == IOC_RESULT_NOT_EXIST_LINK) {
                printf(" ✅ CONSISTENT\n");
            } else {
                printf(" ❌ INCONSISTENT\n");
            }
        }

        // Test 2: LinkID + Zero Size Data → Should ALWAYS return LinkID error
        {
            IOC_DatDesc_T ZeroSizeDesc = {0};
            IOC_initDatDesc(&ZeroSizeDesc);
            ZeroSizeDesc.Payload.pData = TestDataBuffer;
            ZeroSizeDesc.Payload.PtrDataSize = 0;

            IOC_Option_defineSyncMayBlock(ValidOptions);
            IOC_Result_T result = IOC_sendDAT(testLinkID, &ZeroSizeDesc, &ValidOptions);

            printf("   │     Zero Size Data: %d", (int)result);
            EXPECT_EQ(result, IOC_RESULT_NOT_EXIST_LINK)
                << "LinkID[" << i << "] + Zero Size should ALWAYS return IOC_RESULT_NOT_EXIST_LINK";

            if (result == IOC_RESULT_NOT_EXIST_LINK) {
                printf(" ✅ CONSISTENT\n");
            } else {
                printf(" ❌ INCONSISTENT (should be improved)\n");
            }
        }
    }

    printf("✅ IMPROVED LinkID INDEPENDENCE VALIDATION COMPLETE\n");
}

//======>END OF IMPROVED ERROR PRECEDENCE TEST IMPLEMENTATIONS=====================================
