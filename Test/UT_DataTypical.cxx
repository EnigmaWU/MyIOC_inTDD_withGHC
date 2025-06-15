///////////////////////////////////////////////////////////////////////////////////////////////////
// QUICK REFERENCE GUIDE - 快速参考指南
// 📝 用途: DAT（数据传输）典型使用场景单元测试骨架
// 🔄 流程: User Story → Acceptance Criteria → Test Cases → Implementation
// 📂 分类: DataTypical - 专注于DAT数据传输的典型使用场景
// 🎯 重点: 典型的DatSender/DatReceiver数据传输模式和常见使用方法
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief
 *  验证IOC框架中DAT（数据传输）的典型使用场景，专注于最常见、最标准的
 *  DatSender和DatReceiver数据传输模式。
 *
 *-------------------------------------------------------------------------------------------------
 *++DAT是IOC框架中用于数据传输的机制，本测试文件只关注典型场景：
 *
 *  典型使用场景：
 *  - DatSender发送数据到DatReceiver的标准流程
 *  - 常见数据大小和类型的传输
 *  - 标准的连接建立和数据传输模式
 *  - 典型的回调接收处理
 *
 *  不包括：
 *  - 边界条件测试
 *  - 错误处理场景
 *  - 性能优化场景
 *  - 并发和复杂场景
 *
 *  参考文档：
 *  - README_ArchDesign.md::MSG::DAT（典型使用部分）
 *  - README_UserGuide.md::ConetData示例（标准用法）
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING DESIGN==============================================================

/**************************************************************************************************
 * 📋 DAT TYPICAL TEST FOCUS - DAT典型测试重点
 *
 * 🎯 DESIGN PRINCIPLE: 只验证DAT最常见、最标准的使用模式
 * 🔄 PRIORITY: 标准流程 → 常见数据类型 → 典型传输模式
 *
 * ✅ TYPICAL SCENARIOS INCLUDED (包含的典型场景):
 *    � Basic Data Send: DatSender发送常见大小的数据
 *    � Basic Data Receive: DatReceiver通过回调接收数据
 *    🔗 Standard Connection: 标准的连接建立和使用
 *    � Common Data Types: 常见数据类型（文本、二进制、结构体）
 *    � Simple Stream: 简单的数据流传输
 *
 * ❌ NON-TYPICAL SCENARIOS EXCLUDED (排除的非典型场景):
 *    � 边界条件（最大/最小数据、极限情况）
 *    🚫 错误处理（网络中断、数据损坏、超时）
 *    � 性能优化（零拷贝、内存效率、并发）
 *    � 复杂场景（多连接、状态机、恢复机制）
 *    🚫 压力测试（大量数据、高频传输）
 *************************************************************************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================
/**************************************************************************************************
 * @brief 【User Story】
 *
 *  US-1: AS a DatSender developer,
 *    I WANT to **connect** to a DatReceiver Service via IOC_connectService,
 *   SO THAT I can reliably stream data chunks using IOC_sendDAT with NODROP guarantee,
 *       AND the receiver can process data either through automatic callback (CbRecvDat_F)
 *        OR through manual polling (IOC_recvDAT) according to their design preference.
 *
 *  US-2: AS a DatSender developer,
 *    I WANT to **online** a service with IOC_onlineService,
 *   SO THAT I can accept DatReceiver connect to this service,
 *      THEN I can send data to the receiver using IOC_sendDAT,
 *       AND the receiver can process data either through automatic callback (CbRecvDat_F)
 *        OR through manual polling (IOC_recvDAT) according to their design preference.
 *
 *
 *************************************************************************************************/
