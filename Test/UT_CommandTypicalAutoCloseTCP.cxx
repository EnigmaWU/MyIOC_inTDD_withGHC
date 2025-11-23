///////////////////////////////////////////////////////////////////////////////////////////////////
// Command Typical Auto-Close TCP (TCP protocol) — UT skeleton
//
// PURPOSE:
//   Verify TCP protocol layer integration with Automatic Link Closure patterns.
//   This test suite validates that TCP links are automatically and correctly closed
//   under various lifecycle events (Service Offline, Client Disconnect, Errors),
//   ensuring no resource leaks (sockets, threads, memory) occur.
//
// TDD WORKFLOW:
//   Design → Draft → Structure → Test (RED) → Code (GREEN) → Refactor → Repeat
//
// REFERENCE: LLM/CaTDD_DesignPrompt.md for full methodology
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <thread>

#include "_UT_IOC_Common.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief
 *   [WHAT] This file verifies TCP-based Automatic Link Closure (Resource Cleanup)
 *   [WHERE] in the IOC Command API with TCP protocol layer (_IOC_SrvProtoTCP.c)
 *   [WHY] to ensure robust resource management and prevent socket leaks.
 *
 * SCOPE:
 *   - [In scope]: Default Auto-Cleanup behavior (Service Offline → Close All Links)
 *   - [In scope]: Client Disconnect handling (Client Close → Server Resource Free)
 *   - [In scope]: TCP-specific concerns: FIN/RST handling, TIME_WAIT avoidance (SO_REUSEADDR)
 *   - [Out of scope]: Persistent links (IOC_SRVFLAG_KEEP_ACCEPTED_LINK) - see AutoAcceptTCP
 *
 * KEY CONCEPTS:
 *   - Auto-Cleanup: Default behavior where IOC_offlineService closes all accepted links
 *   - Peer Disconnect: Server detects client closure (recv returns 0) and closes link
 *   - Resource Leak: Failure to close socket or join thread
 *
 * KEY DIFFERENCES FROM UT_CommandTypicalAutoAcceptTCP.cxx:
 *   - Focus: Destruction/Cleanup vs Creation/Acceptance
 *   - Protocol: Same (TCP)
 *   - Port Management: Unique ports (18300+) to avoid conflicts
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING DESIGN==============================================================
/**
 * COVERAGE STRATEGY: Trigger × Resource State × Validation Method
 *
 * COVERAGE MATRIX (Systematic Test Planning):
 * ┌──────────────────────┬───────────────────┬─────────────────────┬────────────────────────────┐
 * │ Trigger              │ Resource State    │ Validation Method   │ Key Scenarios              │
 * ├──────────────────────┼───────────────────┼─────────────────────┼────────────────────────────┤
 * │ Service Offline      │ Active Connection │ Client recv()       │ US-1: Service shutdown     │
 * │ Client Disconnect    │ Established Link  │ LinkID validity     │ US-2: Peer-initiated close │
 * │ Immediate Restart    │ TIME_WAIT state   │ Bind success        │ US-3: Port reuse           │
 * └──────────────────────┴───────────────────┴─────────────────────┴────────────────────────────┘
 *
 * PRIORITY FRAMEWORK (P1 → P2 → P3):
 *   P1 🥇 FUNCTIONAL (ValidFunc):
 *     - Typical: Service offline cleanup (TC-1)
 *   P1 🥇 FUNCTIONAL (InvalidFunc):
 *     - Fault: Client disconnect handling (TC-2)
 *   P3 🥉 QUALITY (Usability):
 *     - Configuration: Port reuse verification (TC-3)
 *
 * CONTEXT-SPECIFIC ADJUSTMENT:
 *   - Resource Management Focus: Promote Fault (Client Disconnect) to P1 level
 *   - Rationale: Memory/socket leaks are critical failures in network services
 *
 * RISK ASSESSMENT:
 *   TC-1 (Service Offline): Impact=3, Likelihood=3, Uncertainty=1 → Score=9 (P1 ValidFunc)
 *   TC-2 (Client Disconnect): Impact=3, Likelihood=2, Uncertainty=2 → Score=12 (Promoted to P1)
 *   TC-3 (Port Reuse): Impact=2, Likelihood=2, Uncertainty=1 → Score=4 (Keep P3)
 *
 * Design focus:
 *  - TCP Socket Lifecycle Verification (Open → Connected → Closed)
 *  - Server-side cleanup when Service goes offline
 *  - Server-side cleanup when Client disconnects
 *  - Robustness against abrupt disconnections
 *
 * Test progression:
 *  - Service Offline Auto-Close (Basic - P1 ValidFunc)
 *  - Client Disconnect Auto-Close (Peer initiated - P1 Fault, promoted)
 *  - Port Reuse (SO_REUSEADDR - P3 Usability)
 *
 * QUALITY GATE P1:
 *   ✅ TC-1 GREEN (Service offline closes all links)
 *   ✅ TC-2 GREEN (Client disconnect detected and handled)
 *   ✅ No socket/thread leaks (verified via system tools or AddressSanitizer)
 *   ✅ Client-side observability (recv returns 0/error on close)
 */
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================
/**
 * US-1: As a service developer, I want all accepted TCP links to automatically close
 *       when I take the service offline, so that I don't leak network resources.
 *
 * US-2: As a service developer, I want the service to automatically detect and close
 *       links when a TCP client disconnects, so that I don't keep dead connections.
 *
 * US-3: As a system integrator, I want to ensure TCP ports are released immediately
 *       after service offline, so that I can restart the service without "Address in use" errors.
 */
