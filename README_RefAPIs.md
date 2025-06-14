# IOC API 参考手册

[TOC]

## 概述

本文档提供 MyIOC 库的完整 API 参考。IOC (Inter-Object Communication) 支持两种主要通信模式：

- **ConlesMode（无连接模式）**：适用于单进程内的简单事件通信
- **ConetMode（连接模式）**：适用于跨进程/网络的复杂通信，支持事件(EVT)、命令(CMD)、数据(DAT)

---

## 📚 包含文件

```c
#include "IOC.h"  // 主头文件，包含所有必要的定义
```

IOC.h 会自动包含以下子模块：
- `IOC_EvtAPI.h` - 事件 API
- `IOC_CmdAPI.h` - 命令 API  
- `IOC_DatAPI.h` - 数据传输 API
- `IOC_SrvAPI.h` - 服务管理 API
- `IOC_Types.h` - 类型定义
- `IOC_Option.h` - 选项配置
- `IOC_Helpers.h` - 工具函数

---

## 🎯 核心 API 分类

### 1️⃣ ConlesMode 事件 APIs

#### IOC_subEVT_inConlesMode
```c
IOC_Result_T IOC_subEVT_inConlesMode(const IOC_SubEvtArgs_T *pSubEvtArgs);
```

**功能**：订阅事件（无连接模式）

**参数**：
- `pSubEvtArgs`: 订阅参数，包含回调函数和事件ID列表

**返回值**：
- `IOC_RESULT_SUCCESS`: 订阅成功
- `IOC_RESULT_TOO_MANY_EVENT_CONSUMER`: 事件消费者过多
- `IOC_RESULT_CONFLICT_EVENT_CONSUMER`: 事件消费者冲突

**示例**：
```c
IOC_Result_T MyCallback(IOC_EvtDesc_pT pEvtDesc, void *pCbPriv) {
    printf("收到事件 ID: %d\n", pEvtDesc->EvtID);
    return IOC_RESULT_SUCCESS;
}

void SubscribeEvents() {
    IOC_EvtID_T eventIDs[] = {IOC_EVTID_TEST_KEEPALIVE, IOC_EVTID_SYSTEM_READY};
    IOC_SubEvtArgs_T subArgs = {
        .CbProcEvt_F = MyCallback,
        .pCbPrivData = NULL,
        .EvtNum = IOC_calcArrayElmtCnt(eventIDs),
        .pEvtIDs = eventIDs
    };
    
    IOC_Result_T result = IOC_subEVT_inConlesMode(&subArgs);
    if (result != IOC_RESULT_SUCCESS) {
        printf("订阅失败: %s\n", IOC_getResultStr(result));
    }
}
```

#### IOC_unsubEVT_inConlesMode
```c
IOC_Result_T IOC_unsubEVT_inConlesMode(const IOC_UnsubEvtArgs_T *pUnsubEvtArgs);
```

**功能**：取消事件订阅（无连接模式）

**参数**：
- `pUnsubEvtArgs`: 取消订阅参数，需包含与订阅时相同的回调函数和私有数据指针

**返回值**：
- `IOC_RESULT_SUCCESS`: 取消订阅成功
- `IOC_RESULT_NO_EVENT_CONSUMER`: 未找到对应的事件消费者

#### IOC_postEVT_inConlesMode
```c
IOC_Result_T IOC_postEVT_inConlesMode(const IOC_EvtDesc_T *pEvtDesc, IOC_Options_T *pOption);
```

**功能**：发布事件（无连接模式）

**参数**：
- `pEvtDesc`: 事件描述符
- `pOption`: 选项配置（可选）

**返回值**：
- `IOC_RESULT_SUCCESS`: 发布成功
- `IOC_RESULT_NO_EVENT_CONSUMER`: 无事件消费者
- `IOC_RESULT_TOO_MANY_QUEUING_EVTDESC`: 事件队列已满

**支持的选项**：
- `IOC_OPTID_TIMEOUT`: 设置超时
- `IOC_OPTID_SYNC_MODE`: 同步模式

