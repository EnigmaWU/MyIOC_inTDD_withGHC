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

## Event flow of service side module objects

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
    
    VidCapObj->>VidResizeObj: BizOriVidFrmCapturedEvent
    VidResizeObj->>VidCapObj: BizOriVidFrmRecycledEvent
    VidResizeObj->>LoResVidEncObj: BizLoResVidFrmResizedEvent
    LoResVidEncObj->>VidResizeObj: BizLoResVidFrmRecycledEvent
    LoResVidEncObj->>LoResStrmMuxObj: BizLoResVidStrmBitsEncodedEvent
    LoResStrmMuxObj->>LoResVidEncObj: BizLoResVidStrmBitsRecycledEvent
    
    AudCapObj->>AudEncObj: BizOriAudFrmCapturedEvent
    AudEncObj->>HiResStrmMuxObj: BizAudStrmBitsEncodedEvent
    AudEncObj->>LoResStrmMuxObj: BizAudStrmBitsEncodedEvent

    HiResStrmMuxObj->>SrvObj: BizHiResStrmBitsMuxedEvent
    SrvObj->>HiResStrmMuxObj: BizHiResStrmBitsRecycledEvent

    LoResStrmMuxObj->>SrvObj: BizLoResStrmBitsMuxedEvent
    SrvObj->>LoResStrmMuxObj: BizLoResStrmBitsRecycledEvent
```

### From Management Viewpoint

```mermaid
sequenceDiagram
    participant MgmtObj
    participant XyzObj
    
    MgmtObj->>XyzObj: ModuleStartEvent
    loop KeepAlive
        XyzObj->>MgmtObj: ModuleKeepAliveEvent
    end
    MgmtObj->>XyzObj: ModuleStopEvent
```
