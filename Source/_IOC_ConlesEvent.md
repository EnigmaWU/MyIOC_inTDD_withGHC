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

```mermaid
flowchart TD
    _IOC_postEVT_inConlesMode --> AutoLinkID{ IsValidAutoLinkID? }
    AutoLinkID -- No --> IOC_RESULT_NOT_EXIST_LINK
    AutoLinkID -- Yes --> getClsEvtLinkObj --> EvtSuber{ IsAnyEvtSuber? }
    EvtSuber -- No --> IOC_RESULT_NO_EVENT_CONSUMER
    EvtSuber -- Yes --> OptMode{ SyncOrAsync? }
    OptMode -- Sync --> waitEvtDescQueueEmpty --> procEvtDirectly --> IOC_RESULT_SUCCESS
    OptMode -- Async --> EvtDescQueue{ IsEvtDescQueueFull? }
    EvtDescQueue -- No --> enqueueEvtDescQueue --> IOC_RESULT_SUCCESS
    EvtDescQueue -- Yes --> OptWait{ MayBlock? }
    OptWait -- MayBlock --> waitEvtDescQueueSpace --> enqueueEvtDescQueue --> IOC_RESULT_SUCCESS
    OptWait -- NonBlockOrTimeout --> IOC_RESULT_TOO_MANY_QUEUING_EVTDESC
```
