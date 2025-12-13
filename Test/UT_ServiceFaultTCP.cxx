///////////////////////////////////////////////////////////////////////////////////////////////////
// CaTDD Implementation: UT_ServiceFaultTCP.cxx
//
// CATEGORY: InValidFunc-Fault-TCP (TCP Network Failures & Error Recovery)
// STATUS: 🔴 SKELETON - Tests designed but not implemented
// DEPENDS ON: _IOC_SrvProtoTCP.c implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <chrono>
#include <thread>

#include "_UT_IOC_Common.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief InValidFunc-Fault-TCP Tests: Verify graceful error handling and recovery under TCP network faults.
 *
 *-------------------------------------------------------------------------------------------------
 * @category InValidFunc-Fault-TCP (TCP Network Failures & Error Recovery - External Faults)
 *
 * Part of Test Design Formula:
 *   Service's Functional Test = ValidFunc(Typical + Edge) + InValidFunc(Misuse + Fault)
 *                                                                             ^^^^^
 *                                                                   (TCP network FAULTS!)
 *
 * InValidFunc-Fault = External failures beyond user control (system/network errors)
 *  - Distinguished from Misuse (wrong usage patterns by programmer)
 *  - Fault: Network failures, system resource exhaustion, external disruptions
 *  - Focus: Graceful degradation, error detection, recovery mechanisms
 *
 * This file covers: TCP-specific network failures and error recovery scenarios
 *  - Network connectivity faults (connection refused, host unreachable, timeout)
 *  - TCP connection failures (broken pipe, RST, FIN)
 *  - Network quality degradation (packet loss, high latency, jitter)
 *  - System resource exhaustion (out of FDs, memory allocation failure)
 *  - Partial failures (incomplete send/recv, connection drops mid-transfer)
 *  - Recovery patterns (reconnection, link health monitoring, timeout handling)
 *
 * TCP Protocol Fault Categories (Beyond FIFO):
 *  - Connection establishment faults (refused, timeout, host unreachable)
 *  - Connection disruption (broken pipe, peer reset, network partition)
 *  - Data transfer faults (partial send/recv, buffer overflow, timeout)
 *  - System resource faults (FD limit, memory exhaustion, port exhaustion)
 *  - Network quality faults (high latency, packet loss, congestion)
 *  - Platform-specific faults (SIGPIPE, ECONNRESET, ETIMEDOUT)
 *
 * Test Philosophy - KEY DISTINCTION:
 *  - Misuse: Wrong API usage by programmer (double online, wrong sequence)
 *  - Fault: External failures beyond programmer control (network down, FD limit)
 *  - Focus: Verify TCP APIs handle external faults gracefully with clear diagnostics
 *  - System should detect faults promptly, report clearly, maintain integrity
 *
 * Related Test Files:
 *  - UT_ServiceTypicalTCP.cxx: ValidFunc-Typical with TCP (normal network scenarios)
 *  - UT_ServiceEdgeTCP.cxx: ValidFunc-Edge with TCP (edge cases)
 *  - UT_ServiceMisuseTCP.cxx: InValidFunc-Misuse with TCP (wrong usage)
 *  - UT_ServiceFault.cxx: InValidFunc-Fault with FIFO (general fault patterns)
 *
 *-------------------------------------------------------------------------------------------------
 * @note TCP Protocol Implementation Status
 *     ⚠️ TCP Protocol is PLANNED but NOT YET IMPLEMENTED
 *     Current Status: 🚧 Planning Phase
 *     Required Implementation:
 *         - Source/_IOC_SrvProtoTCP.c: TCP protocol with fault handling
 *         - Network error detection (ECONNREFUSED, ETIMEDOUT, EHOSTUNREACH)
 *         - Connection health monitoring (keep-alive, broken pipe detection)
 *         - Graceful error recovery (cleanup on failures, reconnection support)
 *         - Timeout enforcement (connect, accept, send, recv timeouts)
 *         - Signal handling (SIGPIPE suppression with MSG_NOSIGNAL)
 *     Until TCP protocol is implemented, these tests will SKIP or FAIL gracefully.
 *
 * @note Fault Injection Methods
 *     Testing network faults requires simulation techniques:
 *     - Connection refused: Connect to port with no listener
 *     - Timeout: Connect to unreachable host (10.255.255.1)
 *     - Broken pipe: Close peer socket, then send data
 *     - FD exhaustion: Open file descriptors until limit, then attempt online
 *     - Network latency: Use tc (traffic control) or netem on Linux
 *     - Packet loss: Use iptables DROP rules or network simulator
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING DESIGN==============================================================

/**
 * 📋 TEST CASE DESIGN ASPECTS/CATEGORIES
 *
 * DESIGN PRINCIPLE: IMPROVE VALUE • AVOID LOSS • BALANCE SKILL vs COST
 *
 * PRIORITY FRAMEWORK:
 *   P1 🥇 FUNCTIONAL:     ValidFunc(Typical + Edge) + InvalidFunc(Misuse + Fault)
 *                                                                       ^^^^^
 *                                                            (We are here - Fault for TCP)
 *
 * FAULT CATEGORIZATION FOR TCP
 *
 * What makes a TCP test InValidFunc-Fault?
 *  ✓ External failures beyond programmer control
 *  ✓ Network/system errors that can occur in production
 *  ✓ Tests resilience, error detection, and recovery mechanisms
 *  ✓ System should fail gracefully with clear diagnostic codes
 *
 * TCP-Specific Fault Categories:
 *  1. Connection Establishment Faults: Refused, timeout, unreachable
 *  2. Connection Disruption Faults: Broken pipe, peer reset, network partition
 *  3. Data Transfer Faults: Partial send/recv, timeout, buffer overflow
 *  4. System Resource Faults: FD exhaustion, memory failure, port exhaustion
 *  5. Network Quality Faults: High latency, packet loss, jitter
 *  6. Platform-Specific Faults: SIGPIPE, ECONNRESET, SO_ERROR handling
 *
 * COVERAGE STRATEGY: TCP Fault Dimensions
 * ┌──────────────────────────┬──────────────────────────┬──────────────────────────┬──────────────────────┐
 * │ Fault Category           │ Fault Injection Method   │ Operation                │ Expected Behavior    │
 * ├──────────────────────────┼──────────────────────────┼──────────────────────────┼──────────────────────┤
 * │ Connection Refused       │ No listener on port      │ Connect                  │ REFUSED or TIMEOUT   │
 * │ Connection Timeout       │ Unreachable IP          │ Connect with timeout     │ TIMEOUT after delay  │
 * │ Host Unreachable         │ Invalid route (10.x.x.x) │ Connect                  │ UNREACHABLE          │
 * │ Broken Pipe (SIGPIPE)    │ Close peer, then send    │ Send data                │ LINK_BROKEN (no SIG) │
 * │ Connection Reset (RST)   │ Peer sends RST           │ Send/Recv                │ LINK_BROKEN          │
 * │ Partial Send             │ Slow receiver, full buf  │ Send large data          │ Retry or TIMEOUT     │
 * │ Partial Recv             │ Slow sender, small buf   │ Receive data             │ Buffering or TIMEOUT │
 * │ FD Exhaustion            │ ulimit -n reached        │ Online service           │ RESOURCE_UNAVAILABLE │
 * │ Port Exhaustion          │ All ephemeral ports used │ Connect                  │ PORT_UNAVAILABLE     │
 * │ Memory Exhaustion        │ malloc fails             │ Online/Connect           │ NO_MEMORY, cleanup   │
 * │ High Latency             │ tc qdisc delay           │ Any operation            │ Slower but success   │
 * │ Packet Loss              │ iptables DROP            │ Data transfer            │ Retries, eventual OK │
 * │ Network Partition        │ Disconnect interface     │ Any operation            │ TIMEOUT or UNAVAIL   │
 * └──────────────────────────┴──────────────────────────┴──────────────────────────┴──────────────────────┘
 */