//======>END OF USER STORY=========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//=======>BEGIN OF ACCEPTANCE CRITERIA=============================================================
/**************************************************************************************************
 * @brief 【Acceptance Criteria】
 *
 * 🎯 专注于 DAT TYPICAL 测试 - 只验证最常见、最标准的数据传输使用模式
 *
 * [@US-1] AS a DatSender developer, I WANT to connect to a DatReceiver Service via IOC_connectService,
 *         SO THAT I can reliably stream data chunks using IOC_sendDAT with NODROP guarantee.
 *
 * ⭐ TYPICAL SCENARIOS ONLY - 典型场景验收标准:
 *
 *  AC-1@US-1: GIVEN DatReceiver has onlined a standard service using IOC_onlineService,
 *         WHEN DatSender calls IOC_connectService with typical SrvURI and Usage=IOC_LinkUsageDatSender,
 *         THEN DatSender WILL get IOC_RESULT_SUCCESS and valid LinkID,
 *          AND standard connection is established for typical data streaming.
 *
 *  AC-2@US-1: GIVEN DatSender has connected to DatReceiver service,
 *         WHEN DatSender calls IOC_sendDAT with common data chunk (text/binary, 1KB-100KB) and NODROP,
 *         THEN DatSender WILL get IOC_RESULT_SUCCESS,
 *          AND DatReceiver receives complete data via CbRecvDat_F callback in typical workflow.
 *
 *  AC-3@US-1: GIVEN DatSender has connected to DatReceiver service,
 *         WHEN DatSender calls IOC_sendDAT with typical data chunk and NODROP,
 *         THEN DatReceiver can receive the data via IOC_recvDAT polling,
 *          AND data integrity is maintained in standard usage pattern,
 *          AND DatReceiver gets IOC_RESULT_SUCCESS when data is available,
 *          AND DatReceiver gets IOC_RESULT_NO_DATA when no data is available (for NONBLOCK polling).
 *
 *  AC-4@US-1: GIVEN DatSender streaming typical data types (string, struct, binary array),
 *         WHEN using standard IOC_sendDAT workflow with NODROP guarantee,
 *         THEN all common data types are transmitted successfully,
 *          AND DatReceiver processes them correctly in typical application scenarios.
 *
 *  AC-5@US-1: GIVEN DatSender needs to send simple data stream,
 *         WHEN executing typical connect→send→receive→disconnect sequence,
 *         THEN entire standard workflow completes successfully,
 *          AND demonstrates typical DAT usage pattern for developers.
 *
 */
//=======>END OF ACCEPTANCE CRITERIA================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES=======================================================================
/**************************************************************************************************
 * @brief 【Test Cases】
 *
 * 🎯 专注于 DAT TYPICAL 测试用例 - 基于 FreelyDrafts 模板设计
 *
 * [@AC-1,US-1] - Standard Connection Establishment
 *  TC-1:
 *      @[Name]: verifyDatSenderConnection_byConnectToOnlineService_expectSuccessAndValidLinkID
 *      @[Purpose]: Verify AC-1 complete functionality - DatSender connects to DatReceiver service
 *          using typical parameters
 *      @[Brief]: DatReceiver online service with standard SrvURI {"fifo", "localprocess", "DatReceiver"},
 *          DatSender connect with Usage=IOC_LinkUsageDatSender, verify IOC_RESULT_SUCCESS and valid LinkID
 *
 *  TODO:TC-2...
 *
 *-------------------------------------------------------------------------------------------------
 * [@AC-2,US-1] - Standard Data Transmission with Callback
 *  TC-1:
 *      @[Name]: verifyDatSenderTransmission_bySendCommonData_expectCallbackReceiveSuccess
 *      @[Purpose]: Verify AC-2 complete functionality - DatSender transmits typical data to DatReceiver
 *          using IOC_sendDAT, received via CbRecvDat_F callback
 *      @[Brief]: Establish connection, DatSender send common data chunk (text, 10KB),
 *          verify IOC_RESULT_SUCCESS and DatReceiver gets complete data via callback in typical workflow
 *
 *  TODO:TC-2...
 *-------------------------------------------------------------------------------------------------
 * [@AC-3,US-1] - Standard Data Transmission with Polling
 *  TC-1:
 *      @[Name]: verifyDatPollingReceive_byManualRetrieve_expectCompleteDataIntegrity
 *      @[Purpose]: Verify AC-3 complete functionality - DatReceiver receives data via IOC_recvDAT polling
 *          instead of callback mechanism, maintaining data integrity
 *      @[Brief]: Establish connection without callback, DatSender send common data chunk (binary, 5KB),
 *          DatReceiver poll with IOC_recvDAT in MAYBLOCK mode, verify data integrity and polling workflow
 *      @[Notes]: This complements callback-based reception, demonstrating pull-based data consumption
 *
 *  TODO:TC-2...
 *-------------------------------------------------------------------------------------------------
 *
 *************************************************************************************************/
