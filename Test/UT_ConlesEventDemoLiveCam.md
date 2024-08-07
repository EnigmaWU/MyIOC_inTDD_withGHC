[[_TOC_]]

# UT_ConlesEventDemoLiveCam

* Use this file to write and preview module objects in the UT_ConlesEventDemoLiveCam.

## Data flow of service side module objects

```mermaid
flowchart LR
    VidCapObj -- OriVidFrm --> HiResVidEncObj --HiResVidStrmBits--> HiResStrmMuxObj
    VidCapObj -- OriVidFrm --> VidResizeObj -- RszVidFrm --> LoResVidEncObj -- LoResVidStrmBits --> LoResStrmMuxObj
    AudCapObj -- AudFrm --> AudEncObj -- AudStrmBits --> HiResStrmMuxObj
    AudEncObj -- AudStrmBits --> LoResStrmMuxObj

    HiResStrmMuxObj -- HiResStrmBits --> SrvObj
    LoResStrmMuxObj --LoResStrmBits --> SrvObj
```

## Data flow between service side and client side module objects

```mermaid
flowchart LR
    SrvObj -- HiResStrmBits --> CliObj_ofVIP
    CliObj_ofVIP -- StrmBits --> SrvObj
    SrvObj -- LoResStrmBits --> CliObj
```

## Event sequence flow of service side module objects

### From Biz Viewpoint

```mermaid
sequenceDiagram
    participant VidCapObj
    participant HiResVidEncObj
    participant HiResStrmMuxObj
    participant VidResizeObj
    participant LoResVidEncObj
    participant LoResStrmMuxObj
    participant AudCapObj
    participant AudEncObj
    participant SrvObj
    VidCapObj->>HiResVidEncObj: BizOriVidFrmCapturedEvent
    HiResVidEncObj->>VidCapObj: BizOriVidFrmRecycledEvent
    HiResVidEncObj->>HiResStrmMuxObj: BizHiResVidStrmBitsEncodedEvent
    HiResStrmMuxObj->>HiResVidEncObj: BizHiResVidStrmBitsRecycledEvent

    AudCapObj->>AudEncObj: BizOriAudFrmCapturedEvent
    AudEncObj->>HiResStrmMuxObj: BizAudStrmBitsEncodedEvent

    HiResStrmMuxObj->>SrvObj: BizHiResStrmBitsMuxedEvent
    SrvObj->>HiResStrmMuxObj: BizHiResStrmBitsRecycledEvent
    
    VidCapObj->>VidResizeObj: BizOriVidFrmCapturedEvent
    VidResizeObj->>VidCapObj: BizOriVidFrmRecycledEvent
    VidResizeObj->>LoResVidEncObj: BizLoResVidFrmResizedEvent
    LoResVidEncObj->>VidResizeObj: BizLoResVidFrmRecycledEvent
    LoResVidEncObj->>LoResStrmMuxObj: BizLoResVidStrmBitsEncodedEvent
    
    AudEncObj->>LoResStrmMuxObj: BizAudStrmBitsEncodedEvent

    LoResStrmMuxObj->>SrvObj: BizLoResStrmBitsMuxedEvent
    SrvObj->>LoResStrmMuxObj: BizLoResStrmBitsRecycledEvent
    LoResStrmMuxObj->>LoResVidEncObj: BizLoResVidStrmBitsRecycledEvent
```

### From Management Viewpoint

```mermaid
sequenceDiagram
    participant MgrObj
    participant XyzObj
    
    MgrObj->>XyzObj: ModuleStartEvent
    loop KeepAlive
        XyzObj->>MgrObj: ModuleKeepAliveEvent
    end
    MgrObj->>XyzObj: ModuleStopEvent
```

## Event processing flow of service side module objects

### LoResStrmMuxObj

```mermaid
graph TD
    A -->|EVTID_NOT_SUPPORT| F[BUG]
    A[__Case01_cbProcEvt_LoResStrmMuxObj] -->|EVTID_ModStart| B{IsStateStopped?}
    B -->|Yes| C[setStateRunning]
    C --> D[ModuleStartEventCnt++]
    D --> E[IOC_RESULT_SUCCESS]
    B -->|No| F[BUG]
    
    A -->|EVTID_IS_SUPPORT| G{IsStateRunning?}

    G -->|Yes:EVTID_ModStop| H[setStateStopped]
    H --> I[ModuleStopEventCnt++]
    I --> E[IOC_RESULT_SUCCESS]
    G -->|No| F[BUG]
    
    G -->|Yes:EVTID_BizLoResVidStrmBitsEncoded| M[postBizLoResStrmBitsMuxedEvent]
    M --> N[postBizLoResVidStrmBitsRecycledEvent]
    N --> O[BizLoResVidStrmBitsEncodedEventCnt++\nBizLoResStrmBitsMuxedEventCnt++]
    O --> P[postKeepAliveEvt>=1]
    P --> E[IOC_RESULT_SUCCESS]
    
    G -->|Yes:EVTID_BizAudStrmBitsEncoded| T[BizAudStrmBitsEncodedEventCnt++]
    T --> E[IOC_RESULT_SUCCESS]
    
    G -->|Ye:EVTID_BizLoResStrmBitsRecycled| X[BizLoResStrmBitsRecycledEventCnt++\nBizLoResVidStrmBitsRecycledEventCnt++]
    X --> E[IOC_RESULT_SUCCESS]
    
```

## Event flow between service side and client side module objects

```mermaid
sequenceDiagram
    participant SrvObj
    participant CliObj

    CliObj->>SrvObj: SrvOpenStreamEvent
    alt HiResStrmBits
        loop 
            SrvObj->>CliObj: BizHiResStrmBitsSentEvent
        end
    else LoResStrmBits
        loop 
            SrvObj->>CliObj: BizLoResStrmBitsSentEvent
        end
    end

    CliObj->>SrvObj: CliCloseStreamEvent
```

### TODO: CliObjVIP additional event flow

```mermaid
sequenceDiagram
    participant SrvObj
    participant CliObjVIP

```

## Event flow between client side module objects

```mermaid
sequenceDiagram
    participant CliObjFactory
    participant CliObj

    CliObjFactory->>CliObj: CliStartEvent
    loop KeepAlive
        CliObj->>CliObjFactory: CliKeepAliveEvent
    end
    CliObjFactory->>CliObj: CliStopEvent
```
