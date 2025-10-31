///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE================================================
/**
 * @brief ValidFunc-Typical-TCP Tests: Verify typical/classic TCP scenarios where APIs WORK correctly.
 *
 *-------------------------------------------------------------------------------------------------
 * @category ValidFunc-Typical-TCP (Common TCP Network Scenarios That Work - APIs Function Correctly)
 *
 * Part of Test Design Formula:
 *   Service's Functional Test = ValidFunc(Typical + Boundary) + InValidFunc(Misuse)
 *                                         ^^^^^^^^
 *                                    (Normal cases WORK!)
 *
 * ValidFunc = API WORKS from caller's viewpoint (successful operation)
 *  - Typical: Common scenarios in normal range - happy path success flows
 *  - TCP-Specific: Network communication patterns over TCP sockets
 *
 * This file covers: Typical/classic TCP usage scenarios with expected success
 *  - Single and multiple TCP services with single/multiple clients
 *  - Network event posting, subscribing, and unsubscribing workflows
 *  - TCP socket connection establishment and teardown
 *  - Service producer/consumer role variations over network
 *  - Dynamic resubscription patterns over TCP links
 *  - All TCP operations complete successfully as designed
 *
 * Test Philosophy - KEY DISTINCTION:
 *  - ValidFunc (Typical + Boundary): API WORKS correctly (success or graceful error)
 *  - InValidFunc (Misuse): API usage FAILS (wrong sequence, double calls, violations)
 *  - Focus: Verify common real-world TCP network scenarios execute successfully
 *  - All inputs are valid, all sequences are correct, all TCP operations succeed
 *
 * TCP Protocol Differences from FIFO:
 *  - Uses TCP sockets instead of in-memory FIFO queues
 *  - Network latency considerations (adjust timeouts)
 *  - Port binding and localhost communication
 *  - Connection establishment via socket accept/connect
 *  - Network error handling (connection refused, broken pipe, etc.)
 *
 * Related Test Files:
 *  - UT_ServiceTypical.cxx: ValidFunc-Typical with FIFO protocol (in-memory)
 *  - UT_ServiceBoundary.cxx: ValidFunc-Boundary (edge cases that still work)
 *  - UT_ServiceMisuse.cxx: InValidFunc-Misuse (wrong usage that fails)
 *  - See: Test/UT_ServiceTestDesign.md for complete test taxonomy
 *
 *-------------------------------------------------------------------------------------------------
 * @note API Overview (TCP-Specific)
 *     Service is identified by 'SrvURI' defined in IOC_SrvTypes.h,
 *         which for TCP protocol includes: tcp://host:port/path
 *     TCP Protocol specifics:
 *         - pProtocol = IOC_SRV_PROTO_TCP (instead of IOC_SRV_PROTO_FIFO)
 *         - pHost = "localhost" or IP address (instead of IOC_SRV_HOST_LOCAL_PROCESS)
 *         - Port = Unique port number (8080, 8081, etc.)
 *         - pPath = Service endpoint name
 *     On the server side, we call:
 *         IOC_onlineService() to online a TCP service (binds socket to port),
 *         IOC_offlineService() to offline a TCP service (closes listening socket),
 *         IOC_acceptClient() to accept a TCP connection from client,
 *         IOC_closeLink() to close a TCP link.
 *     On the client side, we call:
 *         IOC_connectService() to connect to a TCP service (establishes socket connection),
 *         IOC_closeLink() to close a TCP link.
 *     On both sides, we can call:
 *         IOC_postEVT() to post an event over TCP, IOC_CbProcEvt_F() to process an event.
 *         IOC_execCMD() to execute a command over TCP, IOC_CbExecCmd_F() to execute a command.
 *         IOC_sendDAT() to send data over TCP, IOC_CbRecvDat_F() to receive data.
 *
 * @note TCP Protocol Implementation Status
 *     ⚠️ TCP Protocol is PLANNED but NOT YET IMPLEMENTED
 *     Current Status: 🚧 Planning Phase
 *     Required Implementation:
 *         - Source/_IOC_SrvProtoTCP.c: TCP protocol implementation
 *         - TCP socket creation, binding, listening, accepting
 *         - TCP socket connect, send, receive operations
 *         - Network error handling and timeout management
 *         - Protocol framing for EVT/CMD/DAT over TCP stream
 *     Until TCP protocol is implemented, these tests will SKIP or FAIL gracefully.
 */
//======END OF OVERVIEW OF THIS UNIT TESTING FILE==================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF INCLUDES==========================================================================
#include <gtest/gtest.h>

#include <thread>

#include "IOC/IOC.h"
#include "_UT_IOC_Common.h"
//======END OF INCLUDES============================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF UNIT TESTING DESIGN===============================================================
/**
 * At least one User Story(a.k.a US),
 *    and at least one Acceptance Criteria(a.k.a AC) for each US,
 *      and at least one Test Case(a.k.a TC) for each AC.
 *
 * US takes VALUE from USR perspective.
 * AC clear CONDITIONS may relate to the USR.
 * TC details each condition's STEPS to verify.
 */
//-------------------------------------------------------------------------------------------------
/**
 * @brief 【User Story】
 *
 *  US-1: AS a EvtProducer using TCP protocol,
 *      I WANT to online one or many TCP services with unique ports,
 *      SO THAT EvtConsumers can connect to my TCP service over network,
 *          AND EACH can subscribe all or part events what I published on connected TCP Links,
 *          AND ANY EvtConsumer can unsubscribe the event what it subscribed at any time.
 *
 *  US-2: AS a EvtConsumer using TCP protocol,
 *      I WANT to online a TCP service on a specific port,
 *      SO THAT EvtProducer can connect to my TCP service over network,
 *          AND publish events on connected TCP Links.
 *
 *  US-3: AS a CmdInitiator using TCP protocol,
 *      I WANT to connect to CmdExecutor's TCP service,
 *      SO THAT I can execute commands over network,
 *          AND receive command results through TCP socket,
 *          AND handle command timeouts over unreliable network.
 *
 *  US-4: AS a CmdExecutor using TCP protocol,
 *      I WANT to online a TCP service and accept command requests,
 *      SO THAT CmdInitiators can connect and execute commands over network,
 *          AND I can process commands and send results back over TCP.
 *
 *  US-5: AS a DatSender using TCP protocol,
 *      I WANT to send bulk data over TCP connection,
 *      SO THAT DatReceiver can receive data stream reliably,
 *          AND large data transfers work efficiently over network,
 *          AND TCP flow control manages transmission rate automatically.
 *
 *  US-6: AS a DatReceiver using TCP protocol,
 *      I WANT to receive bulk data over TCP connection,
 *      SO THAT DatSender's data arrives reliably and in order,
 *          AND I can process streaming data as it arrives,
 *          AND TCP guarantees no data loss or corruption.
 *
 *  US-7: AS a service provider using TCP protocol,
 *      I WANT to handle network-specific scenarios,
 *      SO THAT connection failures, timeouts, and broken pipes are handled gracefully,
 *          AND my application can recover from network errors.
 *
 *  US-8: AS a TCP service developer,
 *      I WANT the same API semantics as FIFO protocol,
 *      SO THAT I can switch between FIFO and TCP by changing only the URI,
 *          AND my application logic remains unchanged for EVT/CMD/DAT operations.
 */

