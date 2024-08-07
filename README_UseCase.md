[[_TOC_]]

# About
* This is IOC(a.k.a Inter-Object-Communication)'s Use Case document, describe how **USER** as a specific role will **USE** IOC in a specific context.
    * **USER**: an object in a thread/process/machine, named as ObjA/ObjB/ObjC/ObjD/ObjE/... in IOC.
    * **UES**: ObjA post event to ObjB/C/...(a.k.a EVT) 
        * OR ObjA execute command over ObjB(a.k.a CMD) 
            * OR ObjA send data to ObjB(a.k.a DAT).

* All use cases are divided into different categories:
    * [ Category-A ]: post event in same process.
    * [ Category-B ]: post event beyond same process(inter-process/machine).
    * [ Category-C ]: execute command in same process.
    * [ Category-D ]: execute command beyond same process(inter-process/machine).
    * [ Category-E ]: send data in same process.
    * [ Category-F ]: send data beyond same process(inter-process/machine).


# [ Category-A ]: post event in same process.
## [ Use Case-01 ]: ObjA post event, ObjB process event by callback or retrive.
### [Scenario-01]
* ObjA and ObjB is in the same process.
    * ObjA post event to IOC,
        * IOC callback ObjB to process the event.
            * ObjB MUST subscribe event before ObjA post event.
```mermaid
flowchart
subgraph same process
    ObjB --> |subscribe event| IOC
    ObjA --> |post event| IOC
    IOC -.-> |callback| ObjB
end
```

#### [Scenario-01.a]
* ObjB will be callbacked only AFTER successfully subscribe the event, and THEN ObjA post a new event.
    * This MEANS ObjA post event will get NO_EVENT_CONSUMER result, if ObjB has NOT subscribed the event.

```mermaid
sequenceDiagram
    participant ObjA
    participant IOC
    participant ObjB
    ObjA->>IOC: post event
    IOC-->>ObjA: NO_EVENT_CONSUMER
    ObjB->>IOC: subscribe event
    ObjA->>IOC: post event
    IOC-->>ObjA: SUCCESS
    IOC--)ObjB: callback
```

#### [Scenario-01.b]
* ObjA post event get NO_EVENT_CONSUMER result,
    * IF ObjB unsubscribe the event before ObjA post event.

```mermaid
sequenceDiagram
    participant ObjA
    participant IOC
    participant ObjB
    ObjB->>IOC: unsubscribe event
    ObjA->>IOC: post event
    IOC-->>ObjA: NO_EVENT_CONSUMER
```

#### [Scenario-01.c]

* ObjA post event to IOC and IOC is callbacking ObjB,
  * IF ObjB unsubscribe the event from ThieadX when ObjB is callbacking by IOC,
    * THEN the unsubscription will block until the callback is done.

```mermaid
sequenceDiagram
    participant ObjA
    participant IOC
    participant ObjB
    participant ThreadX(ObjB)
    ObjA->>IOC: post event
    IOC--)ObjB: callback begin
    activate ObjB
    ThreadX(ObjB)->>IOC: unsubscribe event
    activate IOC
    ObjB -->> IOC: callback end
    deactivate ObjB
    IOC -->> ThreadX(ObjB): unsubscribe event return
    deactivate IOC
```

### [Scenario-02]
* ObjA and ObjB is in the same process.
    * ObjA post event to IOC,
        * ObjB retrive the event from IOC and process it.
            * ObjB DONT need to subscribe event before ObjA post event.
            * ObjB retrive event from IOC MAYBLOCK if no event in IOC.

```mermaid
flowchart
subgraph same process
    ObjA --> |post event| IOC
    ObjB --> |retrive event| IOC
    IOC -.-> |retrive event return| ObjB
    ObjB -.-> |process event| ObjB
end
```

#### [Scenario-02.a]
* ObjB retrive event SHOULDBLOCK until ObjA post new event.
    * This INDICATE ObjA post event will get NO_EVENT_CONSUMER result, if ObjB is NOT waitting to retrive event, which MEANS ObjA's pending event queue depth is ZERO by default.
    
```mermaid
sequenceDiagram
    participant ObjA
    participant IOC
    participant ObjB
    ObjA->>IOC: post event
    IOC-->>ObjA: NO_EVENT_CONSUMER
    ObjB->>IOC: retrive event MAYBLOCK
    ObjA->>IOC: post event
    IOC-->>ObjA: SUCCESS
    IOC--)ObjB: retrive event return
```

#### [Scenario-02.b]
* ObjB MAY set MAX_PENDING_EVENT_QUEUE_DEPTH=N(>0) to avoid NO_EVENT_CONSUMER result, but may get IOC_RESULT_TOO_MANY_QUEUING_EVENT result. 

