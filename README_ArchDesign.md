[TOC]

# About

* This is IOC's Architecture Design, which includes definitions of:
  * Glossary + Concept + Object + Operation + State

# Glossary
* RefDoc: [Glossary](./README_Glossary.md)

---
# Key Concept


## ModMgr vs ModUsr（EvtProducer or EvtConsumer）

* Module Manager(a.k.a 【ModMgr】) is a manager role such as platform manager, calls IOC's MGR_APIs with arguments by product requirements to initModule, or deinitModule before module exit.
* Module User(a.k.a 【ModUsr】) is either EvtProducer or EvtConsumer that calls IOC's USR_APIs.
  * Event Producer(a.k.a 【EvtProducer】) generates/triggers events.
    * EvtProducer will post events to IOC by IOC_postEVT API.
  * Event Consumer(a.k.a 【EvtConsumer】) processes events.
    * EvtConsumer can consume events in two ways:
      * **Callback Mode (Push)**: subscribe events via IOC_subEVT or IOC_unsubEVT API
      * **Polling Mode (Pull)**: actively retrieve events via IOC_pullEVT API

## Conet vs Conles

* Communication has Connect or Connectless Mode(a.k.a 【ConetMode】、【ConlesMode】).
* @ConetMode@:
  * [1] ObjX MUST call IOC_onlineService to online a service with $SrvURI and identified as $SrvID.
  * [2] ObjY MUST call IOC_connectService to that service, and both ObjX/Y will get a $LinkID.
  * [3.1] ObjY calls IOC_execCMD with $LinkID to ask ObjX execute commands and get result, or ObjX calls IOC_execCMD.
    * Command execution is typically request-response pattern: Initiator -> Executor -> Response
    * CmdInitiator calls IOC_execCMD(LinkID, CmdID, CmdDesc) to send command
    * CmdExecutor has two ways to handle commands:
      * **Callback Mode**: CmdExecutor's CbExecCmd_F(LinkID, CmdID, CmdDesc) is invoked to process command
      * **Polling Mode**: CmdExecutor calls IOC_waitCMD(LinkID, CmdID, CmdDesc) to actively wait for commands
    * CmdExecutor sets result in CmdDesc and may call IOC_ackCMD(LinkID, CmdID, CmdDesc) to send response
    * CmdInitiator gets the result synchronously through IOC_execCMD return or separate response handling
  * [3.2] ObjX calls IOC_postEVT with $LinkID to notify ObjY something happened, or ObjY calls IOC_postEVT.
  * [3.3] ObjX calls IOC_sendDAT with $LinkID to send data to ObjY, or ObjY calls IOC_sendDAT.
* @ConlesMode@: ObjX calls IOC_postEVT with pre-defined $AutoLinkID to notify all ObjYZs, who call IOC_waitEVT or IOC_subEVT, without IOC_onlineService and IOC_connectService.
  * NOTE: CMD is NOT supported in ConlesMode because:
    * CMD requires bidirectional communication for request-response pattern
    * ConlesMode is designed for unidirectional broadcast/multicast events
    * No specific target identification mechanism in ConlesMode for command routing

* In ConetMode service has dynamic or static online mode:
  * [D] Dynamic: ObjX calls IOC_onlineService in its context to online a service and identified as $SrvID.
  * [S] Static: ObjX uses IOC_defineService in its source to define and identified by $SrvURI.

### SrvURI
* Service URI(a.k.a 【SrvURI】) is a unique StrID to identify a service in IOC's ConetMode.
  * Server side will use SrvURI to online a service by IOC_onlineService.
  * Client side will use SrvURI to connect to a service by IOC_connectService.
  * SrvURI is following RFC's URI format, plus some IOC's specific extensions, such as:
    * auto://localprocess/SrvNameX
    * udp://localhost:12345/SrvNameY
    * tcp://192.168.0.234:54321/SrvNameZ

### SrvID vs LinkID
* Service ID(a.k.a 【SrvID】) is a unique ID to identify an onlined service in IOC.
  * ONLY the service owner who onlined the service will get and have this SrvID.
* Link ID(a.k.a 【LinkID】) is a unique ID to identify a connected link between ObjX and ObjY in IOC.
  * BOTH ObjX and ObjY will each get a LinkID. For example, when ObjY connects to ObjX who already called IOC_onlineService, then ObjY's LinkID is obtained from IOC_connectService, while ObjX's LinkID is obtained from IOC_acceptClient by SrvID.
  * This means there is a pair of LinkIDs - one for ObjX and one for ObjY - that together identify an established connection between ObjX and ObjY in IOC.
  * Each LinkID pair has one Usage (CMD or EVT or DAT) and bidirectional capability. Such as:
    * ObjX's LinkID is used to postEVT to ObjY, while ObjY's LinkID is used to CbProcEvt_F in IOC's context, 
      * OR ObjY's LinkID is used to postEVT to ObjX, while ObjX's LinkID is used to CbProcEvt_F in IOC's context.
    * ObjX's LinkID is used to execCMD to ObjY, while ObjY's LinkID is used to CbExecCmd_F in IOC's context, 
      * OR ObjY's LinkID is used to waitCMD/ackCMD in ObjY's context for polling mode,
      * OR ObjY's LinkID is used to execCMD to ObjX, while ObjX's LinkID is used to CbExecCmd_F in IOC's context.
    * ObjX's LinkID is used to sendDAT to ObjY, while ObjY's LinkID is used to CbRecvDat_F in IOC's context, 
      * OR ObjY's LinkID is used to sendDAT to ObjX, while ObjX's LinkID is used to CbRecvDat_F in IOC's context.
  * The established LinkID's usage is determined by the Usage argument of connectService and UsageCapabilities argument of onlineService.

## MSG（CMD or EVT or DAT）