//-------------------------------------------------------------------------------------------------
/**
 * @brief 【Acceptance Criteria】
 *
 * [@US-1] EVT: EvtProducer/Consumer over TCP
 *      AC-1: GIVEN a TCP service is onlined by EvtProducer on port 8080,
 *          WHEN EvtConsumer connects to tcp://localhost:8080/service and establishes a TCP Link,
 *          THEN EvtConsumer can subscribe events,
 *              AND EvtProducer can post events over TCP, EvtConsumer can process them.
 *          WHEN EvtConsumer unsubscribe the event,
 *          THEN EvtProducer post events will get NO_EVENT_CONSUMER result,
 *              AND EvtConsumer will NOT process the event.
 *
 *      AC-2: GIVEN a TCP service is onlined by EvtProducer on port 8081,
 *          WHEN MANY EvtConsumers connect to the TCP service and EACH establish a TCP Link,
 *          THEN EACH EvtConsumer can subscribe different events on each's TCP Link,
 *              WHEN EvtProducer post events to all TCP Links,
 *              THEN EACH EvtConsumer will process what it subscribed events only.
 *
 *      AC-3: GIVEN many TCP services with different ports are onlined by EvtProducer,
 *          WHEN EvtConsumer connects to each TCP service and establish a TCP Link,
 *          THEN EvtConsumer can subscribe events on each TCP Link,
 *              AND EvtProducer can post events to each TCP Link, EvtConsumer can process them.
 *
 * [@US-2] EVT: EvtConsumer/Producer reverse pattern over TCP
 *      AC-1: GIVEN MANY TCP services with different ports are onlined by EvtConsumer,
 *          WHEN MANY EvtProducer connects to each TCP service and establish a TCP Link,
 *          THEN EvtProducer can post events to each TCP Link, EvtConsumer can process them.
 *
 * [@US-3] CMD: CmdInitiator/Executor over TCP
 *      AC-1: GIVEN a TCP service is onlined by CmdExecutor on port 9080,
 *          WHEN CmdInitiator connects and executes command over TCP,
 *          THEN CmdExecutor processes command and returns result over TCP,
 *              AND CmdInitiator receives result successfully through network.
 *
 *      AC-2: GIVEN CmdExecutor service processes slow commands,
 *          WHEN CmdInitiator executes command with timeout over TCP,
 *          THEN command timeout is enforced even over network latency,
 *              AND appropriate timeout result is returned.
 *
 * [@US-4] CMD: CmdExecutor/Initiator reverse pattern over TCP
 *      AC-1: GIVEN a TCP service is onlined by CmdInitiator on port 9081,
 *          WHEN CmdExecutor connects to CmdInitiator's TCP service,
 *          THEN CmdInitiator can push commands for execution over network,
 *              AND CmdExecutor processes and returns results over TCP.
 *
 * [@US-5] DAT: DatSender/Receiver over TCP
 *      AC-1: GIVEN a TCP service is onlined by DatReceiver on port 10080,
 *          WHEN DatSender connects and sends data stream over TCP,
 *          THEN DatReceiver receives data reliably and in order,
 *              AND large data transfers complete successfully over network,
 *              AND TCP ensures reliable delivery without data loss.
 *
 *      AC-2: GIVEN DatReceiver processes data slowly,
 *          WHEN DatSender sends data rapidly over TCP,
 *          THEN TCP flow control prevents sender overflow,
 *              AND data transfer adapts to receiver processing rate.
 *
 * [@US-6] DAT: DatReceiver/Sender reverse pattern over TCP
 *      AC-1: GIVEN a TCP service is onlined by DatSender on port 10081,
 *          WHEN DatReceiver connects to DatSender's TCP service,
 *          THEN DatSender can push data stream over network,
 *              AND DatReceiver processes streaming data reliably.
 *
 * [@US-7] Network Error Handling
 *      AC-1: GIVEN a TCP service is onlined but no client connects,
 *          WHEN acceptClient is called with timeout,
 *          THEN it returns IOC_RESULT_TIMEOUT gracefully without hanging.
 *
 *      AC-2: GIVEN a TCP client tries to connect to non-existent service,
 *          WHEN connectService is called,
 *          THEN it returns IOC_RESULT_NOT_EXIST_SERVICE or IOC_RESULT_TIMEOUT.
 *
 *      AC-3: GIVEN a TCP connection is established,
 *          WHEN peer closes connection unexpectedly,
 *          THEN operations return IOC_RESULT_LINK_BROKEN gracefully,
 *              AND application can detect and handle disconnection.
 *
 * [@US-8] Protocol Abstraction
 *      AC-1: GIVEN the same test logic from UT_ServiceTypical.cxx,
 *          WHEN protocol is changed from FIFO to TCP (URI only),
 *          THEN all EVT/CMD/DAT operations work identically with proper timeouts.
 */

