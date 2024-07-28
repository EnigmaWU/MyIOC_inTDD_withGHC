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

    UTB->>IOC: 【1】postEVT(TEST_SLEEP_99MS)
    IOC->>EvtSuber: 【2】enterCbProcEvt->StateBusyCbProcEvt

    EvtSuber -->> UTB: 【3】notifyEnterCbProcEvt

    activate EvtSuber
    EvtSuber->>EvtSuber: 【4】usleep(99000)

    UTB ->> IOC: 【5】EvtSuber::unsubEVT(TEST_SLEEP_99MS)
    activate IOC
    IOC -->> IOC: 【6】BLOCKED::waitStateReady

    EvtSuber -->> IOC: 【7】StateReady <- leaveCbProcEvt
    deactivate EvtSuber

    IOC -->> UTB: 【8】unsubEvtSucceed(…)
    deactivate IOC
```

## Case-05

```mermaid
sequenceDiagram
    participant UTB as UT_Body
    participant IOC as IOC::AutoLink
    participant No1Suber as No1EvtSuber

    UTB->>IOC: 【1】postEVT(TEST_SLEEP_99MS)
    IOC->>No1Suber: 【2】enterCbProcEvt->StateBusyCbProcEvt

    No1Suber -->> UTB: 【3】notifyEnterCbProcEvt

    activate No1Suber
    No1Suber->>No1Suber: 【4】usleep(99000)

    UTB ->> IOC: 【5】No2EvtSuber::subEVT(TEST_KEEPALIVE)
    activate IOC
    IOC -->> IOC: 【6】BLOCKED::waitStateReady

    No1Suber -->> IOC: 【7】StateReady <- leaveCbProcEvt
    deactivate No1Suber

    IOC -->> UTB: 【8】subEvtSucceed(...)
    deactivate IOC

```
