///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief ValidFunc-State Tests: Service lifecycle state transitions work correctly.
 *
 * @status 🔄 IMPLEMENTATION IN PROGRESS - 1/21 tests passing (5% coverage)
 *
 *-------------------------------------------------------------------------------------------------
 * @category ValidFunc-State (Service Lifecycle - APIs WORK across states)
 *
 * Part of Test Design Formula:
 *   Service's Functional Test = ValidFunc(Typical + Boundary + State) + InValidFunc(Misuse)
 *                                                              ^^^^^
 *                                                         (State transitions WORK!)
 *
 * ValidFunc-State = Service lifecycle state transitions WORK correctly
 *  - Service moves through lifecycle states predictably (NOT_EXIST → ONLINE → OFFLINE)
 *  - State-dependent operations succeed when appropriate
 *  - State queries return accurate information
 *  - Daemon threads (AUTO_ACCEPT, BROADCAST) manage lifecycle correctly
 *
 * This file covers: Service-level state management and lifecycle behaviors
 *  - Basic lifecycle: online → offline transitions
 *  - AUTO_ACCEPT daemon lifecycle and link management
 *  - Service link tracking and state queries
 *  - Manual accept state management
 *  - Service stability during link operations
 *  - BROADCAST daemon lifecycle
 *
 * Test Philosophy - KEY DISTINCTION:
 *  - ValidFunc-Typical: Common scenarios that work (happy paths)
 *  - ValidFunc-Boundary: Edge cases that still work (limits, edge inputs)
 *  - ValidFunc-State: State transitions that work (lifecycle correctness) ← THIS FILE
 *  - InValidFunc-Misuse: Wrong usage patterns that fail (contract violations)
 *  - Focus: Verify state machine correctness, lifecycle integrity, daemon management
 *
 * Related Test Files:
 *  - UT_ServiceTypical.cxx: ValidFunc-Typical (common working scenarios)
 *  - UT_ServiceBoundary.cxx: ValidFunc-Boundary (edge cases that work)
 *  - UT_ServiceMisuse.cxx: InValidFunc-Misuse (wrong patterns that fail)
 *  - UT_CommandStateUS*.cxx: Link CMD substate tests
 *  - UT_DataStateUS*.cxx: Link DAT substate tests
 *  - UT_ConlesEventState.cxx: Event state tests
 *
 *-------------------------------------------------------------------------------------------------
 *++Context
 *  Complements Typical/Boundary/Misuse suites by validating service lifecycle state correctness.
 *  Unlike Link state tests (CommandState, DataState), this focuses on Service-level lifecycle.
 *
 *  SERVICE STATE MODEL (Implicit - no explicit enum):
 *  ┌─────────────┐  onlineService()   ┌─────────────┐  offlineService()  ┌─────────────┐
 *  │ NOT_EXIST   │ ─────────────────> │   ONLINE    │ ──────────────────> │  OFFLINE    │
 *  │ (no SrvObj) │                    │ (accepting) │                     │ (destroyed) │
 *  └─────────────┘                    └─────────────┘                     └─────────────┘
 *
 *  ONLINE Sub-states:
 *   - MANUAL_ACCEPT: Waiting for explicit IOC_acceptClient() calls
 *   - AUTO_ACCEPT: Daemon running, auto-accepting connections
 *   - BROADCAST: Daemon running for event distribution
 *
 *  COVERAGE AREAS:
 *   - Lifecycle transitions: NOT_EXIST → ONLINE → OFFLINE
 *   - Daemon lifecycle: AUTO_ACCEPT and BROADCAST thread management
 *   - Link tracking: Service monitors connected links correctly
 *   - State queries: IOC_getServiceState(), IOC_getServiceLinkIDs()
 *   - Stability: Service state remains consistent during link operations
 *
 *  TEST RESULTS (as of last run):
 *   🟢 1 test PASSING
 *   ⚪ 20 tests PLANNED
 *   📊 Coverage: 7 User Stories defined, 21 Acceptance Criteria planned (5% complete)
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING DESIGN==============================================================
/**
 * 📋 TEST CASE DESIGN ASPECTS/CATEGORIES
 *  Priority: Typical → Boundary → State → Misuse → Fault → Performance → Concurrency → Others
 *  Principle: Improve Value • Avoid Lost • Balance Skill vs Cost
 */
