# About::MyIOC_inTDD_withGHC

* This is a practice module named Inter-Object-Communication(a.k.a IOC) and has functions as its name
  * in Test-Driven Development(a.k.a TDD) as Dev-Approach
    * with GitHub Copilot(a.k.a GHC) as Dev-Facilites.
  * Also, I'm exploring and evaluating 'Large Language Model based Software Engineering'(a.k.a LLMSE) in this project.
    * RefMore: [README_LLMSE](README_LLMSE.md)

* 【**module**】 MEANS a static library with C/C++ API implemented all functions in IOC.
  * also MEANS it may be used by nnKB~nnMB scale embedded systems.
* 【**Inter-Object**】 MEANS the communication between objects in a thread/process/operating system/machine.
  * **Object** MEANS an object of struct/class with OO abstraction, named as ObjA/ObjB/ObjC/... in IOC.
* 【**Communication**】 MEANS ObjA post event to ObjB/C/...(a.k.a EVT)
  * OR ObjA execute command over ObjB(a.k.a CMD)
  * OR ObjA send data to ObjB(a.k.a DAT).

## Role As

* 【**USER**】 MEANS the user who will use the IOC in a specific context.
  * read [README_UseCase](./README_UseCase.md) to know what the IOC can do,
    * and then read [README_UserGuide](./README_UserGuide.md) to know how to use the IOC,
      * don't need to read the following sections.
* 【**DEV**】 MEANS the developer who will develop the IOC.
  * keep reading the following sections to deep dive into the IOC.

# Glossary

* [README_Glossary](./README_Glossary.md)

# Requirement and Analysis

* [UseCase](./README_UseCase.md)
  * BETWEEN: End User and IOC's PM before analysis.
  * ABOUT: How USER as a specific role will USE IOC in a specific context known as scenario.
  * TOBEREAD::RefBook: [\<\<Writing Effective Use Cases>>](https://www.amazon.com/Writing-Effective-Cases-Alistair-Cockburn/dp/0201702258)
    * TOBEWRITE::MyReadingNotes: [\<\<Writing Effective Use Cases>>](./MyReadingNotes/WritingEffectiveUseCases.md)

* [Specification](./README_Specification.md) 
  * BETWEEN: IOC's PM and team tech leader such as architect after analysis.
    * Sometimes discuss with End User or ask End User to review it.
  * ABOUT: What IOC will do, what IOC will not do, what IOC will do later.
    * Including functional/non-functional requirements, constraints, assumptions, and acceptance criteria, etc.
  * RefPDF: [IEEE 29148-2011](https://github.com/Orthant/IEEE/blob/master/29148-2011.pdf)
    * TOBEWRITE::MyReadingNotes: [IEEE 29148-2011](./MyReadingNotes/IEEE29148-2011.md)

* [UserStories](./README_UserStories.md)
  * BETWEEN: IOC's team tech leader and team members such as normal developers following Specification and referencing Use Case.
    * Sometimes discuss with PM or End User.
  * ABOUT: How to decompose the requirements into team member's monthly/weekly/daily development pieces.
    * As a「X=Who/Role」，I want 「Y=What/Func」，So that 「Z=Why/Value」
  * RefBook: [\<\<User Story Applied>>](https://www.mountaingoatsoftware.com/books/user-stories-applied)
    * TOBEWRITE::MyReadingNotes: [\<\<User Story Applied>>](./MyReadingNotes/UserStoryApplied.md)


# Architecure and Design

* [ArchDesign](./README_ArchDesign.md)
* [TODO: ModuleDesign](./README_ModuleDesign.md)
* [TODO: VerifyDesign](./README_VerifyDesign.md)