//======>END OF USER STORY==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//=======>BEGIN OF ACCEPTANCE CRITERIA==============================================================
/**
 * [@US-1] Service Offline Auto-Close
 *  AC-1: GIVEN a TCP service with active connections,
 *         WHEN IOC_offlineService is called,
 *         THEN all associated TCP sockets are closed on the server side.
 *  AC-2: GIVEN a client connected to the service,
 *         WHEN the service goes offline,
 *         THEN the client detects the connection closure (recv returns 0 or error).
 *
 * [@US-2] Client Disconnect Auto-Close
 *  AC-1: GIVEN a connected TCP client,
 *         WHEN the client closes the socket,
 *         THEN the service detects the closure and invalidates the LinkID.
 *
 * [@US-3] Port Release & Reuse
 *  AC-1: GIVEN a TCP service that has just gone offline,
 *         WHEN I immediately try to bind to the same port again,
 *         THEN the operation succeeds (SO_REUSEADDR verification).
 */
//=======>END OF ACCEPTANCE CRITERIA================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES=======================================================================
/**************************************************************************************************
 * @brief 【TCP Auto-Close Test Cases】
 *
 * ORGANIZATION STRATEGIES:
 *  - By Trigger: Service Offline vs Client Disconnect
 *  - By Protocol: TCP specific validation
 *
 * STATUS TRACKING: ⚪ = Planned/TODO，🔴 = Implemented/RED, 🟢 = Passed/GREEN, ⚠️ = Issues
 *
 * PORT ALLOCATION STRATEGY:
 *  - Range: 18300 - 18399
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [US-1]: Service Offline Auto-Close
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 *
 * [@AC-1,US-1] Basic Service Offline Cleanup
 *  ⚪ TC-1: verifyTcpAutoClose_byServiceOffline_expectAllLinksClosed
 *      @[Purpose]: Validate that IOC_offlineService closes all accepted TCP sockets
 *      @[Brief]: Service(TCP) → Client connects → Service Offline → Verify Client sees close
 *      @[Protocol]: tcp://localhost:18300/AutoCloseTCP_Offline
 *      @[Status]: TODO
 *      @[Steps]:
 *          1. Start TCP service on port 18300
 *          2. Client connects
 *          3. Verify connection established
 *          4. IOC_offlineService()
 *          5. Client attempts to recv/send → Expect Error/Closed
 *          6. Cleanup
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [US-2]: Client Disconnect Auto-Close
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 *
 * [@AC-1,US-2] Peer Disconnect Detection
 *  ⚪ TC-1: verifyTcpAutoClose_byClientDisconnect_expectLinkInvalidation
 *      @[Purpose]: Validate that service cleans up link when client disconnects
 *      @[Brief]: Service(TCP) → Client connects → Client Closes → Service detects
 *      @[Protocol]: tcp://localhost:18301/AutoCloseTCP_ClientDisc
 *      @[Status]: TODO
 *      @[Steps]:
 *          1. Start TCP service on port 18301
 *          2. Client connects
 *          3. Service accepts (LinkID_Srv)
 *          4. Client closes socket
 *          5. Wait small delay
 *          6. Service attempts to use LinkID_Srv → Expect IOC_RESULT_NOT_EXIST_LINK or similar
 *          7. Cleanup
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 * 📋 [US-3]: Port Release & Reuse
 * ═══════════════════════════════════════════════════════════════════════════════════════════════
 *
 * [@AC-1,US-3] Immediate Port Reuse
 *  ⚪ TC-1: verifyTcpPortReuse_byImmediateRestart_expectSuccess
 *      @[Purpose]: Validate SO_REUSEADDR behavior
 *      @[Brief]: Service Online → Offline → Online (same port) immediately
 *      @[Protocol]: tcp://localhost:18302/AutoCloseTCP_Reuse
 *      @[Status]: TODO
 *      @[Steps]:
 *          1. Start TCP service on port 18302
 *          2. Stop service
 *          3. Immediately Start TCP service on port 18302
 *          4. Expect Success (not Address In Use)
 *          5. Cleanup
 */