//-------------------------------------------------------------------------------------------------
/**
 * @brief 【Test Cases】
 *
 * ========================================
 * EVT (Event) Tests - EvtProducer/Consumer
 * ========================================
 *
 * [@AC-1 of US-1] EVT
 * TC-1:
 *  @[Name]: verifySingleTCPServiceSingleClient_byPostEvtAtSrvSide
 *  @[Purpose]: Verify simple but typical scenario of one EvtProducer as TCP server, one EvtConsumer as TCP client.
 *  @[Brief]: EvtProducer online TCP service on port 8080, EvtConsumer connect via tcp://localhost:8080/service,
 *      subscribe KEEPALIVE event, producer post event over TCP, consumer process it, unsubscribe, verify no consumer.
 *  @[Steps]:
 *      🔧 SETUP: Prepare TCP service URI with port 8080, initialize EvtProducer and EvtConsumer data
 *      🎯 BEHAVIOR: Online TCP service, connect client, subscribe event, post event over TCP, unsubscribe
 *      ✅ VERIFY: All TCP operations succeed, event delivered over network, unsubscribe prevents delivery
 *      🧹 CLEANUP: Close TCP links, offline TCP service
 *  @[Status]: ⚠️ SKIP - TCP protocol not yet implemented, requires _IOC_SrvProtoTCP.c
 *
 * [@AC-2 of US-1] EVT
 * TC-2:
 *  @[Name]: verifySingleTCPServiceMultiClients_byPostEvtAtSrvSide_bySubDiffEvtAtCliSide
 *  @[Purpose]: Verify multiple EvtConsumers can subscribe different events on each's TCP Link.
 *  @[Brief]: EvtProducer online TCP service on port 8081, ConsumerA subscribe MOVE events, ConsumerB subscribe PULL events,
 *      producer post different events to both TCP links, verify only subscribed consumer processes each event type.
 *  @[Steps]:
 *      🔧 SETUP: Prepare TCP service URI with port 8081, initialize multiple consumers with different event subscriptions
 *      🎯 BEHAVIOR: Online TCP service, connect multiple clients, each subscribe different events, post to all TCP links
 *      ✅ VERIFY: Each TCP client receives only subscribed events, event routing works over network
 *      🧹 CLEANUP: Close all TCP links, offline TCP service
 *  @[Status]: ⚠️ SKIP - TCP protocol not yet implemented
 *
 * [@AC-3 of US-1] EVT
 * TC-3:
 *  @[Name]: verifyMultiTCPServiceMultiClient_byPostEvtAtSrvSide_bySubDiffEvtAtCliSide
 *  @[Purpose]: Verify different TCP Services with different ports can be onlined by same EvtProducer,
 *      and each EvtConsumer can connect to each TCP service, then sub&post&proc different events.
 *  @[Brief]: Producer online services on ports 8082, 8083, 8084, consumers connect to different ports,
 *      subscribe different events, verify independent TCP services work concurrently.
 *  @[Steps]:
 *      🔧 SETUP: Prepare multiple TCP service URIs with different ports, initialize multiple services and clients
 *      🎯 BEHAVIOR: Online multiple TCP services, connect clients to different ports, subscribe events, post to each
 *      ✅ VERIFY: Independent TCP services work concurrently, port isolation works correctly
 *      🧹 CLEANUP: Close all TCP links, offline all TCP services
 *  @[Status]: ⚠️ SKIP - TCP protocol not yet implemented
 *
 * [@AC-1 of US-2] EVT
 * TC-4:
 *  @[Name]: verifyMultiTCPServiceMultiClient_byPostEvtAtCliSide_bySubDiffEvtAtSrvSide
 *  @[Purpose]: Verify different TCP Services with different ports can be onlined by same EvtConsumer,
 *      and each EvtProducer can connect to each TCP service, then post different events over network.
 *  @[Brief]: ConsumerA/B each online TCP service on different ports (8085, 8086),
 *      Producer1 connect to ConsumerA and post MOVE events,
 *      Producer2 connect to ConsumerB and post PULL events,
 *      Producer3 connect to both and post PUSH events.
 *  @[Steps]:
 *      🔧 SETUP: Consumers online TCP services on different ports, producers prepare connections
 *      🎯 BEHAVIOR: Producers connect to consumer TCP services, post events over TCP links
 *      ✅ VERIFY: Reverse TCP connection pattern works (consumer as server), event delivery correct
 *      🧹 CLEANUP: Close all TCP links, offline all TCP services
 *  @[Status]: ⚠️ SKIP - TCP protocol not yet implemented
 *
 * [@AC-1 of US-1] EVT (Additional)
 * TC-5:
 *  @[Name]: verifyConsumerResubscribeEvent_overTCP
 *  @[Purpose]: Verify EvtConsumer can dynamically resubscribe to different events on same TCP Link.
 *  @[Brief]: Consumer connect to TCP service, subscribe EVENT_A, receive events, unsubscribe,
 *      subscribe EVENT_B, receive different events, verify dynamic subscription changes over TCP.
 *  @[Steps]:
 *      🔧 SETUP: Prepare TCP service on port 8087, initialize consumer with callback tracking
 *      🎯 BEHAVIOR: Subscribe EVENT_A, post & verify, unsubscribe, subscribe EVENT_B, post & verify
 *      ✅ VERIFY: Dynamic subscription changes work over TCP, events routed correctly after resubscribe
 *      🧹 CLEANUP: Close TCP link, offline TCP service
 *  @[Status]: ⚠️ SKIP - TCP protocol not yet implemented
 *
 * ========================================
 * CMD (Command) Tests - CmdInitiator/Executor
 * ========================================
 *
 * [@AC-1 of US-3] CMD
 * TC-6:
 *  @[Name]: verifyCmdInitiatorExecutor_overTCP_withTimeout
 *  @[Purpose]: Verify CmdInitiator can execute commands on CmdExecutor over TCP with timeout.
 *  @[Brief]: CmdExecutor online TCP service on port 9080, CmdInitiator connect and execute command,
 *      CmdExecutor process and return result over TCP, verify command execution over network.
 *  @[Steps]:
 *      🔧 SETUP: Prepare TCP service URI port 9080, CmdExecutor with command handler, CmdInitiator ready
 *      🎯 BEHAVIOR: Online TCP service, connect initiator, execute command, process on executor, return result
 *      ✅ VERIFY: Command executed successfully over TCP, result received correctly over network
 *      🧹 CLEANUP: Close TCP links, offline TCP service
 *  @[Status]: ⚠️ SKIP - TCP protocol not yet implemented
 *
 * [@AC-2 of US-3] CMD
 * TC-7:
 *  @[Name]: verifyCmdTimeout_overTCP_withSlowExecutor
 *  @[Purpose]: Verify command timeout enforcement works over TCP even with network latency.
 *  @[Brief]: CmdExecutor processes slow command (>timeout), CmdInitiator enforces timeout over TCP,
 *      verify timeout result returned correctly despite network delays.
 *  @[Steps]:
 *      🔧 SETUP: Prepare TCP service port 9080, CmdExecutor with slow handler, CmdInitiator with short timeout
 *      🎯 BEHAVIOR: Execute command with timeout, executor delays, initiator enforces timeout over TCP
 *      ✅ VERIFY: IOC_RESULT_TIMEOUT returned correctly, timeout not affected by network latency
 *      🧹 CLEANUP: Close TCP links, offline TCP service
 *  @[Status]: ⚠️ SKIP - TCP protocol not yet implemented
 *
 * [@AC-1 of US-4] CMD
 * TC-8:
 *  @[Name]: verifyCmdExecutorInitiator_reverseTCP_pattern
 *  @[Purpose]: Verify reverse pattern - CmdInitiator online service, CmdExecutor connects.
 *  @[Brief]: CmdInitiator online TCP service on port 9081, CmdExecutor connects,
 *      Initiator pushes commands for execution over TCP, Executor returns results.
 *  @[Steps]:
 *      🔧 SETUP: Prepare TCP service port 9081, CmdInitiator as server, CmdExecutor as client
 *      🎯 BEHAVIOR: Initiator online service, executor connects, initiator pushes commands over TCP
 *      ✅ VERIFY: Reverse TCP pattern works for commands, results returned correctly
 *      🧹 CLEANUP: Close TCP links, offline TCP service
 *  @[Status]: ⚠️ SKIP - TCP protocol not yet implemented
 *
 * ========================================
 * DAT (Data) Tests - DatSender/Receiver
 * ========================================
 *
 * [@AC-1 of US-5] DAT
 * TC-9:
 *  @[Name]: verifyDatSenderReceiver_overTCP_withBulkData
 *  @[Purpose]: Verify DatSender can send bulk data to DatReceiver over TCP reliably.
 *  @[Brief]: DatReceiver online TCP service on port 10080, DatSender connect and send large data,
 *      DatReceiver receive data stream, verify reliable delivery over network.
 *  @[Steps]:
 *      🔧 SETUP: Prepare TCP service port 10080, DatReceiver with data handler, DatSender with bulk data
 *      🎯 BEHAVIOR: Online TCP service, connect sender, send data stream, receive and verify on receiver
 *      ✅ VERIFY: Large data transferred successfully over TCP, no data loss or corruption
 *      🧹 CLEANUP: Close TCP links, offline TCP service
 *  @[Status]: ⚠️ SKIP - TCP protocol not yet implemented
 *
 * [@AC-2 of US-5] DAT
 * TC-10:
 *  @[Name]: verifyDatFlowControl_overTCP_withSlowReceiver
 *  @[Purpose]: Verify TCP flow control prevents sender overflow when receiver is slow.
 *  @[Brief]: DatReceiver processes data slowly, DatSender sends rapidly,
 *      verify TCP flow control adapts transmission rate automatically.
 *  @[Steps]:
 *      🔧 SETUP: Prepare TCP service port 10080, DatReceiver with slow handler, DatSender with rapid send
 *      🎯 BEHAVIOR: Sender sends data rapidly, receiver processes slowly, TCP manages flow control
 *      ✅ VERIFY: No data loss despite speed mismatch, TCP backpressure works correctly
 *      🧹 CLEANUP: Close TCP links, offline TCP service
 *  @[Status]: ⚠️ SKIP - TCP protocol not yet implemented
 *
 * [@AC-1 of US-6] DAT
 * TC-11:
 *  @[Name]: verifyDatReceiverSender_reverseTCP_pattern
 *  @[Purpose]: Verify reverse pattern - DatSender online service, DatReceiver connects.
 *  @[Brief]: DatSender online TCP service on port 10081, DatReceiver connects,
 *      Sender pushes data stream over TCP, Receiver processes streaming data.
 *  @[Steps]:
 *      🔧 SETUP: Prepare TCP service port 10081, DatSender as server, DatReceiver as client
 *      🎯 BEHAVIOR: Sender online service, receiver connects, sender pushes data stream over TCP
 *      ✅ VERIFY: Reverse TCP pattern works for data transfer, streaming data processed correctly
 *      🧹 CLEANUP: Close TCP links, offline TCP service
 *  @[Status]: ⚠️ SKIP - TCP protocol not yet implemented
 */