#### IOC_forceProcEVT
```c
void IOC_forceProcEVT(void);
```

**功能**：强制处理所有挂起的事件

**注意**：这是阻塞操作，会等待所有事件处理完毕

#### IOC_wakeupProcEVT
```c
void IOC_wakeupProcEVT(void);
```

**功能**：唤醒事件处理线程（异步操作）

---

### 2️⃣ ConetMode 服务管理 APIs

#### IOC_onlineService
```c
IOC_Result_T IOC_onlineService(IOC_SrvID_T *pSrvID, const IOC_SrvArgs_T *pSrvArgs);
```

**功能**：上线一个服务

**参数**：
- `pSrvID`: 输出参数，返回服务ID
- `pSrvArgs`: 服务参数配置

**返回值**：
- `IOC_RESULT_SUCCESS`: 服务上线成功
- `IOC_RESULT_SERVICE_ALREADY_ONLINE`: 服务已上线
- `IOC_RESULT_INVALID_URI`: URI 无效

**示例**：
```c
void StartService() {
    IOC_SrvID_T srvID;
    IOC_SrvArgs_T srvArgs = {
        .SrvURI = {
            .pProtocol = IOC_SRV_PROTO_FIFO,
            .pHost = IOC_SRV_HOST_LOCAL_PROCESS,
            .pPath = "MyTestService",
            .Port = 0
        },
        .Flags = IOC_SRVFLAG_NONE,
        .UsageCapabilites = IOC_LinkUsageEvtReceiver,
        .UsageArgs = {.pEvt = &evtArgs}
    };
    
    IOC_Result_T result = IOC_onlineService(&srvID, &srvArgs);
    if (result == IOC_RESULT_SUCCESS) {
        printf("服务上线成功，服务ID: %lu\n", srvID);
    }
}
```

#### IOC_offlineService
```c
IOC_Result_T IOC_offlineService(IOC_SrvID_T SrvID);
```

**功能**：下线服务

**参数**：
- `SrvID`: 要下线的服务ID

#### IOC_acceptClient
```c
IOC_Result_T IOC_acceptClient(IOC_SrvID_T SrvID, IOC_LinkID_T *pLinkID, const IOC_Options_T *pOption);
```

**功能**：接受客户端连接

**参数**：
- `SrvID`: 服务ID
- `pLinkID`: 输出参数，返回连接ID
- `pOption`: 选项配置（可选）

#### IOC_connectService
```c
IOC_Result_T IOC_connectService(IOC_LinkID_T *pLinkID, const IOC_ConnArgs_T *pConnArgs, const IOC_Options_T *pOption);
```

**功能**：连接到服务

**参数**：
- `pLinkID`: 输出参数，返回连接ID
- `pConnArgs`: 连接参数
- `pOption`: 选项配置（可选）

#### IOC_closeLink
```c
IOC_Result_T IOC_closeLink(IOC_LinkID_T LinkID);
```

**功能**：关闭连接

**参数**：
- `LinkID`: 要关闭的连接ID

---

### 3️⃣ ConetMode 事件 APIs

#### IOC_postEVT
```c
IOC_Result_T IOC_postEVT(IOC_LinkID_T LinkID, const IOC_EvtDesc_T *pEvtDesc, IOC_Options_T *pOption);
```

**功能**：通过指定连接发布事件

**参数**：
- `LinkID`: 连接ID
- `pEvtDesc`: 事件描述符
- `pOption`: 选项配置（可选）

#### IOC_subEVT
```c
IOC_Result_T IOC_subEVT(IOC_LinkID_T LinkID, const IOC_SubEvtArgs_T *pSubEvtArgs);
```

**功能**：通过指定连接订阅事件

#### IOC_unsubEVT
```c
IOC_Result_T IOC_unsubEVT(IOC_LinkID_T LinkID, const IOC_UnsubEvtArgs_T *pUnsubEvtArgs);
```

**功能**：通过指定连接取消事件订阅

#### IOC_broadcastEVT
```c
IOC_Result_T IOC_broadcastEVT(IOC_SrvID_T SrvID, const IOC_EvtDesc_T *pEvtDesc, IOC_Options_T *pOption);
```

