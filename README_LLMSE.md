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
        UseCase -.-> UserStories_ofSystemLevel
        UserStories_ofSystemLevel -.-> AcceptanceTesting
        UserStories_ofSystemLevel -.-> AcceptanceCriterior_ofSystem
        AcceptanceCriterior_ofSystem -.-> AcceptanceTesting
        UseCase --> AcceptanceTesting
    end
    subgraph "Architectural Designing&Verifying"
        UseCase ==> ArchitectureDesign
        UseCase -.-> Specificaton_ofSystem
        Specificaton_ofSystem -.-> ArchitectureDesign
        ArchitectureDesign -.-> UserStories_ofComponentLevel
        UserStories_ofComponentLevel -.-> QualityTesting
        ArchitectureDesign --> QualityTesting

        UserStories_ofSystemLevel -..-> UserStories_ofComponentLevel
        UserStories_ofComponentLevel -.-> AcceptanceCriterior_ofComponent
        AcceptanceCriterior_ofComponent -.-> QualityTesting

        QualityTesting ==> AcceptanceTesting
    end
    subgraph "Module Designing&Implementing&Verifying"
        ArchitectureDesign ==> ModuleDesign
        ArchitectureDesign -.-> Specificaton_ofModule
        Specificaton_ofModule -.-> ModuleDesign
        ModuleDesign -.-> UserStories_ofModuleLevel
        UserStories_ofModuleLevel -.-> UnitTesting
        ModuleDesign ==> UnitTesting

        UserStories_ofComponentLevel -.-> UserStories_ofModuleLevel
        UserStories_ofModuleLevel -.-> AcceptanceCriterior_ofModule
        AcceptanceCriterior_ofModule -.-> UnitTesting

        UnitTesting ==> QualityTesting
    end
```

# References

* [SWEBOKv3](https://www.computer.org/education/bodies-of-knowledge/software-engineering)
* X8X: AI based Software Engineering([link](https://time.geekbang.org/column/intro/100755401?tab=intro))