//======END OF UNIT TESTING DESIGN=================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF HELPER FUNCTIONS==================================================================

/**
 * @brief Helper: Private data structure for EvtConsumer callback tracking
 */
typedef struct {
    int KeepAliveEvtCnt;
    int MoveStartedEvtCnt;
    int MoveKeepingEvtCnt;
    int MoveStoppedEvtCnt;
    int PullStartedEvtCnt;
    int PullKeepingEvtCnt;
    int PullStoppedEvtCnt;
    int PushStartedEvtCnt;
    int PushKeepingEvtCnt;
    int PushStoppedEvtCnt;
} __EvtConsumerPrivData_T;

/**
 * @brief Helper: Event processing callback for EvtConsumer
 */
static IOC_Result_T __CbProcEvt_F(const IOC_EvtDesc_pT pEvtDesc, void* pCbPrivData) {
    __EvtConsumerPrivData_T* pPrivData = (__EvtConsumerPrivData_T*)pCbPrivData;

    switch (pEvtDesc->EvtID) {
        case IOC_EVTID_TEST_KEEPALIVE:
            pPrivData->KeepAliveEvtCnt++;
            break;
        case IOC_EVTID_TEST_MOVE_STARTED:
            pPrivData->MoveStartedEvtCnt++;
            break;
        case IOC_EVTID_TEST_MOVE_KEEPING:
            pPrivData->MoveKeepingEvtCnt++;
            break;
        case IOC_EVTID_TEST_MOVE_STOPPED:
            pPrivData->MoveStoppedEvtCnt++;
            break;
        case IOC_EVTID_TEST_PULL_STARTED:
            pPrivData->PullStartedEvtCnt++;
            break;
        case IOC_EVTID_TEST_PULL_KEEPING:
            pPrivData->PullKeepingEvtCnt++;
            break;
        case IOC_EVTID_TEST_PULL_STOPPED:
            pPrivData->PullStoppedEvtCnt++;
            break;
        case IOC_EVTID_TEST_PUSH_STARTED:
            pPrivData->PushStartedEvtCnt++;
            break;
        case IOC_EVTID_TEST_PUSH_KEEPING:
            pPrivData->PushKeepingEvtCnt++;
            break;
        case IOC_EVTID_TEST_PUSH_STOPPED:
            pPrivData->PushStoppedEvtCnt++;
            break;
        default:
            break;
    }

    return IOC_RESULT_SUCCESS;
}

/**
 * @brief Helper: Private data structure for CmdExecutor callback tracking
 */
typedef struct {
    int ExecutedCmdCnt;
    int LastCmdID;
    IOC_Result_T LastCmdResult;
    bool SimulateSlowExecution;  // For timeout testing
    int SlowExecutionDelayMs;    // Delay in milliseconds
} __CmdExecutorPrivData_T;

/**
 * @brief Helper: Command execution callback for CmdExecutor
 */
static IOC_Result_T __CbExecCmd_F(IOC_LinkID_T LinkID, IOC_CmdDesc_pT pCmdDesc, void* pCbPrivData) {
    __CmdExecutorPrivData_T* pPrivData = (__CmdExecutorPrivData_T*)pCbPrivData;

    pPrivData->ExecutedCmdCnt++;
    pPrivData->LastCmdID = pCmdDesc->CmdID;

    // Simulate slow execution if requested (for timeout testing)
    if (pPrivData->SimulateSlowExecution && pPrivData->SlowExecutionDelayMs > 0) {
        usleep(pPrivData->SlowExecutionDelayMs * 1000);  // Convert ms to us
    }

    // Simple command processing - echo back the command
    pPrivData->LastCmdResult = IOC_RESULT_SUCCESS;
    IOC_CmdDesc_setStatus(pCmdDesc, IOC_CMD_STATUS_SUCCESS);

    return IOC_RESULT_SUCCESS;
}

/**
 * @brief Helper: Private data structure for DatReceiver callback tracking
 */
typedef struct {
    int ReceivedDataCnt;
    size_t TotalBytesReceived;
    bool SimulateSlowProcessing;  // For flow control testing
    int SlowProcessingDelayMs;    // Delay in milliseconds
    char LastReceivedData[1024];  // Cache last received data for verification
    size_t LastReceivedSize;
} __DatReceiverPrivData_T;

/**
 * @brief Helper: Data reception callback for DatReceiver
 */
