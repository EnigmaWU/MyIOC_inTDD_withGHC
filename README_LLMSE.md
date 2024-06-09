# About

* This docuement has 'Body of Knowledge for Large Language Model based Software Engineering'(a.k.a BoK-LLMSE) guiding this project.
  * Most part of this BoK-LLMSE is based on or learning from public courses, books, and articles.
  * Some part of this BoK-LLMSE is my own experience and understanding.

# Overview

* Renewed V-Model for LLM based Software Engineering.

```mermaid
---
title: V-Model for LLM based Software Engineering
---
graph 
    subgraph "Requirements Analysis+Verifying"
        UseCase ==> AcceptanceTesting
        UseCase -.-> UserStoriesOfSys
        UserStoriesOfSys -.-> AcceptanceTesting
        UserStoriesOfSys -.-> AceptCritOfSys
        AceptCritOfSys -.-> AcceptanceTesting
        
    end
    subgraph "Architectural Designing+Verifying"
        UseCase ==> ArchitectureDesign
        UseCase -.-> SpecOfSys
        SpecOfSys -.-> ArchitectureDesign

        ArchitectureDesign <-.-> UserStoriesOfCom
        ArchitectureDesign --> QualityTesting
        
        UserStoriesOfSys -..-> UserStoriesOfCom
        UserStoriesOfCom -.-> QualityTesting
        UserStoriesOfCom -.-> AceptCritOfCom

        AceptCritOfCom -.-> QualityTesting
        QualityTesting ==> AcceptanceTesting
    end
    subgraph "Module Designing+Implementing+Verifying"
        ArchitectureDesign ==> ModuleDesign ==> ModuleImplementing ==> UnitTesting ==> QualityTesting
        ArchitectureDesign -.-> SpecOfMod -.-> ModuleDesign
        ModuleDesign <-.-> UserStoriesOfMod -.-> AceptCritOfMod -.-> UnitTesting
        UserStoriesOfCom -.-> UserStoriesOfMod -.-> UnitTesting 
    end
```

# References

* [SWEBOKv3](https://www.computer.org/education/bodies-of-knowledge/software-engineering)
* X8X: AI based Software Engineering([link](https://time.geekbang.org/column/intro/100755401?tab=intro))