//-------------------------------------------------------------------------------------------------
/**
 * @brief 【User Story】
 *
 *  US-1: AS a TCP network application developer,
 *      I WANT my application to handle connection establishment failures gracefully,
 *      SO THAT connection refused, timeout, and unreachable errors are detected promptly,
 *          AND users get clear error messages about network availability,
 *          AND my application can implement retry logic.
 *
 *  US-2: AS a TCP data transfer developer,
 *      I WANT my application to detect broken connections immediately,
 *      SO THAT broken pipe (SIGPIPE), RST, and FIN are handled without crashes,
 *          AND operations return LINK_BROKEN without waiting for timeout,
 *          AND application can initiate reconnection promptly.
 *
 *  US-3: AS a TCP network application operator,
 *      I WANT partial data transfers to be handled correctly,
 *      SO THAT slow receivers don't cause data loss,
 *          AND TCP flow control prevents buffer overflow,
 *          AND large transfers complete reliably despite network conditions.
 *
 *  US-4: AS a system administrator,
 *      I WANT TCP service to handle resource exhaustion gracefully,
 *      SO THAT FD limit or memory allocation failures don't crash the service,
 *          AND clear error codes indicate resource shortage,
 *          AND existing connections remain stable during resource pressure.
 *
 *  US-5: AS a TCP performance engineer,
 *      I WANT application to work correctly under degraded network quality,
 *      SO THAT high latency, packet loss, or jitter don't cause silent failures,
 *          AND operations complete with retries when network recovers,
 *          AND timeouts are enforced despite poor network conditions.
 *
 *  US-6: AS a TCP security engineer,
 *      I WANT connection attempts to potentially malicious hosts to timeout safely,
 *      SO THAT SYN flood or slowloris attacks don't hang the application,
 *          AND timeout enforcement prevents resource exhaustion,
 *          AND failed attempts are logged for security monitoring.
 *
 *  US-7: AS a TCP service maintainer,
 *      I WANT connection health monitoring to detect dead connections,
 *      SO THAT TCP keep-alive or application-level pings detect silent failures,
 *          AND stale connections are cleaned up automatically,
 *          AND resources are freed for healthy connections.
 */

//-------------------------------------------------------------------------------------------------
/**
 * @brief 【Acceptance Criteria】
 *
 * [@US-1] Connection establishment faults
 *      AC-1: GIVEN TCP service is not listening on port 8300,
 *          WHEN client attempts IOC_connectService to that port,
 *          THEN function returns IOC_RESULT_REFUSED or IOC_RESULT_NOT_EXIST_SERVICE,
 *              AND error is detected quickly (< connect timeout),
 *              AND no partial connection or dangling socket remains.
 *
 *      AC-2: GIVEN unreachable IP address (e.g., 10.255.255.1) with no route,
 *          WHEN client attempts IOC_connectService with 500ms timeout,
 *          THEN function returns IOC_RESULT_TIMEOUT or IOC_RESULT_UNREACHABLE after ~500ms,
 *              AND operation doesn't hang beyond specified timeout,
 *              AND socket is cleaned up properly.
 *
 *      AC-3: GIVEN firewall blocking TCP port 8301 (or host unreachable),
 *          WHEN client attempts IOC_connectService,
 *          THEN function returns IOC_RESULT_UNREACHABLE or IOC_RESULT_TIMEOUT,
 *              AND appropriate errno is logged (EHOSTUNREACH, ENETUNREACH),
 *              AND application can distinguish refusal from unreachability.
 *
 * [@US-2] Connection disruption faults (broken connections)
 *      AC-1: GIVEN established TCP link between client and server,
 *          WHEN peer abruptly closes socket (sends FIN or RST),
 *              AND local side attempts IOC_postEVT or IOC_sendDAT,
 *          THEN function returns IOC_RESULT_LINK_BROKEN,
 *              AND error is detected on first send attempt (not timeout),
 *              AND no SIGPIPE signal is raised (handled with MSG_NOSIGNAL or SIG_IGN).
 *
 *      AC-2: GIVEN TCP link established,
 *          WHEN network partition occurs (cable unplugged, Wi-Fi disconnects),
 *              AND peer cannot send RST/FIN,
 *              AND local side attempts send with timeout,
 *          THEN function returns IOC_RESULT_TIMEOUT after specified timeout,
 *              AND keep-alive (if enabled) eventually detects dead connection,
 *              AND link is marked as broken for subsequent operations.
 *
 *      AC-3: GIVEN TCP link broken mid-transfer (connection reset by peer),
 *          WHEN IOC_sendDAT is sending large payload,
 *          THEN partial data sent before break is detected,
 *              AND function returns IOC_RESULT_LINK_BROKEN with bytes_sent count,
 *              AND remaining data is not sent,
 *              AND application can retry or abort transfer.
 *
 * [@US-3] Partial data transfer faults
 *      AC-1: GIVEN TCP sender sending large data (1MB),
 *          WHEN receiver is slow and TCP send buffer fills up,
 *          THEN send operation blocks or returns partial write,
 *              AND TCP flow control prevents buffer overflow,
 *              AND eventually all data is sent when receiver catches up (no data loss).
 *
 *      AC-2: GIVEN TCP receiver with small application buffer,
 *          WHEN sender sends large data rapidly,
 *          THEN TCP buffers data, receiver reads in chunks,
 *              AND no data is lost due to small application buffer,
 *              AND application can reassemble complete message.
 *
 *      AC-3: GIVEN slow network with high latency (500ms RTT),
 *          WHEN sending small messages over TCP,
 *          THEN operations complete successfully but slower than LAN,
 *              AND timeout values must account for RTT (timeout > 2*RTT),
 *              AND correctness is maintained despite latency.
 *
 * [@US-4] System resource exhaustion faults
 *      AC-1: GIVEN system FD limit reached (ulimit -n),
 *          WHEN attempting IOC_onlineService (needs socket FD),
 *          THEN function returns IOC_RESULT_RESOURCE_UNAVAILABLE or IOC_RESULT_NO_FD,
 *              AND error is logged with errno (EMFILE or ENFILE),
 *              AND no partial service object is leaked,
 *              AND existing services remain functional.
 *
 *      AC-2: GIVEN system memory exhausted (malloc fails during connection setup),
 *          WHEN IOC_connectService attempts to allocate connection object,
 *          THEN function returns IOC_RESULT_NO_MEMORY,
 *              AND socket FD is closed to prevent leak,
 *              AND existing connections remain stable.
 *
 *      AC-3: GIVEN all ephemeral ports exhausted (many outgoing connections),
 *          WHEN attempting new IOC_connectService,
 *          THEN function returns IOC_RESULT_PORT_UNAVAILABLE or system error,
 *              AND error indicates port exhaustion,
 *              AND application can retry after some connections close.
 *
 * [@US-5] Network quality degradation faults
 *      AC-1: GIVEN network with 10% packet loss (simulated with tc or netem),
 *          WHEN transferring data over TCP link,
 *          THEN TCP retransmits lost packets automatically,
 *              AND all data eventually arrives correctly (no application-visible loss),
 *              AND transfer completes successfully but slower.
 *
 *      AC-2: GIVEN network with 500ms added latency (high latency scenario),
 *          WHEN IOC_execCMD is called with 100ms timeout,
 *          THEN function returns IOC_RESULT_TIMEOUT (timeout < RTT),
 *              WHEN called with 2000ms timeout,
 *          THEN function completes successfully despite latency,
 *              AND timeout enforcement works correctly relative to network conditions.
 *
 *      AC-3: GIVEN network with jitter (variable latency 10-500ms),
 *          WHEN continuously transferring data,
 *          THEN operations complete with variable latency,
 *              AND TCP buffering absorbs jitter,
 *              AND no operations fail due to jitter alone.
 *
 * [@US-6] Connection timeout safety (attack scenarios)
 *      AC-1: GIVEN SYN flood attack or malicious slow server,
 *          WHEN IOC_connectService is called with timeout,
 *          THEN function enforces timeout and returns IOC_RESULT_TIMEOUT,
 *              AND connection attempt is aborted,
 *              AND socket resources are freed,
 *              AND application is not vulnerable to resource exhaustion.
 *
 *      AC-2: GIVEN slowloris attack (slow send after connect),
 *          WHEN IOC_execCMD with send timeout,
 *          THEN send timeout is enforced,
 *              AND function returns IOC_RESULT_TIMEOUT,
 *              AND connection is closed,
 *              AND application resources are freed.
 *
 * [@US-7] Connection health monitoring
 *      AC-1: GIVEN TCP link idle for extended period (no data transfer),
 *          WHEN TCP keep-alive is enabled (SO_KEEPALIVE),
 *          THEN keep-alive probes detect peer alive or dead,
 *              AND dead connection is detected within keep-alive interval,
 *              AND link is marked as broken,
 *              AND application is notified of health change.
 *
 *      AC-2: GIVEN application-level ping/pong mechanism over TCP,
 *          WHEN peer stops responding to pings,
 *          THEN application detects dead connection faster than TCP keep-alive,
 *              AND link is closed and reported as broken,
 *              AND reconnection logic can be triggered.
 */

