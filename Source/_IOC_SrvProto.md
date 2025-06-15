# IOC Service Protocol Design

## 目录

- [IOC Service Protocol Design](#ioc-service-protocol-design)
  - [目录](#目录)
  - [概述](#概述)
    - [框架优势](#框架优势)
    - [协议实现示例](#协议实现示例)
  - [分层协议架构](#分层协议架构)
    - [各层职责说明](#各层职责说明)
      - [🎯 应用层 (Application Layer)](#-应用层-application-layer)
      - [⚙️ 服务层 (Service Layer)](#️-服务层-service-layer)
      - [🔌 协议层 (Protocol Layer)](#-协议层-protocol-layer)
      - [📡 传输层 (Transport Layer)](#-传输层-transport-layer)
    - [协议选择策略](#协议选择策略)
  - [核心概念](#核心概念)
    - [服务URI (IOC\_SrvURI\_T)](#服务uri-ioc_srvuri_t)
      - [📋 基本语法](#-基本语法)
      - [🏗️ 结构定义](#️-结构定义)
      - [🔧 支持的协议类型](#-支持的协议类型)
      - [🏠 支持的主机类型](#-支持的主机类型)
      - [📝 URI示例](#-uri示例)
  - [服务协议接口设计](#服务协议接口设计)
    - [核心接口结构](#核心接口结构)
    - [🔑 Command操作的角色分工](#-command操作的角色分工)
      - [📊 命令操作对比表](#-命令操作对比表)
      - [🎯 `OpExecCmd_F` - 命令发起者](#-opexeccmd_f---命令发起者)
      - [🔧 `OpWaitCmd_F + OpAckCmd_F` - 命令执行者](#-opwaitcmd_f--opackcmd_f---命令执行者)
    - [📚 具体代码示例](#-具体代码示例)
    - [💡 选择原则](#-选择原则)
  - [服务协议扩展框架](#服务协议扩展框架)
    - [协议接口定义](#协议接口定义)
    - [开发步骤](#开发步骤)
  - [新协议开发指南](#新协议开发指南)
    - [协议设计原则](#协议设计原则)
    - [常见协议类型](#常见协议类型)
  - [错误处理和诊断](#错误处理和诊断)
    - [常见错误码](#常见错误码)
    - [调试和监控](#调试和监控)
  - [最佳实践](#最佳实践)
    - [性能优化建议](#性能优化建议)
    - [安全考虑](#安全考虑)
  - [ProtoFifo - 服务协议实现示例](#protofifo---服务协议实现示例)
    - [设计目标](#设计目标)
    - [实现架构](#实现架构)
    - [ProtoFifo 实现流程](#protofifo-实现流程)
    - [🚀 ProtoFifo 性能特点](#-protofifo-性能特点)
    - [🎯 ProtoFifo 适用场景](#-protofifo-适用场景)
    - [ProtoFifo 的CMD和DAT功能设计](#protofifo-的cmd和dat功能设计)
      - [命令功能设计示例](#命令功能设计示例)
  - [参考资料和扩展阅读](#参考资料和扩展阅读)

## 概述

IOC（Inter-Object Communication）服务协议是一个强大的对象间通信框架，它提供了统一的接口来支持多种传输协议。框架采用插件化设计，可以实现：

- **进程内通信**：高性能的线程间通信
- **进程间通信**：本机内不同进程间的数据交换  
- **主机间通信**：跨网络的分布式通信

本文档将详细介绍IOC服务协议的设计理念、核心接口，并以 **ProtoFifo** 作为实现示例，展示如何开发和使用服务协议。

### 框架优势

**统一接口**：无论使用哪种底层传输机制，都提供一致的API体验  
**插件架构**：可扩展的协议框架，支持多种协议实现并存  
**高性能**：针对不同场景优化，从零拷贝的进程内通信到高效的网络传输  
**多模式**：支持事件发布/订阅、命令执行、数据流传输三种通信模式

### 协议实现示例

本文档以 **ProtoFifo** 协议为主要示例，展示：
- 如何实现服务协议接口
- 高性能进程内通信的设计技巧  
- 线程安全和并发控制的最佳实践
- 服务协议的扩展和定制方法

## 分层协议架构

IOC服务协议采用分层设计，提供了从应用层到传输层的完整通信栈：

```
┌─────────────────────────────────────────────────────────────┐
│                     应用层 (Application Layer)                │
│  ┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐  │
│  │   Event API     │ │   Command API   │ │    Data API     │  │
│  │ (事件发布/订阅)    │ │  (命令执行/调用)  │ │  (数据传输/接收) │  │
│  └─────────────────┘ └─────────────────┘ └─────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    服务层 (Service Layer)                    │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │                Service Management                       │ │
│  │   • Service URI Resolution                             │ │
│  │   • Link Management                                    │ │
│  │   • Capability Negotiation                             │ │
│  │   • Service Discovery                                  │ │
│  └─────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                   协议层 (Protocol Layer)                    │
│  ┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐  │
│  │   FIFO Proto    │ │   TCP Proto     │ │   UDP Proto     │  │
│  │  (进程内/线程间)   │ │   (网络通信)     │ │   (广播通信)     │  │
│  │                 │ │   [TODO]        │ │   [TODO]        │  │
│  └─────────────────┘ └─────────────────┘ └─────────────────┘  │
│  ┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐  │
│  │   HTTP Proto    │ │   Custom Proto  │ │      ...        │  │
│  │   (RESTful)     │ │  (用户自定义)     │ │                 │  │
│  │   [TODO]        │ │   [TODO]        │ │                 │  │
│  └─────────────────┘ └─────────────────┘ └─────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                   传输层 (Transport Layer)                   │
│  ┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐  │
│  │  Memory Queue   │ │   TCP Socket    │ │   UDP Socket    │  │
│  │  (内存队列)       │ │   (TCP套接字)    │ │   (UDP套接字)    │  │
│  └─────────────────┘ └─────────────────┘ └─────────────────┘  │
│  ┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐  │
│  │  Named Pipe     │ │  Shared Memory  │ │      ...        │  │
│  │  (命名管道)       │ │   (共享内存)     │ │                 │  │
│  └─────────────────┘ └─────────────────┘ └─────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### 各层职责说明

#### 🎯 应用层 (Application Layer)
为开发者提供简洁易用的API接口：
- **Event API**: 异步事件发布/订阅机制，实现松耦合的消息通知
- **Command API**: 同步命令执行/调用机制，支持请求-响应模式
- **Data API**: 高效数据传输/接收机制，适用于大数据块和流式处理

#### ⚙️ 服务层 (Service Layer)  
负责服务管理和连接协调：
- **Service URI Resolution**: 将人类可读的URI转换为具体的网络地址和协议参数
- **Link Management**: 全生命周期管理客户端和服务端之间的连接
- **Capability Negotiation**: 智能协商通信双方支持的功能和性能参数
- **Service Discovery**: 动态服务注册与发现，支持分布式环境

#### 🔌 协议层 (Protocol Layer)
提供多样化的传输协议支持：
- **FIFO Protocol**: ⚡ 超高性能的进程内/线程间通信（已实现）
- **TCP Protocol**: 🌐 可靠的网络通信协议 [规划中]
- **UDP Protocol**: 📡 高效的广播通信协议 [规划中]  
- **HTTP Protocol**: 🔗 标准的RESTful通信协议 [规划中]
- **Custom Protocol**: 🔧 支持用户自定义协议扩展

#### 📡 传输层 (Transport Layer)
底层传输机制的具体实现：
- **Memory Queue**: 内存FIFO队列，实现真正的零拷贝传输
- **TCP Socket**: 标准TCP套接字，保证数据可靠传输
- **UDP Socket**: 标准UDP套接字，支持高效广播和组播
- **Named Pipe**: 操作系统命名管道，跨进程通信的经典方案
- **Shared Memory**: 共享内存机制，提供极致的本地通信性能

### 协议选择策略

IOC框架会根据服务URI自动选择最适合的协议和传输方式：

| 服务URI示例                      | 选择的协议    | 传输实现     | 适用场景             |
| -------------------------------- | ------------- | ------------ | -------------------- |
| `fifo://localprocess:0/service`  | FIFO Protocol | Memory Queue | 🚀 超高性能进程内通信 |
| `tcp://localhost:8080/service`   | TCP Protocol  | TCP Socket   | 🔒 可靠的本地网络通信 |
| `udp://0.0.0.0:9090/broadcast`   | UDP Protocol  | UDP Socket   | 📡 高效广播和组播     |
| `http://api.example.com/service` | HTTP Protocol | HTTP Client  | 🌐 标准Web服务集成    |

💡 **智能选择**：框架会根据URI的scheme自动选择最优的协议栈，开发者无需关心底层实现细节。

## 核心概念

### 服务URI (IOC_SrvURI_T)

服务URI是IOC框架中服务的唯一标识符，它采用标准的RFC 3986 URI格式，让服务定位变得直观和统一。

#### 📋 基本语法
```
scheme://[host[:port]][/path]
```

#### 🏗️ 结构定义
```c
typedef struct {
    union {
        const char *pScheme;     // 协议方案标识
        const char *pProtocol;   // 协议类型，参考 IOC_SRV_PROTO_*
    };
    
    union {
        const char *pHost;       // 主机地址，参考 IOC_SRV_HOST_*
        const char *pDomain;     // 域名
    };
    
    union {
        const char *pPath;       // 服务路径
        const char *pSrvName;    // 服务名称
        const char *pTopic;      // 主题名称
    };
    
    uint16_t Port;              // 端口号（TCP/UDP等协议需要）
} IOC_SrvURI_T;
```

#### 🔧 支持的协议类型

| 协议标识             | 说明                        | 状态     |
| -------------------- | --------------------------- | -------- |
| `IOC_SRV_PROTO_AUTO` | 🤖 框架自动选择最优协议      | ✅ 已实现 |
| `IOC_SRV_PROTO_FIFO` | ⚡ 进程内/线程间FIFO队列通信 | ✅ 已实现 |
| `IOC_SRV_PROTO_TCP`  | 🌐 基于TCP的可靠网络通信     | 🚧 规划中 |
| `IOC_SRV_PROTO_UDP`  | 📡 基于UDP的高效广播通信     | 🚧 规划中 |
| `IOC_SRV_PROTO_HTTP` | 🔗 基于HTTP的RESTful通信     | 🚧 规划中 |

#### 🏠 支持的主机类型

| 主机类型                     | 说明           | 适用场景     |
| ---------------------------- | -------------- | ------------ |
| `IOC_SRV_HOST_LOCAL_PROCESS` | 本地进程内通信 | 🧵 线程间通信 |
| `IOC_SRV_HOST_LOCAL_HOST`    | 本地主机通信   | 🖥️ 进程间通信 |
| `IOC_SRV_HOST_IPV4_ANY`      | 任意IPv4地址   | 🌍 主机间通信 |

#### 📝 URI示例

```c
// 🚀 超高性能的进程内FIFO通信
fifo://localprocess:0/myservice

// 🔒 可靠的本地TCP通信  
tcp://localhost:8080/api

// 🌍 远程HTTP服务通信
http://192.168.1.100:80/service
```

## 服务协议接口设计

IOC服务协议定义了一套统一的接口，所有协议实现都必须遵循这个接口规范。

### 核心接口结构

服务协议通过 `_IOC_SrvProtoMethods_T` 结构体定义统一接口：

```c
typedef struct _IOC_SrvProtoMethodsStru {
    const char *pProtocol;                    // 协议标识符
    
    // 服务管理接口
    IOC_Result_T (*OpOnlineService_F)(...);  // 服务上线
    IOC_Result_T (*OpOfflineService_F)(...); // 服务下线
    IOC_Result_T (*OpConnectService_F)(...); // 连接服务
    IOC_Result_T (*OpAcceptClient_F)(...);   // 接受连接
    IOC_Result_T (*OpCloseLink_F)(...);      // 关闭连接
    
    // 通信接口
    IOC_Result_T (*OpSubEvt_F)(...);         // 订阅事件
    IOC_Result_T (*OpUnsubEvt_F)(...);       // 取消订阅事件
    IOC_Result_T (*OpPostEvt_F)(...);        // 发布事件
    IOC_Result_T (*OpExecCmd_F)(...);        // 执行命令
    IOC_Result_T (*OpWaitCmd_F)(...);        // 等待命令
    IOC_Result_T (*OpAckCmd_F)(...);         // 响应命令
    IOC_Result_T (*OpSendData_F)(...);       // 发送数据
    IOC_Result_T (*OpRecvData_F)(...);       // 接收数据
} _IOC_SrvProtoMethods_T;
```

> **💡 提示**: 为了更好地理解服务协议的实现，建议先阅读[服务协议接口设计](#服务协议接口设计)和[服务协议扩展框架](#服务协议扩展框架)章节，然后参考[ProtoFifo实现示例](#protofifo---服务协议实现示例)了解具体实现细节。

### 🔑 Command操作的角色分工

根据架构设计，命令操作采用以下模式：

#### 📊 命令操作对比表

| 方面           | `OpExecCmd_F`              | `OpWaitCmd_F + OpAckCmd_F`    |
| -------------- | -------------------------- | ----------------------------- |
| **🎯 角色**     | 命令发起者 (同步请求-响应) | 命令执行者 (接收-处理-响应)   |
| **📍 调用位置** | 发送端                     | 接收端                        |
| **⚡ 执行模式** | 发送并等待最终结果         | 等待请求→处理→响应            |
| **📨 响应期望** | 等待响应或CbExecCmd_F返回  | 发送响应                      |
| **🔄 返回值**   | 最终执行结果               | Wait返回命令，Ack返回发送状态 |
| **🎭 类比**     | 打电话询问并等回复         | 接电话并回答                  |

#### 🎯 `OpExecCmd_F` - 命令发起者
同步发送命令并等待最终结果，结果来自CbExecCmd_F返回或OpAckCmd_F响应。

#### 🔧 `OpWaitCmd_F + OpAckCmd_F` - 命令执行者  
接收端等待命令请求，处理后通过OpAckCmd_F发送响应。

### 📚 具体代码示例

```c
// 命令发起者使用OpExecCmd_F
IOC_CmdDesc_T cmd = {.CmdID = CMD_QUERY_STATUS, ...};
IOC_Result_T result = methods->OpExecCmd_F(linkObj, &cmd, NULL);
// result包含最终执行结果

// 命令执行者使用OpWaitCmd_F + OpAckCmd_F
IOC_CmdDesc_T receivedCmd;
IOC_CmdToken_T token;
methods->OpWaitCmd_F(linkObj, &receivedCmd, &token, NULL);
// 处理命令...
IOC_CmdDesc_T response = {...};
methods->OpAckCmd_F(linkObj, &response, token, NULL);
```

### 💡 选择原则

| 场景           | 使用方法                   | 说明                 |
| -------------- | -------------------------- | -------------------- |
| 客户端调用服务 | `OpExecCmd_F`              | 同步等待服务执行结果 |
| 服务端处理请求 | `OpWaitCmd_F + OpAckCmd_F` | 接收请求并发送响应   |
| 查询数据/状态  | `OpExecCmd_F`              | 需要同步返回数据     |
| 执行操作并确认 | `OpExecCmd_F`              | 需要确认操作是否成功 |

## 服务协议扩展框架

IOC框架设计了可扩展的协议架构，开发者可以通过实现标准接口来创建新的协议。

### 协议接口定义

所有协议实现都必须提供 `_IOC_SrvProtoMethods_T` 结构的完整实现：

```c
// 协议实现示例框架
_IOC_SrvProtoMethods_T _gMyCustomProtoMethods = {
    .pProtocol = "my-custom-proto",
    
    // 必须实现的核心接口
    .OpOnlineService_F = __my_onlineService,
    .OpOfflineService_F = __my_offlineService,
    .OpConnectService_F = __my_connectService,
    .OpAcceptClient_F = __my_acceptClient,
    .OpCloseLink_F = __my_closeLink,
    
    // 通信接口（根据需要实现）
    .OpSubEvt_F = __my_subEvt,
    .OpUnsubEvt_F = __my_unsubEvt,
    .OpPostEvt_F = __my_postEvt,
    .OpExecCmd_F = __my_execCmd,
    .OpWaitCmd_F = __my_waitCmd,
    .OpAckCmd_F = __my_ackCmd,
    .OpSendData_F = __my_sendData,
    .OpRecvData_F = __my_recvData,
};
```

### 开发步骤

1. **设计协议特性**: 确定协议的传输机制、性能特点、适用场景
2. **定义数据结构**: 设计链接对象、服务对象等核心数据结构
3. **实现核心接口**: 实现服务管理和连接管理接口
4. **实现通信接口**: 根据需要实现事件、命令、数据传输接口
5. **测试和优化**: 进行功能测试和性能优化
6. **集成注册**: 将协议注册到IOC框架中

## 新协议开发指南

### 协议设计原则

1. **接口一致性**: 严格遵循IOC服务协议接口规范
2. **错误处理**: 提供完善的错误处理和状态反馈
3. **线程安全**: 确保多线程环境下的安全性
4. **资源管理**: 合理管理内存和系统资源
5. **性能优化**: 针对特定场景进行性能优化

### 常见协议类型

- **进程内协议**: 如ProtoFifo，优化进程内通信性能
- **进程间协议**: 如Unix Domain Socket，本机进程间通信
- **网络协议**: 如TCP/UDP，跨主机通信
- **消息队列协议**: 如基于消息队列的异步通信
- **共享内存协议**: 高性能的内存共享通信

## 错误处理和诊断

### 常见错误码

| 错误码                      | 描述       | 处理建议               |
| --------------------------- | ---------- | ---------------------- |
| `IOC_RESULT_SUCCESS`        | 操作成功   | 继续执行               |
| `IOC_RESULT_INVALID_PARAM`  | 参数无效   | 检查参数有效性         |
| `IOC_RESULT_NOT_EXIST_LINK` | 链接不存在 | 重新建立连接           |
| `IOC_RESULT_BUSY`           | 系统繁忙   | 稍后重试               |
| `IOC_RESULT_TIMEOUT`        | 操作超时   | 检查网络或增加超时时间 |
| `IOC_RESULT_LINK_BROKEN`    | 链接断开   | 重新建立连接           |

### 调试和监控

1. **日志记录**: 记录关键操作和错误信息
2. **性能监控**: 监控通信延迟、吞吐量等指标
3. **状态检查**: 定期检查连接状态和资源使用情况
4. **错误恢复**: 实现自动错误恢复机制

## 最佳实践

### 性能优化建议

1. **减少拷贝**: 尽可能实现零拷贝数据传输
2. **批量处理**: 支持批量操作以提高效率
3. **缓存机制**: 合理使用缓存减少重复计算
4. **并发优化**: 优化锁的使用，提高并发性能

### 安全考虑

1. **访问控制**: 实现适当的访问控制机制
2. **数据验证**: 验证所有输入数据的有效性
3. **资源限制**: 防止资源耗尽攻击
4. **错误信息**: 避免泄露敏感信息


## ProtoFifo - 服务协议实现示例

### 设计目标

ProtoFifo协议旨在提供一种高性能、低延迟的进程内通信机制。设计目标包括：

- **超高性能**: 实现毫秒级的事件响应和数据处理能力
- **零拷贝传输**: 通过内存映射文件或共享内存实现数据的零拷贝传输
- **简单易用**: 提供简单的API接口，方便开发者使用
- **可扩展性**: 支持多种数据格式和自定义协议扩展

### 实现架构

ProtoFifo协议的实现架构如下图所示：

```
┌─────────────────────────────────────────────────────────────┐
│                     ProtoFifo 协议架构图                     │
│ ┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐  │
│ │   应用层         │ │   服务层         │ │   传输层         │  │
│ │  (Event/Command) │ │  (Service URI)   │ │  (Memory Queue)  │  │
│ └─────────────────┘ └─────────────────┘ └─────────────────┘  │
│ ┌─────────────────────────────────────────────────────────┐ │
│ │                     ProtoFifo核心                       │ │
│ │   • 零拷贝数据传输机制                                  │ │
│ │   • 高效的事件发布/订阅实现                            │ │
│ │   • 线程安全的命令执行机制                            │ │
│ └─────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

### ProtoFifo 实现流程

ProtoFifo 展示了服务协议的标准实现模式：

1. **服务上线**: 创建服务对象，初始化同步机制，注册到全局服务列表
2. **客户端连接**: 查找服务，创建链接对象，建立对等关系
3. **事件通信**: 零拷贝的直接函数调用，支持订阅/发布模式
4. **命令处理**: 同步请求-响应模式，支持回调和轮询两种处理方式
5. **数据传输**: 基于缓冲区的高效数据交换（设计中）

### 🚀 ProtoFifo 性能特点

作为服务协议实现示例，ProtoFifo 展示了高性能协议设计：

| 性能指标         | ProtoFifo 实现方式       | 优势               |
| ---------------- | ------------------------ | ------------------ |
| **🔥 零拷贝事件** | 直接函数调用传递事件指针 | 避免内存拷贝开销   |
| **⚡ 超低延迟**   | 完全在用户空间执行       | 无系统调用开销     |
| **🚀 高并发**     | 细粒度锁设计             | 支持多线程并发访问 |
| **💾 内存高效**   | 按需分配 + 及时释放      | 最小化内存占用     |
| **🌟 零网络开销** | 进程内通信               | 无网络传输延迟     |

### 🎯 ProtoFifo 适用场景

**✅ 当前支持的场景（事件通信）**：
- **模块间解耦**: 进程内不同模块的事件通知
- **状态同步**: 组件间的状态变化通知  
- **广播通信**: 一对多的事件分发

**🚧 其他协议可能支持的场景（参考设计）**：
- **高性能RPC**: 基于命令机制的远程过程调用
- **服务调用**: 模块间或进程间的服务调用
- **流数据处理**: 基于数据传输的高速数据流管道
- **文件传输**: 跨进程或网络的文件传输

### ProtoFifo 的CMD和DAT功能设计

⚠️ **重要说明**: ProtoFifo 的 CMD 和 DAT 功能尚未实现。以下展示设计参考，可用于其他协议实现。

#### 命令功能设计示例

```c
// 当前 ProtoFifo 实现状态
_IOC_SrvProtoMethods_T _gIOC_SrvProtoFifoMethods = {
    .pProtocol = IOC_SRV_PROTO_FIFO,
    
    .OpOnlineService_F = __IOC_onlineService_ofProtoFifo,     // ✅ 已实现
    .OpOfflineService_F = __IOC_offlineService_ofProtoFifo,   // ✅ 已实现
    .OpConnectService_F = __IOC_connectService_ofProtoFifo,   // ✅ 已实现
    .OpAcceptClient_F = __IOC_acceptClient_ofProtoFifo,       // ✅ 已实现
    .OpCloseLink_F = __IOC_closeLink_ofProtoFifo,            // ✅ 已实现
    
    .OpSubEvt_F = __IOC_subEvt_ofProtoFifo,                  // ✅ 已实现
    .OpUnsubEvt_F = __IOC_unsubEvt_ofProtoFifo,              // ✅ 已实现
    .OpPostEvt_F = __IOC_postEvt_ofProtoFifo,                // ✅ 已实现
    
    // 待实现功能
    .OpExecCmd_F = NULL,    // ❌ 待实现 - 同步命令执行
    .OpWaitCmd_F = NULL,    // ❌ 待实现 - 等待命令请求
    .OpAckCmd_F = NULL,     // ❌ 待实现 - 发送命令响应
    .OpSendData_F = NULL,   // ❌ 待实现
    .OpRecvData_F = NULL,   // ❌ 待实现
};
```


---

> 本文档持续更新中。如有疑问或建议，请联系开发团队。

## 参考资料和扩展阅读

- **[IOC架构设计文档](./README_ArchDesign.md)**: 详细的系统架构和设计理念
- **[IOC使用指南](./README_UserGuide.md)**: 完整的用户使用手册和最佳实践
- **[IOC用例文档](./README_UseCase.md)**: 实际应用场景和使用案例
- **[IOC规范文档](./README_Specification.md)**: 技术规范和接口定义
- **[IOC术语定义](./README_Glossary.md)**: 框架相关术语和概念定义
