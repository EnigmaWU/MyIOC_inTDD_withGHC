[TOC]

# About

This document describes the **State Machine Design** for IOC (Inter-Object Communication) framework. It provides detailed state diagrams and sequence diagrams for managing the lifecycle of services, links, and communication operations.

## How to Read This Document

**For Quick Start** (First-time readers):
1. Read [Communication Modes Overview](#communication-modes-overview) to understand ConetMode vs ConlesMode
2. Read [Overview](#overview) for the big picture
3. Jump to the specific subsystem you're using:
   - [Service States](#service-state-machine) - Service lifecycle (online/offline)
   - [Command States](#command-state-machine) - Request-response (CMD)
   - [Event States](#event-state-machine) - Fire-and-forget (EVT)
   - [Data States](#data-state-machine) - Stream transfer (DAT)
4. See [State Query APIs](#state-query-apis) to inspect runtime state

**For Deep Understanding** (Architecture study):
1. Start from [Overview](#overview) for the hierarchical model
2. Read [Link State Hierarchy](#understanding-link-state-hierarchy) to understand the 3-level state system
3. Study each state machine section with sequence diagrams
4. Review [Practical Decision Guide](#practical-decision-guide) for real-world scenarios

**Symbols Used**:
- ✅ **IMPLEMENTED** - State enum exists in header files, can be queried via API
- 📐 **CONCEPTUAL** - Logical state for documentation, helps understand behavior but not a separate enum
- 🔴 **TODO** - API declared but implementation pending

## Communication Modes Overview

IOC supports **two communication modes** with different connection requirements:

| Mode | Full Name | Connection Required? | Link Management | Use Cases | State Complexity |
|------|-----------|---------------------|-----------------|-----------|------------------|
| **ConetMode** | Connection-Oriented | Yes (TCP/FIFO) | Manual connect/disconnect | Client-Server, P2P, Multi-client services | 3 levels: Connection + Operation + Detail |
| **ConlesMode** | Connectionless | No (Auto LinkID) | Automatic | Broadcast, Pub-Sub, Internal modules | 1 level: Operation only |

**Key Insight**: 
- **ConetMode** requires `IOC_connectService()` and manages transport layer (TCP/FIFO)
- **ConlesMode** uses `IOC_CONLES_MODE_AUTO_LINK_ID`, no connection setup needed
- Choose based on whether you need explicit connection control

# Overview

IOC framework uses **hierarchical state machines** to manage communication lifecycle across multiple components:

- **Service States**: Service online/offline lifecycle (manages multiple links)
- **Link States**: Connection and operation lifecycle (3-level hierarchy for ConetMode)
- **Command States**: Request-response command execution (link + individual command)
- **Event States**: Fire-and-forget event notification (different patterns for Conet/Conles modes)
- **Data States**: Stream-based data transfer (bidirectional with flow control)

**State Machine Characteristics**:
- **Independent**: Each component maintains its own state machine
- **Hierarchical**: Links have 3 levels (Connection → Operation → Detail) in ConetMode
- **Concurrent**: Multiple operations can be active simultaneously
- **Testable**: Clear transitions enable comprehensive unit testing

Each component maintains independent state machines with clear transition rules, enabling reliable, concurrent, and testable communication patterns.

---

# Service State Machine

## Service Lifecycle States ✅ IMPLEMENTED

**Implementation**: `IOC_SrvState_T` enum in `Include/IOC/IOC_SrvTypes.h`

```mermaid
stateDiagram-v2
  [*] --> ServiceOffline: Initial State
  
  ServiceOffline --> ServiceOnlining: IOC_onlineService()
  ServiceOnlining --> ServiceOnline: Online Success
  ServiceOnlining --> ServiceOffline: Online Failed
  
  ServiceOnline --> ServiceAccepting: Client Connection Request
  ServiceAccepting --> ServiceOnline: Accept Complete
  
  ServiceOnline --> ServiceOfflining: IOC_offlineService()
  ServiceOfflining --> ServiceOffline: Offline Success
  ServiceOfflining --> ServiceOnline: Offline Failed
  
  note right of ServiceOffline
    Service not available
    No SrvID assigned
    Cannot accept connections
  end note
  
  note right of ServiceOnline
    Service available at SrvURI
    SrvID assigned
    Ready to accept clients
  end note
```

## Service State Descriptions

| State | Status | Enum Value | Description | Entry Condition | Exit Condition | Valid Operations |
|-------|--------|-----------|-------------|----------------|----------------|------------------|
| **ServiceOffline** | ✅ | `IOC_SrvStateOffline` | Service not available | Initial state or offline complete | IOC_onlineService() called | Define service URI |
| **ServiceOnlining** | ✅ | `IOC_SrvStateOnlining` | Service being registered | onlineService in progress | Success or failure | None (transient) |
| **ServiceOnline** | ✅ | `IOC_SrvStateOnline` | Service available for connections | Online success | offlineService called | acceptClient, getLinkID |
| **ServiceAccepting** | 📐 | N/A (logical) | Processing client connection | Connection request received | Accept complete | None (transient) |
| **ServiceOfflining** | ✅ | `IOC_SrvStateOfflining` | Service being deregistered | offlineService in progress | Success or failure | None (transient) |

## Service Lifecycle Sequence Diagram

```mermaid
sequenceDiagram
    participant App as Application
    participant IOC as IOC Framework
    participant SrvMgr as Service Manager
    participant NetStack as Network Stack
    
    Note over App,NetStack: Service Registration
    App->>IOC: IOC_onlineService(SrvURI, Capabilities)
    IOC->>SrvMgr: Register Service
    SrvMgr->>NetStack: Bind to Protocol (TCP/UDP/FIFO)
    NetStack-->>SrvMgr: Binding Success
    SrvMgr-->>IOC: SrvID assigned
    IOC-->>App: Return SrvID
    
    Note over App,NetStack: Client Connection Handling
    NetStack->>SrvMgr: Incoming Connection
    SrvMgr->>IOC: Notify Connection Request
    IOC->>App: Callback CbAcceptClient_F()
    App-->>IOC: Accept/Reject Decision
    IOC->>SrvMgr: Process Decision
    SrvMgr->>NetStack: Establish/Refuse Connection
    NetStack-->>SrvMgr: LinkID created
    SrvMgr-->>IOC: LinkID assigned
    IOC-->>App: Return LinkID
    
    Note over App,NetStack: Service Deregistration
    App->>IOC: IOC_offlineService(SrvID)
    IOC->>SrvMgr: Close Service
    SrvMgr->>NetStack: Unbind Protocol
    NetStack-->>SrvMgr: Unbind Success
    SrvMgr-->>IOC: Service Closed
    IOC-->>App: Return Success
```

## Service Flags Configuration

### IOC_SrvFlags_T Enumeration

```c
typedef enum {
    IOC_SRVFLAG_NONE = 0,
    IOC_SRVFLAG_BROADCAST_EVENT    = 1 << 0,  // Enable broadcast events to all clients
    IOC_SRVFLAG_AUTO_ACCEPT        = 1 << 1,  // Automatically accept client connections
    IOC_SRVFLAG_KEEP_ACCEPTED_LINK = 1 << 2,  // Keep links alive after service shutdown
} IOC_SrvFlags_T;
```

### IOC_SRVFLAG_AUTO_ACCEPT

**Purpose**: Automatically accept client connections without manual `IOC_acceptClient()` calls.

**Behavior**:
- **Background Accept Loop**: Starts daemon thread to accept connections automatically
- **Link Storage**: Accepted links stored inside service, discoverable via `IOC_getServiceLinkIDs()`
- **Callback Preservation**: Callbacks remain where they belong (DAT recv on receiver, CMD exec on executor, EVT consume on consumer)
- **Notification Options**:
  - **Polling Mode**: Periodically call `IOC_getServiceLinkIDs()` to discover new links
  - **Immediate Mode**: Provide `OnAutoAccepted_F(SrvID, LinkID, pPriv)` callback for instant notification

**State Machine Impact**:
```
ServiceOnline → ServiceAccepting (automatic, background)
              → ServiceOnline (with new LinkID added)
```

**Usage Examples**:

| Scenario | Service Capability | Client Usage | Callback Location |
|----------|-------------------|--------------|-------------------|
| **DAT Receiver** | `IOC_LinkUsageDatReceiver` | `IOC_LinkUsageDatSender` | `SrvArgs.UsageArgs.pDat->CbRecvDat_F` |
| **DAT Sender** | `IOC_LinkUsageDatSender` | `IOC_LinkUsageDatReceiver` | `ConnArgs.UsageArgs.pDat->CbRecvDat_F` (client-side) |
| **CMD Executor** | `IOC_LinkUsageCmdExecutor` | `IOC_LinkUsageCmdInitiator` | `SrvArgs.UsageArgs.pCmd->CbExecCmd_F` |
| **CMD Initiator** | `IOC_LinkUsageCmdInitiator` | `IOC_LinkUsageCmdExecutor` | `ConnArgs.UsageArgs.pCmd->CbExecCmd_F` (client-side) |
| **EVT Producer** | `IOC_LinkUsageEvtProducer` | `IOC_LinkUsageEvtConsumer` | `ConnArgs.UsageArgs.pEvt->CbProcEvt_F` (client-side) |
| **EVT Consumer** | `IOC_LinkUsageEvtConsumer` | `IOC_LinkUsageEvtProducer` | `SrvArgs.UsageArgs.pEvt->CbProcEvt_F` |

**Configuration Example**:
```c
IOC_SrvArgs_T srvArgs;
IOC_Helper_initSrvArgs(&srvArgs);
srvArgs.Flags = IOC_SRVFLAG_AUTO_ACCEPT;
srvArgs.UsageCapabilites = IOC_LinkUsageDatReceiver;
srvArgs.UsageArgs.pDat->CbRecvDat_F = MyServiceRecvCallback;
srvArgs.OnAutoAccepted_F = OnNewClientAccepted;  // Optional immediate notification

IOC_SrvID_T srvID;
IOC_onlineService(&srvID, &srvArgs);

// Option 1: Polling mode
IOC_LinkID_T linkIDs[100];
size_t linkCount = 100;
IOC_getServiceLinkIDs(srvID, linkIDs, &linkCount);

// Option 2: Immediate callback already invoked
void OnNewClientAccepted(IOC_SrvID_T srvID, IOC_LinkID_T newLinkID, void* pPriv) {
    // Initialize per-link tracking, start operations, etc.
}
```

### IOC_SRVFLAG_KEEP_ACCEPTED_LINK

**Purpose**: Control accepted link lifecycle during service shutdown.

**Default Behavior (flag NOT set)**:
- `IOC_offlineService()` **automatically closes ALL accepted links**
- Prevents resource leaks and ensures clean shutdown
- Callbacks registered on those links will no longer be invoked
- Client-side links (CliLinkIDs) become invalid/disconnected

**With IOC_SRVFLAG_KEEP_ACCEPTED_LINK**:
- Accepted links **survive service shutdown**
- Links remain valid and functional after service goes offline
- Developer is **responsible for manual link cleanup** via `IOC_closeLink()`
- Useful for advanced scenarios like service restart while preserving connections

**State Machine Impact**:

| Scenario | Without Flag | With Flag |
|----------|-------------|----------|
| **Service Offline** | ServiceOnline → ServiceOfflining → **All Links Closed** → ServiceOffline | ServiceOnline → ServiceOfflining → **Links Remain Open** → ServiceOffline |
| **Link State** | LinkReady → **LinkDisconnected** (automatic) | LinkReady → **LinkReady** (manual cleanup needed) |
| **Cleanup** | Automatic (safe by default) | Manual (`IOC_closeLink()` required) |

**Usage Comparison**:

```c
// Scenario 1: Default behavior (safe, automatic cleanup)
IOC_SrvArgs_T srvArgs1;
srvArgs1.Flags = IOC_SRVFLAG_AUTO_ACCEPT;
IOC_onlineService(&srvID, &srvArgs1);
// ... service operates ...
IOC_offlineService(srvID);  // ✅ All accepted links automatically closed

// Scenario 2: Keep links alive (advanced, manual cleanup)
IOC_SrvArgs_T srvArgs2;
srvArgs2.Flags = IOC_SRVFLAG_AUTO_ACCEPT | IOC_SRVFLAG_KEEP_ACCEPTED_LINK;
IOC_onlineService(&srvID, &srvArgs2);
// ... service operates ...
IOC_offlineService(srvID);  // ⚠️ Links still open, service can restart
// Developer must manually close links:
IOC_LinkID_T linkIDs[100];
size_t linkCount = 100;
IOC_getServiceLinkIDs(srvID, linkIDs, &linkCount);
for (size_t i = 0; i < linkCount; i++) {
    IOC_closeLink(linkIDs[i]);  // 🔴 Manual cleanup required
}
```

**Use Cases**:

| Use Case | Recommended Flag Configuration | Reason |
|----------|-------------------------------|--------|
| **Standard Service** | `IOC_SRVFLAG_AUTO_ACCEPT` | Safe default, automatic cleanup |
| **Hot-Reload Service** | `IOC_SRVFLAG_AUTO_ACCEPT \| IOC_SRVFLAG_KEEP_ACCEPTED_LINK` | Preserve connections during restart |
| **Stateful Service** | `IOC_SRVFLAG_AUTO_ACCEPT \| IOC_SRVFLAG_KEEP_ACCEPTED_LINK` | Maintain session state across service lifecycle |
| **Debug/Test Service** | `IOC_SRVFLAG_AUTO_ACCEPT` | Simplify cleanup, prevent resource leaks |

### IOC_SRVFLAG_BROADCAST_EVENT

**Purpose**: Enable broadcast event delivery to all connected clients.

**Behavior**:
- **Default (P2P)**: Events use link-specific LinkIDs (AcptLinkID ↔ ConnLinkID)
  - `postEVT(AcptLinkID)` → **ONLY** ConnLinkID receives event
  - `postEVT(ConnLinkID)` → **ONLY** AcptLinkID receives event
  
- **With BROADCAST_EVENT**: `SrvID` is aliased to `SrvLinkID` for broadcast
  - `postEVT(SrvLinkID)` → **ALL** ConnLinkIDs receive event
  - Multicast pattern: One-to-many event delivery

**Configuration**:
```c
IOC_SrvArgs_T srvArgs;
srvArgs.Flags = IOC_SRVFLAG_AUTO_ACCEPT | IOC_SRVFLAG_BROADCAST_EVENT;
srvArgs.UsageCapabilites = IOC_LinkUsageEvtProducer;
IOC_onlineService(&srvID, &srvArgs);

// Service can now broadcast to all clients
IOC_postEVT(srvID, evtID, &evtDesc);  // All connected clients receive
```

### Service Flags Combination Matrix

| Flags | Auto Accept | Keep Links | Broadcast EVT | Use Case |
|-------|------------|-----------|---------------|----------|
| `NONE` | ❌ | ❌ | ❌ | Manual accept, manual cleanup, P2P only |
| `AUTO_ACCEPT` | ✅ | ❌ | ❌ | Auto accept, auto cleanup (standard) |
| `AUTO_ACCEPT \| KEEP_ACCEPTED_LINK` | ✅ | ✅ | ❌ | Auto accept, manual cleanup (hot-reload) |
| `AUTO_ACCEPT \| BROADCAST_EVENT` | ✅ | ❌ | ✅ | Auto accept, auto cleanup, broadcast events |
| `AUTO_ACCEPT \| KEEP_ACCEPTED_LINK \| BROADCAST_EVENT` | ✅ | ✅ | ✅ | Full control with all features |

---

# Link State Machine

## Understanding Link State Hierarchy 🔑

**KEY CONCEPT**: IOC uses a **3-level state hierarchy** to track link behavior, but which levels you use depends on your communication mode:

### The Three Levels Explained

```
Level 1: Connection State (IOC_LinkConnState_T)
  What:   TCP/FIFO connection lifecycle
  Who:    ConetMode ONLY
  States: Disconnected → Connecting → Connected → Disconnecting → Broken
  API:    IOC_getLinkConnState(linkID, &connState)

         ↓ (sits on top of)

Level 2: Operation State (IOC_LinkState_T)  
  What:   Application readiness (can I operate now?)
  Who:    BOTH ConetMode & ConlesMode
  States: Ready ↔ Busy (BusyCbProcEvt, BusySubEvt, BusyUnsubEvt)
  API:    IOC_getLinkState(linkID, &linkState)

         ↓ (provides details for)

Level 3: Operation Detail (IOC_LinkSubState_T)
  What:   Specific CMD/DAT operation in progress
  Who:    ConetMode CMD/DAT ONLY (not EVT!)
  States: CmdInitiatorReady, CmdInitiatorBusyExecCmd, DatSenderBusySendDat, ...
  API:    IOC_getLinkSubState(linkID, &subState)
```

## Practical Decision Guide

**"Which state should I check for my use case?"**

| Your Question | Check This State | Mode | Example |
|---------------|------------------|------|---------|  
| "Is network connected?" | Level 1: `IOC_LinkConnState_T` | ConetMode | `Connected`, `Broken` |
| "Can I send/receive now?" | Level 2: `IOC_LinkState_T` | Both | `Ready` vs `BusyCbProcEvt` |
| "What operation is happening?" | Level 3: `IOC_LinkSubState_T` | ConetMode CMD/DAT | `CmdInitiatorBusyExecCmd` |
| "Is my event subscription active?" | Level 2: `IOC_LinkState_T` | ConlesMode | `BusySubEvt`, `BusyUnsubEvt` |

### Quick Comparison: LinkConnState vs LinkState

| Aspect | IOC_LinkConnState_T | IOC_LinkState_T |
|--------|--------------------|-----------------|
| **Purpose** | Connection establishment | Operational readiness |
| **Layer** | Transport (TCP/UDP/FIFO) | Application (CMD/EVT/DAT) |
| **Applies To** | ConetMode only | Both ConetMode & ConlesMode |
| **Lifecycle** | Connect → Connected → Disconnect | Ready ↔ Busy |
| **Query API** | `IOC_getLinkConnState()` ✅ | `IOC_getLinkState()` 🔴 TODO |
| **Example States** | Connecting, Connected, Broken | Ready, BusyCbProcEvt |

### Mode-Specific State Usage

**ConetMode** (Connection-Oriented):
- Uses **ALL 3 levels**: Connection State + Operation State + SubState
- Example flow: `Connected` (L1) → `Ready` (L2) → `CmdInitiatorBusyExecCmd` (L3)
- Requires: `IOC_connectService()` before any operations

**ConlesMode** (Connectionless):
- Uses **Level 2 only**: Operation State (no connection phase)
- Uses `IOC_CONLES_MODE_AUTO_LINK_ID` (no connect/disconnect needed)
- Example flow: `Ready` → `BusyCbProcEvt` → `Ready`
- No connection setup: start posting/subscribing immediately

### Why Three Levels?

1. **Separation of Concerns**: Network problems ≠ Application problems
   - Level 1 handles TCP disconnects, timeouts, network errors
   - Level 2/3 handle operation flow, busy states, processing

2. **Mode Flexibility**: ConlesMode doesn't need Level 1 (no connections)
   - Simpler programming model for broadcast/pub-sub
   - No connect/disconnect APIs needed

3. **Independent Lifecycles**: Connection can be `Connected` but operation `Busy`
   - Prevents state conflicts
   - Clearer error handling (network vs application)

---

## Link Connection States (Level 1) ✅ IMPLEMENTED

**Implementation**: `IOC_LinkConnState_T` enum in `Include/IOC/IOC_Types.h`

**Scope**: **ConetMode ONLY** (connection-oriented communication)

```mermaid
stateDiagram-v2
  [*] --> LinkDisconnected: Initial State
  
  LinkDisconnected --> LinkConnecting: IOC_connectService()
  LinkConnecting --> LinkConnected: Connection Success
  LinkConnecting --> LinkDisconnected: Connection Failed
  
  LinkConnected --> LinkReady: Protocol Handshake Complete
  
  LinkReady --> LinkBusy: Communication Activity
  LinkBusy --> LinkReady: Activity Complete
  
  LinkReady --> LinkDisconnecting: IOC_disconnectService()
  LinkBusy --> LinkDisconnecting: IOC_disconnectService()
  LinkDisconnecting --> LinkDisconnected: Disconnect Complete
  
  LinkReady --> LinkBroken: Link Error Detected
  LinkBusy --> LinkBroken: Link Error Detected
  LinkBroken --> LinkDisconnected: Error Cleanup Complete
  
  note right of LinkDisconnected
    No connection established
    No LinkID assigned
  end note
  
  note right of LinkReady
    Connection established
    LinkID assigned
    Ready for CMD/EVT/DAT
  end note
  
  note right of LinkBusy
    Active communication
    Processing MSG operations
  end note
```

## Link State Descriptions

| State | Status | Enum Value | Description | Entry Condition | Exit Condition | Valid Operations |
|-------|--------|-----------|-------------|----------------|----------------|------------------|
| **LinkDisconnected** | ✅ | `IOC_LinkConnStateDisconnected` | No connection exists | Initial or disconnect complete | connectService called | None |
| **LinkConnecting** | ✅ | `IOC_LinkConnStateConnecting` | Connection being established | connectService in progress | Success or failure | None (transient) |
| **LinkConnected** | ✅ | `IOC_LinkConnStateConnected` | Transport connection ready | Connection success | Handshake complete | Protocol negotiation |
| **LinkReady** | ✅ | `IOC_LinkStateReady` | Link ready for communication | Handshake complete or activity done | Communication starts or disconnect | execCMD, postEVT, sendDAT |
| **LinkBusy** | 📐 | See substates | Communication in progress | MSG operation started | Operation complete | Continue operation |
| **LinkDisconnecting** | ✅ | `IOC_LinkConnStateDisconnecting` | Link being closed | disconnectService called | Cleanup complete | None (transient) |
| **LinkBroken** | ✅ | `IOC_LinkConnStateBroken` | Connection error occurred | Error detected | Cleanup complete | Error reporting |

**Note**: 
- **Connection states** (`IOC_LinkConnState_T`): Track TCP/UDP/FIFO connection lifecycle - **ConetMode ONLY**
- **Link main states** (`IOC_LinkState_T`): Track operational readiness - **Both ConetMode & ConlesMode**
- **Link substates** (`IOC_LinkSubState_T`): Track CMD/DAT/EVT specific operations - **ConetMode CMD/DAT**

**Why Three State Levels?**

1. **Separation of Concerns**: Connection management (transport) vs operation management (application)
2. **Mode Compatibility**: ConlesMode doesn't have connections but still needs operational tracking
3. **Independent Lifecycle**: Connection can be `Connected` but not `Ready`, or `Ready` but connection `Broken`
4. **Error Isolation**: Connection errors (network) vs operation errors (application logic)

**Practical Examples**:

```c
// ConetMode: Check both connection and operation states
IOC_LinkID_T linkID;
IOC_connectService(&linkID, &connArgs);

IOC_LinkConnState_T connState;
IOC_getLinkConnState(linkID, &connState);  // Transport layer: Connected?

IOC_LinkState_T linkState;
IOC_getLinkState(linkID, &linkState);      // Application layer: Ready?

IOC_LinkSubState_T subState;
IOC_getLinkSubState(linkID, &subState);    // Operation detail: CmdInitiatorReady?

// ConlesMode: Only check operation state (no connection)
IOC_LinkID_T autoLinkID = IOC_CONLES_MODE_AUTO_LINK_ID;
IOC_getLinkState(autoLinkID, &linkState);  // Ready? BusyCbProcEvt?
// No IOC_getLinkConnState() needed - no connection phase!
```

## Link Connection Sequence Diagram (ConetMode)

```mermaid
sequenceDiagram
    participant Client as Client App
    participant IOC_C as IOC (Client)
    participant Network as Network Stack
    participant IOC_S as IOC (Server)
    participant Server as Server App
    
    Note over Client,Server: Connection Establishment
    Client->>IOC_C: IOC_connectService(SrvURI, Usage)
    IOC_C->>Network: Connect Request
    Network->>IOC_S: Connection Arrival
    IOC_S->>Server: CbAcceptClient_F(SrvID)
    Server-->>IOC_S: Accept Decision
    IOC_S->>Network: Accept Connection
    Network-->>IOC_C: Connection Established
    IOC_C-->>Client: Return Client LinkID
    IOC_S-->>Server: Return Server LinkID
    
    Note over Client,Server: Link Ready State
    Client->>IOC_C: Communication Operations (CMD/EVT/DAT)
    IOC_C->>Network: Transfer Data
    Network->>IOC_S: Deliver Data
    IOC_S->>Server: Process Data
    
    Note over Client,Server: Link Disconnection
    Client->>IOC_C: IOC_disconnectService(LinkID)
    IOC_C->>Network: Close Connection
    Network->>IOC_S: Connection Closed
    IOC_S->>Server: CbLinkBroken_F(LinkID)
    IOC_C-->>Client: Disconnect Complete
```

---

# Command State Machine

## Link-Level Command States (CMD::Conet) ✅ IMPLEMENTED

**Implementation**: `IOC_LinkSubState_T` enum in `Include/IOC/IOC_Types.h`

**Pattern**: **Synchronous Request-Response** - CmdInitiator sends command and **blocks waiting** for result from CmdExecutor.

**Key Characteristics** (from UserGuide_CMD.md):
- **Synchronous Execution**: `IOC_execCMD()` blocks until executor responds
- **Request-Response**: Guaranteed response with result data or error
- **NODROP Guarantee**: Reliable command delivery
- **Timeout Support**: Configurable timeout for command execution
- **ConetMode ONLY**: Requires established link (connection-oriented)

Commands operate in **ConetMode** with bidirectional roles on each link. Each link maintains independent **Initiator** and **Executor** sub-state machines for concurrent command processing in both directions.

### Link Usage Types (Configuration)

- **IOC_LinkUsageCmdInitiator**: Link configured to **send commands** via `IOC_execCMD()`
  - Corresponds to **Initiator** role in state machine
  - Sends commands and **waits synchronously** for responses
  - States: InitiatorReady ↔ InitiatorBusyExecCmd (blocked during execution)

- **IOC_LinkUsageCmdExecutor**: Link configured to **receive and execute commands**
  - Corresponds to **Executor** role in state machine
  - Processes commands and sends responses back
  - Two modes:
    - **Callback Mode**: ExecutorReady → ExecutorBusyCbExecCmd (via `CbExecCmd_F` callback)
    - **Polling Mode**: ExecutorReady → ExecutorBusyWaitCmd (via `IOC_waitCMD()` + `IOC_ackCMD()`)

**Note**: A single link can have **both usage types enabled** for bidirectional command execution (service can both send and receive commands).

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
      ExecutorBusyWaitCmd --> ExecutorReady: cmdProcessed_ackCmdComplete
    }
  }
```

### Link Command State Descriptions

**Implementation Note**: These logical states map to `IOC_LinkSubState_T` enum values in `IOC_Types.h`:
- InitiatorReady → `IOC_LinkSubStateCmdInitiatorReady` ✅
- InitiatorBusyExecCmd → `IOC_LinkSubStateCmdInitiatorBusyExecCmd` ✅
- ExecutorReady → `IOC_LinkSubStateCmdExecutorReady` ✅
- ExecutorBusyCbExecCmd → `IOC_LinkSubStateCmdExecutorBusyExecCmd` ✅
- ExecutorBusyWaitCmd → `IOC_LinkSubStateCmdExecutorBusyWaitCmd` ✅

| State | Status | Enum Value | Role | Description | Entry | Exit | Operations |
|-------|--------|-----------|------|-------------|-------|------|------------|
| **InitiatorReady** | ✅ | `IOC_LinkSubStateCmdInitiatorReady` | Initiator | Ready to send commands | Init or command complete | execCMD called | IOC_execCMD() |
| **InitiatorBusyExecCmd** | ✅ | `IOC_LinkSubStateCmdInitiatorBusyExecCmd` | Initiator | Awaiting command response | execCMD called | Response received or timeout | Wait for result |
| **ExecutorReady** | ✅ | `IOC_LinkSubStateCmdExecutorReady` | Executor | Ready to receive commands | Init or processing complete | Command received or waitCMD called | IOC_waitCMD() |
| **ExecutorBusyCbExecCmd** | ✅ | `IOC_LinkSubStateCmdExecutorBusyExecCmd` | Executor | Processing command (callback) | CbExecCmd_F invoked | Callback returns | Execute logic |
| **ExecutorBusyWaitCmd** | ✅ | `IOC_LinkSubStateCmdExecutorBusyWaitCmd` | Executor | Waiting for commands (polling) | waitCMD called | Command received | Block until command, then IOC_ackCMD() |

### Design Principles

1. **Composite State Machine**: Hierarchical states with independent sub-state machines
2. **Concurrent Operations**: Allow simultaneous command execution in both directions
3. **Role-Specific Configuration**: Different timeout, retry policies for each role
4. **State Isolation**: Prevent state interference between initiator and executor roles
5. **Error Handling**: Independent error recovery for each role

### Key Advantages

1. **No Deadlock Risk**: Initiator and executor states don't block each other
2. **Better Throughput**: Process commands bidirectionally without mutual exclusion
3. **Clear Semantics**: Each role has well-defined state transitions
4. **Easy Testing**: Independent state machines are easier to unit test
5. **Future Extensibility**: Easy to add more complex command patterns

## Individual Command States (IOC_CmdDesc_T)

Individual command descriptors maintain their own state lifecycle **independent** of link states. This provides fine-grained command tracking for error handling, timeout management, and execution verification.

```mermaid
stateDiagram-v2
  [*] --> INITIALIZED: IOC_CmdDesc_initVar()
  
  INITIALIZED --> PENDING: IOC_execCMD() called
  INITIALIZED --> PENDING: Command queued in polling mode
  
  PENDING --> PROCESSING: Callback invoked (callback mode)
  PENDING --> PROCESSING: IOC_waitCMD() receives command (polling mode)
  PENDING --> TIMEOUT: Timeout expires before processing
  
  PROCESSING --> SUCCESS: Command executed successfully
  PROCESSING --> FAILED: Command execution error
  PROCESSING --> TIMEOUT: Processing timeout expires
  
  SUCCESS --> [*]: Command lifecycle complete
  FAILED --> [*]: Command lifecycle complete
  TIMEOUT --> [*]: Command lifecycle complete
  
  note right of INITIALIZED
    Status: IOC_CMD_STATUS_INITIALIZED
    Result: IOC_RESULT_SUCCESS
    Created via IOC_CmdDesc_initVar()
  end note
  
  note right of PENDING
    Status: IOC_CMD_STATUS_PENDING
    Brief state during command routing
    Framework-managed transition
  end note
  
  note right of PROCESSING
    Status: IOC_CMD_STATUS_PROCESSING
    Command being executed
    Callback mode: In CbExecCmd_F()
    Polling mode: Between waitCMD/ackCMD
  end note
  
  note right of SUCCESS
    Status: IOC_CMD_STATUS_SUCCESS
    Result: IOC_RESULT_SUCCESS
    Response data available
  end note
  
  note right of FAILED
    Status: IOC_CMD_STATUS_FAILED
    Result: Error code (e.g., IOC_RESULT_NOT_SUPPORT)
    Error details preserved
  end note
  
  note right of TIMEOUT
    Status: IOC_CMD_STATUS_TIMEOUT
    Result: IOC_RESULT_TIMEOUT
    Partial state may be preserved
  end note
```

### Individual Command State Descriptions

**Implementation**: `IOC_CmdStatus_E` enum in `Include/IOC/IOC_CmdDesc.h` ✅

| State | Status | Enum Value | Description | Entry | Exit | Result Code |
|-------|--------|-----------|-------------|-------|------|-------------|
| **INITIALIZED** | ✅ | `IOC_CMD_STATUS_INITIALIZED` | Command descriptor created | initVar() called | execCMD or queue | IOC_RESULT_SUCCESS |
| **PENDING** | ✅ | `IOC_CMD_STATUS_PENDING` | Queued for processing | Command submitted | Executor receives or timeout | IOC_RESULT_SUCCESS |
| **PROCESSING** | ✅ | `IOC_CMD_STATUS_PROCESSING` | Being executed | Executor starts | Complete or timeout | IOC_RESULT_SUCCESS |
| **SUCCESS** | ✅ | `IOC_CMD_STATUS_SUCCESS` | Executed successfully | Execution complete | Terminal state | IOC_RESULT_SUCCESS |
| **FAILED** | ✅ | `IOC_CMD_STATUS_FAILED` | Execution failed | Error detected | Terminal state | Error code |
| **TIMEOUT** | ✅ | `IOC_CMD_STATUS_TIMEOUT` | Execution timeout | Timeout expires | Terminal state | IOC_RESULT_TIMEOUT |

### Design Principles

1. **Independent Lifecycle**: Each IOC_CmdDesc_T maintains state regardless of link state
2. **Atomic Transitions**: State changes are atomic and thread-safe
3. **Terminal States**: SUCCESS/FAILED/TIMEOUT are immutable final states
4. **Error Preservation**: Failed commands preserve error context
5. **Timeout Flexibility**: Timeout can occur during PENDING or PROCESSING

### State Correlation with Link States

- **Individual Command State** (IOC_CmdDesc_T): Tracks single command execution lifecycle
- **Link Command State** (IOC_LinkID_T): Tracks aggregate command activity on link
- **Relationship**: Multiple individual commands can be PROCESSING on same link concurrently
- **Independence**: Individual command completion doesn't directly affect link state
- **Coordination**: Link busy states reflect presence of active individual commands

## Command Execution Sequence Diagrams

### Pattern 1: Callback Mode (Automatic)

```mermaid
sequenceDiagram
    participant Init as Initiator (ObjX)
    participant IOC_I as IOC (Initiator)
    participant IOC_E as IOC (Executor)
    participant Exec as Executor (ObjY)
    
    Note over Init,Exec: Command Execution - Callback Mode
    
    Init->>Init: IOC_CmdDesc_initVar(&cmdDesc)
    Note right of Init: State: INITIALIZED
    
    Init->>IOC_I: IOC_execCMD(LinkID, CmdID, &cmdDesc)
    Note right of Init: State: PENDING (brief)
    IOC_I->>IOC_E: Route Command
    
    IOC_E->>Exec: CbExecCmd_F(LinkID, CmdID, &cmdDesc)
    Note right of Exec: State: PROCESSING
    
    Exec->>Exec: Process Command Logic
    Exec->>Exec: Set Result in cmdDesc
    Exec-->>IOC_E: Return Result
    Note right of Exec: State: SUCCESS/FAILED
    
    IOC_E-->>IOC_I: Deliver Result
    IOC_I-->>Init: IOC_execCMD returns
    
    Init->>Init: Check cmdDesc.Status & Result
    Note right of Init: State: SUCCESS/FAILED/TIMEOUT
```

### Pattern 2: Polling Mode (Manual)

```mermaid
sequenceDiagram
    participant Init as Initiator (ObjX)
    participant IOC_I as IOC (Initiator)
    participant IOC_E as IOC (Executor)
    participant Exec as Executor (ObjY)
    
    Note over Init,Exec: Command Execution - Polling Mode
    
    Exec->>IOC_E: IOC_waitCMD(LinkID, &CmdID, &cmdDesc)
    Note right of Exec: Blocking wait for commands
    
    Init->>Init: IOC_CmdDesc_initVar(&cmdDesc)
    Note right of Init: State: INITIALIZED
    
    Init->>IOC_I: IOC_execCMD(LinkID, CmdID, &cmdDesc)
    Note right of Init: State: PENDING
    IOC_I->>IOC_E: Route Command
    
    IOC_E-->>Exec: IOC_waitCMD returns with command
    Note right of Exec: State: PROCESSING
    
    Exec->>Exec: Process Command Logic
    Exec->>Exec: Set Result in cmdDesc
    Exec->>IOC_E: IOC_ackCMD(LinkID, CmdID, &cmdDesc)
    Note right of Exec: State: SUCCESS/FAILED
    
    IOC_E-->>IOC_I: Deliver Result
    IOC_I-->>Init: IOC_execCMD returns
    
    Init->>Init: Check cmdDesc.Status & Result
    Note right of Init: State: SUCCESS/FAILED/TIMEOUT
    
    Exec->>IOC_E: IOC_waitCMD() again for next command
```

### Pattern 3: Timeout Scenario

```mermaid
sequenceDiagram
    participant Init as Initiator (ObjX)
    participant IOC_I as IOC (Initiator)
    participant IOC_E as IOC (Executor)
    participant Exec as Executor (ObjY)
    
    Note over Init,Exec: Command Timeout Scenario
    
    Init->>Init: IOC_CmdDesc_initVar(&cmdDesc)
    Note right of Init: State: INITIALIZED<br/>Timeout: 1000ms
    
    Init->>IOC_I: IOC_execCMD(LinkID, CmdID, &cmdDesc)
    Note right of Init: State: PENDING
    IOC_I->>IOC_E: Route Command
    
    IOC_E->>Exec: CbExecCmd_F(LinkID, CmdID, &cmdDesc)
    Note right of Exec: State: PROCESSING
    
    Exec->>Exec: Processing takes too long...
    
    Note over IOC_I: Timeout Timer Expires (1000ms)
    IOC_I->>IOC_I: Detect Timeout
    IOC_I->>IOC_I: Set Status=TIMEOUT, Result=TIMEOUT
    
    IOC_I-->>Init: IOC_execCMD returns
    Note right of Init: State: TIMEOUT<br/>Result: IOC_RESULT_TIMEOUT
    
    Init->>Init: Handle Timeout
    
    Note over Exec: Executor may still be processing<br/>Framework handles cleanup
```

---

# Event State Machine

**Pattern**: **Fire-and-Forget Notification** - EvtProducer posts event and **returns immediately** without waiting. Event delivery happens asynchronously.

**Key Characteristics** (from UserGuide_EVT.md):
- **Fire-and-Forget**: `IOC_postEVT()` / `IOC_postEVT_inConlesMode()` returns immediately after queuing
- **Asynchronous Delivery**: Events delivered to consumers via callback or polling, separate from posting
- **Dual Modes**: Both **ConlesMode** (connectionless broadcast) and **ConetMode** (connection-oriented P2P)
- **Publish-Subscribe Pattern**: EvtProducer posts to EventID, EvtConsumers subscribe by EventID
- **Flexible Processing**: Callback-based (`CbProcEvt_F`) or polling-based (`IOC_waitEVT`)
- **Performance Options**: Synchronous, asynchronous, blocking, and non-blocking modes (`IOC_OPTID_SYNC_MODE`, `IOC_OPTID_ASYNC_MODE`)
- **No EVT SubStates**: Unlike CMD/DAT, EVT uses main `IOC_LinkState_T` states only (see [Why No EVT SubStates?](#-why-no-evt-substates))

**Why No EVT SubStates?** (Critical Design Decision explained later)
- EVT is **instant queue operation**: post to queue and return immediately, no waiting needed
- CMD/DAT **block and wait**: need substates to track "waiting for response" or "sending chunks with flow control"
- EVT callback processing tracked by `IOC_LinkStateBusyCbProcEvt` (main state, not substate)
- See detailed comparison table in [Why No EVT SubStates?](#-why-no-evt-substates) section

## Link-Level Event States (EVT::Conet) 📐 CONCEPTUAL

**Implementation Note**: ConetMode event states are **conceptual/logical** for documentation. In `IOC_Types.h`, ConetMode events use main `IOC_LinkStateReady` without dedicated substates (unlike CMD/DAT). Event operations are fire-and-forget, implemented via callbacks and usage configuration.

Events operate in **ConetMode** (connection-oriented) with **Publisher-Subscriber** pattern. Each link maintains independent **Publisher** and **Subscriber** sub-state machines for concurrent event handling.

### Link Usage Types

- **IOC_LinkUsageEvtProducer**: Link configured to post events via `IOC_postEVT()`
  - Corresponds to **Publisher** role in state machine
  - Publishes events to subscribers
  - States: EventPublisherReady ↔ EventPublisherBusyPostEvt

- **IOC_LinkUsageEvtConsumer**: Link configured to receive events via callback or polling
  - Corresponds to **Subscriber** role in state machine
  - Receives and processes events
  - States: EventSubscriberReady → EventSubscriberBusySubEvt/UnsubEvt/CbProcEvt/WaitEvt

**Note**: A single link can have both usage types enabled for bidirectional event notification.

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

### Link Event State Descriptions (ConetMode)

**Implementation Note**: Event states in ConetMode are **conceptual/logical** states for documentation purposes. In `IOC_Types.h`, ConetMode events do not have dedicated link sub-states like CMD/DAT. Event operations use the main `IOC_LinkStateReady` state with default sub-state. The Publisher/Subscriber pattern is implemented through usage configuration and callbacks, not explicit state machine states.

### ❓ Why No EVT SubStates?

**Critical Design Decision**: EVT operations are fundamentally different from CMD/DAT:

| Aspect | EVT (Event) | CMD (Command) | DAT (Data) |
|--------|------------|---------------|------------|
| **Pattern** | Fire-and-forget | Request-response | Stream transfer |
| **Blocking** | ❌ Non-blocking | ✅ Blocks on response | ⚠️ May block on flow control |
| **Duration** | Instant (queue only) | Variable (wait response) | Variable (transfer chunks) |
| **State Tracking** | Not needed | **Needed** (waiting state) | **Needed** (buffer state) |
| **Publisher/Sender** | Post → Done immediately | Send → **Wait** → Done | Send → **Track buffer** → Done |
| **Subscriber/Receiver** | Async callback | Sync response required | Stream processing |

**Why CMD/DAT Need SubStates**:
```c
// CMD: Must track "waiting for response" state
IOC_execCMD(linkID, &cmdDesc);  // Initiator → BusyExecCmd
// ⏳ Block here waiting for executor response
// → InitiatorReady when response received

// DAT: Must track "buffer sending" state  
IOC_sendDAT(linkID, buffer, size);  // Sender → BusySendDat
// ⏳ May block if buffer full (flow control)
// → SenderReady when buffer space available
```

**Why EVT Doesn't Need SubStates**:
```c
// EVT: Instant queue operation, no waiting
IOC_postEVT(linkID, evtID, &evtDesc);  // Post → Queue → Done
// ✅ Returns immediately, no blocking
// Link stays in Ready state (or goes Ready → Ready)

// Event delivery happens asynchronously via callback
// Tracked by IOC_LinkState_T::BusyCbProcEvt (not substate)
```

**State Tracking for EVT**:
- **ConetMode**: Uses `IOC_LinkStateReady` with `IOC_LinkSubStateDefault`
  - `postEVT()`: Instant queue → no state change needed
  - Event delivery: Asynchronous, doesn't block sender
  
- **ConlesMode**: Uses `IOC_LinkState_T` main states:
  - `IOC_LinkStateBusyCbProcEvt`: Processing event in callback
  - `IOC_LinkStateBusySubEvt`: Subscribing to events
  - `IOC_LinkStateBusyUnsubEvt`: Unsubscribing from events

**Design Benefits**:
1. ✅ **Simplicity**: No unnecessary state complexity for instant operations
2. ✅ **Performance**: Avoids state machine overhead for fire-and-forget
3. ✅ **Clarity**: State tracking matches operation semantics
4. ✅ **Consistency**: ConlesMode events already use main states, not substates

| State | Status | Implementation | Role | Description | Entry | Exit | Operations |
|-------|--------|---------------|------|-------------|-------|------|------------|
| **EventPublisherReady** | 📐 | Logical concept | Publisher | Ready to post events | Init or post complete | postEVT called | IOC_postEVT() |
| **EventPublisherBusyPostEvt** | 📐 | Logical concept | Publisher | Posting event (brief) | postEVT called | Event queued | Queue operation |
| **EventSubscriberReady** | 📐 | Logical concept | Subscriber | Ready to manage subscriptions | Init or operation complete | subEVT/unsubEVT/waitEVT called | IOC_subEVT(), IOC_waitEVT() |
| **EventSubscriberBusySubEvt** | 📐 | Logical concept | Subscriber | Establishing subscription | subEVT called | Subscription registered | Register callback |
| **EventSubscriberBusyUnsubEvt** | 📐 | Logical concept | Subscriber | Removing subscription | unsubEVT called | Subscription removed | Deregister callback |
| **EventSubscriberBusyCbProcEvt** | 📐 | Logical concept | Subscriber | Processing event (callback) | CbProcEvt_F invoked | Callback returns | Handle event |
| **EventSubscriberBusyWaitEvt** | 📐 | Logical concept | Subscriber | Waiting for events (polling) | waitEVT called | Event received | Block until event |

### Design Principles

1. **Composite State Machine**: Hierarchical states with independent sub-state machines
2. **Concurrent Operations**: Allow simultaneous event publishing and subscription management
3. **Asynchronous Publishing**: Event posting is non-blocking and doesn't wait for delivery
4. **Subscription Management**: Independent subscription/unsubscription operations
5. **Dual Reception Modes**: Support both callback and polling reception patterns
6. **Role-Specific Configuration**: Different timeout, reliability policies for publisher/subscriber
7. **State Isolation**: Prevent state interference between publisher and subscriber roles

### Key Advantages

1. **No Blocking Risk**: Publisher doesn't block on subscriber availability
2. **Better Performance**: Asynchronous event delivery optimized for throughput
3. **Clear Semantics**: Each role has well-defined state transitions
4. **Flexible Reception**: Support both push (callback) and pull (polling) patterns
5. **Independent Subscriptions**: Multiple event types managed independently
6. **Easy Testing**: Independent state machines are easier to unit test
7. **Future Extensibility**: Easy to add event filtering, priority handling, or batching

### EVT::Conet vs CMD::Conet Key Differences

| Aspect | EVT::Conet | CMD::Conet |
|--------|-----------|-----------|
| **Response Pattern** | Fire-and-forget | Request-response |
| **Blocking Behavior** | Non-blocking default | Blocking default |
| **Publisher/Initiator State** | Simpler (no wait for response) | Complex (wait management) |
| **Subscriber/Executor Features** | Subscription management | Simple execution |
| **Error Handling** | Best-effort delivery | Guaranteed response |
| **Performance** | Optimized for throughput | Optimized for reliability |

## Event States in ConlesMode (EVT::Conles)

In **ConlesMode** (connectionless), events use **AutoLinkID** for broadcast/multicast delivery without connection establishment. State machine is simpler with focus on subscription management.

### Link Usage Types

- **IOC_LinkUsageEvtProducer**: Implicit usage for posting events to `AutoLinkID`
  - No explicit link configuration required
  - Any object can post events via `IOC_postEVT(AutoLinkID, EvtID, EvtDesc)`
  
- **IOC_LinkUsageEvtConsumer**: Implicit usage for receiving events from `AutoLinkID`
  - Subscription-based reception via `IOC_subEVT()` or direct polling via `IOC_waitEVT()`
  - No explicit link configuration required

**Note**: ConlesMode doesn't require explicit link usage configuration - roles are implicit based on API usage.

```mermaid
stateDiagram-v2
  [*] --> LinkStateReady: _initCRuntimeSuccess
  LinkStateReady --> LinkStateBusyCbProcEvt: enterCbProcEvt
  LinkStateBusyCbProcEvt --> LinkStateReady: leaveCbProcEvt

  LinkStateReady --> LinkStateBusySubEvt: enterSubEvt
  LinkStateBusySubEvt --> LinkStateReady: leaveSubEvt

  LinkStateReady --> LinkStateBusyUnsubEvt: enterUnsubEvt
  LinkStateBusyUnsubEvt --> LinkStateReady: leaveUnsubEvt
  
  note right of LinkStateReady
    Ready for subEvt/unsubEvt/CbProcEvt
    postEvt can happen in ANY state
  end note
  
  note right of LinkStateBusyCbProcEvt
    Processing event in callback
    CbProcEvt_F() executing
  end note
  
  note right of LinkStateBusySubEvt
    Establishing subscription
    IOC_subEVT() in progress
  end note
  
  note right of LinkStateBusyUnsubEvt
    Removing subscription
    IOC_unsubEVT() in progress
  end note
```

### Link Event State Descriptions (ConlesMode)

**Implementation Note**: These states map to actual `IOC_LinkState_T` main states in `IOC_Types.h`:
- LinkStateReady → `IOC_LinkStateReady`
- LinkStateBusyCbProcEvt → `IOC_LinkStateBusyCbProcEvt`
- LinkStateBusySubEvt → `IOC_LinkStateBusySubEvt`
- LinkStateBusyUnsubEvt → `IOC_LinkStateBusyUnsubEvt`

| State | Description | Entry | Exit | Operations |
|-------|-------------|-------|------|------------|
| **LinkStateReady** | Ready for subscription operations | Init or operation complete | subEVT/unsubEVT/event received | IOC_postEVT(), IOC_subEVT(), IOC_unsubEVT() |
| **LinkStateBusyCbProcEvt** | Processing event in callback | CbProcEvt_F invoked | Callback returns | Handle event |
| **LinkStateBusySubEvt** | Establishing subscription | subEVT called | Subscription complete | Register callback/interest |
| **LinkStateBusyUnsubEvt** | Removing subscription | unsubEVT called | Unsubscription complete | Deregister callback/interest |

### Key Characteristics

1. **No Connection Required**: Events posted to AutoLinkID without prior connection
2. **Broadcast/Multicast**: One-to-many event delivery to all subscribers
3. **Flexible Subscription**: Three reception modes supported:
   - **Mandatory Callback**: IOC_subEVT(EvtID, CbProcEvt_F) - callback required
   - **Optional Polling**: IOC_waitEVT(AutoLinkID, EvtID, EvtDesc) - subEVT optional
   - **Hybrid**: IOC_subEVT(EvtID, NULL) + IOC_waitEVT() - optional subscription
4. **PostEvt Anytime**: Can post events in any state (Ready/BusyXXX)
5. **Simpler State Machine**: No publisher/subscriber separation like ConetMode

## Event Processing Sequence Diagrams

### Pattern 1: ConetMode Callback Reception

```mermaid
sequenceDiagram
    participant Pub as Publisher (ObjX)
    participant IOC_P as IOC (Publisher)
    participant IOC_S as IOC (Subscriber)
    participant Sub as Subscriber (ObjY)
    
    Note over Pub,Sub: ConetMode Event - Callback Mode
    
    Note over Pub,Sub: Link Already Established (LinkID exists)
    
    Pub->>Pub: IOC_EvtDesc_initVar(&evtDesc)
    
    Pub->>IOC_P: IOC_postEVT(LinkID, EvtID, &evtDesc)
    Note right of Pub: Non-blocking return
    
    IOC_P->>IOC_P: Queue Event
    IOC_P-->>Pub: Return IOC_RESULT_SUCCESS
    
    IOC_P->>IOC_S: Route Event to Subscriber
    
    IOC_S->>Sub: CbProcEvt_F(LinkID, EvtID, &evtDesc)
    Note right of Sub: Process event notification
    
    Sub->>Sub: Handle Event
    Sub-->>IOC_S: Callback Complete
    
    Note over Pub: Publisher continues<br/>without waiting
```

### Pattern 2: ConetMode Polling Reception

```mermaid
sequenceDiagram
    participant Pub as Publisher (ObjX)
    participant IOC_P as IOC (Publisher)
    participant IOC_S as IOC (Subscriber)
    participant Sub as Subscriber (ObjY)
    
    Note over Pub,Sub: ConetMode Event - Polling Mode
    
    Note over Pub,Sub: Link Already Established (LinkID exists)
    
    Sub->>IOC_S: IOC_waitEVT(LinkID, &EvtID, &evtDesc)
    Note right of Sub: Blocking wait for events
    
    Pub->>Pub: IOC_EvtDesc_initVar(&evtDesc)
    
    Pub->>IOC_P: IOC_postEVT(LinkID, EvtID, &evtDesc)
    Note right of Pub: Non-blocking return
    
    IOC_P->>IOC_P: Queue Event
    IOC_P-->>Pub: Return IOC_RESULT_SUCCESS
    
    IOC_P->>IOC_S: Route Event to Subscriber
    
    IOC_S-->>Sub: IOC_waitEVT returns with event
    Note right of Sub: Event received
    
    Sub->>Sub: Handle Event
    
    Sub->>IOC_S: IOC_waitEVT() again for next event
```

### Pattern 3: ConlesMode Subscription Callback

```mermaid
sequenceDiagram
    participant Pub as Publisher (ObjX)
    participant IOC as IOC Framework
    participant Sub1 as Subscriber 1 (ObjY)
    participant Sub2 as Subscriber 2 (ObjZ)
    
    Note over Pub,Sub2: ConlesMode Event - Subscription Callback (MANDATORY)
    
    Note over Sub1,Sub2: No Connection Setup Required
    
    Sub1->>IOC: IOC_subEVT(EvtID, CbProcEvt_F)
    Note right of Sub1: Must provide callback function
    IOC-->>Sub1: Subscription Registered
    
    Sub2->>IOC: IOC_subEVT(EvtID, CbProcEvt_F)
    Note right of Sub2: Must provide callback function
    IOC-->>Sub2: Subscription Registered
    
    Pub->>Pub: IOC_EvtDesc_initVar(&evtDesc)
    
    Pub->>IOC: IOC_postEVT(AutoLinkID, EvtID, &evtDesc)
    Note right of Pub: Broadcast to all subscribers
    IOC-->>Pub: Return IOC_RESULT_SUCCESS
    
    par Event Delivery to All Subscribers
        IOC->>Sub1: CbProcEvt_F(AutoLinkID, EvtID, &evtDesc)
        Sub1->>Sub1: Handle Event
        Sub1-->>IOC: Callback Complete
    and
        IOC->>Sub2: CbProcEvt_F(AutoLinkID, EvtID, &evtDesc)
        Sub2->>Sub2: Handle Event
        Sub2-->>IOC: Callback Complete
    end
```

### Pattern 4: ConlesMode Polling Reception

```mermaid
sequenceDiagram
    participant Pub as Publisher (ObjX)
    participant IOC as IOC Framework
    participant Sub1 as Subscriber 1 (ObjY)
    participant Sub2 as Subscriber 2 (ObjZ)
    
    Note over Pub,Sub2: ConlesMode Event - Polling (subEVT OPTIONAL)
    
    Note over Sub1,Sub2: No Connection Setup Required
    
    Sub1->>IOC: IOC_subEVT(EvtID, NULL)
    Note right of Sub1: Optional: Register interest<br/>No callback provided
    IOC-->>Sub1: Subscription Registered
    
    Sub1->>IOC: IOC_waitEVT(AutoLinkID, &EvtID, &evtDesc)
    Note right of Sub1: Blocking wait
    
    Sub2->>IOC: IOC_waitEVT(AutoLinkID, &EvtID, &evtDesc)
    Note right of Sub2: Direct wait<br/>No prior subEVT needed
    
    Pub->>Pub: IOC_EvtDesc_initVar(&evtDesc)
    
    Pub->>IOC: IOC_postEVT(AutoLinkID, EvtID, &evtDesc)
    Note right of Pub: Broadcast to all waiters
    IOC-->>Pub: Return IOC_RESULT_SUCCESS
    
    par Event Delivery to Waiting Subscribers
        IOC-->>Sub1: IOC_waitEVT returns with event
        Sub1->>Sub1: Handle Event
        Sub1->>IOC: IOC_waitEVT() again
    and
        IOC-->>Sub2: IOC_waitEVT returns with event
        Sub2->>Sub2: Handle Event
        Sub2->>IOC: IOC_waitEVT() again
    end
```

---

# Data State Machine

**Pattern**: **Stream-based Transfer** - DatSender transmits data chunks asynchronously, DatReceiver processes them via callback or polling.

**Key Characteristics** (from UserGuide_DAT.md):
- **NODROP Guarantee**: Reliable data delivery with no data loss (like CMD, unlike fire-and-forget EVT)
- **Stream-based**: Continuous data streaming with multiple chunks (not single-shot like CMD)
- **Asynchronous Send**: `IOC_sendDAT()` queues data and returns immediately (may block on flow control if buffer full)
- **Flexible Reception**: Callback-based (`CbRecvDat_F` automatic) or polling-based (`IOC_recvDAT()` manual)
- **Two Service Patterns** (User Stories from UserGuide_DAT.md):
  - **US-1: DatReceiver Service**: Service receives data, clients send (e.g., file upload, log collection, data ingestion)
  - **US-2: DatSender Service**: Service sends data, clients receive (e.g., video streaming, data broadcast, live feeds)
- **ConetMode ONLY**: Requires established link for reliable transfer and flow control
- **Bidirectional**: Single link can have both Sender and Receiver roles for full-duplex streaming
- **Flow Control**: May block on `IOC_sendDAT()` if receiver buffer full (prevents data loss via backpressure)
- **Flush Support**: `IOC_flushDAT()` ensures all queued chunks transmitted immediately

## Link-Level Data States (DAT::Conet) ✅ IMPLEMENTED

**Implementation**: `IOC_LinkSubState_T` enum in `Include/IOC/IOC_Types.h`

Data streaming operates in **ConetMode** (connection-oriented) with **Sender-Receiver** pattern. Each link maintains independent **Sender** and **Receiver** sub-state machines for concurrent bidirectional streaming.

### Link Usage Types

- **IOC_LinkUsageDatSender**: Link configured to send data streams via `IOC_sendDAT()`
  - Corresponds to **Sender** role in state machine
  - Sends data chunks with flow control
  - States: DataSenderReady ↔ DataSenderBusySendDat

- **IOC_LinkUsageDatReceiver**: Link configured to receive data streams via callback or polling
  - Corresponds to **Receiver** role in state machine
  - Receives and processes data chunks
  - States: DataReceiverReady → DataReceiverBusyRecvDat (polling) or DataReceiverBusyCbRecvDat (callback)

**Note**: A single link can have both usage types enabled for full-duplex bidirectional streaming.

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

### Link Data State Descriptions

**Implementation Note**: These logical states map to `IOC_LinkSubState_T` enum values in `IOC_Types.h`:
- DataSenderReady → `IOC_LinkSubStateDatSenderReady` ✅
- DataSenderBusySendDat → `IOC_LinkSubStateDatSenderBusySendDat` ✅
- DataReceiverReady → `IOC_LinkSubStateDatReceiverReady` ✅
- DataReceiverBusyRecvDat → `IOC_LinkSubStateDatReceiverBusyRecvDat` ✅
- DataReceiverBusyCbRecvDat → `IOC_LinkSubStateDatReceiverBusyCbRecvDat` ✅

| State | Status | Enum Value | Role | Description | Entry | Exit | Operations |
|-------|--------|-----------|------|-------------|-------|------|------------|
| **DataSenderReady** | ✅ | `IOC_LinkSubStateDatSenderReady` | Sender | Ready to send data | Init or send complete | sendDAT called | IOC_sendDAT(), IOC_flushDAT() |
| **DataSenderBusySendDat** | ✅ | `IOC_LinkSubStateDatSenderBusySendDat` | Sender | Sending data chunk | sendDAT called | Data queued/sent | Buffer operation |
| **DataReceiverReady** | ✅ | `IOC_LinkSubStateDatReceiverReady` | Receiver | Ready to receive data | Init or receive complete | recvDAT called or data arrives | IOC_recvDAT() |
| **DataReceiverBusyRecvDat** | ✅ | `IOC_LinkSubStateDatReceiverBusyRecvDat` | Receiver | Receiving data (polling) | recvDAT called | Data received | Block until data |
| **DataReceiverBusyCbRecvDat** | ✅ | `IOC_LinkSubStateDatReceiverBusyCbRecvDat` | Receiver | Processing data (callback) | CbRecvDat_F invoked | Callback returns | Handle data chunk |

### Design Principles

1. **Composite State Machine**: Hierarchical states with independent sub-state machines
2. **Concurrent Operations**: Allow simultaneous data sending and receiving
3. **Asynchronous Sending**: Data sending is non-blocking (default MAYBLOCK with flow control)
4. **Stream Semantics**: ALWAYS NODROP - maintains data integrity and order
5. **Dual Reception Modes**: Support both callback and polling reception patterns
6. **Role-Specific Configuration**: Different buffer sizes, flow control policies
7. **State Isolation**: Prevent state interference between sender and receiver roles

### Key Advantages

1. **Reliable Streaming**: NODROP guarantee ensures no data loss
2. **Flow Control**: Automatic backpressure prevents buffer overflow
3. **Clear Semantics**: Each role has well-defined state transitions
4. **Flexible Reception**: Support both push (callback) and pull (polling) patterns
5. **Bidirectional**: Full-duplex streaming on same link
6. **Easy Testing**: Independent state machines are easier to unit test
7. **Minimalist Design**: Auto-initialization, only flushDAT() needed for control

### DAT vs CMD vs EVT Key Differences

| Aspect | DAT | EVT | CMD |
|--------|-----|-----|-----|
| **Data Type** | STREAM (continuous) | DGRAM (discrete) | DGRAM (discrete) |
| **Synchronization** | ASYNC (always) | ASYNC (always) | SYNC (always) |
| **Response** | Flow control acknowledgment | Fire-and-forget | Request-response |
| **Reliability** | NODROP (always) | MAYDROP (default) | NODROP (default) |
| **Blocking** | MAYBLOCK (default) | NONBLOCK (default) | MAYBLOCK (default) |
| **Communication** | Point-to-Point | Point-to-Point + Broadcast | Point-to-Point |
| **Buffer Management** | Complex with flow control | Simple queuing | Minimal buffering |
| **Ordering** | Maintains order | No guarantee | Single request ordering |

## Data Streaming Sequence Diagrams

### Pattern 1: Callback Reception Mode

```mermaid
sequenceDiagram
    participant Send as Sender (ObjX)
    participant IOC_S as IOC (Sender)
    participant IOC_R as IOC (Receiver)
    participant Recv as Receiver (ObjY)
    
    Note over Send,Recv: Data Streaming - Callback Mode
    
    Note over Send,Recv: Link Already Established (LinkID exists)
    
    Send->>Send: Prepare Data Chunks
    
    loop For Each Data Chunk
        Send->>IOC_S: IOC_sendDAT(LinkID, chunk)
        Note right of Send: Auto-initialization on first call
        
        IOC_S->>IOC_S: Buffer Data
        IOC_S-->>Send: Return (may block if buffer full)
        
        IOC_S->>IOC_R: Transmit Chunk
        
        IOC_R->>Recv: CbRecvDat_F(LinkID, chunk)
        Note right of Recv: Process chunk
        
        Recv->>Recv: Handle Data Chunk
        Recv-->>IOC_R: Callback Complete
        
        IOC_R->>IOC_S: Flow Control Feedback
    end
    
    Send->>IOC_S: IOC_flushDAT(LinkID)
    Note right of Send: Force transmission of buffered data
    IOC_S->>IOC_R: Flush Buffer
    IOC_S-->>Send: Flush Complete
    
    Note over Send,Recv: Stream auto-closes when LinkID closes
```

### Pattern 2: Polling Reception Mode

```mermaid
sequenceDiagram
    participant Send as Sender (ObjX)
    participant IOC_S as IOC (Sender)
    participant IOC_R as IOC (Receiver)
    participant Recv as Receiver (ObjY)
    
    Note over Send,Recv: Data Streaming - Polling Mode
    
    Note over Send,Recv: Link Already Established (LinkID exists)
    
    Recv->>IOC_R: IOC_recvDAT(LinkID, &chunk)
    Note right of Recv: Blocking wait for data
    
    Send->>Send: Prepare Data Chunks
    
    loop For Each Data Chunk
        Send->>IOC_S: IOC_sendDAT(LinkID, chunk)
        Note right of Send: Auto-initialization on first call
        
        IOC_S->>IOC_S: Buffer Data
        IOC_S-->>Send: Return (may block if buffer full)
        
        IOC_S->>IOC_R: Transmit Chunk
        
        IOC_R-->>Recv: IOC_recvDAT returns with chunk
        Note right of Recv: Data received
        
        Recv->>Recv: Handle Data Chunk
        
        Recv->>IOC_R: IOC_recvDAT(LinkID, &chunk)
        Note right of Recv: Wait for next chunk
        
        IOC_R->>IOC_S: Flow Control Feedback
    end
    
    Send->>IOC_S: IOC_flushDAT(LinkID)
    Note right of Send: Force transmission
    IOC_S->>IOC_R: Flush Buffer
    IOC_S-->>Send: Flush Complete
    
    IOC_R-->>Recv: IOC_recvDAT returns final chunk
```

### Pattern 3: Bidirectional Streaming

```mermaid
sequenceDiagram
    participant ObjA as Object A
    participant IOC_A as IOC (A)
    participant IOC_B as IOC (B)
    participant ObjB as Object B
    
    Note over ObjA,ObjB: Bidirectional Data Streaming
    
    Note over ObjA,ObjB: Link Established (LinkID exists on both sides)
    
    par A to B Streaming
        loop ObjA sends data to ObjB
            ObjA->>IOC_A: IOC_sendDAT(LinkID, chunkA)
            IOC_A->>IOC_B: Transmit chunkA
            IOC_B->>ObjB: CbRecvDat_F(LinkID, chunkA)
            ObjB->>ObjB: Process chunkA
            ObjB-->>IOC_B: Callback Complete
            IOC_B->>IOC_A: Flow Control Feedback
        end
    and B to A Streaming
        loop ObjB sends data to ObjA
            ObjB->>IOC_B: IOC_sendDAT(LinkID, chunkB)
            IOC_B->>IOC_A: Transmit chunkB
            IOC_A->>ObjA: CbRecvDat_F(LinkID, chunkB)
            ObjA->>ObjA: Process chunkB
            ObjA-->>IOC_A: Callback Complete
            IOC_A->>IOC_B: Flow Control Feedback
        end
    end
    
    Note over ObjA,ObjB: Both directions stream independently<br/>with separate flow control
```

### Pattern 4: Flow Control and Backpressure

```mermaid
sequenceDiagram
    participant Send as Sender
    participant IOC_S as IOC (Sender)
    participant IOC_R as IOC (Receiver)
    participant Recv as Receiver
    
    Note over Send,Recv: Flow Control Example
    
    loop Fast Sending
        Send->>IOC_S: IOC_sendDAT(LinkID, chunk)
        IOC_S->>IOC_S: Buffer Filling Up
        IOC_S-->>Send: Return (non-blocking)
    end
    
    IOC_S->>IOC_S: Buffer Full
    
    Send->>IOC_S: IOC_sendDAT(LinkID, chunk)
    Note right of Send: Next send blocked<br/>(MAYBLOCK default)
    
    IOC_S->>IOC_R: Transmit Buffered Data
    IOC_R->>Recv: CbRecvDat_F(LinkID, chunk)
    Recv->>Recv: Slow Processing...
    
    IOC_R->>IOC_R: Receiver Processing
    
    Recv-->>IOC_R: Callback Complete
    IOC_R->>IOC_S: Buffer Space Available
    
    IOC_S-->>Send: sendDAT unblocked
    Note right of Send: Sender resumes
    
    Send->>IOC_S: Continue sending
```

---

# State Machine Integration Summary

## Component State Hierarchy

```
IOC Framework
├── Service States (Online/Offline lifecycle)
│   └── Manages multiple Links
│
├── Link States (Connection lifecycle)
│   ├── Conet Links (Connection-oriented)
│   │   ├── Command Sub-States (Initiator + Executor)
│   │   ├── Event Sub-States (Publisher + Subscriber)
│   │   └── Data Sub-States (Sender + Receiver)
│   │
│   └── Conles Links (Connectionless - EVT only)
│       └── Event Sub-States (Subscription management)
│
└── Individual Message States
    ├── Individual Command States (INITIALIZED → SUCCESS/FAILED/TIMEOUT)
    └── Individual Event States (Queued → Delivered/Dropped)
```

## State Machine Implementation Reference

### Header File Mapping

All state machine definitions are implemented in IOC header files:

**Command States** (`IOC_CmdDesc.h`):
```c
typedef enum {
    IOC_CMD_STATUS_INVALID = 0,
    IOC_CMD_STATUS_INITIALIZED = 1,
    IOC_CMD_STATUS_PENDING = 2,
    IOC_CMD_STATUS_PROCESSING = 3,
    IOC_CMD_STATUS_SUCCESS = 4,
    IOC_CMD_STATUS_FAILED = 5,
    IOC_CMD_STATUS_TIMEOUT = 6,
} IOC_CmdStatus_E;
```

**Link States** (`IOC_Types.h`):
```c
// Main States (ConlesMode events)
typedef enum {
    IOC_LinkStateUndefined = 0,
    IOC_LinkStateReady = 1,
    IOC_LinkStateBusyCbProcEvt,    // ConlesMode event processing
    IOC_LinkStateBusySubEvt,       // ConlesMode subscription
    IOC_LinkStateBusyUnsubEvt,     // ConlesMode unsubscription
} IOC_LinkState_T;

// Sub-States (ConetMode CMD/DAT operations)
typedef enum {
    IOC_LinkSubStateDefault = 0,
    // DAT states
    IOC_LinkSubStateDatSenderReady,
    IOC_LinkSubStateDatSenderBusySendDat,
    IOC_LinkSubStateDatReceiverReady,
    IOC_LinkSubStateDatReceiverBusyRecvDat,
    IOC_LinkSubStateDatReceiverBusyCbRecvDat,
    // CMD states
    IOC_LinkSubStateCmdInitiatorReady,
    IOC_LinkSubStateCmdInitiatorBusyExecCmd,
    IOC_LinkSubStateCmdExecutorReady,
    IOC_LinkSubStateCmdExecutorBusyExecCmd,
    IOC_LinkSubStateCmdExecutorBusyWaitCmd,
} IOC_LinkSubState_T;
```

**Key Implementation Details**:
- **ConetMode Events**: No dedicated sub-states; use `IOC_LinkStateReady` with default sub-state
- **ConlesMode Events**: Use main `IOC_LinkState_T` states (BusyCbProcEvt, BusySubEvt, BusyUnsubEvt)
- **CMD/DAT**: Use `IOC_LinkSubState_T` sub-states while main state remains `IOC_LinkStateReady`

## State Machine Principles

1. **Hierarchical Organization**: Clear parent-child relationships between state machines
2. **Independent Lifecycles**: Each level maintains state independently
3. **Concurrent Operations**: Multiple state machines operate simultaneously
4. **Clear Boundaries**: Well-defined state ownership and transition rules
5. **Error Isolation**: Failures in one state machine don't cascade to others
6. **Testability**: Each state machine can be tested in isolation
7. **Extensibility**: New states/transitions can be added without breaking existing ones

## Link Usage Configuration

### IOC_LinkUsage_T Enumeration

```c
typedef enum {
    IOC_LinkUsageEvtProducer  = 0x01,  // Link can post events
    IOC_LinkUsageEvtConsumer  = 0x02,  // Link can receive events
    IOC_LinkUsageCmdInitiator = 0x04,  // Link can initiate commands
    IOC_LinkUsageCmdExecutor  = 0x08,  // Link can execute commands
    IOC_LinkUsageDatSender    = 0x10,  // Link can send data streams
    IOC_LinkUsageDatReceiver  = 0x20,  // Link can receive data streams
} IOC_LinkUsage_T;
```

### Link Usage Configuration Examples

| Scenario | Link Usage Configuration | State Machines Enabled |
|----------|-------------------------|------------------------|
| **Command Client** | `IOC_LinkUsageCmdInitiator` | InitiatorReady ↔ InitiatorBusyExecCmd |
| **Command Server** | `IOC_LinkUsageCmdExecutor` | ExecutorReady → ExecutorBusyCbExecCmd (callback) or ExecutorBusyWaitCmd (polling) |
| **Bidirectional CMD** | `IOC_LinkUsageCmdInitiator \| IOC_LinkUsageCmdExecutor` | Both Initiator and Executor sub-states |
| **Event Publisher** | `IOC_LinkUsageEvtProducer` | EventPublisherReady ↔ EventPublisherBusyPostEvt |
| **Event Subscriber** | `IOC_LinkUsageEvtConsumer` | EventSubscriberReady → EventSubscriberBusy* |
| **Event Pub-Sub** | `IOC_LinkUsageEvtProducer \| IOC_LinkUsageEvtConsumer` | Both Publisher and Subscriber sub-states |
| **Data Streaming Client** | `IOC_LinkUsageDatSender` | DataSenderReady ↔ DataSenderBusySendDat |
| **Data Streaming Server** | `IOC_LinkUsageDatReceiver` | DataReceiverReady → DataReceiverBusy* |
| **Full-Duplex Data** | `IOC_LinkUsageDatSender \| IOC_LinkUsageDatReceiver` | Both Sender and Receiver sub-states |
| **Full Capabilities** | All flags combined | All sub-state machines enabled |

### Usage Configuration APIs

```c
// Service side: Specify supported usage capabilities and behavior flags
IOC_Result_T IOC_onlineService(
    IOC_SrvID_T* pSrvID,
    IOC_SrvArgs_T* pSrvArgs  // Contains: SrvURI, Flags, UsageCapabilities, UsageArgs, OnAutoAccepted_F
);

// Service side: Complete configuration structure
typedef struct {
    IOC_SrvURI_T SrvURI;                      // Service URI (protocol://host:port/path)
    IOC_SrvFlags_T Flags;                     // Behavior flags (AUTO_ACCEPT, KEEP_ACCEPTED_LINK, BROADCAST_EVENT)
    IOC_LinkUsage_T UsageCapabilities;        // Bitwise OR of supported usages
    struct {
        IOC_EvtUsageArgs_pT pEvt;             // Event usage arguments
        IOC_CmdUsageArgs_pT pCmd;             // Command usage arguments
        IOC_DatUsageArgs_pT pDat;             // Data usage arguments
    } UsageArgs;
    IOC_CbOnAutoAccepted_F OnAutoAccepted_F;  // Optional callback for auto-accepted links
    void* pSrvPriv;                           // Service private context
} IOC_SrvArgs_T;

// Client side: Specify required usage
IOC_Result_T IOC_connectService(
    IOC_LinkID_T* pLinkID,
    IOC_ConnArgs_T* pConnArgs  // Contains: SrvURI, Usage, UsageArgs
);

// Client side: Complete configuration structure
typedef struct {
    IOC_SrvURI_T SrvURI;           // Service URI to connect to
    IOC_LinkUsage_T Usage;         // Bitwise OR of required usages
    union {
        IOC_EvtUsageArgs_pT pEvt;  // Event usage arguments
        IOC_CmdUsageArgs_pT pCmd;  // Command usage arguments
        IOC_DatUsageArgs_pT pDat;  // Data usage arguments
    } UsageArgs;
} IOC_ConnArgs_T;

// Query link usage configuration
IOC_Result_T IOC_getLinkUsage(
    IOC_LinkID_T LinkID,
    IOC_LinkUsage_T* pUsage
);

// Query accepted links (useful with AUTO_ACCEPT)
IOC_Result_T IOC_getServiceLinkIDs(
    IOC_SrvID_T SrvID,
    IOC_LinkID_T* pLinkIDs,
    size_t* pLinkCount
);
```

## State Query APIs

**Purpose**: Query runtime state for monitoring, debugging, error handling, and operational decisions.

**Cross-Reference**: See User Guides for complete API documentation and usage examples:
- [UserGuide_SRV.md](Doc/UserGuide_SRV.md) - Service lifecycle and link management APIs
- [UserGuide_CMD.md](Doc/UserGuide_CMD.md) - Command execution patterns and timeout handling  
- [UserGuide_EVT.md](Doc/UserGuide_EVT.md) - Event subscription and processing patterns
- [UserGuide_DAT.md](Doc/UserGuide_DAT.md) - Data transmission and flow control patterns

| Component | Query API | Returns | Status | Header | When to Use |
|-----------|-----------|---------|--------|--------|-------------|
| **Service Lifecycle** | `IOC_getSrvState(SrvID, &state)` | `IOC_SrvState_T` | ✅ IMPLEMENTED | IOC_SrvTypes.h | Check if service is online before connecting |
| **Link Connection** | `IOC_getLinkConnState(LinkID, &state)` | `IOC_LinkConnState_T` | ✅ IMPLEMENTED | IOC_Types.h | Detect network issues (ConetMode) |
| **Link Operation** | `IOC_getLinkState(LinkID, &state)` | `IOC_LinkState_T` | 🔴 TODO | IOC_Types.h | Check if link is Ready vs Busy |
| **Link Detail** | `IOC_getLinkSubState(LinkID, &substate)` | `IOC_LinkSubState_T` | 🔴 TODO | IOC_Types.h | Debug what operation is blocking |
| **Command Status** | `IOC_CmdDesc_getStatus(pCmdDesc)` | `IOC_CmdStatus_E` | ✅ IMPLEMENTED | IOC_CmdDesc.h | Check command result after `IOC_execCMD()` |
| **Link Usage** | `IOC_getLinkUsage(LinkID, &usage)` | `IOC_LinkUsage_T` | 🔴 TODO | IOC_Types.h | Verify link capabilities |

**Legend**:
- ✅ **IMPLEMENTED**: API declared in header, state enum defined, can be called in code
- 🔴 **TODO**: API declared but implementation pending
- 📐 **CONCEPTUAL**: Logical state for documentation only, no runtime enum exists

## Testing Strategy

### Unit Testing per Component

1. **Service State Tests**: Test service lifecycle transitions
2. **Link State Tests**: Test connection establishment and breakdown
3. **Command State Tests**: Test individual command state machine (US-1 ACs)
4. **Event State Tests**: Test event delivery patterns
5. **Data State Tests**: Test streaming and flow control

### Integration Testing

1. **Service-Link Integration**: Test service accepting multiple clients
2. **Link-Command Integration**: Test concurrent commands on same link
3. **Link-Event Integration**: Test event broadcast/multicast patterns
4. **Link-Data Integration**: Test bidirectional streaming
5. **Multi-Protocol Integration**: Test TCP/UDP/FIFO protocol state consistency

### State Invariant Verification

1. **Terminal State Immutability**: SUCCESS/FAILED/TIMEOUT states never change
2. **State Transition Atomicity**: State changes are atomic and thread-safe
3. **State Consistency**: Link states correlate with individual message states
4. **Timeout Enforcement**: Timeouts properly detected and state updated
5. **Error Context Preservation**: Failed states preserve diagnostic information

---

# References

- **Architecture Design**: [README_ArchDesign.md](./README_ArchDesign.md)
- **User Guide**: [README_UserGuide.md](./README_UserGuide.md)
- **Specification**: [README_Specification.md](./README_Specification.md)
- **Glossary**: [README_Glossary.md](./README_Glossary.md)
- **Use Cases**: [README_UseCase.md](./README_UseCase.md)

---

# Revision History

| Version | Date | Author | Description |
|---------|------|--------|-------------|
| 1.0 | 2025-01-30 | IOC Team | Initial state machine documentation extracted from README_ArchDesign.md |
| 1.0 | 2025-01-30 | IOC Team | Added comprehensive sequence diagrams for all patterns |
| 1.0 | 2025-01-30 | IOC Team | Added state correlation analysis and testing strategy |