//-------------------------------------------------------------------------------------------------
/**
 * @brief 【Test Cases】
 *
 * ========================================
 * CONNECTION ESTABLISHMENT FAULTS (US-1)
 * ========================================
 *
 * [@AC-1 of US-1] Connection refused
 * TC-1:
 *  @[Name]: verifyConnectService_byConnectionRefused_expectRefused
 *  @[Category]: InValidFunc-Fault-TCP (Connection Establishment Fault)
 *  @[FaultType]: CONNECTION_REFUSED - Port not listening
 *  @[Purpose]: Verify connect fails quickly when port has no listener
 *  @[Brief]: Attempt connect to port with no service, verify REFUSED or NOT_EXIST_SERVICE
 *  @[Steps]:
 *      🔧 SETUP: Choose port with no listener (e.g., 8300), prepare connect args
 *      🎯 BEHAVIOR: Call IOC_connectService with timeout=500ms
 *      ✅ VERIFY: Returns REFUSED or NOT_EXIST_SERVICE within timeout, socket cleaned up
 *      🧹 CLEANUP: None needed (connection failed)
 *  @[Status]: ⚪ TODO - Core connection fault test
 *  @[Notes]: On localhost, refused detected immediately. Remote may take longer.
 *
 * [@AC-2 of US-1] Connection timeout (unreachable IP)
 * TC-2:
 *  @[Name]: verifyConnectService_byUnreachableIP_expectTimeout
 *  @[Category]: InValidFunc-Fault-TCP (Connection Timeout Fault)
 *  @[FaultType]: CONNECTION_TIMEOUT - Host unreachable
 *  @[Purpose]: Verify connect timeout enforced for unreachable hosts
 *  @[Brief]: Connect to 10.255.255.1 (RFC5737 TEST-NET), verify timeout after 500ms
 *  @[Steps]:
 *      🔧 SETUP: Prepare URI with unreachable IP (10.255.255.1), timeout=500ms
 *      🎯 BEHAVIOR: Call IOC_connectService, measure elapsed time
 *      ✅ VERIFY: Returns TIMEOUT after ~500ms (±100ms), socket cleaned up
 *      🧹 CLEANUP: None needed
 *  @[Status]: ⚪ TODO - Timeout enforcement test
 *  @[Notes]: Use TEST-NET IP to avoid actually trying to reach external hosts
 *
 * [@AC-3 of US-1] Host unreachable (EHOSTUNREACH)
 * TC-3:
 *  @[Name]: verifyConnectService_byFirewalledHost_expectUnreachable
 *  @[Category]: InValidFunc-Fault-TCP (Host Unreachable Fault)
 *  @[FaultType]: HOST_UNREACHABLE - Network route blocked
 *  @[Purpose]: Verify UNREACHABLE error when host is blocked by firewall/routing
 *  @[Brief]: Connect to localhost with iptables DROP rule, verify UNREACHABLE or TIMEOUT
 *  @[Steps]:
 *      🔧 SETUP: Add iptables rule to DROP port 8301 (requires root), prepare connect
 *      🎯 BEHAVIOR: Call IOC_connectService with timeout=500ms
 *      ✅ VERIFY: Returns UNREACHABLE or TIMEOUT, errno logged (EHOSTUNREACH)
 *      🧹 CLEANUP: Remove iptables rule
 *  @[Status]: ⚪ TODO - Requires root/sudo for iptables, may skip on CI
 *  @[Notes]: Platform-dependent. May need to simulate with closed port instead.
 *
 * ========================================
 * CONNECTION DISRUPTION FAULTS (US-2)
 * ========================================
 *
 * [@AC-1 of US-2] Broken pipe (SIGPIPE)
 * TC-4:
 *  @[Name]: verifyPostEVT_byBrokenPipe_expectLinkBrokenNoSIGPIPE
 *  @[Category]: InValidFunc-Fault-TCP (Broken Pipe Fault)
 *  @[FaultType]: BROKEN_PIPE - Peer closed connection, then send
 *  @[Purpose]: Verify SIGPIPE is handled and LINK_BROKEN returned
 *  @[Brief]: Establish link, peer closes socket, send data, verify LINK_BROKEN (no signal)
 *  @[Steps]:
 *      🔧 SETUP: Establish TCP link between client and server
 *      🎯 BEHAVIOR: Server closes socket, client sends data (FAULT - broken pipe)
 *      ✅ VERIFY: Returns LINK_BROKEN, no SIGPIPE signal raised, errno=EPIPE
 *      🧹 CLEANUP: Close remaining resources
 *  @[Status]: ⚪ TODO - CRITICAL for TCP, must handle SIGPIPE with MSG_NOSIGNAL or SIG_IGN
 *  @[Notes]: SIGPIPE can crash process if not handled. Use MSG_NOSIGNAL on send().
 *
 * [@AC-2 of US-2] Network partition (silent failure)
 * TC-5:
 *  @[Name]: verifyPostEVT_byNetworkPartition_expectTimeoutThenBroken
 *  @[Category]: InValidFunc-Fault-TCP (Network Partition Fault)
 *  @[FaultType]: NETWORK_PARTITION - Connection lost without RST/FIN
 *  @[Purpose]: Verify timeout detection when network partition occurs
 *  @[Brief]: Establish link, simulate partition (no RST), send with timeout, verify TIMEOUT
 *  @[Steps]:
 *      🔧 SETUP: Establish TCP link, prepare to block all packets (iptables DROP or similar)
 *      🎯 BEHAVIOR: Partition network, attempt IOC_postEVT with timeout=500ms
 *      ✅ VERIFY: Returns TIMEOUT after ~500ms, eventually detected as broken
 *      🧹 CLEANUP: Restore network, close resources
 *  @[Status]: ⚪ TODO - Complex simulation, requires iptables or network namespace
 *  @[Notes]: TCP won't detect partition without keep-alive or timeout on send/recv
 *
 * [@AC-3 of US-2] Connection reset mid-transfer (RST)
 * TC-6:
 *  @[Name]: verifyPostEVT_byConnectionReset_expectLinkBrokenMidTransfer
 *  @[Category]: InValidFunc-Fault-TCP (Connection Reset Fault)
 *  @[FaultType]: CONNECTION_RESET - Peer sends RST during transfer
 *  @[Purpose]: Verify partial transfer detection when connection reset occurs
 *  @[Brief]: Start large data transfer, peer sends RST, verify LINK_BROKEN with partial count
 *  @[Steps]:
 *      🔧 SETUP: Establish link, prepare large payload (1MB)
 *      🎯 BEHAVIOR: Start send, peer sends RST mid-transfer (FAULT)
 *      ✅ VERIFY: Returns LINK_BROKEN, partial bytes_sent reported, errno=ECONNRESET
 *      🧹 CLEANUP: Close resources
 *  @[Status]: ⚪ TODO - Requires peer to send RST at specific time
 *  @[Notes]: Can simulate by calling shutdown(fd, SHUT_RDWR) and close() on peer
 *
 * ========================================
 * PARTIAL DATA TRANSFER FAULTS (US-3)
 * ========================================
 *
 * [@AC-1 of US-3] Slow receiver, full send buffer
 * TC-7:
 *  @[Name]: verifyPostEVT_bySlowReceiver_expectBlockOrPartialSend
 *  @[Category]: InValidFunc-Fault-TCP (Slow Receiver Fault)
 *  @[FaultType]: SLOW_RECEIVER - TCP flow control activates
 *  @[Purpose]: Verify TCP flow control prevents data loss with slow receiver
 *  @[Brief]: Sender sends large data rapidly, receiver reads slowly, verify all data arrives
 *  @[Steps]:
 *      🔧 SETUP: Establish link, receiver reads slowly (10KB/sec), sender sends 100KB
 *      🎯 BEHAVIOR: Sender sends rapidly, TCP buffers fill, flow control activates
 *      ✅ VERIFY: All 100KB eventually received, no data loss, sender may block or timeout
 *      🧹 CLEANUP: Close link
 *  @[Status]: ⚪ TODO - Tests TCP flow control and backpressure
 *  @[Notes]: Sender may experience send() blocking or partial writes, this is normal
 *
 * [@AC-2 of US-3] Small receive buffer
 * TC-8:
 *  @[Name]: verifyPostEVT_bySmallRecvBuffer_expectChunkedReceive
 *  @[Category]: InValidFunc-Fault-TCP (Small Buffer Fault)
 *  @[FaultType]: SMALL_BUFFER - Application buffer smaller than data
 *  @[Purpose]: Verify TCP streams data correctly despite small application buffer
 *  @[Brief]: Send 100KB data, receiver has 1KB buffer, verify complete delivery in chunks
 *  @[Steps]:
 *      🔧 SETUP: Establish link, receiver allocates 1KB buffer, sender sends 100KB
 *      🎯 BEHAVIOR: Receiver reads in 1KB chunks until all data received
 *      ✅ VERIFY: All 100KB received correctly, data integrity maintained
 *      🧹 CLEANUP: Close link
 *  @[Status]: ⚪ TODO - Tests TCP stream semantics
 *
 * [@AC-3 of US-3] High latency network
 * TC-9:
 *  @[Name]: verifyPostEVT_byHighLatency500ms_expectSlowerButSuccess
 *  @[Category]: InValidFunc-Fault-TCP (High Latency Fault)
 *  @[FaultType]: HIGH_LATENCY - 500ms RTT added to network
 *  @[Purpose]: Verify operations complete correctly under high latency
 *  @[Brief]: Add 500ms latency with tc/netem, send data, verify completion with adjusted timeout
 *  @[Steps]:
 *      🔧 SETUP: Add 500ms latency (tc qdisc add dev lo root netem delay 500ms)
 *      🎯 BEHAVIOR: Send data over TCP link, use timeout=2000ms
 *      ✅ VERIFY: Data arrives correctly, operation slower, no failures due to latency
 *      🧹 CLEANUP: Remove tc rule (tc qdisc del dev lo root)
 *  @[Status]: ⚪ TODO - Requires Linux tc/netem, skip on non-Linux platforms
 *  @[Notes]: Requires root/sudo. Can skip if not available, document requirement.
 *
 * ========================================
 * SYSTEM RESOURCE EXHAUSTION FAULTS (US-4)
 * ========================================
 *
 * [@AC-1 of US-4] FD limit exhaustion
 * TC-10:
 *  @[Name]: verifyOnlineService_byFDLimitReached_expectResourceUnavailable
 *  @[Category]: InValidFunc-Fault-TCP (FD Exhaustion Fault)
 *  @[FaultType]: FD_EXHAUSTION - ulimit -n reached
 *  @[Purpose]: Verify online service fails gracefully when FD limit reached
 *  @[Brief]: Open FDs until limit, attempt online service, verify RESOURCE_UNAVAILABLE
 *  @[Steps]:
 *      🔧 SETUP: Query ulimit -n, open dummy FDs until limit-1
 *      🎯 BEHAVIOR: Attempt IOC_onlineService (FAULT - no FDs available)
 *      ✅ VERIFY: Returns RESOURCE_UNAVAILABLE, errno=EMFILE/ENFILE, no leak
 *      🧹 CLEANUP: Close dummy FDs
 *  @[Status]: ⚪ TODO - Requires FD management, may be fragile in CI
 *  @[Notes]: Test should not affect other processes, use prlimit if available
 *
 * [@AC-2 of US-4] Memory allocation failure
 * TC-11:
 *  @[Name]: verifyConnectService_byMallocFail_expectNoMemory
 *  @[Category]: InValidFunc-Fault-TCP (Memory Exhaustion Fault)
 *  @[FaultType]: MALLOC_FAILURE - Memory allocation fails during connect
 *  @[Purpose]: Verify socket FD cleaned up when malloc fails during connection setup
 *  @[Brief]: Inject malloc failure, attempt connect, verify NO_MEMORY and FD cleanup
 *  @[Steps]:
 *      🔧 SETUP: Setup malloc failure injection (test hook or LD_PRELOAD)
 *      🎯 BEHAVIOR: Attempt IOC_connectService, malloc fails (FAULT)
 *      ✅ VERIFY: Returns NO_MEMORY, socket FD closed (no leak), other connections stable
 *      🧹 CLEANUP: Disable malloc failure injection
 *  @[Status]: ⚪ TODO - Requires fault injection mechanism (test hook in IOC code)
 *  @[Notes]: Similar to existing IOC_test_setFailNextAlloc pattern for FIFO tests
 *
 * [@AC-3 of US-4] Ephemeral port exhaustion
 * TC-12:
 *  @[Name]: verifyConnectService_byPortExhaustion_expectPortUnavailable
 *  @[Category]: InValidFunc-Fault-TCP (Port Exhaustion Fault)
 *  @[FaultType]: PORT_EXHAUSTION - All ephemeral ports used
 *  @[Purpose]: Verify connect fails gracefully when no ephemeral ports available
 *  @[Brief]: Create many outgoing connections until ports exhausted, verify error
 *  @[Steps]:
 *      🔧 SETUP: Create thousands of outgoing connections to exhaust ephemeral port range
 *      🎯 BEHAVIOR: Attempt one more IOC_connectService (FAULT - no ports)
 *      ✅ VERIFY: Returns PORT_UNAVAILABLE or RESOURCE_UNAVAILABLE, errno=EADDRNOTAVAIL
 *      🧹 CLEANUP: Close all connections
 *  @[Status]: ⚪ TODO - Very heavy test, may skip in CI, takes long time
 *  @[Notes]: Ephemeral range typically 32768-60999 (~28K ports), requires many connections
 *
 * ========================================
 * NETWORK QUALITY DEGRADATION FAULTS (US-5)
 * ========================================
 *
 * [@AC-1 of US-5] Packet loss (10%)
 * TC-13:
 *  @[Name]: verifyDataTransfer_by10PercentPacketLoss_expectRetryAndSuccess
 *  @[Category]: InValidFunc-Fault-TCP (Packet Loss Fault)
 *  @[FaultType]: PACKET_LOSS - 10% loss rate simulated
 *  @[Purpose]: Verify TCP retransmits lost packets and data arrives correctly
 *  @[Brief]: Add 10% packet loss with tc/netem, transfer data, verify complete delivery
 *  @[Steps]:
 *      🔧 SETUP: Add packet loss (tc qdisc add dev lo root netem loss 10%)
 *      🎯 BEHAVIOR: Transfer 1MB data over TCP link
 *      ✅ VERIFY: All data arrives correctly (TCP retransmits), transfer slower but complete
 *      🧹 CLEANUP: Remove tc rule
 *  @[Status]: ⚪ TODO - Requires Linux tc/netem and root privileges
 *  @[Notes]: TCP should handle retransmission transparently, application sees no loss
 *
 * [@AC-2 of US-5] Timeout under high latency
 * TC-14:
 *  @[Name]: verifyExecCMD_byHighLatencyShortTimeout_expectTimeout
 *  @[Category]: InValidFunc-Fault-TCP (High Latency + Timeout Fault)
 *  @[FaultType]: LATENCY_TIMEOUT - Timeout < RTT
 *  @[Purpose]: Verify timeout enforcement when timeout is less than network RTT
 *  @[Brief]: Add 500ms latency, execCMD with 100ms timeout, verify TIMEOUT
 *  @[Steps]:
 *      🔧 SETUP: Add 500ms latency, establish link, prepare command
 *      🎯 BEHAVIOR: IOC_execCMD with timeout=100ms (FAULT - timeout < RTT)
 *      ✅ VERIFY: Returns TIMEOUT after ~100ms, operation aborted
 *      🧹 CLEANUP: Remove tc rule, close link
 *  @[Status]: ⚪ TODO - Tests timeout enforcement vs network latency
 *
 * [@AC-3 of US-5] Jitter (variable latency)
 * TC-15:
 *  @[Name]: verifyDataTransfer_byJitter_expectVariableLatencyButSuccess
 *  @[Category]: InValidFunc-Fault-TCP (Jitter Fault)
 *  @[FaultType]: JITTER - Variable latency 10-500ms
 *  @[Purpose]: Verify TCP handles jitter gracefully, buffering absorbs variation
 *  @[Brief]: Add jitter with tc/netem, transfer data, verify completion despite variance
 *  @[Steps]:
 *      🔧 SETUP: Add jitter (tc qdisc add dev lo root netem delay 200ms 150ms)
 *      🎯 BEHAVIOR: Continuously transfer small messages over 10 seconds
 *      ✅ VERIFY: All messages arrive, latency varies 10-500ms, no failures due to jitter
 *      🧹 CLEANUP: Remove tc rule
 *  @[Status]: ⚪ TODO - Requires Linux tc/netem
 *
 * ========================================
 * CONNECTION TIMEOUT SAFETY (US-6)
 * ========================================
 *
 * [@AC-1 of US-6] SYN flood defense (timeout enforcement)
 * TC-16:
 *  @[Name]: verifyConnectService_bySYNFlood_expectTimeoutNoResourceExhaustion
 *  @[Category]: InValidFunc-Fault-TCP (Attack Defense - SYN Flood)
 *  @[FaultType]: SYN_FLOOD - Malicious slow server or SYN flood
 *  @[Purpose]: Verify connect timeout prevents resource exhaustion from SYN flood
 *  @[Brief]: Server doesn't complete handshake, connect with timeout, verify TIMEOUT and cleanup
 *  @[Steps]:
 *      🔧 SETUP: Create malicious server that accepts SYN but never sends SYN-ACK
 *      🎯 BEHAVIOR: IOC_connectService with timeout=500ms (FAULT - no handshake completion)
 *      ✅ VERIFY: Returns TIMEOUT, socket cleaned up, no resource leak
 *      🧹 CLEANUP: Stop malicious server
 *  @[Status]: ⚪ TODO - Requires custom test server, complex simulation
 *  @[Notes]: Tests defense against slowloris/SYN flood attacks
 *
 * [@AC-2 of US-6] Slowloris attack (slow send timeout)
 * TC-17:
 *  @[Name]: verifyExecCMD_bySlowloris_expectSendTimeoutAndCleanup
 *  @[Category]: InValidFunc-Fault-TCP (Attack Defense - Slowloris)
 *  @[FaultType]: SLOWLORIS - Slow send after connect
 *  @[Purpose]: Verify send timeout prevents resource exhaustion from slow send attacks
 *  @[Brief]: Server sends data very slowly, execCMD with timeout, verify TIMEOUT
 *  @[Steps]:
 *      🔧 SETUP: Create server that sends 1 byte every 500ms
 *      🎯 BEHAVIOR: IOC_execCMD with timeout=1000ms (FAULT - server too slow)
 *      ✅ VERIFY: Returns TIMEOUT, connection closed, resources freed
 *      🧹 CLEANUP: Stop slow server
 *  @[Status]: ⚪ TODO - Requires custom slow test server
 *
 * ========================================
 * CONNECTION HEALTH MONITORING (US-7)
 * ========================================
 *
 * [@AC-1 of US-7] TCP keep-alive detection
 * TC-18:
 *  @[Name]: verifyLinkHealth_byTCPKeepalive_expectDeadDetection
 *  @[Category]: InValidFunc-Fault-TCP (Connection Health Monitoring)
 *  @[FaultType]: DEAD_CONNECTION - Idle connection, peer dead
 *  @[Purpose]: Verify TCP keep-alive probes detect dead connections
 *  @[Brief]: Enable SO_KEEPALIVE, establish link, kill peer, verify dead detected
 *  @[Steps]:
 *      🔧 SETUP: Enable SO_KEEPALIVE with short intervals, establish link
 *      🎯 BEHAVIOR: Peer crashes/killed, keep-alive probes sent (FAULT - peer dead)
 *      ✅ VERIFY: Dead connection detected within keep-alive interval, link marked broken
 *      🧹 CLEANUP: Close resources
 *  @[Status]: ⚪ TODO - Requires SO_KEEPALIVE configuration and peer simulation
 *  @[Notes]: Keep-alive intervals are platform-dependent (Linux: TCP_KEEPIDLE, etc.)
 *
 * [@AC-2 of US-7] Application-level ping/pong
 * TC-19:
 *  @[Name]: verifyLinkHealth_byAppPingPong_expectFasterDeadDetection
 *  @[Category]: InValidFunc-Fault-TCP (Application Health Check)
 *  @[FaultType]: DEAD_CONNECTION - Application-level health check
 *  @[Purpose]: Verify application-level pings detect dead connections faster than TCP keep-alive
 *  @[Brief]: Send ping every 100ms, peer stops responding, verify dead detected < 500ms
 *  @[Steps]:
 *      🔧 SETUP: Establish link, start application ping thread (100ms interval)
 *      🎯 BEHAVIOR: Peer stops responding to pings (FAULT)
 *      ✅ VERIFY: Dead detected after 3 missed pings (< 500ms), faster than TCP keep-alive
 *      🧹 CLEANUP: Stop ping thread, close resources
 *  @[Status]: ⚪ TODO - Requires application-level ping/pong protocol implementation
 */