static IOC_Result_T __CbRecvDat_F(IOC_LinkID_T LinkID, const IOC_DatDesc_pT pDatDesc, void* pCbPrivData) {
    __DatReceiverPrivData_T* pPrivData = (__DatReceiverPrivData_T*)pCbPrivData;

    pPrivData->ReceivedDataCnt++;
    pPrivData->TotalBytesReceived += pDatDesc->Payload.PtrDataSize;

    // Cache last received data for verification
    if (pDatDesc->Payload.PtrDataSize <= sizeof(pPrivData->LastReceivedData)) {
        memcpy(pPrivData->LastReceivedData, pDatDesc->Payload.pData, pDatDesc->Payload.PtrDataSize);
        pPrivData->LastReceivedSize = pDatDesc->Payload.PtrDataSize;
    }

    // Simulate slow processing if requested (for flow control testing)
    if (pPrivData->SimulateSlowProcessing && pPrivData->SlowProcessingDelayMs > 0) {
        usleep(pPrivData->SlowProcessingDelayMs * 1000);  // Convert ms to us
    }

    return IOC_RESULT_SUCCESS;
}

//======END OF HELPER FUNCTIONS====================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF TEST CASES========================================================================

/**
 * @[Name]: <US1-AC1-TC1> verifySingleTCPServiceSingleClient_byPostEvtAtSrvSide
 * @[Purpose]: Verify simple but typical TCP scenario of one EvtProducer as server, one EvtConsumer as client.
 * @[Brief]: TCP server on port 8080, client connects, subscribes KEEPALIVE event, server posts event,
 *     client processes it, unsubscribes, verify no consumer for subsequent posts.
 * @[Steps]:
 *   🔧 SETUP:
 *     1) Prepare TCP service URI: tcp://localhost:8080/SingleServiceSingleClient
 *     2) Initialize EvtProducer service arguments with IOC_LinkUsageEvtProducer capability
 *     3) Initialize EvtConsumer connection arguments with IOC_LinkUsageEvtConsumer usage
 *     4) Prepare event subscription arguments for KEEPALIVE event
 *   🎯 BEHAVIOR:
 *     1) EvtProducer call IOC_onlineService() to bind TCP socket on port 8080
 *     2) EvtConsumer call IOC_connectService() in thread to establish TCP connection
 *        a) Call IOC_subEVT() to subscribe KEEPALIVE event on TCP link
 *     3) EvtProducer call IOC_acceptClient() to accept TCP socket connection
 *     4) EvtProducer call IOC_postEVT() to send KEEPALIVE event over TCP
 *        a) Call IOC_forceProcEVT() to process event immediately
 *     5) EvtConsumer call IOC_unsubEVT() to unsubscribe KEEPALIVE event
 *     6) EvtProducer call IOC_postEVT() again to send another KEEPALIVE event over TCP
 *   ✅ VERIFY:
 *     1) Service online succeeds with valid SrvID
 *     2) Client connect succeeds with valid LinkID
 *     3) Event subscription succeeds
 *     4) First post succeeds, consumer callback invoked (KeepAliveEvtCnt = 1)
 *     5) Unsubscribe succeeds
 *     6) Second post returns IOC_RESULT_NO_EVENT_CONSUMER (no active subscription)
 *   🧹 CLEANUP:
 *     1) Close producer TCP link
 *     2) Close consumer TCP link
 *     3) Offline TCP service (close listening socket)
 * @[Expect]: All steps pass, TCP communication works correctly, events delivered over network
 * @[Status]: ⚠️ SKIP - TCP protocol not yet implemented
 */