//======>END OF TEST CASES=========================================================================
//======>END OF UNIT TESTING DESIGN================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF UNIT TESTING IMPLEMENTATION=======================================================
#include "_UT_IOC_Common.h"

//===TEMPLATE FOR DAT TYPICAL TESTS===

/**
 * @[Name]: verifyDatSenderConnection_byConnectToOnlineService_expectSuccessAndValidLinkID
 * @[Steps]:
 *   1) Setup DatReceiver service online with standard SrvURI {"fifo", "localprocess", "DatReceiver"} AS SETUP.
 *      |-> SrvArgs.UsageCapabilites = IOC_LinkUsageDatReceiver
 *      |-> SrvArgs.SrvURI.pProtocol = IOC_SRV_PROTO_FIFO
 *      |-> SrvArgs.SrvURI.pHost = IOC_SRV_HOST_LOCAL_PROCESS
 *      |-> SrvArgs.SrvURI.pPath = "DatReceiver"
 *   2) DatSender connect to the service with typical parameters AS BEHAVIOR.
 *      |-> ConnArgs.Usage = IOC_LinkUsageDatSender
 *      |-> ConnArgs.SrvURI = same as service SrvURI
 *   3) Verify connection success and valid LinkID AS VERIFY.
 *      |-> IOC_connectService() returns IOC_RESULT_SUCCESS
 *      |-> LinkID is valid (not IOC_ID_INVALID)
 *   4) Cleanup: close connection and offline service AS CLEANUP.
 * @[Expect]: Connection established successfully with valid LinkID for typical DAT scenario.
 * @[Notes]: Focus on standard data transmission connection only, no boundary/fault/performance cases.
 */
TEST(UT_DataTypical, verifyDatSenderConnection_byConnectToOnlineService_expectSuccessAndValidLinkID) {
    //===SETUP===
    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_SrvID_T DatReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatReceiverLinkID = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID = IOC_ID_INVALID;

    // Standard SrvURI for typical DAT communication
    IOC_SrvURI_T CSURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = (const char *)"DatReceiver",
    };

    // Step-1: Setup DatReceiver service online
    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = CSURI,
        .UsageCapabilites = IOC_LinkUsageDatReceiver,
    };

    Result = IOC_onlineService(&DatReceiverSrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint: Service online success

    //===BEHAVIOR===
    printf("BEHAVIOR: verifyDatSenderConnection_byConnectToOnlineService_expectSuccessAndValidLinkID\n");

    // Step-2: DatSender connect to the service with typical parameters
    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = CSURI,
        .Usage = IOC_LinkUsageDatSender,
    };

    std::thread DatSenderThread([&] {
        Result = IOC_connectService(&DatSenderLinkID, &ConnArgs, NULL);
        // VerifyPoint: Connection success in thread context
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
        // VerifyPoint: Valid LinkID returned
        ASSERT_NE(IOC_ID_INVALID, DatSenderLinkID);
    });

    // Step-3: DatReceiver accept the connection
    Result = IOC_acceptClient(DatReceiverSrvID, &DatReceiverLinkID, NULL);

    DatSenderThread.join();

    //===VERIFY===
    // KeyVerifyPoint-1: Service accept connection success
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);
    // KeyVerifyPoint-2: Valid receiver LinkID returned
    ASSERT_NE(IOC_ID_INVALID, DatReceiverLinkID);
    // KeyVerifyPoint-3: Valid sender LinkID confirmed (checked in thread)

    //===CLEANUP===
    // Close both links
    if (DatSenderLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatSenderLinkID);
    }
    if (DatReceiverLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatReceiverLinkID);
    }
    // Offline service
    if (DatReceiverSrvID != IOC_ID_INVALID) {
        IOC_offlineService(DatReceiverSrvID);
    }
}

