# About::MyIOC_inTDD_withGHC

* This is a practice module named Inter-Object-Communication(a.k.a IOC) and has functions as its name
  * in Test-Driven Development(a.k.a TDD) as Dev-Approach
    * with GitHub Copilot(a.k.a GHC) as Dev-Facilites.
  * Also, I'm exploring and evaluating 'Large Language Model based Software Engineering'(a.k.a LLMSE) in this project.
    * RefMore: [README_LLMSE](README_LLMSE.md)

* **module** MEANS a static library with C/C++ API implemented all functions in IOC.
* **Inter-Object** MEANS the communication between objects in a thread/process/machine.
  * **Object** MEANS an object of struct/class with OO abstraction, named as ObjA/ObjB/ObjC/... in IOC.
* **Communication** MEANS ObjA post event to ObjB/C/...(a.k.a EVT) 
    * OR ObjA execute command over ObjB(a.k.a CMD) 
    * OR ObjA send data to ObjB(a.k.a DAT).

# Glossary
* [README_Glossary](./README_Glossary.md)

# Requirement and Analysis

* [UseCase](./README_UseCase.md)
  * BETWEEN: End User and IOC's PM before analysis.
  * ABOUT: How USER as a specific role will USE IOC in a specific context known as scenario.

* [Specification](./README_Specification.md) 
  * BETWEEN: IOC's PM and team tech leader such as architect after analysis.
    * Sometimes discuss with End User or ask End User to review it.
  * ABOUT: What IOC will do, what IOC will not do, what IOC will do later.
    * Including functional/non-functional requirements, constraints, assumptions, and acceptance criteria, etc.
  * RefPDF: [IEEE 29148-2011](https://github.com/Orthant/IEEE/blob/master/29148-2011.pdf)

* [UserStories](./README_UserStories.md)      
  * BETWEEN: IOC's team tech leader and team members such as normal developers following Specification and referencing Use Case.
    * Sometimes discuss with PM or End User.
  * ABOUT: How to decompose the requirements into team member's monthly/weekly/daily development pieces.
    * As a「X=Who/Role」，I want 「Y=What/Func」，So that 「Z=Why/Value」
  * RefBook: [<<User Story Applied>>](https://www.mountaingoatsoftware.com/books/user-stories-applied)


# Architecure and Design

* [ArchDesign](./README_ArchDesign.md)
* [TODO: ModuleDesign](./README_ModuleDesign.md)
* [TODO: VerifyDesign](./README_VerifyDesign.md)