```mermaid
sequenceDiagram
    participant ObjA
    participant IOC
    participant ObjB
    ObjB->>IOC: set MAX_PENDING_EVENT_QUEUE_DEPTH=N(>0)
    loop new event
        ObjA->>IOC: post event
        alt <=MAX_PENDING_EVENT_QUEUE_DEPTH
            IOC-->>ObjA: SUCCESS
        else
            IOC-->>ObjA: TOO_MANY_PENDING_EVENT_IN_QUEUE
        end
    end
```

#### [Scenario-02.c]
* ObjB MAY retrive event with timeout arguement to avoid MAYBLOCK.
    * IF ObjA not post event within timeout, 
        * THEN ObjB will get RETRIVE_TIMEOUT result.

```mermaid
sequenceDiagram
    participant ObjA
    participant IOC
    participant ObjB
    ObjB->>IOC: retrive event TIMEOUT=1000ms
    IOC--)ObjB: retrive event return RETRIVE_TIMEOUT

```


## [ Use Case-02 ]: ObjA post event, ObjB process event by callback, ObjC process event by retrive.
### [Scenario-01]
* ObjA and ObjB/C is in the same process.
    * ObjA post event to IOC,
        * IOC callback ObjB to process the event.
        * ObjC retrive the event from IOC and process it.
```mermaid
flowchart
subgraph same process
    ObjA --> |post event| IOC
    IOC -.-> |callback| ObjB
    ObjC --> |retrive event| IOC
    IOC -.-> |retrive event return| ObjC
    ObjC -.-> |process event| ObjC
end
```

## [ Use Case-03 ]: ObjA post class-b/-c event, ObjB process class-b event, ObjC process class-c event.
### [Scenario-01]
* ObjA post event of class-b and class-c to IOC,
    * ObjB subscribe class-b event from IOC.
        * IF ObjA post event of class-b,
            * THEN IOC callback ObjB to process the class-b event.
    * ObjC subscribe class-c event IOC.
        * IF ObjA post event of class-c,
            * THEN IOC callback ObjC to process the class-c event.
    * ObjB and ObjC MAY also retrive the event from IOC.
```mermaid
flowchart
subgraph same process
    ObjA --> |post event class-b/-c| IOC
    IOC -.-> |callback ONLY class-b| ObjB
    IOC -.-> |callback ONLY class-c| ObjC
    ObjB --> |subscribe class-b event| IOC
    ObjC --> |subscribe class-c event| IOC
end
```

### [Scenario-02]
* ObjA post event of class-b and class-c to IOC,
    * ObjB retrive with class-b event arguement from IOC.
        * IF ObjA post event of class-b,
            * THEN ObjB retrive the class-b event from IOC and process it.
    * ObjC retrive with class-c event arguement from IOC.
        * IF ObjA post event of class-c,
            * THEN ObjC retrive the class-c event from IOC and process it.
```mermaid
flowchart
subgraph same process
    ObjA --> |post event class-b/-c| IOC
    ObjB --> |retrive class-b event| IOC
    IOC -.-> |retrive class-b event return| ObjB
    ObjB -.-> |process class-b event| ObjB
    ObjC --> |retrive class-c event| IOC
    IOC -.-> |retrive class-c event return| ObjC
    ObjC -.-> |process class-c event| ObjC
end
```

## [ Use Case-04 ]: ObjA post class-a event, ObjB post class-b event, ObjC process class-a event, ObjD process class-b event, ObjE process class-a/-b event.
### [Scenario-01]
* ObjA post event of class-a to IOC, ObjB post event of class-b to IOC.
    * ObjC subscribe class-a event from IOC.
        * IF ObjA post event of class-a,
            * THEN IOC callback ObjC to process the class-a event.
    * ObjD subscribe class-b event from IOC.
        * IF ObjB post event of class-b,
            * THEN IOC callback ObjD to process the class-b event.
    * ObjE subscribe class-a/-b event from IOC.
        * IF ObjA post event of class-a,
            * THEN IOC callback ObjE to process the class-a event.
        * IF ObjB post event of class-b,
            * THEN IOC callback ObjE to process the class-b event.
    * ObjC/D/E MAY also retrive the event from IOC.
```mermaid
flowchart
subgraph same process
    ObjA --> |post event class-a| IOC
    ObjB --> |post event class-b| IOC
    IOC -.-> |callback ONLY class-a| ObjC
    IOC -.-> |callback ONLY class-b| ObjD
    ObjC --> |subscribe class-a event| IOC
    ObjD --> |subscribe class-b event| IOC
    ObjE --> |subscribe class-a/-b event| IOC
    IOC -.-> |callback class-a/-b| ObjE
end
```

## [ Use Case-05 ]: ObjA post class-b event, ObjB subscribe class-b and in callback post class-c event, ObjC is subscribing class-c and is callbacked
### [Scenario-01]
* ObjB subscribe class-b event from IOC, ObjC subscribe class-c event from IOC.
    * IF ObjA post event of class-b,
        * THEN IOC callback ObjB to process the class-b event.
            * AND in callback IF ObjB post class-c event to IOC.
                * THEN IOC callback ObjC to process the class-c event.