**功能**：向服务的所有连接广播事件

---

### 4️⃣ 命令执行 APIs

#### IOC_execCMD
```c
IOC_Result_T IOC_execCMD(IOC_LinkID_T LinkID, IOC_CmdDesc_T *pCmdDesc, IOC_Options_T *pOption);
```

**功能**：执行命令（同步操作）

**参数**：
- `LinkID`: 连接ID
- `pCmdDesc`: 命令描述符（输入参数+输出结果）
- `pOption`: 选项配置（可选）

**返回值**：
- `IOC_RESULT_SUCCESS`: 命令执行成功
- `IOC_RESULT_NO_CMD_EXECUTOR`: 无命令执行器
- `IOC_RESULT_BUSY`: 执行器忙碌
- `IOC_RESULT_TIMEOUT`: 执行超时
- `IOC_RESULT_CMD_EXEC_FAILED`: 命令执行失败

**示例**：
```c
void ExecuteCommand() {
    IOC_CmdDesc_T cmdDesc = {
        .CmdID = IOC_CMDID_TEST_PING,
        .ParamLen = 0,
        .pParam = NULL,
        .ResultBufSize = 256,
        .pResultBuf = resultBuffer
    };
    
    IOC_Option_defineTimeout(timeoutOpt, 5000000); // 5秒超时
    
    IOC_Result_T result = IOC_execCMD(linkID, &cmdDesc, &timeoutOpt);
    if (result == IOC_RESULT_SUCCESS) {
        printf("命令执行成功，结果: %s\n", (char*)cmdDesc.pResultBuf);
    }
}
```

#### IOC_waitCMD
```c
IOC_Result_T IOC_waitCMD(IOC_LinkID_T LinkID, IOC_CmdDesc_T *pCmdDesc, IOC_Options_T *pOption);
```

**功能**：等待命令（轮询模式）

**参数**：
- `LinkID`: 连接ID
- `pCmdDesc`: 命令描述符缓冲区
- `pOption`: 选项配置（可选）

#### IOC_ackCMD
```c
IOC_Result_T IOC_ackCMD(IOC_LinkID_T LinkID, IOC_CmdDesc_T *pCmdDesc, IOC_Options_T *pOption);
```

**功能**：确认命令执行结果

**参数**：
- `LinkID`: 连接ID
- `pCmdDesc`: 包含执行结果的命令描述符
- `pOption`: 选项配置（可选）

---

### 5️⃣ 数据传输 APIs

#### IOC_sendDAT
```c
IOC_Result_T IOC_sendDAT(IOC_LinkID_T LinkID, IOC_DatDesc_T *pDatDesc, IOC_Options_T *pOption);
```

**功能**：发送数据块

**参数**：
- `LinkID`: 连接ID
- `pDatDesc`: 数据描述符
- `pOption`: 选项配置（可选）

**返回值**：
- `IOC_RESULT_SUCCESS`: 数据发送成功
- `IOC_RESULT_BUFFER_FULL`: 缓冲区满
- `IOC_RESULT_TIMEOUT`: 发送超时
- `IOC_RESULT_STREAM_CLOSED`: 数据流已关闭
- `IOC_RESULT_DATA_TOO_LARGE`: 数据过大

**示例**：
```c
void SendData() {
    const char *data = "Hello, World!";
    IOC_DatDesc_T datDesc = {
        .BufPtr = (void*)data,
        .BufSize = strlen(data),
        .Flags = IOC_DATFLAG_PTR_MODE  // 使用指针模式避免拷贝
    };
    
    IOC_Result_T result = IOC_sendDAT(linkID, &datDesc, NULL);
    if (result == IOC_RESULT_SUCCESS) {
        printf("数据发送成功\n");
    }
}
```

#### IOC_recvDAT
```c
IOC_Result_T IOC_recvDAT(IOC_LinkID_T LinkID, IOC_DatDesc_T *pDatDesc, IOC_Options_T *pOption);
```

**功能**：接收数据块（轮询模式）