* Message(a.k.a 【MSG】) is a Command(a.k.a 【CMD】) or an Event(a.k.a 【EVT】) or a piece of Data(a.k.a 【DAT】).
  * CMD is always SYNC and DGRAM defined by IOC identified by CmdID;
  * EVT is always ASYNC and DGRAM defined by IOC identified by EvtID;
  * DAT is always ASYNC and STREAM defined by IOC knowns only by Link pair;

### EVT

* 【EVT】 is always ASYNC and DGRAM defined by IOC identified by EvtID, and each event is described by EvtDesc;
  * Its default property is 【ASYNC+NONBLOCK+MAYDROP】, and may be changed by setLinkParams or IOC_Options_T.
  * ->[ASYNC]: means ObjX in its current context postEVT to LinkID,
      then ObjY's CbProcEvt_F will be callbacked in IOC's context, not in ObjX's context.
      Here IOC's context is designed&implemented by IOC, may be a standalone thread or a thread pool.
      EVT is ALWAYS ASYNC and CANNOT be changed to SYNC because events are fire-and-forget notifications designed for asynchronous processing.
      This ensures non-blocking event posting and prevents caller from being blocked by event processing time.
  * ->[NONBLOCK]: means ObjX's postEVT may not be blocked if not enough resource to postEVT,
      such as no free space to queuing the EvtDesc.
      This includes immediate return (true NONBLOCK) and timeout-based return (NONBLOCK with timeout).
      USE setLinkParams to change Link's each postEvt to MAYBLOCK,
      USE IOC_Options_T to change Link's current postEvt to MAYBLOCK,
          by set enable timeout checking and with timeout value 'ULONG_MAX',
          which means ObjX's postEVT will block until has resource to postEVT.
      NOTE: TIMEOUT is a special condition of NONBLOCK - operation returns after specified time limit.
  * ->[MAYDROP]: means after ObjX's postEVT success, if IOC's internal MAY drop this EVT,
      such as IOC's internal subsystem or submodule is busy or not enough resource to process this EVT.
      Here assume IOC is a complex system, such as ObjX vs ObjY is inter-process or inter-machine communication.
      USE setLinkParams to change Link's each postEvt to NODROP,
      USE IOC_Options_T to change Link's current postEvt to NODROP,
          which means ObjX's postEVT success, IOC will try best effect to make the EVT to be processed by ObjY, including save to local persistent storage before transfer the EVT to remote machine's ObjY.

#### EVT Lifecycle
* Event processing follows these phases:
  1. **Initiation**: ObjX calls IOC_postEVT(LinkID, EvtID, EvtDesc) to post event
  2. **Validation**: IOC validates EvtID, LinkID, and EvtDesc format
  3. **Routing**: IOC routes event to target ObjY or all subscribers based on LinkID/AutoLinkID
  4. **Queuing**: IOC queues event for processing (may drop if MAYDROP and resources constrained)
  5. **Reception**: ObjY receives event through one of five modes:
     * **ConetMode Callback**: ObjY's CbProcEvt_F(LinkID, EvtID, EvtDesc) is automatically invoked
     * **ConetMode Polling**: ObjY calls IOC_waitEVT(LinkID, EvtID, EvtDesc) to actively wait for events
     * **ConlesMode <M> subEVT+CbProcEvt**: ObjY calls IOC_subEVT(EvtID, CbProcEvt_F) (MANDATORY for callback)
     * **ConlesMode <O> waitEVT**: ObjY calls IOC_waitEVT(AutoLinkID, EvtID, EvtDesc) directly (subEVT OPTIONAL)
     * **ConlesMode <O> subEVT+waitEVT**: ObjY calls IOC_subEVT(EvtID, NULL) then IOC_waitEVT() (subEVT optional)
  6. **Processing**: ObjY processes the event notification
  7. **Completion**: Event processing completes (no response required for fire-and-forget nature)
  
* **MAYDROP Behavior**: In phases 1-7, events may be dropped during phases 3-4 if resources are constrained and MAYDROP is configured. Unlike CMD's NODROP guarantee, EVT accepts potential message loss for better performance.

#### EVT Reception Patterns
* **Pattern 1 - ConetMode (Point-to-Point)**:
  ```
  // Requires established LinkID from IOC_connectService
  // Supports both callback and polling reception modes
  
  // Callback Mode:
  ObjX: IOC_postEVT(LinkID, EvtID, EvtDesc) 
    -> IOC routes to ObjY
    -> ObjY: CbProcEvt_F(LinkID, EvtID, EvtDesc) invoked
    -> ObjY: process event notification
    -> Completion (no response required)
  
  // Polling Mode:
  ObjY: IOC_waitEVT(LinkID, EvtID, EvtDesc) // blocking wait for event
  ObjX: IOC_postEVT(LinkID, EvtID, EvtDesc) // post event
    -> IOC routes to ObjY
    -> ObjY: IOC_waitEVT returns with event
    -> ObjY: process event notification
    -> Completion (no response required)
  ```

* **Pattern 2 - ConlesMode (Broadcast/Multicast)**:
  ```
  // No connection establishment required, uses AutoLinkID
  // Supports subscription-based reception with callback or polling
  
  // Subscription Callback Mode (subEVT MANDATORY):
  ObjY: IOC_subEVT(EvtID, CbProcEvt_F) // must subscribe with callback function
  ObjX: IOC_postEVT(AutoLinkID, EvtID, EvtDesc) // broadcast event
    -> IOC routes to all subscribers
    -> ObjY: CbProcEvt_F(AutoLinkID, EvtID, EvtDesc) invoked automatically
    -> ObjY: process event notification
    -> Completion (no response required)
  
  // Subscription Polling Mode (subEVT OPTIONAL):
  ObjY: IOC_subEVT(EvtID, NULL) // optional subscription without callback
  ObjY: IOC_waitEVT(AutoLinkID, EvtID, EvtDesc) // wait for events
  ObjX: IOC_postEVT(AutoLinkID, EvtID, EvtDesc) // broadcast event
    -> IOC routes to subscribers or waiting objects
    -> ObjY: IOC_waitEVT returns with event
    -> ObjY: process event notification
    -> Completion (no response required)
  ```