//======>END OF UNIT TESTING DESIGN================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF UNIT TESTING IMPLEMENTATION=======================================================

// Test implementation placeholders following CaTDD TDD Red→Green methodology
// All tests marked with ⚪ TODO until TCP protocol is implemented in _IOC_SrvProtoTCP.c

//=== CONNECTION ESTABLISHMENT FAULTS ===
TEST(UT_ServiceFaultTCP, verifyConnectService_byConnectionRefused_expectRefused) {
    GTEST_SKIP() << "⚠️ TCP protocol not yet implemented - requires _IOC_SrvProtoTCP.c";
    // SETUP: Port with no listener (8300)
    // BEHAVIOR: Connect with timeout=500ms (FAULT - refused)
    // VERIFY: Returns REFUSED or NOT_EXIST_SERVICE, socket cleaned
    // CLEANUP: None needed
}

TEST(UT_ServiceFaultTCP, verifyConnectService_byUnreachableIP_expectTimeout) {
    GTEST_SKIP() << "⚠️ TCP protocol not yet implemented - requires _IOC_SrvProtoTCP.c";
    // SETUP: URI with unreachable IP (10.255.255.1), timeout=500ms
    // BEHAVIOR: Connect (FAULT - unreachable)
    // VERIFY: Returns TIMEOUT after ~500ms
    // CLEANUP: None needed
}