**参数**：
- `LinkID`: 连接ID
- `pDatDesc`: 数据描述符缓冲区
- `pOption`: 选项配置（可选）

**返回值**：
- `IOC_RESULT_SUCCESS`: 数据接收成功
- `IOC_RESULT_TIMEOUT`: 接收超时
- `IOC_RESULT_NO_DATA`: 无数据可用
- `IOC_RESULT_STREAM_CLOSED`: 数据流已关闭

#### IOC_flushDAT
```c
IOC_Result_T IOC_flushDAT(IOC_LinkID_T LinkID, IOC_Options_T *pOption);
```

**功能**：刷新数据缓冲区，强制发送

**参数**：
- `LinkID`: 连接ID
- `pOption`: 选项配置（可选）

---

### 6️⃣ 工具和帮助 APIs

#### IOC_getCapability
```c
IOC_Result_T IOC_getCapability(IOC_CapabilityDescription_T *pCapDesc);
```

**功能**：获取 IOC 库的能力信息

#### IOC_getLinkState
```c
IOC_Result_T IOC_getLinkState(IOC_LinkID_T LinkID, IOC_LinkState_T *pLinkMainState, IOC_LinkSubState_T *pLinkSubState);
```

**功能**：获取连接状态（主要用于调试）

#### IOC_getResultStr
```c
const char* IOC_getResultStr(IOC_Result_T result);
```

**功能**：获取错误码的字符串描述

**示例**：
```c
IOC_Result_T result = IOC_postEVT_inConlesMode(&event, NULL);
if (result != IOC_RESULT_SUCCESS) {
    printf("操作失败: %s\n", IOC_getResultStr(result));
}
```

---

## 🔧 选项配置系统

### 选项宏定义

#### 非阻塞模式
```c
IOC_Option_defineNonBlock(optName);
// 等价于：
IOC_Options_T optName = {
    .IDs = IOC_OPTID_TIMEOUT,
    .Payload.TimeoutUS = 0
};
```

#### 超时模式
```c
IOC_Option_defineTimeout(optName, timeoutUS);
// 等价于：
IOC_Options_T optName = {
    .IDs = IOC_OPTID_TIMEOUT,
    .Payload.TimeoutUS = timeoutUS
};
```

#### 同步模式
```c
IOC_Option_defineSync(optName);
// 等价于：
IOC_Options_T optName = {
    .IDs = IOC_OPTID_SYNC_MODE
};
```

#### 同步+非阻塞
```c
IOC_Option_defineSyncNonBlock(optName);
// 等价于：
IOC_Options_T optName = {
    .IDs = IOC_OPTID_SYNC_MODE | IOC_OPTID_TIMEOUT,
    .Payload.TimeoutUS = 0
};
```

### 选项检查函数

```c
IOC_BoolResult_T IOC_Option_isAsyncMode(IOC_Options_T *pOption);
IOC_BoolResult_T IOC_Option_isSyncMode(IOC_Options_T *pOption);
IOC_BoolResult_T IOC_Option_isNonBlockMode(IOC_Options_T *pOption);
IOC_BoolResult_T IOC_Option_isTimeoutMode(IOC_Options_T *pOption);
IOC_BoolResult_T IOC_Option_isMayBlockMode(IOC_Options_T *pOption);
ULONG_T IOC_Option_getTimeoutUS(IOC_Options_T *pOption);
```

---

## 📊 数据结构参考

### 事件相关

#### IOC_EvtDesc_T
```c
typedef struct {
    IOC_EvtID_T EvtID;        // 事件ID
    ULONG_T DataLen;          // 数据长度
    void *pData;              // 数据指针
    // 其他内部字段...
} IOC_EvtDesc_T;
```

#### IOC_SubEvtArgs_T
```c
typedef struct {
    IOC_CbProcEvt_F CbProcEvt_F;    // 回调函数
    void *pCbPrivData;              // 私有数据
    ULONG_T EvtNum;                 // 事件数量
    IOC_EvtID_T *pEvtIDs;           // 事件ID数组
} IOC_SubEvtArgs_T;
```