#### EVT Communication Modes
* **ConetMode (Point-to-Point)**:
  * Requires IOC_onlineService and IOC_connectService to establish LinkID
  * Direct event delivery between specific ObjX and ObjY
  * Supports two reception mechanisms:
    * **Callback**: Direct CbProcEvt_F invocation upon event arrival
    * **Polling**: IOC_waitEVT for active event retrieval
  * Better reliability and flow control for targeted communication
  
* **ConlesMode (Broadcast/Multicast)**:
  * Uses pre-defined AutoLinkID for broadcast delivery
  * No connection establishment required
  * Supports subscription-based reception with two mechanisms:
    * **Subscription Callback**: IOC_subEVT(EvtID, CbProcEvt_F) - MANDATORY for callback mode
    * **Subscription Polling**: IOC_subEVT(EvtID, NULL) + IOC_waitEVT() - subEVT OPTIONAL for polling mode
  * Higher scalability for one-to-many communication, lower reliability guarantees

#### EVT Error Handling
* Event processing may encounter these conditions:
  * **IOC_RESULT_INVALID_PARAM**: Invalid EvtID, LinkID, or EvtDesc
  * **IOC_RESULT_NOT_EXIST_LINK**: LinkID does not exist or already closed
  * **IOC_RESULT_BUSY**: IOC event queue is full (when immediate NONBLOCK mode)
  * **IOC_RESULT_TIMEOUT**: Event posting timeout (when NONBLOCK mode with timeout configured)
  * **IOC_RESULT_EVT_DROPPED**: Event dropped due to resource constraints (MAYDROP mode)
  * **IOC_RESULT_NO_SUBSCRIBER**: No subscriber for this EvtID (ConlesMode)
  * **IOC_RESULT_WAIT_EVT_TIMEOUT**: IOC_waitEVT timeout (when configured with timeout)
  * **IOC_RESULT_LINK_BROKEN**: Communication link is broken during event delivery

#### Blocking Behavior Clarification
* **NONBLOCK (EVT default)**: Non-blocking behavior with two sub-modes:
  * **Immediate NONBLOCK**: Returns immediately with IOC_RESULT_BUSY if resource unavailable
  * **Timeout NONBLOCK**: Returns with IOC_RESULT_TIMEOUT after specified time limit
* **MAYBLOCK**: Infinite blocking until operation completes or fails

#### Reliability Behavior Clarification  
* **MAYDROP (EVT default)**: Allows event dropping under resource constraints for better performance
  * Acceptable behavior for notifications where occasional loss is tolerable
  * Examples: status updates, progress notifications, non-critical alerts
* **NODROP**: Guarantees event delivery with best effort including persistent storage
  * For critical events that must not be lost
  * Examples: error notifications, state change events, critical alarms

#### Synchronization Behavior Clarification
* **ASYNC (EVT always)**: Event processing in IOC's context (separate thread/thread pool)
  * Better isolation and concurrency
  * ObjX continues execution immediately after IOC_postEVT
  * ObjY's CbProcEvt_F executes in IOC's managed context
  * EVT is designed for asynchronous notifications and cannot be changed to SYNC

#### EVT vs CMD Key Differences
* **Purpose**: EVT for notifications, CMD for request-response
* **Response**: EVT fire-and-forget, CMD always expects result  
* **Reliability**: EVT may drop (MAYDROP default), CMD never drops (NODROP default)
* **Performance**: EVT optimized for throughput, CMD optimized for reliability
* **Scalability**: EVT supports broadcast/multicast, CMD only point-to-point
* **Blocking**: EVT non-blocking default, CMD blocking default

### CMD

