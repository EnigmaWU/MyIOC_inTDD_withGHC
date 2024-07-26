# About

* This is design document for IOC's UT_ConlesEventState.
* The architectural design section of Conles Event State is in [README_ArchDesign](../README_ArchDesign.md).

# Flowchart

## Case-04
  
```mermaid
sequenceDiagram
    participant UTB as UT_Body
    participant IOC as IOC::AutoLink
    participant EvtSuber

    UTB->>IOC: postEVT(TEST_SLEEP_99MS)
    IOC->>EvtSuber: enterCbProcEvt->StateBusyCbProcEvt

    EvtSuber -->> UTB: notifyEnterCbProcEvt

    activate EvtSuber
    EvtSuber->>EvtSuber: usleep(99000)

    UTB ->> IOC: EvtSuber::unsubEVT(…)
    activate IOC
    IOC -->> IOC: BLOCKED::waitStateReady

    EvtSuber -->> IOC: StateReady <- leaveCbProcEvt
    deactivate EvtSuber

    IOC -->> UTB: unsubSucceed(…)
    deactivate IOC
```

## Case-05

```mermaid
sequenceDiagram
    participant UTB as UT_Body
    participant IOC as IOC::AutoLink
    participant No1Suber as No1EvtSuber

    UTB->>IOC: postEVT(TEST_SLEEP_99MS)
    IOC->>No1Suber: enterCbProcEvt->StateBusyCbProcEvt

    No1Suber -->> UTB: notifyEnterCbProcEvt

    activate No1Suber
    No1Suber->>No1Suber: usleep(99000)

    UTB ->> IOC: No2EvtSuber::subEVT(TEST_KEEPALIVE)
    activate IOC
    IOC -->> IOC: BLOCKED::waitStateReady

    No1Suber -->> IOC: StateReady <- leaveCbProcEvt
    deactivate No1Suber

    IOC -->> UTB: subSucceed(TEST_KEEPALIVE)
    deactivate IOC

```