### 服务相关

#### IOC_SrvURI_T
```c
typedef struct {
    const char *pProtocol;    // 协议名称 (如 "fifo", "tcp")
    const char *pHost;        // 主机名
    const char *pPath;        // 路径
    uint16_t Port;            // 端口号
} IOC_SrvURI_T;
```

#### IOC_SrvArgs_T
```c
typedef struct {
    IOC_SrvURI_T SrvURI;                // 服务URI
    IOC_SrvFlags_T Flags;               // 服务标志
    IOC_LinkUsage_T UsageCapabilites;   // 服务能力
    struct {
        IOC_EvtUsageArgs_T *pEvt;       // 事件参数
        IOC_CmdUsageArgs_T *pCmd;       // 命令参数
        IOC_DatUsageArgs_T *pDat;       // 数据参数
        void *pGeneric;                 // 通用扩展
    } UsageArgs;
} IOC_SrvArgs_T;
```

### 数据传输相关

#### IOC_DatDesc_T
```c
typedef struct {
    void *BufPtr;             // 数据缓冲区指针
    ULONG_T BufSize;          // 缓冲区大小
    ULONG_T DataLen;          // 实际数据长度
    IOC_DatFlags_T Flags;     // 数据标志
    // 其他内部字段...
} IOC_DatDesc_T;
```

---

## ⚠️ 错误码完整列表

| 错误码                                | 数值 | 含义           | 处理建议                  |
| ------------------------------------- | ---- | -------------- | ------------------------- |
| `IOC_RESULT_SUCCESS`                  | 0    | 操作成功       | 继续执行                  |
| `IOC_RESULT_BUG`                      | -1   | 程序内部错误   | 检查代码逻辑              |
| `IOC_RESULT_INVALID_PARAM`            | -2   | 参数无效       | 检查参数值                |
| `IOC_RESULT_NO_MEMORY`                | -3   | 内存不足       | 释放资源或增加内存        |
| `IOC_RESULT_TIMEOUT`                  | -4   | 操作超时       | 增加超时时间或重试        |
| `IOC_RESULT_TOO_MANY_QUEUING_EVTDESC` | -100 | 事件队列满     | 调用 `IOC_forceProcEVT()` |
| `IOC_RESULT_NO_EVENT_CONSUMER`        | -101 | 无事件消费者   | 确保先订阅再发布          |
| `IOC_RESULT_CONFLICT_EVENT_CONSUMER`  | -102 | 事件消费者冲突 | 避免重复订阅              |
| `IOC_RESULT_NO_CMD_EXECUTOR`          | -200 | 无命令执行器   | 确保服务端在线            |
| `IOC_RESULT_CMD_EXEC_FAILED`          | -201 | 命令执行失败   | 检查命令参数              |
| `IOC_RESULT_BUSY`                     | -202 | 资源忙碌       | 稍后重试                  |
| `IOC_RESULT_BUFFER_FULL`              | -300 | 缓冲区满       | 等待或使用非阻塞模式      |
| `IOC_RESULT_STREAM_CLOSED`            | -301 | 数据流关闭     | 重新建立连接              |
| `IOC_RESULT_DATA_TOO_LARGE`           | -302 | 数据过大       | 分块传输                  |
| `IOC_RESULT_NOT_EXIST_LINK`           | -400 | 连接不存在     | 重新建立连接              |
| `IOC_RESULT_LINK_BROKEN`              | -401 | 连接断开       | 重新建立连接              |
| `IOC_RESULT_SERVICE_ALREADY_ONLINE`   | -500 | 服务已上线     | 使用已有服务              |

---

## 🛠️ 工具宏

### 数组大小计算
```c
#define IOC_calcArrayElmtCnt(array) (sizeof(array) / sizeof(array[0]))

// 使用示例
IOC_EvtID_T eventIDs[] = {IOC_EVTID_TEST_KEEPALIVE, IOC_EVTID_SYSTEM_READY};
ULONG_T count = IOC_calcArrayElmtCnt(eventIDs);  // 结果: 2
```