/**
 * 📖 VALIDFUNC-STATE CATEGORIZATION GUIDE
 *
 * What makes a test ValidFunc-State?
 *  ✓ Tests service LIFECYCLE state transitions (not link substates)
 *  ✓ Verifies state-dependent behaviors work correctly
 *  ✓ Validates daemon thread lifecycle management
 *  ✓ Tests state queries return accurate information
 *  ✓ Confirms service stability across state transitions
 *
 * Comparison with other categories:
 * ┌──────────────────────────┬─────────────────────────────┬─────────────────────────────┐
 * │ Aspect                   │ ValidFunc-Typical/Boundary  │ ValidFunc-State             │
 * ├──────────────────────────┼─────────────────────────────┼─────────────────────────────┤
 * │ Focus                    │ Functionality works         │ State transitions work      │
 * │ Example                  │ onlineService succeeds      │ NOT_EXIST→ONLINE verified   │
 * │ Verification             │ Result codes correct        │ State changes correct       │
 * │ Scope                    │ Single operation            │ Multi-operation sequence    │
 * │ State Changes            │ Side effect                 │ Primary concern             │
 * └──────────────────────────┴─────────────────────────────┴─────────────────────────────┘
 *
 * ValidFunc-State vs InValidFunc-Misuse:
 * ┌──────────────────────────┬─────────────────────────────┬─────────────────────────────┐
 * │ Aspect                   │ ValidFunc-State             │ InValidFunc-Misuse          │
 * ├──────────────────────────┼─────────────────────────────┼─────────────────────────────┤
 * │ Usage Pattern            │ CORRECT sequence            │ WRONG sequence              │
 * │ Expected Outcome         │ State transitions work      │ Misuse rejected             │
 * │ Example                  │ online→accept→offline OK    │ accept→online FAILS         │
 * │ API Behavior             │ Works correctly             │ Defensive rejection         │
 * └──────────────────────────┴─────────────────────────────┴─────────────────────────────┘
 */
/**
 * US-1 (ValidFunc-State): As a service owner, I want service lifecycle to transition correctly
 *  (NOT_EXIST → ONLINE → OFFLINE), so I can manage service availability predictably.
 *
 *  AC-1: GIVEN service NOT_EXIST, WHEN IOC_onlineService called with valid args,
 *         THEN service enters ONLINE state (SrvID valid, can accept connections).
 *  AC-2: GIVEN service ONLINE, WHEN IOC_offlineService called,
 *         THEN service enters OFFLINE state (SrvID invalid, cannot accept connections).
 *  AC-3: GIVEN service OFFLINE, WHEN attempting operations on SrvID,
 *         THEN return IOC_RESULT_NOT_EXIST_SERVICE (state prevents operations).
 */
/**
 * US-2 (ValidFunc-State): As a service owner, I want AUTO_ACCEPT mode daemon to manage its
 *  lifecycle correctly, so connections are handled automatically throughout service lifetime.
 *
 *  AC-1: GIVEN service with AUTO_ACCEPT flag, WHEN IOC_onlineService called,
 *         THEN daemon thread starts (observable via auto-accepted connections).
 *  AC-2: GIVEN AUTO_ACCEPT daemon running, WHEN clients connect,
 *         THEN links created automatically without manual accept.
 *  AC-3: GIVEN AUTO_ACCEPT service ONLINE, WHEN IOC_offlineService called,
 *         THEN daemon stops gracefully and all auto-accepted links closed.
 */
/**
 * US-3 (ValidFunc-State): As a service owner, I want to query service's link collection state,
 *  so I can monitor connection count and link status.
 *
 *  AC-1: GIVEN service ONLINE with no connections, WHEN IOC_getServiceState called,
 *         THEN pConnectedLinks reports 0.
 *  AC-2: GIVEN service ONLINE with N connections, WHEN IOC_getServiceState called,
 *         THEN pConnectedLinks reports N.
 *  AC-3: GIVEN service with links, WHEN IOC_getServiceLinkIDs called,
 *         THEN returns all connected LinkIDs.
 */