TEST(UT_ServiceTypicalTCP, verifySingleTCPServiceSingleClient_byPostEvtAtSrvSide) {
    // ⚠️ TCP Protocol Check: Skip test if TCP protocol not implemented
    // TODO: Remove this check once _IOC_SrvProtoTCP.c is implemented
    GTEST_SKIP() << "⚠️ TCP Protocol not yet implemented - requires Source/_IOC_SrvProtoTCP.c";

    IOC_Result_T Result = IOC_RESULT_BUG;
    IOC_SrvID_T EvtProducerSrvID = IOC_ID_INVALID;
    IOC_LinkID_T EvtProducerLinkID = IOC_ID_INVALID;
    IOC_LinkID_T EvtConsumerLinkID = IOC_ID_INVALID;

    // 🔧 SETUP: Prepare TCP service URI with port 8080
    IOC_SrvURI_T CSURI = {
        .pProtocol = "tcp",                        // TCP protocol (IOC_SRV_PROTO_TCP not yet defined)
        .pHost = "localhost",                      // Network host instead of LOCAL_PROCESS
        .pPath = "SingleServiceSingleClient",      // Service endpoint name
        .Port = 8080,                              // TCP port binding
    };

    // 🔧 SETUP: Initialize EvtProducer service arguments
    IOC_SrvArgs_T SrvArgs = {
        .SrvURI = CSURI,
        .UsageCapabilites = IOC_LinkUsageEvtProducer,
    };

    // 🎯 BEHAVIOR: Online TCP service (bind socket to port 8080)
    Result = IOC_onlineService(&EvtProducerSrvID, &SrvArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "❌ Failed to online TCP service on port 8080";

    // 🔧 SETUP: Prepare EvtConsumer private data and subscription
    __EvtConsumerPrivData_T EvtConsumerPrivData = {0};

    IOC_EvtID_T EvtIDs[] = {IOC_EVTID_TEST_KEEPALIVE};
    IOC_SubEvtArgs_T SubEvtArgs = {
        .CbProcEvt_F = __CbProcEvt_F,
        .pCbPrivData = &EvtConsumerPrivData,
        .EvtNum = IOC_calcArrayElmtCnt(EvtIDs),
        .pEvtIDs = EvtIDs,
    };

    // 🔧 SETUP: Initialize EvtConsumer connection arguments
    IOC_ConnArgs_T ConnArgs = {
        .SrvURI = CSURI,
        .Usage = IOC_LinkUsageEvtConsumer,
    };

    // 🎯 BEHAVIOR: EvtConsumer connects in thread (TCP socket connect)
    std::thread EvtConsumerThread([&] {
        IOC_Result_T Result = IOC_connectService(&EvtConsumerLinkID, &ConnArgs, NULL);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "❌ Failed to connect to TCP service";

        // 🎯 BEHAVIOR: Subscribe to KEEPALIVE event over TCP link
        Result = IOC_subEVT(EvtConsumerLinkID, &SubEvtArgs);
        ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "❌ Failed to subscribe event over TCP";
    });

    // 🎯 BEHAVIOR: EvtProducer accepts TCP socket connection
    Result = IOC_acceptClient(EvtProducerSrvID, &EvtProducerLinkID, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "❌ Failed to accept TCP client connection";

    EvtConsumerThread.join();

    // 🎯 BEHAVIOR: Post KEEPALIVE event over TCP link
    IOC_EvtDesc_T EvtDesc = {.EvtID = IOC_EVTID_TEST_KEEPALIVE};
    Result = IOC_postEVT(EvtProducerLinkID, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "❌ Failed to post event over TCP";

    IOC_forceProcEVT();

    // ✅ VERIFY: Event was delivered over TCP and processed
    ASSERT_EQ(1, EvtConsumerPrivData.KeepAliveEvtCnt) << "❌ Event not received over TCP";

    // 🎯 BEHAVIOR: Unsubscribe KEEPALIVE event
    IOC_UnsubEvtArgs_T UnsubEvtArgs = {
        .CbProcEvt_F = __CbProcEvt_F,
        .pCbPrivData = &EvtConsumerPrivData,
    };
    Result = IOC_unsubEVT(EvtConsumerLinkID, &UnsubEvtArgs);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "❌ Failed to unsubscribe event";

    // 🎯 BEHAVIOR: Post event again after unsubscribe
    Result = IOC_postEVT(EvtProducerLinkID, &EvtDesc, NULL);
    ASSERT_EQ(IOC_RESULT_NO_EVENT_CONSUMER, Result) << "❌ Expected no consumer after unsubscribe";

    // 🧹 CLEANUP: Close TCP links
    Result = IOC_closeLink(EvtProducerLinkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "❌ Failed to close producer TCP link";

    Result = IOC_closeLink(EvtConsumerLinkID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "❌ Failed to close consumer TCP link";

    // 🧹 CLEANUP: Offline TCP service
    Result = IOC_offlineService(EvtProducerSrvID);
    ASSERT_EQ(IOC_RESULT_SUCCESS, Result) << "❌ Failed to offline TCP service";
}

/**
 * @[Name]: <US1-AC2-TC2> verifySingleTCPServiceMultiClients_byPostEvtAtSrvSide_bySubDiffEvtAtCliSide
 * @[Purpose]: Verify multiple EvtConsumers can subscribe different events on each's TCP Link.
 * @[Brief]: TCP server on port 8081, ConsumerA subscribes MOVE events, ConsumerB subscribes PULL events,
 *     server posts different events to both, verify routing works correctly over TCP.
 * @[Steps]:
 *   🔧 SETUP: Prepare TCP service on port 8081, two consumers with different event interests
 *   🎯 BEHAVIOR: Online TCP service, both consumers connect, subscribe different events, server posts to both
 *   ✅ VERIFY: Each consumer receives only subscribed events over TCP, event routing correct
 *   🧹 CLEANUP: Close all TCP links, offline TCP service
 * @[Expect]: Multiple TCP clients work concurrently, event routing per subscription
 * @[Status]: ⚠️ SKIP - TCP protocol not yet implemented
 */
TEST(UT_ServiceTypicalTCP, verifySingleTCPServiceMultiClients_byPostEvtAtSrvSide_bySubDiffEvtAtCliSide) {
    GTEST_SKIP() << "⚠️ TCP Protocol not yet implemented - requires Source/_IOC_SrvProtoTCP.c";

    // TODO: Implement test body similar to FIFO version but with TCP URIs (port 8081)
    // Key differences:
    // - Use tcp://localhost:8081/MultiClients URI
    // - Accept multiple TCP socket connections
    // - Verify concurrent TCP clients work independently
    // - Test network event delivery to multiple clients
}

/**
 * @[Name]: <US1-AC3-TC3> verifyMultiTCPServiceMultiClient_byPostEvtAtSrvSide_bySubDiffEvtAtCliSide
 * @[Purpose]: Verify different TCP Services with different ports can coexist and work independently.
 * @[Brief]: Producer online services on ports 8082, 8083, 8084, consumers connect to different ports,
 *     verify independent TCP services work concurrently without interference.
 * @[Steps]:
 *   🔧 SETUP: Prepare 3 TCP services on different ports, consumers for each
 *   🎯 BEHAVIOR: Online all TCP services, consumers connect to different ports, subscribe & post events
 *   ✅ VERIFY: Independent TCP services work concurrently, port isolation correct
 *   🧹 CLEANUP: Close all TCP links, offline all TCP services
 * @[Expect]: Multiple TCP services on different ports work independently
 * @[Status]: ⚠️ SKIP - TCP protocol not yet implemented
 */
TEST(UT_ServiceTypicalTCP, verifyMultiTCPServiceMultiClient_byPostEvtAtSrvSide_bySubDiffEvtAtCliSide) {
    GTEST_SKIP() << "⚠️ TCP Protocol not yet implemented - requires Source/_IOC_SrvProtoTCP.c";

    // TODO: Implement test body with multiple TCP services on different ports
    // Key aspects:
    // - Use ports 8082, 8083, 8084 for different services
    // - Verify port binding succeeds for all
    // - Test concurrent TCP services
    // - Verify no cross-talk between services on different ports
}

/**
 * @[Name]: <US2-AC1-TC4> verifyMultiTCPServiceMultiClient_byPostEvtAtCliSide_bySubDiffEvtAtSrvSide
 * @[Purpose]: Verify reverse TCP pattern - consumers online services, producers connect.
 * @[Brief]: ConsumerA/B online TCP services on ports 8085/8086, producers connect and post events.
 * @[Steps]:
 *   🔧 SETUP: Consumers online TCP services on different ports, producers prepare connections
 *   🎯 BEHAVIOR: Producers connect to consumer TCP services, post events over TCP
 *   ✅ VERIFY: Reverse TCP pattern works (consumer as server), event delivery correct
 *   🧹 CLEANUP: Close all TCP links, offline all TCP services
 * @[Expect]: Role reversal works over TCP, same API semantics
 * @[Status]: ⚠️ SKIP - TCP protocol not yet implemented
 */
TEST(UT_ServiceTypicalTCP, verifyMultiTCPServiceMultiClient_byPostEvtAtCliSide_bySubDiffEvtAtSrvSide) {
    GTEST_SKIP() << "⚠️ TCP Protocol not yet implemented - requires Source/_IOC_SrvProtoTCP.c";

    // TODO: Implement reverse TCP pattern test
    // Key aspects:
    // - Consumers bind TCP sockets (ports 8085, 8086)
    // - Producers connect to consumer services
    // - Verify role flexibility over TCP
    // - Test bidirectional communication patterns
}

/**
 * @[Name]: <US1-AC1-TC5> verifyConsumerResubscribeEvent_overTCP
 * @[Purpose]: Verify dynamic resubscription works over TCP links.
 * @[Brief]: Consumer connect to TCP service, subscribe EVENT_A, unsubscribe, subscribe EVENT_B,
 *     verify subscription changes work correctly over network connection.
 * @[Steps]:
 *   🔧 SETUP: Prepare TCP service on port 8087, consumer with callback tracking
 *   🎯 BEHAVIOR: Subscribe EVENT_A, post & verify, unsubscribe, subscribe EVENT_B, post & verify
 *   ✅ VERIFY: Dynamic subscription changes work over TCP, events routed after resubscribe
 *   🧹 CLEANUP: Close TCP link, offline TCP service
 * @[Expect]: Dynamic event subscription changes work over persistent TCP connection
 * @[Status]: ⚠️ SKIP - TCP protocol not yet implemented
 */
TEST(UT_ServiceTypicalTCP, verifyConsumerResubscribeEvent_overTCP) {
    GTEST_SKIP() << "⚠️ TCP Protocol not yet implemented - requires Source/_IOC_SrvProtoTCP.c";

    // TODO: Implement resubscription test over TCP
    // Key aspects:
    // - Use tcp://localhost:8087/Resubscribe URI
    // - Test subscription state changes over TCP link
    // - Verify TCP connection persists across resubscribe
    // - Test event routing changes dynamically
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF CMD TEST CASES====================================================================

/**
 * @[Name]: <US3-AC1-TC6> verifyCmdInitiatorExecutor_overTCP_withTimeout
 * @[Purpose]: Verify CmdInitiator can execute commands on CmdExecutor over TCP with timeout.
 * @[Brief]: CmdExecutor online TCP service on port 9080, CmdInitiator connect and execute command,
 *     CmdExecutor process and return result over TCP, verify command execution over network.
 * @[Steps]:
 *   🔧 SETUP: Prepare TCP service URI port 9080, CmdExecutor with command handler, CmdInitiator ready
 *   🎯 BEHAVIOR: Online TCP service, connect initiator, execute command, process on executor, return result
 *   ✅ VERIFY: Command executed successfully over TCP, result received correctly over network
 *   🧹 CLEANUP: Close TCP links, offline TCP service
 * @[Expect]: Command execution works over TCP, results delivered reliably
 * @[Status]: ⚠️ SKIP - TCP protocol not yet implemented
 */
TEST(UT_ServiceTypicalTCP, verifyCmdInitiatorExecutor_overTCP_withTimeout) {
    GTEST_SKIP() << "⚠️ TCP Protocol not yet implemented - requires Source/_IOC_SrvProtoTCP.c";

    // TODO: Implement CMD test over TCP
    // Key aspects:
    // - Use tcp://localhost:9080/CmdService URI
    // - CmdExecutor with __CbExecCmd_F callback
    // - CmdInitiator executes IOC_execCMD over TCP
    // - Verify command result returned over network
    // - Test timeout enforcement over TCP
}

/**
 * @[Name]: <US3-AC2-TC7> verifyCmdTimeout_overTCP_withSlowExecutor
 * @[Purpose]: Verify command timeout enforcement works over TCP even with network latency.
 * @[Brief]: CmdExecutor processes slow command (>timeout), CmdInitiator enforces timeout over TCP,
 *     verify timeout result returned correctly despite network delays.
 * @[Steps]:
 *   🔧 SETUP: Prepare TCP service port 9080, CmdExecutor with slow handler, CmdInitiator with short timeout
 *   🎯 BEHAVIOR: Execute command with timeout, executor delays, initiator enforces timeout over TCP
 *   ✅ VERIFY: IOC_RESULT_TIMEOUT returned correctly, timeout not affected by network latency
 *   🧹 CLEANUP: Close TCP links, offline TCP service
 * @[Expect]: Timeout enforcement works correctly over network
 * @[Status]: ⚠️ SKIP - TCP protocol not yet implemented
 */
TEST(UT_ServiceTypicalTCP, verifyCmdTimeout_overTCP_withSlowExecutor) {
    GTEST_SKIP() << "⚠️ TCP Protocol not yet implemented - requires Source/_IOC_SrvProtoTCP.c";

    // TODO: Implement CMD timeout test over TCP
    // Key aspects:
    // - CmdExecutor with SimulateSlowExecution = true
    // - SlowExecutionDelayMs > timeout (e.g., 2000ms delay, 500ms timeout)
    // - Verify IOC_RESULT_TIMEOUT returned
    // - Ensure timeout works despite network round-trip time
}

/**
 * @[Name]: <US4-AC1-TC8> verifyCmdExecutorInitiator_reverseTCP_pattern
 * @[Purpose]: Verify reverse pattern - CmdInitiator online service, CmdExecutor connects.
 * @[Brief]: CmdInitiator online TCP service on port 9081, CmdExecutor connects,
 *     Initiator pushes commands for execution over TCP, Executor returns results.
 * @[Steps]:
 *   🔧 SETUP: Prepare TCP service port 9081, CmdInitiator as server, CmdExecutor as client
 *   🎯 BEHAVIOR: Initiator online service, executor connects, initiator pushes commands over TCP
 *   ✅ VERIFY: Reverse TCP pattern works for commands, results returned correctly
 *   🧹 CLEANUP: Close TCP links, offline TCP service
 * @[Expect]: Role reversal works for CMD over TCP
 * @[Status]: ⚠️ SKIP - TCP protocol not yet implemented
 */
TEST(UT_ServiceTypicalTCP, verifyCmdExecutorInitiator_reverseTCP_pattern) {
    GTEST_SKIP() << "⚠️ TCP Protocol not yet implemented - requires Source/_IOC_SrvProtoTCP.c";

    // TODO: Implement reverse CMD pattern over TCP
    // Key aspects:
    // - CmdInitiator online service on port 9081
    // - CmdExecutor connect to initiator's service
    // - Verify push-model command execution over TCP
    // - Test bidirectional command flow
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF DAT TEST CASES====================================================================

/**
 * @[Name]: <US5-AC1-TC9> verifyDatSenderReceiver_overTCP_withBulkData
 * @[Purpose]: Verify DatSender can send bulk data to DatReceiver over TCP reliably.
 * @[Brief]: DatReceiver online TCP service on port 10080, DatSender connect and send large data,
 *     DatReceiver receive data stream, verify reliable delivery over network.
 * @[Steps]:
 *   🔧 SETUP: Prepare TCP service port 10080, DatReceiver with data handler, DatSender with bulk data
 *   🎯 BEHAVIOR: Online TCP service, connect sender, send data stream, receive and verify on receiver
 *   ✅ VERIFY: Large data transferred successfully over TCP, no data loss or corruption
 *   🧹 CLEANUP: Close TCP links, offline TCP service
 * @[Expect]: Bulk data transfer works reliably over TCP
 * @[Status]: ⚠️ SKIP - TCP protocol not yet implemented
 */
TEST(UT_ServiceTypicalTCP, verifyDatSenderReceiver_overTCP_withBulkData) {
    GTEST_SKIP() << "⚠️ TCP Protocol not yet implemented - requires Source/_IOC_SrvProtoTCP.c";

    // TODO: Implement DAT bulk transfer test over TCP
    // Key aspects:
    // - Use tcp://localhost:10080/DatService URI
    // - DatReceiver with __CbRecvDat_F callback
    // - DatSender sends large data (e.g., 10MB) via IOC_sendDAT
    // - Verify TotalBytesReceived matches sent bytes
    // - Test TCP reliability for large transfers
}

/**
 * @[Name]: <US5-AC2-TC10> verifyDatFlowControl_overTCP_withSlowReceiver
 * @[Purpose]: Verify TCP flow control prevents sender overflow when receiver is slow.
 * @[Brief]: DatReceiver processes data slowly, DatSender sends rapidly,
 *     verify TCP flow control adapts transmission rate automatically.
 * @[Steps]:
 *   🔧 SETUP: Prepare TCP service port 10080, DatReceiver with slow handler, DatSender with rapid send
 *   🎯 BEHAVIOR: Sender sends data rapidly, receiver processes slowly, TCP manages flow control
 *   ✅ VERIFY: No data loss despite speed mismatch, TCP backpressure works correctly
 *   🧹 CLEANUP: Close TCP links, offline TCP service
 * @[Expect]: TCP flow control prevents overflow, data delivered reliably
 * @[Status]: ⚠️ SKIP - TCP protocol not yet implemented
 */
TEST(UT_ServiceTypicalTCP, verifyDatFlowControl_overTCP_withSlowReceiver) {
    GTEST_SKIP() << "⚠️ TCP Protocol not yet implemented - requires Source/_IOC_SrvProtoTCP.c";

    // TODO: Implement DAT flow control test over TCP
    // Key aspects:
    // - DatReceiver with SimulateSlowProcessing = true
    // - SlowProcessingDelayMs = 100ms (slow receiver)
    // - DatSender sends rapidly (many small chunks)
    // - Verify all data received despite speed mismatch
    // - Test TCP window management and backpressure
}

/**
 * @[Name]: <US6-AC1-TC11> verifyDatReceiverSender_reverseTCP_pattern
 * @[Purpose]: Verify reverse pattern - DatSender online service, DatReceiver connects.
 * @[Brief]: DatSender online TCP service on port 10081, DatReceiver connects,
 *     Sender pushes data stream over TCP, Receiver processes streaming data.
 * @[Steps]:
 *   🔧 SETUP: Prepare TCP service port 10081, DatSender as server, DatReceiver as client
 *   🎯 BEHAVIOR: Sender online service, receiver connects, sender pushes data stream over TCP
 *   ✅ VERIFY: Reverse TCP pattern works for data transfer, streaming data processed correctly
 *   🧹 CLEANUP: Close TCP links, offline TCP service
 * @[Expect]: Role reversal works for DAT over TCP
 * @[Status]: ⚠️ SKIP - TCP protocol not yet implemented
 */
TEST(UT_ServiceTypicalTCP, verifyDatReceiverSender_reverseTCP_pattern) {
    GTEST_SKIP() << "⚠️ TCP Protocol not yet implemented - requires Source/_IOC_SrvProtoTCP.c";

    // TODO: Implement reverse DAT pattern over TCP
    // Key aspects:
    // - DatSender online service on port 10081
    // - DatReceiver connect to sender's service
    // - Verify push-model data streaming over TCP
    // - Test bidirectional data flow
}

//======END OF TEST CASES==========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF TEST SUITE SUMMARY================================================================
/**
 * @brief Test Suite Summary
 *
 * Total Tests: 11 TCP-specific test cases covering EVT/CMD/DAT
 *
 * Test Coverage by Category:
 *
 * 📡 EVT (Event) Tests: 5 tests (TC-1 to TC-5)
 *  ✅ Single TCP service with single EvtConsumer (port 8080)
 *  ✅ Single TCP service with multiple EvtConsumers (port 8081)
 *  ✅ Multiple TCP services with different ports (8082-8084)
 *  ✅ Reverse TCP pattern - EvtConsumer as server (8085-8086)
 *  ✅ Dynamic event resubscription over TCP (port 8087)
 *
 * ⚙️ CMD (Command) Tests: 3 tests (TC-6 to TC-8)
 *  ✅ CmdInitiator/Executor over TCP with timeout (port 9080)
 *  ✅ Command timeout enforcement over network (port 9080)
 *  ✅ Reverse TCP pattern - CmdInitiator as server (port 9081)
 *
 * 📦 DAT (Data) Tests: 3 tests (TC-9 to TC-11)
 *  ✅ Bulk data transfer over TCP (port 10080)
 *  ✅ TCP flow control with slow receiver (port 10080)
 *  ✅ Reverse TCP pattern - DatSender as server (port 10081)
 *
 * Port Allocation Strategy:
 *  - 8080-8087: EVT (Event) services
 *  - 9080-9081: CMD (Command) services
 *  - 10080-10081: DAT (Data) services
 *
 * Implementation Status:
 *  ⚠️ ALL 11 TESTS SKIPPED - TCP protocol not yet implemented
 *  Required: Source/_IOC_SrvProtoTCP.c with TCP socket operations
 *
 * TCP Protocol Requirements:
 *  - Socket creation, binding, listening, accepting
 *  - Socket connect, send, receive operations
 *  - Network error handling (connection refused, broken pipe, timeout)
 *  - Protocol framing for EVT/CMD/DAT over TCP stream
 *  - Port management and concurrent service support
 *  - Flow control and backpressure management
 *  - Timeout enforcement over network latency
 *
 * Key TCP-Specific Features to Test:
 *  1. Network reliability - data integrity over TCP stream
 *  2. Timeout handling - command/operation timeouts with network latency
 *  3. Flow control - sender/receiver speed mismatch handling
 *  4. Port isolation - multiple services on different ports
 *  5. Connection management - accept/connect/close over sockets
 *  6. Role flexibility - producer/consumer, initiator/executor, sender/receiver role reversal
 *  7. Concurrent services - multiple TCP services work independently
 *  8. Error scenarios - connection refused, broken pipe, network errors
 *
 * Next Steps:
 *  1. Implement _IOC_SrvProtoTCP.c with TCP protocol methods:
 *     - __IOC_onlineService_ofProtoTCP (bind socket to port)
 *     - __IOC_offlineService_ofProtoTCP (close listening socket)
 *     - __IOC_connectService_ofProtoTCP (TCP socket connect)
 *     - __IOC_acceptClient_ofProtoTCP (accept TCP connection)
 *     - __IOC_closeLink_ofProtoTCP (close TCP socket)
 *     - __IOC_postEvt_ofProtoTCP (send event over TCP)
 *     - __IOC_execCmd_ofProtoTCP (execute command over TCP)
 *     - __IOC_sendData_ofProtoTCP (send data over TCP)
 *  2. Implement TCP protocol framing (message boundaries over stream)
 *  3. Remove GTEST_SKIP() guards from test cases
 *  4. Adjust timeouts for network latency (longer than FIFO)
 *  5. Add TCP-specific error scenarios (connection refused, broken pipe)
 *  6. Test cross-machine communication (not just localhost)
 *  7. Performance testing - throughput, latency measurements
 */
//======END OF TEST SUITE SUMMARY==================================================================
