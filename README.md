
# About::MyIOC_inTDD_withGHC

* This is a practice module named 【Inter-Object-Communication(a.k.a IOC) 】and has functions as its name implied:
  * use Test-Driven Development(a.k.a TDD) as Dev-Approach
    * with GitHub Copilot(a.k.a GHC) as Dev-Assistent.
  * Also, I'm exploring and evaluating 'Large Language Model based Software Engineering'(a.k.a LLMSE) in this project.
    * RefMore: [README_LLMSE](README_LLMSE.md)

* 这是一个名为【对象间通信（又称 IOC）】的练习模块，具有其名称所暗示的功能：
  * 使用测试驱动开发（又称 TDD）作为开发方法
    * 使用 GitHub Copilot（又称 GHC）作为开发助手。
  * 此外，我在这个项目中探索和评估基于大语言模型的软件工程（又称 LLMSE）。

---

* 【**module**】 MEANS a static library with C/C++ API implemented all functions in IOC.
  * also MEANS it may be used by nnKB~nnMB scale embedded systems.
* 【**Inter-Object**】 MEANS the communication between objects in a thread/process/operating system/machine.
  * **Object** MEANS an object of struct/class with OO abstraction, named as ObjA/ObjB/ObjC/... in IOC.
* 【**Communication**】 MEANS ObjA post event to ObjB/C/...(a.k.a EVT)
  * OR ObjA execute command over ObjB(a.k.a CMD)
  * OR ObjA send data to ObjB(a.k.a DAT).

* 【**模块**】指的是一个使用 C/C++ API 实现所有 IOC 功能的静态库。
  * 也意味着它可能被 nnKB~nnMB 规模的嵌入式系统使用。
* 【**对象间**】指的是在线程/进程/操作系统/机器中对象之间的通信。
  * **对象**指的是具有面向对象抽象的结构体/类的对象，在 IOC 中命名为 ObjA/ObjB/ObjC/...
* 【**通信**】指的是 ObjA 向 ObjB/C/... 发送事件（即 EVT）
  * 或者 ObjA 执行 ObjB 上的命令（即 CMD）
  * 或者 ObjA 向 ObjB 发送数据（即 DAT）。

---

## Role As

* 【**USER**】 MEANS the user who will use the IOC in a bussiness scenario.
  * read [README_UseCase](./README_UseCase.md) to know what the IOC can do,
    * and then read [README_UserGuide](./README_UserGuide.md) to know how to use the IOC,
      * don't need to read the following sections.
* 【**DEV**】 MEANS the developer who will accept new requirments, archdesign&implement the IOC.
  * keep reading the following sections to deep dive into the IOC.

* 【**用户**】指的是在业务场景中使用 IOC 的用户。
  * 阅读 [README_UseCase](./README_UseCase.md) 了解 IOC 的功能，
    * 然后阅读 [README_UserGuide](./README_UserGuide.md) 了解如何使用 IOC，
      * 不需要阅读以下部分。
* 【**开发者**】指的是接受新需求、架构设计和实现 IOC 的开发者。
  * 继续阅读以下部分深入了解 IOC。

---

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

# AboutMe

* EnigmaWU@Hangzhou, Zhejiang, China
  * ![](./Doc/MyAvatar.png)