### 调试宏
```c
#define IOC_BugAbort() \
    do { \
        fprintf(stderr, "BUG: %s:%d\n", __FILE__, __LINE__); \
        abort(); \
    } while (0)
```

### 时间相关常量
```c
#define IOC_TIMEOUT_INFINITE  ULONG_MAX     // 无限等待
#define IOC_TIMEOUT_IMMEDIATE 0             // 立即返回（非阻塞）
#define IOC_TIMEOUT_MAX       86400000000   // 最大超时（24小时）
```

---

## 🚀 使用模式示例

### 1. 简单事件通知
```c
// 发布者
IOC_EvtDesc_T event = {
    .EvtID = IOC_EVTID_SYSTEM_READY,
    .DataLen = 0,
    .pData = NULL
};
IOC_postEVT_inConlesMode(&event, NULL);

// 消费者
IOC_Result_T HandleEvent(IOC_EvtDesc_pT pEvtDesc, void *pCbPriv) {
    printf("系统就绪事件\n");
    return IOC_RESULT_SUCCESS;
}

IOC_EvtID_T events[] = {IOC_EVTID_SYSTEM_READY};
IOC_SubEvtArgs_T subArgs = {
    .CbProcEvt_F = HandleEvent,
    .EvtNum = 1,
    .pEvtIDs = events
};
IOC_subEVT_inConlesMode(&subArgs);
```

### 2. 跨进程命令执行
```c
// 客户端
IOC_LinkID_T linkID;
IOC_ConnArgs_T connArgs = {
    .SrvURI = {"fifo", "localhost", "CommandService", 0},
    .Usage = IOC_LinkUsageCmdInitiator
};
IOC_connectService(&linkID, &connArgs, NULL);

IOC_CmdDesc_T cmdDesc = {
    .CmdID = IOC_CMDID_SYSTEM_GET_STATUS,
    .ParamLen = 0,
    .ResultBufSize = 1024,
    .pResultBuf = resultBuffer
};
IOC_execCMD(linkID, &cmdDesc, NULL);

// 服务端
IOC_Result_T HandleCommand(IOC_LinkID_T linkID, IOC_CmdDesc_pT pCmdDesc, void *pCbPriv) {
    // 处理命令并设置结果
    strcpy((char*)pCmdDesc->pResultBuf, "System OK");
    pCmdDesc->ResultLen = strlen("System OK");
    return IOC_RESULT_SUCCESS;
}
```

### 3. 流式数据传输
```c
// 发送端
char data[] = "Large data chunk...";
IOC_DatDesc_T datDesc = {
    .BufPtr = data,
    .BufSize = sizeof(data),
    .Flags = IOC_DATFLAG_PTR_MODE
};
IOC_sendDAT(linkID, &datDesc, NULL);
IOC_flushDAT(linkID, NULL);  // 强制发送

// 接收端
IOC_DatDesc_T recvDesc;
while (IOC_recvDAT(linkID, &recvDesc, NULL) == IOC_RESULT_SUCCESS) {
    printf("接收到 %lu 字节数据\n", recvDesc.DataLen);
    // 处理数据...
}
```

---

## 🎯 性能优化建议

### 事件处理优化
- 使用 `IOC_Option_defineNonBlock()` 进行高频事件发布
- 批量处理事件后调用 `IOC_forceProcEVT()`
- 避免在回调函数中执行长时间操作

### 数据传输优化
- 使用指针模式 (`IOC_DATFLAG_PTR_MODE`) 避免内存拷贝
- 适当的数据块大小（建议 4KB-64KB）
- 在需要时使用 `IOC_flushDAT()` 强制发送

### 内存管理
- 及时关闭不再使用的连接
- 下线不再需要的服务
- 避免在高频路径上进行动态内存分配

---

## 📞 技术支持

如需更多信息，请参考：
- [用户指南](README_UserGuide.md) - 详细使用说明
- [架构设计](README_ArchDesign.md) - 内部设计原理  
- [用例说明](README_UseCase.md) - 典型使用场景

---

*文档版本: v1.0*  
*最后更新: 2025-06-14*