TEST(UT_ServiceFaultTCP, verifyConnectService_byFirewalledHost_expectUnreachable) {
    GTEST_SKIP() << "⚠️ TCP protocol not yet implemented - Requires iptables (root)";
    // SETUP: Add iptables DROP rule port 8301
    // BEHAVIOR: Connect (FAULT - firewalled)
    // VERIFY: Returns UNREACHABLE or TIMEOUT
    // CLEANUP: Remove iptables rule
}

//=== CONNECTION DISRUPTION FAULTS ===
TEST(UT_ServiceFaultTCP, verifyPostEVT_byBrokenPipe_expectLinkBrokenNoSIGPIPE) {
    GTEST_SKIP() << "⚠️ TCP protocol not yet implemented - CRITICAL SIGPIPE handling";
    // SETUP: Establish TCP link
    // BEHAVIOR: Peer closes, send data (FAULT - broken pipe)
    // VERIFY: Returns LINK_BROKEN, no SIGPIPE signal
    // CLEANUP: Close resources
}

TEST(UT_ServiceFaultTCP, verifyPostEVT_byNetworkPartition_expectTimeoutThenBroken) {
    GTEST_SKIP() << "⚠️ TCP protocol not yet implemented - Requires iptables (root)";
    // SETUP: Establish link, prepare iptables DROP
    // BEHAVIOR: Partition network, send with timeout (FAULT)
    // VERIFY: Returns TIMEOUT, eventually broken
    // CLEANUP: Restore network
}

