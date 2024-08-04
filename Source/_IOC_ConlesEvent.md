# About

* This file contains diagrams for ConlesEvent submodule design.

# DataType(Class) Diagram

```mermaid
classDiagram
    class _IOC_ClsEvtLinkObj_T{
        +EvtDescQueue
        +EvtSuberList
        +EvtProcThread

        subEvt()
        unsubEvt()
        postEvt()
    }
```

# FlowChart Diagram

## _IOC_postEVT_inConlesMode

```mermaid
flowchart TD
    _IOC_postEVT_inConlesMode --> AutoLinkObj{ IsAutoLinkObjExist? }
    AutoLinkObj -- NotExist --> IOC_RESULT_INVALID_AUTO_LINK_ID
    AutoLinkObj == Exist ==> EvtSuber{ IsAnyEvtSuber? }
    EvtSuber -- No --> IOC_RESULT_NO_EVENT_CONSUMER
    EvtSuber == Yes ==> OptMode{ SyncOrAsync? }

    OptMode -- Sync --> IsEvtDescQueueEmpty{ IsEvtDescQueueEmpty? }
    IsEvtDescQueueEmpty -- Yes --> cbProcEvtDirectly
    IsEvtDescQueueEmpty -- No --> OptWaitSync{ SyncMayBlock? }
    OptWaitSync -- MayBlock --> waitEvtDescQueueEmpty --> cbProcEvtDirectly --> IOC_RESULT_SUCCESS
    OptWaitSync --> SyncWaitMode{ NonBlockOrTimeout } 
    SyncWaitMode --> SyncWaitTimeout{Timeout} 
    SyncWaitTimeout -- No --> cbProcEvtDirectly
    SyncWaitTimeout -- Yes --> IOC_RESULT_TOO_LONG_EMPTYING_EVTDESC_QUEUE
    SyncWaitMode --> SyncNonBlock --> IOC_RESULT_TOO_LONG_EMPTYING_EVTDESC_QUEUE
    
    OptMode == Async ==> EvtDescQueue{ IsEvtDescQueueFull? }
    EvtDescQueue == No ==> enqueueEvtDescQueue
    EvtDescQueue -- Yes --> OptWaitASync{ AsyncMayBlock? }
    OptWaitASync -- MayBlock --> waitEvtDescQueueSpace --> enqueueEvtDescQueue ===> IOC_RESULT_SUCCESS
    OptWaitASync --> ASyncWaitMode{ NonBlockOrTimeout } 
    ASyncWaitMode --> ASyncWaitTimeout{Timeout}
    ASyncWaitTimeout -- No --> enqueueEvtDescQueue
    ASyncWaitTimeout -- Yes --> IOC_RESULT_TOO_MANY_QUEUING_EVTDESC
    ASyncWaitMode --> AsyncNonBlock --> IOC_RESULT_TOO_MANY_QUEUING_EVTDESC

```