//===TEST FIXTURE FOR TYPICAL DAT SCENARIOS===

/**
 * @brief DataTypical测试夹具类，用于管理典型DAT传输测试场景
 */
class UT_DataTypicalFixture : public ::testing::Test {
   protected:
    static void SetUpTestSuite() {
        printf("UT_DataTypicalFixture->SETUP: SetUpTestSuite - Initializing typical DAT test environment\n");
        // TODO: Global typical test environment setup
    }

    static void TearDownTestSuite() {
        printf("UT_DataTypicalFixture->CLEANUP: TearDownTestSuite - Cleaning up typical DAT test environment\n");
        // TODO: Global typical test environment cleanup
    }

    void SetUp() override {
        printf("UT_DataTypicalFixture->SETUP: SetUp - Preparing typical DAT test scenario\n");
        // TODO: Per-test typical setup
    }

    void TearDown() override {
        printf("UT_DataTypicalFixture->CLEANUP: TearDown - Cleaning typical DAT test scenario\n");
        // TODO: Per-test typical cleanup
    }

    // TODO: Add helper methods for typical DAT operations during TDD development

    // TODO: Add typical test data members during TDD development
};

/**
 * @[Name]: templateTypicalFixtureTestCase
 * @[Steps]: TODO
 * @[Expect]: TODO
 * @[Notes]: Template for typical DAT fixture-based tests
 */
TEST_F(UT_DataTypicalFixture, templateTypicalFixtureTestCase) {
    //===SETUP===
    // TODO: Test-specific typical setup (Fixture handles common setup)

    //===BEHAVIOR===
    printf("DataTypicalFixture->BEHAVIOR: ${typical DAT operation}\n");
    // TODO: Execute typical DAT behavior

    //===VERIFY===
    // TODO: Verify typical expected results

    //===CLEANUP===
    // TODO: Test-specific typical cleanup (Fixture handles common cleanup)
}

//======END OF UNIT TESTING IMPLEMENTATION=========================================================
///////////////////////////////////////////////////////////////////////////////////////////////////

// TODO(@DataTypical): Add typical DAT test cases during TDD development
//  Focus ONLY on typical scenarios:
//  - Standard DatSender/DatReceiver operations
//  - Common data sizes and types
//  - Normal connection and transfer flows
//  - Regular callback handling

///////////////////////////////////////////////////////////////////////////////////////////////////
// 💡 TYPICAL DAT EXAMPLES - 典型DAT使用场景示例
///////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief 【示例：典型DAT传输】
 * 演示最常见的DatSender到DatReceiver数据传输场景
 */
TEST(UT_DataTypical_Examples, typicalDatTransfer_example) {
    printf("EXAMPLE: Most common DAT transfer scenario\n");

    // 典型场景演示：
    // 1. 建立标准连接
    // 2. 发送常见大小的数据
    // 3. 接收方正常接收和处理
    // 4. 正常完成传输

    ASSERT_TRUE(true);  // 典型场景演示，无实际验证
}

// Private data structure for DAT receiver callback (TDD Design)
typedef struct {
    int ReceivedDataCnt;
    ULONG_T TotalReceivedSize;
    char ReceivedContent[20480];  // Buffer for 10KB+ data
    bool CallbackExecuted;
} __DatReceiverPrivData_T;

// Callback function for receiving DAT data (TDD Design)
static IOC_Result_T __CbRecvDat_F(IOC_LinkID_T LinkID, void *pData, ULONG_T DataSize, void *pCbPriv) {
    __DatReceiverPrivData_T *pPrivData = (__DatReceiverPrivData_T *)pCbPriv;

    pPrivData->ReceivedDataCnt++;
    pPrivData->CallbackExecuted = true;

    // Copy received data to buffer for verification
    if (pPrivData->TotalReceivedSize + DataSize < sizeof(pPrivData->ReceivedContent)) {
        memcpy(pPrivData->ReceivedContent + pPrivData->TotalReceivedSize, pData, DataSize);
        pPrivData->TotalReceivedSize += DataSize;
    }

    printf("DAT Callback: Received %lu bytes, total: %lu bytes\n", DataSize, pPrivData->TotalReceivedSize);
    return IOC_RESULT_SUCCESS;
}