* 【CMD】 is SYNC and DGRAM defined by IOC identified by CmdID, and each command is described by CmdDesc;
  * Its default property is 【SYNC+MAYBLOCK+NODROP】, and may be changed by setLinkParams or IOC_Options_T.
  * ->[SYNC]: means ObjX in its current context execCMD to LinkID,
      then ObjY's CbExecCmd_F will be callbacked and ObjX will wait for the response synchronously.
      This is the ONLY mode for CMD execution - commands are always synchronous request-response operations.
      CMD CANNOT be changed to ASYNC because it fundamentally requires a return value from the executor.
  * ->[MAYBLOCK]: means ObjX's execCMD may be blocked if not enough resource to execCMD,
      such as ObjY is busy processing other commands or network congestion in remote case.
      This is infinite blocking until resource becomes available.
      USE setLinkParams to change Link's each execCmd to NONBLOCK,
      USE IOC_Options_T to change Link's current execCmd to NONBLOCK,
          which means ObjX's execCMD will return immediately with IOC_RESULT_BUSY if resource not available,
          or return with IOC_RESULT_TIMEOUT if timeout is configured and time limit is reached.
      NOTE: TIMEOUT is a special condition of NONBLOCK - operation returns after specified time limit.
  * ->[NODROP]: means after ObjX's execCMD success, IOC will guarantee this CMD to be processed by ObjY,
      and ObjX will ALWAYS get CMD's final result - either the processed result or the reason why it can't be processed.
      CMD will NEVER be pending in IOC indefinitely. This includes:
      * Success case: ObjY processes CMD and returns result to ObjX
      * Failure case: IOC returns failure reason (busy, timeout, link broken, etc.) to ObjX
      * Error case: IOC detects error and returns error code to ObjX
      IOC uses retry mechanism, persistent storage for inter-process/inter-machine communication to ensure delivery.
      USE setLinkParams to change Link's each execCmd to MAYDROP,
      USE IOC_Options_T to change Link's current execCmd to MAYDROP,
          which means IOC may drop the CMD if system is overloaded or resource constrained,
          and ObjX may not get any response (fire-and-forget behavior, which breaks CMD's request-response semantics).

#### CMD Lifecycle
* Command execution follows these phases:
  1. **Initiation**: ObjX calls IOC_execCMD(LinkID, CmdID, CmdDesc) to initiate command
  2. **Validation**: IOC validates CmdID, LinkID, and CmdDesc format
  3. **Routing**: IOC routes command to target ObjY based on LinkID
  4. **Reception**: ObjY receives command through one of two modes:
     * **Callback Mode**: ObjY's CbExecCmd_F(LinkID, CmdID, CmdDesc) is automatically invoked
     * **Polling Mode**: ObjY calls IOC_waitCMD(LinkID, CmdID, CmdDesc) to actively receive command
  5. **Execution**: ObjY processes the command and prepares result
  6. **Response**: ObjY sends response through one of two modes:
     * **Direct Return**: Return result directly from CbExecCmd_F callback function
     * **Explicit Ack**: Call IOC_ackCMD(LinkID, CmdID, CmdDesc) to send response separately
  7. **Completion**: ObjX receives the command result synchronously
  
* **NODROP Guarantee**: In phases 1-7, if any failure occurs, ObjX will receive a specific error code rather than the command being lost. IOC ensures ObjX always gets either success result or failure reason.

#### CMD Execution Patterns
* **Pattern 1 - Callback Mode (Automatic)**:
  ```
  ObjX: IOC_execCMD(LinkID, CmdID, CmdDesc) 
    -> IOC routes to ObjY
    -> ObjY: CbExecCmd_F(LinkID, CmdID, CmdDesc) invoked
    -> ObjY: process and return result
    -> ObjX: gets result from IOC_execCMD return
  ```

* **Pattern 2 - Polling Mode (Manual)**:
  ```
  ObjY: IOC_waitCMD(LinkID, CmdID, CmdDesc) // blocking wait for command
  ObjX: IOC_execCMD(LinkID, CmdID, CmdDesc) // send command
    -> IOC routes to ObjY
    -> ObjY: IOC_waitCMD returns with command
    -> ObjY: process command
    -> ObjY: IOC_ackCMD(LinkID, CmdID, CmdDesc) // send response
    -> ObjX: gets result from IOC_execCMD return
  ```

#### CMD Error Handling
* Command execution may fail at different phases:
  * **IOC_RESULT_INVALID_PARAM**: Invalid CmdID, LinkID, or CmdDesc
  * **IOC_RESULT_NOT_EXIST_LINK**: LinkID does not exist or already closed
  * **IOC_RESULT_BUSY**: Target ObjY is busy (when immediate NONBLOCK mode)
  * **IOC_RESULT_TIMEOUT**: Command execution timeout (when NONBLOCK mode with timeout configured)
  * **IOC_RESULT_CMD_EXEC_FAILED**: ObjY's CbExecCmd_F returns failure
  * **IOC_RESULT_LINK_BROKEN**: Communication link is broken during execution
  * **IOC_RESULT_WAIT_CMD_TIMEOUT**: IOC_waitCMD timeout (when configured with timeout)
  * **IOC_RESULT_ACK_CMD_FAILED**: IOC_ackCMD failed to send response

#### Blocking Behavior Clarification
* **MAYBLOCK**: Infinite blocking until operation completes or fails
* **NONBLOCK**: Non-blocking behavior with two sub-modes:
  * **Immediate NONBLOCK**: Returns immediately with IOC_RESULT_BUSY if resource unavailable
  * **Timeout NONBLOCK**: Returns with IOC_RESULT_TIMEOUT after specified time limit

#### Reliability Behavior Clarification  
* **NODROP (CMD default)**: Guarantees response delivery - ObjX always gets final result or failure reason
  * Success: Command processed successfully, result returned
  * Failure: Command failed, specific error code returned (BUSY, TIMEOUT, EXEC_FAILED, etc.)
  * Never: Command lost in IOC without any response
* **MAYDROP**: Allows message dropping under resource constraints
  * For CMD: Breaks request-response contract, not recommended
  * For EVT: Acceptable behavior for notifications

#### CMD vs EVT Comparison
| Aspect          | CMD                                        | EVT                                                                                                                                      |
| --------------- | ------------------------------------------ | ---------------------------------------------------------------------------------------------------------------------------------------- |
| Synchronization | SYNC (always)                              | ASYNC (always)                                                                                                                           |
| Response        | Always expects result                      | Fire-and-forget                                                                                                                          |
| Blocking        | MAYBLOCK (default) / NONBLOCK (optional)   | NONBLOCK (default) / MAYBLOCK (optional)                                                                                                 |
| Timeout         | TIMEOUT is special case of NONBLOCK        | TIMEOUT is special case of NONBLOCK                                                                                                      |
| Reliability     | NODROP (default) - always get final result | MAYDROP (default) - may lose events                                                                                                      |
| Drop Behavior   | MAYDROP breaks request-response semantics  | MAYDROP is acceptable for notifications                                                                                                  |
| Communication   | Point-to-Point only                        | Point-to-Point + Broadcast/Multicast                                                                                                     |
| Connection      | ConetMode only                             | ConetMode + ConlesMode                                                                                                                   |
| Reception Mode  | Callback + Polling                         | ConetMode: Callback + Polling<br/>ConlesMode: <M>subEVT+CbProcEvt(mandatory) + <O>waitEVT(optional subEVT) + <O>subEVT+waitEVT(optional) |
| Use Case        | Request-Response operations                | Notifications and status updates                                                                                                         |
| Performance     | Higher latency, guaranteed delivery        | Lower latency, optimized throughput                                                                                                      |
| Scalability     | One-to-One communication                   | One-to-Many communication supported                                                                                                      |
| Context Switch  | May execute in caller's or IOC's context   | May execute in caller's or IOC's context                                                                                                 |
| Error Handling  | Comprehensive error reporting              | Best-effort delivery with basic errors                                                                                                   |
| Resource Usage  | Higher (persistent state for responses)    | Lower (stateless notifications)                                                                                                          |

### DAT

* 【DAT】 is ASYNC and STREAM defined by IOC known only by object pair, and each data chunk is described by DatDesc;
  * Its default property is 【ASYNC+MAYBLOCK+NODROP】, where ASYNC and NODROP are IMMUTABLE for stream consistency.
  * ->[ASYNC]: means ObjX in its current context sendDAT to LinkID,
      then ObjY's CbRecvDat_F will be callbacked in IOC's context, not in ObjX's context.
      Here IOC's context is designed&implemented by IOC, may be a standalone thread or a thread pool.
      DAT is ALWAYS ASYNC and CANNOT be changed to SYNC because data streaming is designed for asynchronous processing.
      This ensures non-blocking data sending and allows for efficient streaming without blocking the sender.
  * ->[MAYBLOCK]: means ObjX's sendDAT may be blocked if not enough resource to sendDAT,
      such as IOC's internal buffer is full or network congestion in remote case.
      This is infinite blocking until resource becomes available for data transmission.
      USE setLinkParams to change Link's each sendDat to NONBLOCK,
      USE IOC_Options_T to change Link's current sendDat to NONBLOCK,
          which means ObjX's sendDAT will return immediately with IOC_RESULT_BUSY if buffer not available,
          or return with IOC_RESULT_TIMEOUT if timeout is configured and time limit is reached.
      NOTE: TIMEOUT is a special condition of NONBLOCK - operation returns after specified time limit.
  * ->[NODROP]: means after ObjX's sendDAT success, IOC will guarantee this data chunk to be delivered to ObjY,
      using flow control, buffering, and retry mechanisms to ensure reliable data streaming.
      This includes persistent storage for inter-process/inter-machine communication to prevent data loss.
      DAT is ALWAYS NODROP and CANNOT be changed to MAYDROP because stream consistency is fundamental:
      * Stream data must maintain sequential integrity and completeness
      * Dropping chunks would break stream semantics and cause data corruption
      * Flow control and backpressure are used instead of dropping to handle resource constraints
      * For performance optimization, use NONBLOCK mode rather than compromising stream reliability

#### DAT Lifecycle
* Data streaming follows these phases:
  1. **Initiation**: ObjX calls IOC_sendDAT(LinkID, DatDesc) to send data chunk
  2. **Validation**: IOC validates LinkID and DatDesc format/size
  3. **Buffering**: IOC buffers data in internal streaming buffer with flow control
  4. **Transmission**: IOC transmits data to target ObjY through established LinkID
  5. **Reception**: ObjY receives data through one of two modes:
     * **Callback Mode**: ObjY's CbRecvDat_F(LinkID, DatDesc) is automatically invoked for each chunk
     * **Polling Mode**: ObjY calls IOC_recvDAT(LinkID, DatDesc) to actively receive data chunks
  6. **Processing**: ObjY processes the received data chunk
  7. **Flow Control**: IOC manages buffer space and applies backpressure if necessary
  8. **Completion**: Data streaming continues until sender closes stream or error occurs

* **NODROP Guarantee**: In phases 1-8, data chunks are never lost due to IOC internal issues. Flow control ensures sender waits when buffers are full rather than dropping data.

#### DAT Streaming Patterns
* **Pattern 1 - Callback Mode (Automatic)**:
  ```
  ObjX: IOC_sendDAT(LinkID, DatDesc) // send data chunk
    -> IOC buffers and routes to ObjY
    -> ObjY: CbRecvDat_F(LinkID, DatDesc) invoked for each chunk
    -> ObjY: process received data
    -> Flow control manages buffer space automatically
  ```

* **Pattern 2 - Polling Mode (Manual)**:
  ```
  ObjY: IOC_recvDAT(LinkID, DatDesc) // blocking wait for data
  ObjX: IOC_sendDAT(LinkID, DatDesc) // send data chunk
    -> IOC buffers and routes to ObjY
    -> ObjY: IOC_recvDAT returns with data chunk
    -> ObjY: process received data
    -> ObjY: continue calling IOC_recvDAT for more chunks
  ```

#### DAT Stream Management (Minimalist Design)
* **Auto-Management Philosophy**:
  * **Auto-Initialization**: First IOC_sendDAT(LinkID, DatDesc) automatically initializes stream
  * **Auto-Termination**: Stream closes when LinkID closes or after configured idle timeout
  * **No Explicit Open/Close**: Eliminates API complexity and potential misuse

* **Single Essential Operation**:
  * **IOC_flushDAT(LinkID)**: Force transmission of buffered data
    - Only explicit control needed by sender (DAT is stream by definition)
    - Critical for ensuring data delivery at specific points
    - Useful before critical operations or at logical boundaries
    - Provides deterministic transmission timing control

* **Ultra-Simplified Flow**:
  ```c
  // Pure data streaming - no setup/teardown overhead
  IOC_sendDAT(linkID, chunk1);      // Stream auto-starts
  IOC_sendDAT(linkID, chunk2);
  IOC_flushDAT(linkID);             // Only when deterministic delivery needed
  IOC_sendDAT(linkID, chunk3);
  // Stream auto-closes when LinkID closes
  ```

#### DAT Error Handling
* Data streaming may encounter these conditions:
  * **IOC_RESULT_INVALID_PARAM**: Invalid LinkID or DatDesc
  * **IOC_RESULT_NOT_EXIST_LINK**: LinkID does not exist or already closed
  * **IOC_RESULT_BUFFER_FULL**: IOC buffer is full (when immediate NONBLOCK mode)
  * **IOC_RESULT_TIMEOUT**: Data transmission timeout (when NONBLOCK mode with timeout configured)
  * **IOC_RESULT_LINK_BROKEN**: Communication link is broken during data transmission
  * **IOC_RESULT_RECV_DAT_TIMEOUT**: IOC_recvDAT timeout (when configured with timeout)

#### Blocking Behavior Clarification
* **MAYBLOCK (DAT default)**: Infinite blocking until operation completes or fails
  * Ensures reliable data delivery without loss
  * Sender waits when buffers are full rather than dropping data
* **NONBLOCK**: Non-blocking behavior with two sub-modes:
  * **Immediate NONBLOCK**: Returns immediately with IOC_RESULT_BUFFER_FULL if buffer unavailable
  * **Timeout NONBLOCK**: Returns with IOC_RESULT_TIMEOUT after specified time limit

#### Reliability Behavior Clarification  
* **NODROP (DAT always)**: Guarantees data chunk delivery with flow control - CANNOT be changed
  * Uses buffering, retry mechanisms, and persistent storage for reliable streaming
  * Flow control prevents buffer overflow by applying backpressure to sender
  * Essential for maintaining stream consistency and data integrity
  * Stream semantics require sequential completeness - no chunks can be lost
  * Examples: file transfers, database replication, protocol implementations, log streaming
* **Why MAYDROP is NOT supported for DAT**:
  * Stream data must maintain sequential integrity and order
  * Missing chunks would corrupt the entire data stream
  * Receiver cannot distinguish between legitimate stream end and dropped data
  * Would break fundamental streaming protocols and applications
  * Use NONBLOCK mode instead for performance optimization without compromising reliability

#### Synchronization Behavior Clarification
* **ASYNC (DAT always)**: Data processing in IOC's context (separate thread/thread pool)
  * Better streaming performance and concurrency
  * ObjX continues execution immediately after IOC_sendDAT
  * ObjY's CbRecvDat_F executes in IOC's managed context
  * DAT is designed for asynchronous streaming and cannot be changed to SYNC

#### DAT vs CMD vs EVT Comparison
| Aspect                | DAT                                            | EVT                                                 | CMD                                        |
| --------------------- | ---------------------------------------------- | --------------------------------------------------- | ------------------------------------------ |
| **Data Type**         | STREAM (continuous data flow)                  | DGRAM (discrete messages)                           | DGRAM (discrete messages)                  |
| **Synchronization**   | ASYNC (always)                                 | ASYNC (always)                                      | SYNC (always)                              |
| **Response**          | Stream acknowledgment + flow control           | Fire-and-forget                                     | Always expects result                      |
| **Blocking**          | MAYBLOCK (default) / NONBLOCK (optional)       | NONBLOCK (default) / MAYBLOCK (optional)            | MAYBLOCK (default) / NONBLOCK (optional)   |
| **Reliability**       | NODROP (always) - reliable streaming           | MAYDROP (default) - may lose events                 | NODROP (default) - always get final result |
| **Communication**     | Point-to-Point only                            | Point-to-Point + Broadcast/Multicast                | Point-to-Point only                        |
| **Connection**        | ConetMode only                                 | ConetMode + ConlesMode                              | ConetMode only                             |
| **Use Case**          | File transfer, database replication, bulk data | Notifications and status updates                    | Request-Response operations                |
| **Performance**       | Optimized for throughput and streaming         | Lower latency, optimized throughput                 | Higher latency, guaranteed delivery        |
| **Buffer Management** | Complex buffering with flow control            | Simple queuing                                      | Minimal buffering                          |
| **Ordering**          | Maintains data order within stream             | No ordering guarantee (except single producer case) | Single request-response ordering           |
| **Fragmentation**     | Supports large data fragmentation/reassembly   | Single message per event                            | Single message per command                 |
| **Resource Usage**    | Highest (buffers, flow control, ordering)      | Lower (stateless notifications)                     | Higher (persistent state for responses)    |

#### DAT Use Cases and Examples (Minimalist)
* **File Transfer**: 
  ```
  // Large file streaming - completely automatic
  ObjSender: IOC_sendDAT(LinkID, fileChunk1)    // Stream auto-starts
  ObjSender: IOC_sendDAT(LinkID, fileChunk2), IOC_sendDAT(LinkID, fileChunk3), ...
  ObjReceiver: CbRecvDat_F receives chunks and reassembles file
  ObjSender: IOC_flushDAT(LinkID)               // Ensure file completion
  // Stream auto-closes when LinkID closes
  ```

* **Log File Streaming**:
  ```
  // Continuous log streaming - minimal overhead
  ObjLogger: IOC_sendDAT(LinkID, logEntry1)     // Stream auto-starts
  ObjLogger: IOC_sendDAT(LinkID, logEntry2), IOC_sendDAT(LinkID, logEntry3), ...
  ObjAnalyzer: CbRecvDat_F processes each log entry sequentially
  ObjLogger: IOC_flushDAT(LinkID)               // Only when immediate delivery needed
  // Stream continues until LinkID closes
  ```

* **Backup Data Streaming**:
  ```
  // Backup streaming - zero management overhead
  ObjBackupAgent: IOC_sendDAT(LinkID, backupChunk1)  // Stream auto-starts
  ObjBackupAgent: IOC_sendDAT(LinkID, backupChunk2), ...
  ObjBackupServer: CbRecvDat_F stores backup chunks sequentially  
  ObjBackupAgent: IOC_flushDAT(LinkID)                // Only at checkpoint boundaries
  // Stream lifecycle matches backup session
  ```

* **Database Replication**:
  ```
  // Critical data synchronization - NODROP ensures consistency
  ObjMaster: IOC_sendDAT(LinkID, transactionLog) // NODROP for consistency
  ObjSlave: CbRecvDat_F applies transactions to maintain consistency
  ```

* **Protocol Implementation**:
  ```
  // Network protocol implementation requiring complete data
  ObjSender: IOC_sendDAT(LinkID, protocolFrame) // NODROP for protocol integrity
  ObjReceiver: CbRecvDat_F processes complete protocol frames
  ```

---
# Object

## TODO: Link

---
# Operation

- RefDoc: [README_UserGuide](README_UserGuide.md)

---
# State

## CMD::Conet
```mermaid
stateDiagram-v2
  [*] --> LinkStateReady: _initCRuntimeSuccess
  
  state LinkStateReady {
    [*] --> InitiatorReady
    [*] --> ExecutorReady
    
    state "Initiator States" as InitiatorStates {
      InitiatorReady --> InitiatorBusyExecCmd: execCmd
      InitiatorBusyExecCmd --> InitiatorReady: cmdCompleted
      InitiatorBusyExecCmd --> InitiatorBusyExecCmd: execCmdInProgress
    }
    
    state "Executor States" as ExecutorStates {
      ExecutorReady --> ExecutorBusyCbExecCmd: cmdReceived_CallbackMode
      ExecutorBusyCbExecCmd --> ExecutorReady: cbExecCmdCompleted
      
      ExecutorReady --> ExecutorBusyWaitCmd: waitCmd
      ExecutorBusyWaitCmd --> ExecutorBusyAckCmd: cmdReceived_PollingMode
      ExecutorBusyAckCmd --> ExecutorReady: ackCmdCompleted
    }
  }
```

* **State Descriptions**:
  * **LinkStateReady**: Main state containing both initiator and executor sub-states
    * **InitiatorReady**: Ready to send commands via IOC_execCMD
    * **ExecutorReady**: Ready to receive commands via callback or polling
    * **InitiatorBusyExecCmd**: Currently executing outbound command, waiting for response
    * **ExecutorBusyCbExecCmd**: Currently processing inbound command in callback mode
    * **ExecutorBusyWaitCmd**: Actively waiting for commands in polling mode
    * **ExecutorBusyAckCmd**: Sending response in polling mode

* **Design Implementation**:
  1. **Composite State Machine**: Use hierarchical states with independent sub-state machines
  2. **Concurrent Operations**: Allow simultaneous command execution in both directions
  3. **Role-Specific Configuration**: Different timeout, retry policies for each role
  4. **State Isolation**: Prevent state interference between initiator and executor roles
  5. **Error Handling**: Independent error recovery for each role

* **Key Advantages**:
  1. **No Deadlock Risk**: Initiator and executor states don't block each other
  2. **Better Throughput**: Can process commands bidirectionally without mutual exclusion
  3. **Clear Semantics**: Each role has well-defined state transitions
  4. **Easy Testing**: Independent state machines are easier to unit test
  5. **Future Extensibility**: Easy to add more complex command patterns

## EVT::Conet

```mermaid
stateDiagram-v2
  [*] --> LinkStateReady: _initCRuntimeSuccess
  
  state LinkStateReady {
    [*] --> EventPublisherReady
    [*] --> EventSubscriberReady
    
    state "Event Publisher States" as PublisherStates {
      EventPublisherReady --> EventPublisherBusyPostEvt: postEvt
      EventPublisherBusyPostEvt --> EventPublisherReady: postEvtCompleted
      EventPublisherBusyPostEvt --> EventPublisherBusyPostEvt: postEvtInProgress
    }
    
    state "Event Subscriber States" as SubscriberStates {
      EventSubscriberReady --> EventSubscriberBusySubEvt: subEvt
      EventSubscriberBusySubEvt --> EventSubscriberReady: subEvtCompleted
      
      EventSubscriberReady --> EventSubscriberBusyUnsubEvt: unsubEvt
      EventSubscriberBusyUnsubEvt --> EventSubscriberReady: unsubEvtCompleted
      
      EventSubscriberReady --> EventSubscriberBusyCbProcEvt: evtReceived_CallbackMode
      EventSubscriberBusyCbProcEvt --> EventSubscriberReady: cbProcEvtCompleted
      
      EventSubscriberReady --> EventSubscriberBusyWaitEvt: waitEvt
      EventSubscriberBusyWaitEvt --> EventSubscriberReady: evtReceived_PollingMode
    }
  }
```

* **State Descriptions**:
  * **LinkStateReady**: Main state containing both publisher and subscriber sub-states
    * **EventPublisherReady**: Ready to send events via IOC_postEVT
    * **EventSubscriberReady**: Ready to manage subscriptions and receive events
    * **EventPublisherBusyPostEvt**: Currently posting outbound event (ASYNC, non-blocking by default)
    * **EventSubscriberBusySubEvt**: Currently establishing event subscription
    * **EventSubscriberBusyUnsubEvt**: Currently removing event subscription
    * **EventSubscriberBusyCbProcEvt**: Currently processing received event in callback mode
    * **EventSubscriberBusyWaitEvt**: Actively waiting for events in polling mode

* **Design Implementation**:
  1. **Composite State Machine**: Use hierarchical states with independent sub-state machines
  2. **Concurrent Operations**: Allow simultaneous event publishing and subscription management
  3. **Asynchronous Publishing**: Event posting is non-blocking and doesn't wait for delivery confirmation
  4. **Subscription Management**: Independent subscription/unsubscription operations
  5. **Dual Reception Modes**: Support both callback and polling reception patterns
  6. **Role-Specific Configuration**: Different timeout, reliability policies for publisher/subscriber roles
  7. **State Isolation**: Prevent state interference between publisher and subscriber roles

* **Key Advantages**:
  1. **No Blocking Risk**: Publisher doesn't block on subscriber availability or processing speed
  2. **Better Performance**: Asynchronous event delivery optimized for throughput
  3. **Clear Semantics**: Each role has well-defined state transitions
  4. **Flexible Reception**: Support both push (callback) and pull (polling) patterns
  5. **Independent Subscriptions**: Multiple event types can be managed independently
  6. **Easy Testing**: Independent state machines are easier to unit test
  7. **Future Extensibility**: Easy to add event filtering, priority handling, or batching

* **EVT::Conet vs CMD::Conet Key Differences**:
  1. **Response Pattern**: EVT fire-and-forget vs CMD request-response
  2. **Blocking Behavior**: EVT non-blocking default vs CMD blocking default
  3. **Publisher State Complexity**: EVT publisher simpler (no wait for response) vs CMD initiator complex (wait management)
  4. **Subscriber Features**: EVT has subscription management vs CMD has simple execution
  5. **Error Handling**: EVT best-effort delivery vs CMD guaranteed response
  6. **Performance**: EVT optimized for throughput vs CMD optimized for reliability

* **State Transition Details**:

  **Publisher Transitions**:
  - `EventPublisherReady → EventPublisherBusyPostEvt`: On IOC_postEVT call
  - `EventPublisherBusyPostEvt → EventPublisherReady`: Event posted to IOC queue (immediate for NONBLOCK)
  - `EventPublisherBusyPostEvt → EventPublisherBusyPostEvt`: Self-loop for MAYBLOCK until resource available

  **Subscriber Transitions**:
  - `EventSubscriberReady → EventSubscriberBusySubEvt`: On IOC_subEVT call
  - `EventSubscriberBusySubEvt → EventSubscriberReady`: Subscription established
  - `EventSubscriberReady → EventSubscriberBusyUnsubEvt`: On IOC_unsubEVT call
  - `EventSubscriberBusyUnsubEvt → EventSubscriberReady`: Subscription removed
  - `EventSubscriberReady → EventSubscriberBusyCbProcEvt`: Event received (callback mode)
  - `EventSubscriberBusyCbProcEvt → EventSubscriberReady`: Event processing completed
  - `EventSubscriberReady → EventSubscriberBusyWaitEvt`: On IOC_waitEVT call
  - `EventSubscriberBusyWaitEvt → EventSubscriberReady`: Event received (polling mode)

## EVT::Conles（AutoLink）

```mermaid
stateDiagram-v2
  [*] --> LinkStateReady: _initCRuntimeSuccess
  LinkStateReady --> LinkStateBusyCbProcEvt: enterCbProcEvt
  LinkStateBusyCbProcEvt --> LinkStateReady: leaveCbProcEvt

  LinkStateReady --> LinkStateBusySubEvt: enterSubEvt
  LinkStateBusySubEvt --> LinkStateReady: leaveSubEvt

  LinkStateReady --> LinkStateBusyUnsubEvt: enterUnsubEvt
  LinkStateBusyUnsubEvt --> LinkStateReady: leaveUnsubEvt
```

* LinkStateReady is means we may perform subEvt/unsubEvt/CbProcEvt behaviors.
  * LinkStateBusyCbProcEvt is means we are in CbProcEvt callback progress.
  * LinkStateBusySubEvt is means we are in subEvt progress.
  * LinkStateBusyUnsubEvt is means we are in unsubEvt progress.
* Attention:
  * 1) all LinkState is its main state, and its default substate is LinkSubStateDefault if not specified.
  * 2) we may postEvt in any main state, which means we may postEvt in LinkStateReady/BusyXXX.