//======>END OF TEST CASES=========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TODO/IMPLEMENTATION TRACKING SECTION=============================================
// 🔴 IMPLEMENTATION STATUS TRACKING
//
// STATUS LEGEND:
//   ⚪ TODO/PLANNED:      Designed but not implemented yet.
//   🔴 RED/FAILING:       Test written, but production code is missing or incorrect.
//   🟢 GREEN/PASSED:      Test written and passing.
//
// PRIORITY LEVELS:
//   P1 🥇 FUNCTIONAL:     Service Offline (TC-1)
//   P2 🥈 ROBUSTNESS:     Client Disconnect (TC-2)
//   P3 🥉 USABILITY:      Port Reuse (TC-3)
//
// TRACKING:
//   ⚪ [@AC-1,US-1] TC-1: verifyTcpAutoClose_byServiceOffline_expectAllLinksClosed
//   ⚪ [@AC-1,US-2] TC-1: verifyTcpAutoClose_byClientDisconnect_expectLinkInvalidation
//   ⚪ [@AC-1,US-3] TC-1: verifyTcpPortReuse_byImmediateRestart_expectSuccess
//
//======>END OF TODO/IMPLEMENTATION TRACKING SECTION===============================================

///////////////////////////////////////////////////////////////////////////////////////////////////
// Helper: Simple TCP Client for testing
///////////////////////////////////////////////////////////////////////////////////////////////////
class TcpClient {
    int m_sock = -1;

   public:
    TcpClient() {}
    ~TcpClient() { closeSocket(); }

    bool connectToServer(const char* ip, uint16_t port) {
        m_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (m_sock < 0) return false;

        struct sockaddr_in serv_addr;
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);
        inet_pton(AF_INET, ip, &serv_addr.sin_addr);

        if (connect(m_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
            closeSocket();
            return false;
        }
        return true;
    }

    // Returns true if socket is closed by peer (recv returns 0)
    bool waitForClose(int timeoutMs) {
        if (m_sock < 0) return true;

        struct timeval tv;
        tv.tv_sec = timeoutMs / 1000;
        tv.tv_usec = (timeoutMs % 1000) * 1000;
        setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

        char buffer[16];
        int n = recv(m_sock, buffer, sizeof(buffer), 0);
        if (n == 0) return true;  // Peer closed
        if (n < 0) {
            // If error is not EAGAIN/EWOULDBLOCK, it might be a reset
            if (errno != EAGAIN && errno != EWOULDBLOCK) return true;
        }
        return false;
    }

    void closeSocket() {
        if (m_sock >= 0) {
            close(m_sock);
            m_sock = -1;
        }
    }

    int getSocket() const { return m_sock; }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Test Case Implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

// [@AC-1,US-1] TC-1: verifyTcpAutoClose_byServiceOffline_expectAllLinksClosed
TEST(UT_CommandTypicalAutoCloseTCP, verifyTcpAutoClose_byServiceOffline_expectAllLinksClosed) {
    // 1. Setup Service
    const uint16_t PORT = 18300;
    IOC_SrvURI_T srvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP, .pHost = "0.0.0.0", .Port = PORT, .pPath = "AutoCloseTCP_Offline"};

    IOC_SrvID_T srvID;
    IOC_SrvArgs_T srvArgs = {0};
    srvArgs.SrvURI = srvURI;

    IOC_Result_T res = IOC_onlineService(&srvID, &srvArgs);
    ASSERT_EQ(res, IOC_RESULT_SUCCESS);

    // 2. Client Connects
    TcpClient client;
    bool connected = client.connectToServer("127.0.0.1", PORT);
    ASSERT_TRUE(connected);

    // Allow time for accept
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 3. Service Offline -> Should close all links
    res = IOC_offlineService(srvID);
    ASSERT_EQ(res, IOC_RESULT_SUCCESS);