TEST(UT_ServiceFaultTCP, verifyPostEVT_byConnectionReset_expectLinkBrokenMidTransfer) {
    GTEST_SKIP() << "⚠️ TCP protocol not yet implemented - requires _IOC_SrvProtoTCP.c";
    // SETUP: Establish link, prepare large payload
    // BEHAVIOR: Start send, peer sends RST (FAULT)
    // VERIFY: Returns LINK_BROKEN, partial bytes_sent
    // CLEANUP: Close resources
}

//=== PARTIAL DATA TRANSFER FAULTS ===
TEST(UT_ServiceFaultTCP, verifyPostEVT_bySlowReceiver_expectBlockOrPartialSend) {
    GTEST_SKIP() << "⚠️ TCP protocol not yet implemented - requires _IOC_SrvProtoTCP.c";
    // SETUP: Establish link, receiver reads slowly
    // BEHAVIOR: Sender sends 100KB rapidly (FAULT - buffer fills)
    // VERIFY: All data arrives, no loss, TCP flow control works
    // CLEANUP: Close link
}

TEST(UT_ServiceFaultTCP, verifyPostEVT_bySmallRecvBuffer_expectChunkedReceive) {
    GTEST_SKIP() << "⚠️ TCP protocol not yet implemented - requires _IOC_SrvProtoTCP.c";
    // SETUP: Receiver 1KB buffer, sender 100KB
    // BEHAVIOR: Receiver reads in chunks
    // VERIFY: All 100KB received correctly
    // CLEANUP: Close link
}