## DAT::Conet

```mermaid
stateDiagram-v2
  [*] --> LinkStateReady: _initCRuntimeSuccess
  
  state LinkStateReady {
    [*] --> DataSenderReady
    [*] --> DataReceiverReady
    
    state "Data Sender States" as SenderStates {
      DataSenderReady --> DataSenderBusySendDat: sendDat
      DataSenderBusySendDat --> DataSenderReady: sendDatCompleted
      DataSenderBusySendDat --> DataSenderBusySendDat: sendDatInProgress
    }
    
    state "Data Receiver States" as ReceiverStates {
      DataReceiverReady --> DataReceiverBusyRecvDat: recvDat
      DataReceiverBusyRecvDat --> DataReceiverReady: recvDatCompleted
      
      DataReceiverReady --> DataReceiverBusyCbRecvDat: datReceived_CallbackMode
      DataReceiverBusyCbRecvDat --> DataReceiverReady: cbRecvDatCompleted
    }
  }
```
* **State Descriptions**:
  * **LinkStateReady**: Main state containing both sender and receiver sub-states
    * **DataSenderReady**: Ready to send data via IOC_sendDAT
    * **DataReceiverReady**: Ready to manage data reception and processing
    * **DataSenderBusySendDat**: Currently sending outbound data chunk (ASYNC, non-blocking by default)
    * **DataReceiverBusyRecvDat**: Currently receiving inbound data chunk in polling mode
    * **DataReceiverBusyCbRecvDat**: Currently processing received data in callback mode