    // 4. Verify Client sees closure
    // recv() should return 0 (FIN) or error (RST) immediately
    bool closedByPeer = client.waitForClose(1000);  // 1s timeout
    EXPECT_TRUE(closedByPeer) << "Client socket should detect closure from server side";
}

// [@AC-1,US-2] TC-1: verifyTcpAutoClose_byClientDisconnect_expectLinkInvalidation
TEST(UT_CommandTypicalAutoCloseTCP, verifyTcpAutoClose_byClientDisconnect_expectLinkInvalidation) {
    // 1. Setup Service
    const uint16_t PORT = 18301;
    IOC_SrvURI_T srvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP, .pHost = "0.0.0.0", .Port = PORT, .pPath = "AutoCloseTCP_ClientDisc"};

    IOC_SrvID_T srvID;
    IOC_SrvArgs_T srvArgs = {0};
    srvArgs.SrvURI = srvURI;
    // We need CmdExecutor capability to call execCMD later (even if it fails)
    // But actually we just want to check if LinkID is valid.
    // Any operation on invalid link should fail.
    srvArgs.UsageCapabilites = IOC_LinkUsageCmdExecutor;
    // We need to provide dummy callback if we declare Executor capability
    IOC_CmdUsageArgs_T cmdArgs = {0};
    cmdArgs.CbExecCmd_F = [](IOC_LinkID_T, IOC_CmdDesc_pT, void*) -> IOC_Result_T { return IOC_RESULT_SUCCESS; };
    srvArgs.UsageArgs.pCmd = &cmdArgs;

    ASSERT_EQ(IOC_onlineService(&srvID, &srvArgs), IOC_RESULT_SUCCESS);

    // 2. Client Connects
    TcpClient client;
    ASSERT_TRUE(client.connectToServer("127.0.0.1", PORT));

    // 3. Server Accepts
    IOC_LinkID_T linkID;
    IOC_Result_T acceptRes = IOC_RESULT_BUG;
    for (int i = 0; i < 20; i++) {
        acceptRes = IOC_acceptClient(srvID, &linkID, NULL);
        if (acceptRes == IOC_RESULT_SUCCESS) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    ASSERT_EQ(acceptRes, IOC_RESULT_SUCCESS);

    // 4. Client Closes (Destructor calls closeSocket)
    client.closeSocket();

    // 5. Wait for Server to detect disconnect
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // 6. Verify Link is Invalid
    // Try to send a command to the old LinkID
    IOC_CmdDesc_T cmdDesc = {0};
    cmdDesc.CmdID = 1;
    IOC_Result_T res = IOC_execCMD(linkID, &cmdDesc, NULL);

    // Expect failure because link should be gone
    EXPECT_NE(res, IOC_RESULT_SUCCESS) << "Link should be invalid after client disconnect";
    // Specifically, it should be NOT_EXIST_LINK or LINK_BROKEN
    EXPECT_TRUE(res == IOC_RESULT_NOT_EXIST_LINK || res == IOC_RESULT_LINK_BROKEN);

    IOC_offlineService(srvID);
}

// [@AC-1,US-3] TC-1: verifyTcpPortReuse_byImmediateRestart_expectSuccess
TEST(UT_CommandTypicalAutoCloseTCP, verifyTcpPortReuse_byImmediateRestart_expectSuccess) {
    const uint16_t PORT = 18302;
    IOC_SrvURI_T srvURI = {
        .pProtocol = IOC_SRV_PROTO_TCP, .pHost = "0.0.0.0", .Port = PORT, .pPath = "AutoCloseTCP_Reuse"};

    // 1. Start First Instance
    IOC_SrvID_T srvID1;
    IOC_SrvArgs_T srvArgs1 = {0};
    srvArgs1.SrvURI = srvURI;

    ASSERT_EQ(IOC_onlineService(&srvID1, &srvArgs1), IOC_RESULT_SUCCESS);

    // 2. Stop First Instance
    ASSERT_EQ(IOC_offlineService(srvID1), IOC_RESULT_SUCCESS);

    // 3. Immediately Start Second Instance (Same Port)
    IOC_SrvID_T srvID2;
    IOC_SrvArgs_T srvArgs2 = {0};
    srvArgs2.SrvURI = srvURI;

    IOC_Result_T res = IOC_onlineService(&srvID2, &srvArgs2);
    EXPECT_EQ(res, IOC_RESULT_SUCCESS) << "Should be able to reuse port immediately (SO_REUSEADDR)";

    IOC_offlineService(srvID2);
}