/**
 * @[Name]: verifyDatSenderTransmission_bySendCommonData_expectCallbackReceiveSuccess
 * @[Steps]:
 *   1) Setup DatReceiver service with CbRecvDat_F callback and DatSender connection AS SETUP.
 *      |-> DatReceiver online service with IOC_LinkUsageDatReceiver
 *      |-> Configure DatUsageArgs with __CbRecvDat_F callback
 *      |-> DatSender connect with IOC_LinkUsageDatSender
 *   2) DatSender send typical 10KB text data using IOC_sendDAT AS BEHAVIOR.
 *      |-> Create common text data (10KB)
 *      |-> Setup IOC_DatDesc_T with data payload
 *      |-> Call IOC_sendDAT with typical data chunk
 *   3) Verify data transmission success and callback reception AS VERIFY.
 *      |-> IOC_sendDAT returns IOC_RESULT_SUCCESS
 *      |-> Callback receives complete data correctly
 *      |-> Data integrity maintained (content matches)
 *   4) Cleanup: close connections and offline service AS CLEANUP.
 * @[Expect]: Data transmitted successfully and received via callback with integrity preserved.
 * @[Notes]: Focus on typical 10KB text数据传输与回调模式。
 */
TEST(UT_DataTypical, verifyDatSenderTransmission_bySendCommonData_expectCallbackReceiveSuccess) {
    //===SETUP===
    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_SrvID_T DatReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatReceiverLinkID = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID = IOC_ID_INVALID;

    // Private data for callback
    __DatReceiverPrivData_T DatReceiverPrivData = {0};

    // Standard SrvURI for typical DAT communication
    IOC_SrvURI_T CSURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = (const char *)"DatReceiverCallback",
    };

    // Step-1: Setup DatReceiver service with callback
    IOC_DatUsageArgs_T DatUsageArgs = {
        .CbRecvDat_F = __CbRecvDat_F,
        .pCbPrivData = &DatReceiverPrivData,
    };

    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = CSURI,
        .UsageCapabilites = IOC_LinkUsageDatReceiver,
        .UsageArgs =
            {
                .pDat = &DatUsageArgs,
            },
    };

    Result = IOC_onlineService(&DatReceiverSrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint: Service online success

    // Setup DatSender connection
    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = CSURI,
        .Usage = IOC_LinkUsageDatSender,
    };

    std::thread DatSenderThread([&] {
        Result = IOC_connectService(&DatSenderLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);       // VerifyPoint: Connection success
        ASSERT_NE(IOC_ID_INVALID, DatSenderLinkID);  // VerifyPoint: Valid LinkID
    });

    // Accept the connection
    Result = IOC_acceptClient(DatReceiverSrvID, &DatReceiverLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint: Accept success

    DatSenderThread.join();

    //===BEHAVIOR===
    printf("BEHAVIOR: verifyDatSenderTransmission_bySendCommonData_expectCallbackReceiveSuccess\n");

    // Step-2: Create and send typical 10KB text data
    const char *TextPattern = "TypicalDATTest_";
    const int PatternLen = strlen(TextPattern);
    const int TargetSize = 10240;  // 10KB
    char *TestData = (char *)malloc(TargetSize + 1);

    // Fill with repeating pattern
    for (int i = 0; i < TargetSize; i++) {
        TestData[i] = TextPattern[i % PatternLen];
    }
    TestData[TargetSize] = '\0';

    // Setup IOC_DatDesc_T for transmission
    IOC_DatDesc_T DatDesc = {0};
    IOC_initDatDesc(&DatDesc);
    DatDesc.Payload.pData = TestData;
    DatDesc.Payload.PtrDataSize = TargetSize;

    // Send data using IOC_sendDAT
    Result = IOC_sendDAT(DatSenderLinkID, &DatDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint: Send success
    printf("DAT Sender: Sent %d bytes of data\n", TargetSize);

    // Force send the data to ensure callback execution
    IOC_flushDAT(DatSenderLinkID, NULL);

    //===VERIFY===
    // KeyVerifyPoint-1: IOC_sendDAT returns success
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);

    // KeyVerifyPoint-2: Callback was executed and received data
    ASSERT_TRUE(DatReceiverPrivData.CallbackExecuted);
    ASSERT_EQ(1, DatReceiverPrivData.ReceivedDataCnt);
    ASSERT_EQ(TargetSize, DatReceiverPrivData.TotalReceivedSize);

    // KeyVerifyPoint-3: Data integrity maintained
    ASSERT_EQ(0, memcmp(TestData, DatReceiverPrivData.ReceivedContent, TargetSize));

    printf("TDD VERIFY: All simulated verifications passed\n");

    //===CLEANUP===
    // Free test data
    free(TestData);

    // Close both links
    if (DatSenderLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatSenderLinkID);
    }
    if (DatReceiverLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatReceiverLinkID);
    }
    // Offline service
    if (DatReceiverSrvID != IOC_ID_INVALID) {
        IOC_offlineService(DatReceiverSrvID);
    }
}

