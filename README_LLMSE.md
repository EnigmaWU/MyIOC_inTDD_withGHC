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
graph LR
    subgraph "Requirements Analysis&Verifying"
        UseCase -.-> UserStories_ofSys
        UserStories_ofSys -.-> AcceptanceTesting
        UserStories_ofSys -.-> AceptCriterior_ofSys
        AceptCriterior_ofSys -.-> AcceptanceTesting
        UseCase --> AcceptanceTesting
    end
    subgraph "Architectural Designing&Verifying"
        UseCase ==> ArchitectureDesign
        UseCase -.-> Spec_ofSys
        Spec_ofSys -.-> ArchitectureDesign
        ArchitectureDesign -.-> UserStories_ofCom
        UserStories_ofCom -.-> QualityTesting
        ArchitectureDesign --> QualityTesting

        UserStories_ofSys -..-> UserStories_ofCom
        UserStories_ofCom -.-> AceptCriterior_ofCom
        AceptCriterior_ofCom -.-> QualityTesting

        QualityTesting ==> AcceptanceTesting
    end
    subgraph "Module Designing&Implementing&Verifying"
        ArchitectureDesign ==> ModuleDesign
        ArchitectureDesign -.-> Spec_ofMod
        Spec_ofMod -.-> ModuleDesign
        ModuleDesign -.-> UserStories_ofMod
        UserStories_ofMod -.-> UnitTesting
        ModuleDesign ==> UnitTesting

        UserStories_ofCom -.-> UserStories_ofMod
        UserStories_ofMod -.-> AceptCriterior_ofMod
        AceptCriterior_ofMod -.-> UnitTesting

        UnitTesting ==> QualityTesting
    end
```

# References

* [SWEBOKv3](https://www.computer.org/education/bodies-of-knowledge/software-engineering)
* X8X: AI based Software Engineering([link](https://time.geekbang.org/column/intro/100755401?tab=intro))