* **Design Implementation**:
  1. **Composite State Machine**: Use hierarchical states with independent sub-state machines
  2. **Concurrent Operations**: Allow simultaneous data sending and reception management
  3. **Asynchronous Sending**: Data sending is non-blocking and doesn't wait for delivery confirmation
  4. **Dual Reception Modes**: Support both callback and polling reception patterns
  5. **Role-Specific Configuration**: Different timeout, reliability policies for sender/receiver roles
  6. **State Isolation**: Prevent state interference between sender and receiver roles
* **Key Advantages**: 
  1. **No Blocking Risk**: Sender doesn't block on receiver availability or processing speed
  2. **Better Performance**: Asynchronous data delivery optimized for throughput
  3. **Clear Semantics**: Each role has well-defined state transitions
  4. **Flexible Reception**: Support both push (callback) and pull (polling) patterns
  5. **Easy Testing**: Independent state machines are easier to unit test
  6. **Future Extensibility**: Easy to add data integrity checks, flow control, or batching
* **State Transition Details**:
  **Sender Transitions**:
  - `DataSenderReady → DataSenderBusySendDat`: On IOC_sendDAT call
  - `DataSenderBusySendDat → DataSenderReady`: Data sent to IOC queue (immediate for NONBLOCK)
  - `DataSenderBusySendDat → DataSenderBusySendDat`: Self-loop for MAYBLOCK until resource available

  **Receiver Transitions**:
  - `DataReceiverReady → DataReceiverBusyRecvDat`: On IOC_recvDAT call
  - `DataReceiverBusyRecvDat → DataReceiverReady`: Data received in polling mode
  - `DataReceiverReady → DataReceiverBusyCbRecvDat`: Data received (callback mode)
  - `DataReceiverBusyCbRecvDat → DataReceiverReady`: Data processing completed