/**
 * @[Name]: verifyDatPollingReceive_byManualRetrieve_expectCompleteDataIntegrity
 * @[Steps]:
 *   1) Setup DatReceiver service WITHOUT callback (polling mode) and DatSender connection AS SETUP.
 *      |-> DatReceiver online service with IOC_LinkUsageDatReceiver (no callback configured)
 *      |-> DatSender connect with IOC_LinkUsageDatSender
 *   2) DatSender send typical 5KB binary data using IOC_sendDAT AS BEHAVIOR.
 *      |-> Create typical binary data (5KB pattern)
 *      |-> Setup IOC_DatDesc_T with data payload
 *      |-> Call IOC_sendDAT and IOC_flushDAT
 *   3) DatReceiver poll for data using IOC_recvDAT AS BEHAVIOR.
 *      |-> Setup IOC_DatDesc_T for receiving
 *      |-> Call IOC_recvDAT with MAYBLOCK option
 *      |-> Verify data reception and integrity
 *   4) Cleanup: close connections and offline service AS CLEANUP.
 * @[Expect]: Complete data polling functionality verified - data integrity, size correctness, and NONBLOCK behavior.
 *           THIS TEST WILL FAIL (TDD RED) until IOC_recvDAT is fully implemented with actual data reception logic.
 * @[Notes]: TDD RED phase - specifies full requirements for polling-based data reception.
 */