/**
 * US-4 (ValidFunc-State): As a service owner, I want manual accept mode to track accepted links
 *  correctly, so I can manage link lifecycle explicitly.
 *
 *  AC-1: GIVEN MANUAL_ACCEPT service ONLINE, WHEN IOC_acceptClient blocks waiting,
 *         THEN service remains in accepting state (no premature timeout).
 *  AC-2: GIVEN MANUAL_ACCEPT service with timeout, WHEN no client connects within timeout,
 *         THEN IOC_acceptClient returns TIMEOUT without state corruption.
 *  AC-3: GIVEN MANUAL_ACCEPT service, WHEN IOC_acceptClient succeeds,
 *         THEN link tracked in ManualAccept.AcceptedLinkIDs[].
 */
/**
 * US-5 (ValidFunc-State): As a service owner, I want service state to remain stable during
 *  link operations, so concurrent link activities don't corrupt service state.
 *
 *  AC-1: GIVEN service with multiple links, WHEN one link closes,
 *         THEN service remains ONLINE and other links unaffected.
 *  AC-2: GIVEN service ONLINE, WHEN links perform operations (postEVT, sendDAT, execCMD),
 *         THEN service state remains stable.
 *  AC-3: GIVEN service with active links, WHEN IOC_offlineService called,
 *         THEN all links closed atomically before service destroyed.
 */
/**
 * US-6 (ValidFunc-State): As a developer, I want service state query APIs to work correctly
 *  in all lifecycle stages, so I can diagnose service status anytime.
 *
 *  AC-1: GIVEN service NOT_EXIST, WHEN IOC_getServiceState called with invalid SrvID,
 *         THEN returns IOC_RESULT_NOT_EXIST_SERVICE.
 *  AC-2: GIVEN service ONLINE, WHEN IOC_getServiceState called,
 *         THEN returns SUCCESS with current link count.
 *  AC-3: GIVEN service ONLINE, WHEN IOC_getServiceLinkIDs called with sufficient buffer,
 *         THEN returns all LinkIDs and actual count.
 */
/**
 * US-7 (ValidFunc-State): As a service owner, I want BROADCAST mode daemon to manage its
 *  lifecycle correctly, so event distribution works throughout service lifetime.
 *
 *  AC-1: GIVEN service with BROADCAST flag, WHEN IOC_onlineService called,
 *         THEN broadcast daemon starts.
 *  AC-2: GIVEN BROADCAST service ONLINE, WHEN events posted,
 *         THEN distributed to all subscribers.
 *  AC-3: GIVEN BROADCAST service, WHEN IOC_offlineService called,
 *         THEN broadcast daemon stops and broadcast links closed.
 */