TEST(UT_ServiceFaultTCP, verifyPostEVT_byHighLatency500ms_expectSlowerButSuccess) {
    GTEST_SKIP() << "⚠️ Requires Linux tc/netem and root privileges - skip on CI";
    // SETUP: Add 500ms latency (tc qdisc)
    // BEHAVIOR: Send data with timeout=2000ms
    // VERIFY: Data arrives, slower but correct
    // CLEANUP: Remove tc rule
}

//=== SYSTEM RESOURCE EXHAUSTION FAULTS ===
TEST(UT_ServiceFaultTCP, verifyOnlineService_byFDLimitReached_expectResourceUnavailable) {
    GTEST_SKIP() << "⚠️ TCP protocol not yet implemented - Requires FD management";
    // SETUP: Open FDs until limit-1
    // BEHAVIOR: Online service (FAULT - no FDs)
    // VERIFY: Returns RESOURCE_UNAVAILABLE, no leak
    // CLEANUP: Close dummy FDs
}

TEST(UT_ServiceFaultTCP, verifyConnectService_byMallocFail_expectNoMemory) {
    GTEST_SKIP() << "⚠️ TCP protocol not yet implemented - Requires fault injection";
    // SETUP: Inject malloc failure
    // BEHAVIOR: Connect (FAULT - malloc fails)
    // VERIFY: Returns NO_MEMORY, socket FD closed
    // CLEANUP: Disable injection
}

TEST(UT_ServiceFaultTCP, verifyConnectService_byPortExhaustion_expectPortUnavailable) {
    GTEST_SKIP() << "⚠️ TCP protocol not yet implemented - Very heavy test, skip in CI";
    // SETUP: Create ~28K connections to exhaust ephemeral ports
    // BEHAVIOR: One more connect (FAULT - no ports)
    // VERIFY: Returns PORT_UNAVAILABLE
    // CLEANUP: Close all connections
}

//=== NETWORK QUALITY DEGRADATION FAULTS ===
TEST(UT_ServiceFaultTCP, verifyDataTransfer_by10PercentPacketLoss_expectRetryAndSuccess) {
    GTEST_SKIP() << "⚠️ Requires Linux tc/netem and root privileges - skip on CI";
    // SETUP: Add 10% packet loss (tc qdisc)
    // BEHAVIOR: Transfer 1MB data
    // VERIFY: All data arrives (TCP retransmits)
    // CLEANUP: Remove tc rule
}

TEST(UT_ServiceFaultTCP, verifyExecCMD_byHighLatencyShortTimeout_expectTimeout) {
    GTEST_SKIP() << "⚠️ Requires Linux tc/netem and root privileges - skip on CI";
    // SETUP: Add 500ms latency
    // BEHAVIOR: execCMD with timeout=100ms (FAULT)
    // VERIFY: Returns TIMEOUT after ~100ms
    // CLEANUP: Remove tc rule
}

TEST(UT_ServiceFaultTCP, verifyDataTransfer_byJitter_expectVariableLatencyButSuccess) {
    GTEST_SKIP() << "⚠️ Requires Linux tc/netem and root privileges - skip on CI";
    // SETUP: Add jitter (delay 200ms 150ms)
    // BEHAVIOR: Transfer messages for 10s
    // VERIFY: All arrive, latency varies, no failures
    // CLEANUP: Remove tc rule
}

//=== CONNECTION TIMEOUT SAFETY ===
TEST(UT_ServiceFaultTCP, verifyConnectService_bySYNFlood_expectTimeoutNoResourceExhaustion) {
    GTEST_SKIP() << "⚠️ TCP protocol not yet implemented - Requires custom malicious server";
    // SETUP: Create server that doesn't complete handshake
    // BEHAVIOR: Connect with timeout=500ms (FAULT)
    // VERIFY: Returns TIMEOUT, socket cleaned, no leak
    // CLEANUP: Stop malicious server
}

TEST(UT_ServiceFaultTCP, verifyExecCMD_bySlowloris_expectSendTimeoutAndCleanup) {
    GTEST_SKIP() << "⚠️ TCP protocol not yet implemented - Requires custom slow server";
    // SETUP: Create server that sends 1 byte/500ms
    // BEHAVIOR: execCMD with timeout=1000ms (FAULT)
    // VERIFY: Returns TIMEOUT, connection closed
    // CLEANUP: Stop slow server
}

//=== CONNECTION HEALTH MONITORING ===
TEST(UT_ServiceFaultTCP, verifyLinkHealth_byTCPKeepalive_expectDeadDetection) {
    GTEST_SKIP() << "⚠️ TCP protocol not yet implemented - Requires SO_KEEPALIVE";
    // SETUP: Enable SO_KEEPALIVE, establish link
    // BEHAVIOR: Peer killed (FAULT)
    // VERIFY: Dead detected within keep-alive interval
    // CLEANUP: Close resources
}