```mermaid
flowchart
subgraph same process
    ObjB --> |subscribe class-b event| IOC
    ObjC --> |subscribe class-c event| IOC
    ObjA --> |post class-b event| IOC
    IOC -.-> |callback class-b| ObjB
    ObjB --> |post class-c event| IOC
    IOC -.-> |callback class-c| ObjC
end
```

```mermaid
sequenceDiagram
    participant ObjA
    participant IOC
    participant ObjB
    participant ObjC
    ObjB->>IOC: subscribe class-b event
    ObjC->>IOC: subscribe class-c event
    ObjA->>IOC: post class-b event
    IOC--)ObjB: callback class-b begin
    activate ObjB
    ObjB->>IOC: post class-c event
    ObjB -->> IOC: callback class-b end
    deactivate ObjB
    IOC--)ObjC: callback class-c
```

#### [Scenario-01.a]
* IOC callback ObjC before ObjB's callback retrun

```mermaid
sequenceDiagram
    participant ObjA
    participant IOC
    participant ObjB
    participant ObjC
    ObjB->>IOC: subscribe class-b event
    ObjC->>IOC: subscribe class-c event
    ObjA->>IOC: post class-b event
    IOC--)ObjB: callback class-b begin
    activate ObjB
    ObjB->>IOC: post class-c event
    IOC--)ObjC: callback class-c
    ObjB -->> IOC: callback class-b end
    deactivate ObjB
```

# [ Category-B ]: post event beyond same process(inter-process/machine)
## Overview vs Category-A
* Category-B is similar to Category-A, but ObjA and ObjB are in different process/machine.
    * When Category-A: we assume ObjA MAY post event via IOC to ObjB directlly, which means ObjA and ObjB has a default \<auto>Link built by IOC.
    * While Category-B: ObjA and ObjB MAY still post event via IOC to ObjB directly, which means even ObjA and ObjB are in different process/machine, they still have a default \<auto>Link built by IOC automatically.
        * And this also means following scenarios are possible: Process-X has ObjA and ObjA1, Process-Y has ObjB, Machine-Z has ObjC, ObjA post event to IOC, ObjA1/B/C will get the event until they subscribe the event. Its IOC's responsibility to establish the \<auto>Link between ObjA and ObjB/C by any means.
            * IOC MAY use IPC/Socket/MessageQueue/SharedMemory/... to establish the \<auto>Link between ObjA and ObjB/C which are inter-process/machine by initialization or configuration.
            * ObjA MAY forbidden or unwill to post event to ObjB/C by each post's option, or by IOC's configuration.
    * More Category-B: ObjA and ObjB MAY establish a \<user>Link via IOC before ObjA post event to ObjB, which means if ObjA post event use this \<user>Link, only ObjB will get the event, even some other ObjC/D/E... has subscribed the same event on \<auto>Link. 
* In Summary: Category-B is a superset of Category-A, and Category-A is a subset of Category-B.
    * Category-B is more complex than Category-A, and Category-A is more simple than Category-B.

## [Use Case-01]: post event beyond same process via \<auto>Link.
### [Scenario-01]
* IOC-X in Process-X, IOC-Y in Process-Y and establish \<auto>Link to IOC-X.
    * ObjA in Process-X post event to IOC, ObjB in Process-Y subscribe the event.
        * IOC in Process-X forward the event to IOC in Process-Y,
            * IOC in Process-Y callback ObjB in Process-Y to process the event.
```mermaid
flowchart
subgraph Process-X
    ObjA --> |post event| IOC-X
end
subgraph Process-Y
    IOC-Y o==o |establish <^auto>Link| IOC-X
    IOC-X -.-> |forward event| IOC-Y
    ObjB --> |subscribe event| IOC-Y
    IOC-Y -.-> |callback| ObjB
end
```

## [Use Case-02]: post event beyond same process via \<user>Link.
### [Scenario-01]
* IOC-X in Process-X, IOC-Y in Process-Y.
    * ObjB establish \<user>Link to ObjA via IOC-X and IOC-Y.
    * ObjA in Process-X post event to ObjB via \<user>Link.
        * IOC-X route the event to IOC-Y, and IOC-Y callback ObjB to process the event finally.
```mermaid
flowchart
subgraph Process-X
    ObjA --> |post event via <^user>Link| IOC-X
end
subgraph Process-Y
    ObjB o===o |establish <^user>Link| IOC-Y o==o IOC-X o===o ObjA
    IOC-X -.-> |route event| IOC-Y
    IOC-Y -.-> |callback| ObjB
end
```

# [ Category-C ]: execute command in same process.

# [ Category-D ]: execute command beyond same process(inter-process/machine)

# [ Category-E ]: send data in same process.

# [ Category-F ]: send data beyond same process(inter-process/machine)