TEST(UT_DataTypical, verifyDatPollingReceive_byManualRetrieve_expectCompleteDataIntegrity) {
    //===SETUP===
    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_SrvID_T DatReceiverSrvID = IOC_ID_INVALID;
    IOC_LinkID_T DatReceiverLinkID = IOC_ID_INVALID;
    IOC_LinkID_T DatSenderLinkID = IOC_ID_INVALID;

    // Standard SrvURI for typical DAT communication
    IOC_SrvURI_T CSURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = (const char *)"DatReceiverPolling",
    };

    // Step-1: Setup DatReceiver service WITHOUT callback (polling mode)
    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = CSURI, .UsageCapabilites = IOC_LinkUsageDatReceiver,
        // No UsageArgs means no callback - pure polling mode
    };

    Result = IOC_onlineService(&DatReceiverSrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint: Service online success

    // Setup DatSender connection
    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = CSURI,
        .Usage = IOC_LinkUsageDatSender,
    };

    std::thread DatSenderThread([&] {
        Result = IOC_connectService(&DatSenderLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result);       // VerifyPoint: Connection success
        ASSERT_NE(IOC_ID_INVALID, DatSenderLinkID);  // VerifyPoint: Valid LinkID
    });

    // Accept the connection
    Result = IOC_acceptClient(DatReceiverSrvID, &DatReceiverLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint: Accept success

    DatSenderThread.join();

    //===BEHAVIOR===
    printf("BEHAVIOR: verifyDatPollingReceive_byManualRetrieve_expectCompleteDataIntegrity\n");

    // Step-2: Create and send typical 5KB binary data
    const int TargetSize = 5120;  // 5KB
    char *TestData = (char *)malloc(TargetSize);

    // Fill with binary pattern (0x00, 0x01, 0x02, ..., 0xFF, 0x00, 0x01, ...)
    for (int i = 0; i < TargetSize; i++) {
        TestData[i] = (char)(i % 256);
    }

    // Setup IOC_DatDesc_T for transmission
    IOC_DatDesc_T DatDesc = {0};
    IOC_initDatDesc(&DatDesc);
    DatDesc.Payload.pData = TestData;
    DatDesc.Payload.PtrDataSize = TargetSize;

    // Send data using IOC_sendDAT
    Result = IOC_sendDAT(DatSenderLinkID, &DatDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result);  // VerifyPoint: Send success
    printf("DAT Sender: Sent %d bytes of binary data\n", TargetSize);

    // Force send the data
    IOC_flushDAT(DatSenderLinkID, NULL);

    // Step-3: DatReceiver poll for data using IOC_recvDAT
    IOC_DatDesc_T ReceivedDatDesc = {0};
    IOC_initDatDesc(&ReceivedDatDesc);

    // Allocate buffer for received data
    char *ReceivedData = (char *)malloc(TargetSize);
    ReceivedDatDesc.Payload.pData = ReceivedData;
    ReceivedDatDesc.Payload.PtrDataSize = TargetSize;

    // Setup options for blocking polling
    IOC_Option_defineSyncMayBlock(RecvOptions);

    // Poll for data with MAYBLOCK (will wait until data arrives)
    Result = IOC_recvDAT(DatReceiverLinkID, &ReceivedDatDesc, &RecvOptions);

    //===VERIFY===
    // TDD RED PHASE: Verify actual data reception functionality
    // KeyVerifyPoint-1: IOC_recvDAT should return SUCCESS
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "IOC_recvDAT should succeed when data is available";

    // KeyVerifyPoint-2: Should receive the exact amount of data sent
    ASSERT_EQ(TargetSize, ReceivedDatDesc.Payload.PtrDataSize)
        << "Received data size should match sent data size. Expected: " << TargetSize
        << ", Actual: " << ReceivedDatDesc.Payload.PtrDataSize;

    // KeyVerifyPoint-3: Data integrity should be maintained
    ASSERT_EQ(0, memcmp(TestData, ReceivedData, TargetSize)) << "Received data content should match sent data content";

    printf("DAT Receiver: Received %lu bytes via polling, data integrity verified\n",
           ReceivedDatDesc.Payload.PtrDataSize);

    // Test NONBLOCK polling API (TDD verification)
    IOC_DatDesc_T NoDataDesc = {0};
    IOC_initDatDesc(&NoDataDesc);
    char NoDataBuffer[100];
    NoDataDesc.Payload.pData = NoDataBuffer;
    NoDataDesc.Payload.PtrDataSize = sizeof(NoDataBuffer);

    IOC_Option_defineSyncNonBlock(NoDataOptions);
    Result = IOC_recvDAT(DatReceiverLinkID, &NoDataDesc, &NoDataOptions);

    // KeyVerifyPoint-4: NONBLOCK polling should return NO_DATA when no more data available
    ASSERT_EQ(IOC_RESULT_NO_DATA, Result)
        << "IOC_recvDAT in NONBLOCK mode should return NO_DATA when no data available";

    printf(
        "TDD RED PHASE: All polling functionality verification specified (will FAIL until IOC_recvDAT is fully "
        "implemented)\n");

    //===CLEANUP===
    // Free test data
    free(TestData);
    free(ReceivedData);

    // Close both links
    if (DatSenderLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatSenderLinkID);
    }
    if (DatReceiverLinkID != IOC_ID_INVALID) {
        IOC_closeLink(DatReceiverLinkID);
    }
    // Offline service
    if (DatReceiverSrvID != IOC_ID_INVALID) {
        IOC_offlineService(DatReceiverSrvID);
    }
}