TEST(UT_ServiceFaultTCP, verifyLinkHealth_byAppPingPong_expectFasterDeadDetection) {
    GTEST_SKIP() << "⚠️ TCP protocol not yet implemented - Requires ping/pong protocol";
    // SETUP: Establish link, start ping thread (100ms)
    // BEHAVIOR: Peer stops responding (FAULT)
    // VERIFY: Dead detected < 500ms (3 missed pings)
    // CLEANUP: Stop ping thread
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TODO/IMPLEMENTATION TRACKING SECTION=============================================
/**
 * 🔴 IMPLEMENTATION STATUS TRACKING - TCP Fault Tests
 *
 * STATUS LEGEND:
 *   ⚪ TODO/PLANNED:      Designed but not implemented (ALL TESTS HERE)
 *   🔴 RED/IMPLEMENTED:   Test written and failing (need TCP protocol)
 *   🟢 GREEN/PASSED:      Test written and passing
 *
 * PRIORITY LEVELS:
 *   P1 🥇 FUNCTIONAL:     ValidFunc(Typical + Edge) + InvalidFunc(Misuse + Fault)
 *                                                                       ^^^^^
 *                                                             (We are P1-Fault for TCP)
 *
 * DEPENDENCY: ALL tests depend on Source/_IOC_SrvProtoTCP.c with fault handling
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════
 * P1 🥇 FUNCTIONAL TESTING – InValidFunc-Fault-TCP (19 tests planned)
 * ═══════════════════════════════════════════════════════════════════════════════════════════
 *
 * CONNECTION ESTABLISHMENT FAULTS (3 tests) - US-1
 *   ⚪ TC-1: verifyConnectService_byConnectionRefused_expectRefused
 *   ⚪ TC-2: verifyConnectService_byUnreachableIP_expectTimeout
 *   ⚪ TC-3: verifyConnectService_byFirewalledHost_expectUnreachable (Requires root)
 *
 * CONNECTION DISRUPTION FAULTS (3 tests) - US-2
 *   ⚪ TC-4: verifyPostEVT_byBrokenPipe_expectLinkBrokenNoSIGPIPE (CRITICAL - SIGPIPE)
 *   ⚪ TC-5: verifyPostEVT_byNetworkPartition_expectTimeoutThenBroken (Requires root)
 *   ⚪ TC-6: verifyPostEVT_byConnectionReset_expectLinkBrokenMidTransfer
 *
 * PARTIAL DATA TRANSFER FAULTS (3 tests) - US-3
 *   ⚪ TC-7: verifyPostEVT_bySlowReceiver_expectBlockOrPartialSend
 *   ⚪ TC-8: verifyPostEVT_bySmallRecvBuffer_expectChunkedReceive
 *   ⚪ TC-9: verifyPostEVT_byHighLatency500ms_expectSlowerButSuccess (Requires root/tc)
 *
 * SYSTEM RESOURCE EXHAUSTION FAULTS (3 tests) - US-4
 *   ⚪ TC-10: verifyOnlineService_byFDLimitReached_expectResourceUnavailable
 *   ⚪ TC-11: verifyConnectService_byMallocFail_expectNoMemory (Requires fault injection)
 *   ⚪ TC-12: verifyConnectService_byPortExhaustion_expectPortUnavailable (Heavy, skip CI)
 *
 * NETWORK QUALITY DEGRADATION FAULTS (3 tests) - US-5
 *   ⚪ TC-13: verifyDataTransfer_by10PercentPacketLoss_expectRetryAndSuccess (Requires root/tc)
 *   ⚪ TC-14: verifyExecCMD_byHighLatencyShortTimeout_expectTimeout (Requires root/tc)
 *   ⚪ TC-15: verifyDataTransfer_byJitter_expectVariableLatencyButSuccess (Requires root/tc)
 *
 * CONNECTION TIMEOUT SAFETY (2 tests) - US-6
 *   ⚪ TC-16: verifyConnectService_bySYNFlood_expectTimeoutNoResourceExhaustion (Complex)
 *   ⚪ TC-17: verifyExecCMD_bySlowloris_expectSendTimeoutAndCleanup (Complex)
 *
 * CONNECTION HEALTH MONITORING (2 tests) - US-7
 *   ⚪ TC-18: verifyLinkHealth_byTCPKeepalive_expectDeadDetection
 *   ⚪ TC-19: verifyLinkHealth_byAppPingPong_expectFasterDeadDetection
 *
 * ═══════════════════════════════════════════════════════════════════════════════════════════
 * 🚪 GATE P1-FAULT: Completes P1 Testing for TCP
 * ═══════════════════════════════════════════════════════════════════════════════════════════
 *   ✅ All critical fault tests GREEN (TC-1, TC-2, TC-4, TC-6, TC-7, TC-8, TC-10)
 *   ✅ SIGPIPE handling working (TC-4 CRITICAL)
 *   ✅ Network error detection working (REFUSED, TIMEOUT, RESET)
 *   ✅ Resource cleanup on faults working (no leaks)
 *   ✅ TCP flow control working (TC-7, TC-8)
 *   ✅ Ready for P2 testing (State, Capability, Concurrency)
 *
 * CRITICAL TCP-SPECIFIC FAULT TESTS (Must Pass):
 *   1. TC-4: Broken pipe SIGPIPE handling (CRITICAL - can crash process)
 *   2. TC-1: Connection refused detection
 *   3. TC-2: Connection timeout enforcement
 *   4. TC-6: Connection reset mid-transfer
 *   5. TC-7: TCP flow control with slow receiver
 *   6. TC-10: FD exhaustion handling
 *
 * PLATFORM-DEPENDENT TESTS (May Skip on CI):
 *   - TC-3: Firewall (requires iptables/root)
 *   - TC-5: Network partition (requires iptables/root)
 *   - TC-9, TC-13, TC-14, TC-15: Network quality tests (require tc/netem/root)
 *   - TC-12: Port exhaustion (very heavy, long runtime)
 *
 * COMPLEX SIMULATION TESTS (May Defer):
 *   - TC-16: SYN flood defense (requires custom malicious server)
 *   - TC-17: Slowloris defense (requires custom slow server)
 *
 * NEXT STEPS:
 *   1. Implement _IOC_SrvProtoTCP.c with comprehensive error handling
 *   2. Implement SIGPIPE suppression (MSG_NOSIGNAL or signal(SIGPIPE, SIG_IGN))
 *   3. Implement socket error detection (SO_ERROR, errno handling)
 *   4. Remove GTEST_SKIP() from critical tests (TC-1, TC-2, TC-4, TC-6, TC-7, TC-8)
 *   5. Test critical scenarios first (broken pipe, connection refused, timeout)
 *   6. Implement resource leak prevention (FD cleanup on errors)
 *   7. Add fault injection hooks for malloc failures (TC-11)
 *   8. Document platform-specific test requirements (root, tc/netem)
 *   9. Provide scripts for network simulation tests (iptables, tc commands)
 *   10. Consider Docker/network namespace for isolated fault testing
 */
///////////////////////////////////////////////////////////////////////////////////////////////////
//======>END OF TODO/IMPLEMENTATION TRACKING SECTION===============================================

// END OF UT_ServiceFaultTCP.cxx
