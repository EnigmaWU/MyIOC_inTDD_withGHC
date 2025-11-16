# IOC Framework User Guide

**Welcome to IOC (Inter-Object Communication) Framework!** 🎉

This is your main entrance to IOC documentation. As a **USER**, this guide will help you quickly navigate to the right resources.

---

## 📖 Documentation Navigation

Choose your path based on your needs:

### 🚀 Quick Start
- **New to IOC?** → Start with [Quick Start (5 Minutes)](#quick-start-5-minutes)
- **Simple module communication?** → Use [ConlesMode Quick Example](#conlesmode-quick-example)
- **Cross-process communication?** → Check [ConetMode Overview](#conetmode-overview)

### 📚 Detailed User Guides
Based on your communication pattern, select the appropriate guide:

| Guide | Purpose | When to Use |
|-------|---------|-------------|
| **[UserGuide_SRV](UserGuide_SRV.md)** | Service Management | • Service lifecycle<br/>• Connection management<br/>• Auto-accept patterns<br/>• Broadcast capabilities |
| **[UserGuide_EVT](UserGuide_EVT.md)** | Event Handling | • Publish-subscribe events<br/>• ConlesMode & ConetMode<br/>• Event filtering<br/>• Callback vs polling |
| **[UserGuide_CMD](UserGuide_CMD.md)** | Command Execution | • Request-response commands<br/>• Synchronous control<br/>• Guaranteed results<br/>• Timeout handling |
| **[UserGuide_DAT](UserGuide_DAT.md)** | Data Transmission | • Stream-based data transfer<br/>• Large data chunks<br/>• NODROP guarantee<br/>• Bidirectional patterns |

### 🔍 Reference Materials
- **[API Reference Manual](README_RefAPIs.md)** - Complete API documentation
- **[Architecture Design](README_ArchDesign.md)** - System architecture and design principles
- **[Specification](README_Specification.md)** - Technical specifications and requirements
- **[Use Cases](README_UseCase.md)** - Real-world usage scenarios

---

## Quick Start (5 Minutes)

### ConlesMode Quick Example

**ConlesMode** (connection-less) is the simplest way to use IOC, perfect for single-process module-to-module communication.

```c
#include "IOC.h"

// 1. Define event callback
IOC_Result_T MyCallback(IOC_EvtDesc_pT pEvtDesc, void *pCbPriv) {
    printf("Received Event ID: %llu\n", pEvtDesc->EvtID);
    return IOC_RESULT_SUCCESS;
}

int main() {
    // 2. Subscribe to events
    IOC_EvtID_T eventIDs[] = {IOC_EVTID_TEST_KEEPALIVE};
    IOC_SubEvtArgs_T subArgs = {
        .CbProcEvt_F = MyCallback,
        .pCbPrivData = NULL,
        .EvtNum = 1,
        .pEvtIDs = eventIDs
    };
    IOC_subEVT_inConlesMode(&subArgs);
    
    // 3. Post event
    IOC_EvtDesc_T event = {.EvtID = IOC_EVTID_TEST_KEEPALIVE};
    IOC_postEVT_inConlesMode(&event, NULL);
    
    // 4. Process events
    IOC_forceProcEVT();
    
    return 0;
}
```

That's it! 🎉 For more details, see **[UserGuide_EVT](UserGuide_EVT.md)**.

---

## ConetMode Overview

**ConetMode** (connection-oriented) provides service-client based communication with explicit connection management, suitable for cross-process scenarios.

### Basic Service-Client Pattern

```c
// Service Side
IOC_SrvID_T srvID;
IOC_SrvArgs_T srvArgs = {
    .SrvURI = {
        .pProtocol = IOC_SRV_PROTO_FIFO,
        .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
        .pPath = "MyService"
    },
    .UsageCapabilites = IOC_LinkUsageEvtProducer,
    .Flags = IOC_SRVFLAG_AUTO_ACCEPT
};
IOC_onlineService(&srvID, &srvArgs);

// Client Side
IOC_LinkID_T linkID;
IOC_ConnArgs_T connArgs = {
    .SrvURI = srvArgs.SrvURI,
    .Usage = IOC_LinkUsageEvtConsumer
};
IOC_connectService(&linkID, &connArgs, NULL);

// Now client can receive events from service
```

For comprehensive service management patterns, see **[UserGuide_SRV](UserGuide_SRV.md)**.

---

## Use Scenarios

Choose the right version based on your runtime scale:

### 🔹 TinyVersion (xxKB scale)
- **Memory**: < 64KB
- **Best for**: Embedded systems, resource-constrained devices
- **Features**: ConlesMode (connection-less events)
- **Recommendation**: Perfect for simple module coordination

### 🔹 TypicalVersion (xxMB scale)
- **Memory**: < 16MB
- **Best for**: Standard applications, desktop software
- **Features**: ConlesMode + ConetMode (events, commands, data)
- **Recommendations**:
  - Use **ConlesMode** by default for internal module events
  - Use **ConetMode EVT** to avoid unpredictable event latency
  - Use **ConetMode CMD** when you need command execution results
  - Use **ConetMode DAT** for data transfer between components

### 🔹 TitanVersion (xxGB scale)
- **Memory**: < 1GB
- **Best for**: Large-scale systems (e.g., SimuX64)
- **Features**: Full IOC capabilities with high scalability
- **Recommendation**: Enterprise-level distributed systems

---

## Performance Metrics

| Metric | TinyVersion | TypicalVersion | TitanVersion |
|--------|-------------|----------------|--------------|
| Memory Footprint | < 64KB | < 16MB | < 1GB |
| Event Queue Depth | 16 | 4096 | 65536 |
| Max Connections | 4 | 1024 | 16384 |
| Event Latency | < 1ms | < 10ms | < 100ms |
| Throughput (events/sec) | 1K | 100K | 10M |

---

## System Limitations

### ConlesMode Limitations
- ✅ Single-process only
- ✅ Fixed event queue size
- ❌ No event priority support
- ❌ Cannot get event processing results

### ConetMode Limitations
- ⚠️ Requires explicit connection lifecycle management
- ⚠️ Network latency affects performance
- ⚠️ Must handle connection failures
- ⚠️ Memory usage grows with connection count

---

## Performance Optimization Tips

### High-Frequency Events
```c
// ✅ Use non-blocking mode
IOC_Option_defineNonBlock(opt);
IOC_postEVT_inConlesMode(&evt, &opt);
```

### Low-Latency Scenarios
```c
// ✅ Use synchronous mode for immediate processing
IOC_Option_defineSync(opt);
IOC_postEVT_inConlesMode(&evt, &opt);
```

### Large Data Transfer
```c
// ✅ Use pointer mode to avoid copying
IOC_DatDesc_T datDesc = {
    .BufPtr = pLargeData,
    .BufSize = largeSize,
    .Flags = IOC_DATFLAG_PTR_MODE
};
IOC_sendDAT(linkID, &datDesc, NULL);
```

---

## Communication Patterns Quick Reference

### When to Use Each Pattern

| Pattern | User Guide | Use When You Need... |
|---------|-----------|---------------------|
| **Service Management** | [UserGuide_SRV](UserGuide_SRV.md) | • Manage service lifecycle<br/>• Accept multiple clients<br/>• Broadcast to all connected clients |
| **Event (EVT)** | [UserGuide_EVT](UserGuide_EVT.md) | • Loose coupling notifications<br/>• One-to-many communication<br/>• Asynchronous updates |
| **Command (CMD)** | [UserGuide_CMD](UserGuide_CMD.md) | • Synchronous control operations<br/>• Request-response pattern<br/>• Guaranteed execution results |
| **Data (DAT)** | [UserGuide_DAT](UserGuide_DAT.md) | • Continuous data streaming<br/>• Reliable data transfer<br/>• Large payload transmission |

---

## Next Steps

1. **Learn the Basics**: Read the [Quick Start](#quick-start-5-minutes) section above
2. **Choose Your Pattern**: Select from the [Detailed User Guides](#-detailed-user-guides) table
3. **Explore Examples**: Check test files in `Test/UT_*.cxx` for real-world usage
4. **Reference APIs**: Consult [API Reference Manual](README_RefAPIs.md) when needed

---

## Getting Help

- **Common Issues**: See Troubleshooting sections in each user guide
- **API Questions**: Check [README_RefAPIs.md](README_RefAPIs.md)
- **Design Concepts**: Review [README_ArchDesign.md](README_ArchDesign.md)
- **Technical Details**: Refer to [README_Specification.md](README_Specification.md)

---

**Happy coding with IOC Framework!** 🚀