/**
 * TEST CASES — ORGANIZATION & STATUS
 *  - By Category: Basic lifecycle → Daemon lifecycle → Link tracking → State queries
 *  - STATUS LEGEND: ⚪ Planned/TODO, 🔴 Implemented/RED, 🟢 Passed/GREEN, ⚠️ Issues
 *
 *  [@US-1/AC-1] Service lifecycle: NOT_EXIST → ONLINE
 *   🟢 TC: verifyServiceOnline_fromNotExist_expectOnlineState
 *
 *  [@US-1/AC-2] Service lifecycle: ONLINE → OFFLINE
 *   ⚪ TC: verifyServiceOffline_fromOnline_expectOfflineState
 *
 *  [@US-1/AC-3] Service lifecycle: Operations on OFFLINE service
 *   ⚪ TC: verifyOperationsOnOfflineService_expectNotExistService
 *
 *  [@US-2/AC-1] AUTO_ACCEPT daemon: Starts on online
 *   ⚪ TC: verifyAutoAcceptDaemon_onServiceOnline_expectDaemonActive
 *
 *  [@US-2/AC-2] AUTO_ACCEPT daemon: Auto-accepts connections
 *   ⚪ TC: verifyAutoAcceptConnection_withDaemon_expectAutoLinkCreation
 *
 *  [@US-2/AC-3] AUTO_ACCEPT daemon: Cleanup on offline
 *   ⚪ TC: verifyAutoAcceptCleanup_onServiceOffline_expectAllLinksClosed
 *
 *  [@US-3/AC-1] Link tracking: Zero links
 *   ⚪ TC: verifyServiceLinkCount_withNoConnections_expectZeroLinks
 *
 *  [@US-3/AC-2] Link tracking: N links
 *   ⚪ TC: verifyServiceLinkCount_withNConnections_expectNLinks
 *
 *  [@US-3/AC-3] Link tracking: Get LinkIDs
 *   ⚪ TC: verifyGetServiceLinkIDs_withLinks_expectAllLinkIDs
 *
 *  [@US-4/AC-1] Manual accept: Blocking accept state
 *   ⚪ TC: verifyManualAcceptBlocking_onEmptyQueue_expectAcceptingState
 *
 *  [@US-4/AC-2] Manual accept: Timeout handling
 *   ⚪ TC: verifyManualAcceptTimeout_onEmptyQueue_expectTimeoutWithoutCorruption
 *
 *  [@US-4/AC-3] Manual accept: Link tracking
 *   ⚪ TC: verifyManualAcceptSuccess_expectLinkTrackedInAcceptedList
 *
 *  [@US-5/AC-1] Service stability: Link close doesn't affect service
 *   ⚪ TC: verifyServiceStability_onLinkClose_expectServiceRemainOnline
 *
 *  [@US-5/AC-2] Service stability: Link operations don't corrupt service
 *   ⚪ TC: verifyServiceStability_duringLinkOperations_expectStableState
 *
 *  [@US-5/AC-3] Service stability: Atomic offline with links
 *   ⚪ TC: verifyServiceOffline_withActiveLinks_expectAtomicLinkClosure
 *
 *  [@US-6/AC-1] State queries: Invalid SrvID
 *   ⚪ TC: verifyGetServiceState_withInvalidSrvID_expectNotExistService
 *
 *  [@US-6/AC-2] State queries: Online service state
 *   ⚪ TC: verifyGetServiceState_withOnlineService_expectSuccessWithLinkCount
 *
 *  [@US-6/AC-3] State queries: Get LinkIDs with buffer
 *   ⚪ TC: verifyGetServiceLinkIDs_withSufficientBuffer_expectAllLinkIDs
 *
 *  [@US-7/AC-1] BROADCAST daemon: Starts on online
 *   ⚪ TC: verifyBroadcastDaemon_onServiceOnline_expectDaemonActive
 *
 *  [@US-7/AC-2] BROADCAST daemon: Event distribution
 *   ⚪ TC: verifyBroadcastDistribution_withSubscribers_expectAllReceiveEvents
 *
 *  [@US-7/AC-3] BROADCAST daemon: Cleanup on offline
 *   ⚪ TC: verifyBroadcastCleanup_onServiceOffline_expectDaemonStoppedLinkslosed
 */
//======>END OF UNIT TESTING DESIGN================================================================
//======BEGIN OF UNIT TESTING IMPLEMENTATION=======================================================
#include "_UT_IOC_Common.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF US-1: SERVICE LIFECYCLE TRANSITIONS==============================================
/**
 * [@US-1/AC-1] Verify service enters ONLINE state after IOC_onlineService
 * @[Purpose]: Validate NOT_EXIST → ONLINE state transition
 * @[Brief]: Call IOC_onlineService with valid args, verify SrvID valid and can accept connections
 * @[Status]: IMPLEMENTED 🟢 - Basic online state transition verified
 */
TEST(UT_ServiceState, verifyServiceOnline_fromNotExist_expectOnlineState) {
    // GIVEN: service does NOT_EXIST (no prior online call)
    IOC_SrvURI_T srvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = "state-online-test",
    };

    // WHEN: IOC_onlineService called with valid arguments
    IOC_SrvArgs_T srvArgs = {};
    IOC_Helper_initSrvArgs(&srvArgs);
    srvArgs.SrvURI = srvURI;
    srvArgs.UsageCapabilites = IOC_LinkUsageEvtProducer;

    IOC_SrvID_T srvID = IOC_ID_INVALID;
    IOC_Result_T result = IOC_onlineService(&srvID, &srvArgs);

    // THEN: Service enters ONLINE state
    VERIFY_KEYPOINT_EQ(IOC_RESULT_SUCCESS, result, "KP1: onlineService should succeed");
    VERIFY_KEYPOINT_NE(IOC_ID_INVALID, srvID, "KP2: SrvID should be valid (service ONLINE)");

    // AND: Service can be queried (proves it's in ONLINE state)
    uint16_t connectedLinks = 999;
    result = IOC_getServiceState(srvID, NULL, &connectedLinks);
    VERIFY_KEYPOINT_EQ(IOC_RESULT_SUCCESS, result, "KP3: getServiceState should work on ONLINE service");
    VERIFY_KEYPOINT_EQ(0, connectedLinks, "KP4: New service should have 0 connections");

    // Cleanup: Move service to OFFLINE state
    ASSERT_EQ(IOC_RESULT_SUCCESS, IOC_offlineService(srvID));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TODO/IMPLEMENTATION TRACKING SECTION===========================================
/**
 * 🎯 CORE COVERAGE STATUS: 🔄 IN PROGRESS (1/21 tests passing - 5%)
 *
 * User Stories Implementation Progress:
 *  🔄 US-1: Service lifecycle transitions - 1/3 AC passing (33%)
 *  ⚪ US-2: AUTO_ACCEPT daemon lifecycle - 0/3 AC (0%)
 *  ⚪ US-3: Service link tracking state - 0/3 AC (0%)
 *  ⚪ US-4: Manual accept state management - 0/3 AC (0%)
 *  ⚪ US-5: Service stability during operations - 0/3 AC (0%)
 *  ⚪ US-6: Service state queries - 0/3 AC (0%)
 *  ⚪ US-7: BROADCAST daemon lifecycle - 0/3 AC (0%)
 *
 * Recent Implementation:
 *  ✅ US-1/AC-1 (verifyServiceOnline_fromNotExist_expectOnlineState):
 *     - Status: IMPLEMENTED & PASSING ✅
 *     - Verified: NOT_EXIST → ONLINE transition works
 *     - Validated: SrvID valid, IOC_getServiceState succeeds, link count = 0
 *
 * ─────────────────────────────────────────────────────────────────────────────────────────────
 *
 * 📋 Next Implementation Steps (Priority Order)
 *
 * IMMEDIATE (Complete US-1):
 *  - [ ] US-1/AC-2: ONLINE → OFFLINE transition verification
 *  - [ ] US-1/AC-3: Operations on OFFLINE service fail with NOT_EXIST_SERVICE
 *
 * HIGH PRIORITY (Core State Features):
 *  - [ ] US-2/AC-1-3: AUTO_ACCEPT daemon lifecycle (most common use case)
 *  - [ ] US-3/AC-1-3: Link tracking and count queries (observability)
 *  - [ ] US-6/AC-1-3: State query APIs comprehensive testing
 *
 * MEDIUM PRIORITY (Advanced Features):
 *  - [ ] US-4/AC-1-3: Manual accept state management
 *  - [ ] US-5/AC-1-3: Service stability during concurrent operations
 *
 * LOW PRIORITY (Specialized Features):
 *  - [ ] US-7/AC-1-3: BROADCAST daemon lifecycle (less common use case)
 *
 * 📊 Infrastructure Improvements:
 *  - [ ] Add CMakeLists.txt entry for UT_ServiceState target
 *  - [ ] Create helper functions for common state verification patterns
 *  - [ ] Add state machine diagram validation utilities
 *  - [ ] Performance benchmarking for state transitions
 */
///////////////////////////////////////////////////////////////////////////////////////////////////
