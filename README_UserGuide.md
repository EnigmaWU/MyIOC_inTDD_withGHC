[TOC]

# About

* As a **USER**，you just need to read this document to know how to use the IOC.

## 📖 如何使用本文档

根据您的需求选择阅读路径：

🚀 **初次使用？** → 阅读 [Quick Start](#quick-start-5分钟上手)

🔧 **简单事件通信？** → 阅读 [ConlesMode 用法](#tiny-usageconlesmode)

🌐 **跨进程通信？** → 阅读 [ConetMode 用法](#typical-usageconetmode)  

📚 **查找 API？** → 查阅 [API 参考手册](README_RefAPIs.md)

❓ **遇到问题？** → 查看 [常见问题 FAQ](#常见问题-faq) 和 [故障排除](#故障排除指南)

💡 **最佳实践？** → 阅读各节的"最佳实践"部分

# Quick Start (5分钟上手)

想要快速体验 IOC？以下是最简单的示例：

## 最小示例 - 事件通信

```c
#include "IOC.h"

// 1. 定义事件处理回调
IOC_Result_T MyCallback(IOC_EvtDesc_pT pEvtDesc, void *pCbPriv) {
    printf("收到事件 ID: %d\n", pEvtDesc->EvtID);
    return IOC_RESULT_SUCCESS;
}

int main() {
    // 2. 订阅事件
    IOC_EvtID_T eventIDs[] = {IOC_EVTID_TEST_KEEPALIVE};
    IOC_SubEvtArgs_T subArgs = {
        .CbProcEvt_F = MyCallback,
        .pCbPrivData = NULL,
        .EvtNum = 1,
        .pEvtIDs = eventIDs
    };
    IOC_subEVT_inConlesMode(&subArgs);
    
    // 3. 发布事件
    IOC_EvtDesc_T event = {.EvtID = IOC_EVTID_TEST_KEEPALIVE};
    IOC_postEVT_inConlesMode(&event, NULL);
    
    // 4. 处理事件
    IOC_forceProcEVT();
    
    return 0;
}
```

这就是全部！🎉 继续阅读了解更多功能...

---

# Use Scenarios

* 【TinyVersion】：IF your runtime is xxKB scale, use this version, and connectless event(a.k.a ConlesEvent) is good enough for you.
* 【TypicalVersion】：IF your runtime is xxMB scale, use this version, consider ConlesEvent by default.
  * IF you want to avoid unpredictable event processing latency, use connection-based event(a.k.a ConetEvent).
  * IF you want to get the result of controlling command execution, use connection-based command(a.k.a ConetCmd).
  * IF you want to transfer data between objects, use connection-based data(a.k.a ConetData).
* 【TitanVersion】：IF your runtime is xxGB scale such as SimuX64, use this version.

---

* 【TinyVersion】：如果你的运行时规模是 xxKB 级别，使用这个版本，无连接事件（也叫 ConlesEvent）对你来说已经足够了。
* 【TypicalVersion】：如果你的运行时规模是 xxMB 级别，使用这个版本，默认考虑使用无连接事件（也叫 ConlesEvent）。
  * 如果你想避免不可预测的事件处理延迟，使用基于连接的事件（也叫 ConetEvent）。
  * 如果你想知道控制命令执行的结果，使用基于连接的命令（也叫 ConetCmd）。
  * 如果你想在对象之间传输数据，使用基于连接的数据（也叫 ConetData）。
* 【TitanVersion】：如果你的运行时规模是 xxGB 级别，比如 SimuX64，使用这个版本吧。

---

# 性能指标与系统限制

## 性能基准
| 指标              | TinyVersion | TypicalVersion | TitanVersion |
| ----------------- | ----------- | -------------- | ------------ |
| 内存占用          | < 64KB      | < 16MB         | < 1GB        |
| 事件队列深度      | 16          | 4096           | 65536        |
| 最大并发连接      | 4           | 1024           | 16384        |
| 事件处理延迟      | < 1ms       | < 10ms         | < 100ms      |
| 吞吐量（事件/秒） | 1K          | 100K           | 10M          |

## 系统限制
### ConlesMode 限制
- 仅支持单进程内通信
- 事件队列有固定大小限制
- 不支持事件优先级
- 无法获取事件处理结果

### ConetMode 限制
- 需要显式管理连接生命周期
- 网络延迟影响性能
- 需要处理连接断开等异常情况
- 内存使用量随连接数增长

## 性能优化建议
### 高频事件场景
```c
// ✅ 推荐：使用非阻塞模式
IOC_Option_defineNonBlock(opt);
IOC_postEVT_inConlesMode(&evt, &opt);
```

### 低延迟场景
```c
// ✅ 推荐：使用同步模式确保立即处理
IOC_Option_defineSync(opt);
IOC_postEVT_inConlesMode(&evt, &opt);
```

### 大数据传输
```c
// ✅ 推荐：使用数据指针避免拷贝
IOC_DatDesc_T datDesc = {
    .BufPtr = pLargeData,  // 直接传递指针
    .BufSize = largeSize,
    .Flags = IOC_DATFLAG_PTR_MODE
};
```


---

# 【Tiny Usage（ConlesMode）】

ConlesMode（无连接模式）是 IOC 最简单的使用方式，适用于单进程内的模块间通信。所有事件通过一个自动管理的内部链接进行传递，无需显式建立连接。

## asEvtConsumer（事件消费者）

### 1. 基础事件订阅

作为事件消费者，您需要订阅感兴趣的事件。使用 `IOC_subEVT_inConlesMode` 函数来订阅事件。

```c
// 定义回调函数的私有数据结构
typedef struct {
    uint32_t processedCount;
    char moduleName[32];
} MyPrivateData_T;

// 实现事件处理回调函数
static IOC_Result_T MyCallback(IOC_EvtDesc_pT pEvtDesc, void *pCbPriv) {
    MyPrivateData_T *pPrivData = (MyPrivateData_T *)pCbPriv;
    
    switch (pEvtDesc->EvtID) {
        case IOC_EVTID_TEST_KEEPALIVE:
            pPrivData->processedCount++;
            printf("[%s] 处理 KeepAlive 事件，计数: %d\n", 
                   pPrivData->moduleName, pPrivData->processedCount);
            break;
            
        case IOC_EVTID_YOUR_CUSTOM_EVENT:
            // 处理自定义事件
            printf("[%s] 处理自定义事件\n", pPrivData->moduleName);
            break;
            
        default:
            printf("[%s] 收到未知事件 ID: %d\n", pPrivData->moduleName, pEvtDesc->EvtID);
            return IOC_RESULT_BUG;
    }
    
    return IOC_RESULT_SUCCESS;
}

void SubscribeEvent() {
    // 初始化私有数据
    static MyPrivateData_T privateData = {
        .processedCount = 0,
        .moduleName = "VideoDecoder"
    };
    
    // 定义要订阅的事件 ID 列表
    IOC_EvtID_T EventIDs[] = {
        IOC_EVTID_TEST_KEEPALIVE,
        IOC_EVTID_YOUR_CUSTOM_EVENT
    };
    
    // 设置订阅参数
    IOC_SubEvtArgs_T SubEvtArgs = {
        .CbProcEvt_F = MyCallback,        // 事件处理回调函数
        .pCbPrivData = &privateData,      // 传递给回调的私有数据
        .EvtNum      = IOC_calcArrayElmtCnt(EventIDs), // 事件数量
        .pEvtIDs     = EventIDs,          // 事件 ID 数组
    };

    // 执行订阅
    IOC_Result_T Result = IOC_subEVT_inConlesMode(&SubEvtArgs);
    if (Result != IOC_RESULT_SUCCESS) {
        printf("订阅事件失败: %s\n", IOC_getResultStr(Result));
        return;
    }
    
    printf("成功订阅 %lu 个事件\n", SubEvtArgs.EvtNum);
}
```

### 2. 高级事件处理模式

#### 2.1 多模块协作示例

```c
// 模块状态管理
typedef struct {
    bool isRunning;
    uint32_t frameCount;
    uint32_t errorCount;
    time_t lastHeartbeat;
} ModuleContext_T;

// 视频捕获模块的事件处理
static IOC_Result_T VideoCaptureCallback(IOC_EvtDesc_pT pEvtDesc, void *pCbPriv) {
    ModuleContext_T *pContext = (ModuleContext_T *)pCbPriv;
    
    switch (pEvtDesc->EvtID) {
        case IOC_EVTID_MODULE_START:
            pContext->isRunning = true;
            pContext->lastHeartbeat = time(NULL);
            printf("视频捕获模块启动\n");
            break;
            
        case IOC_EVTID_MODULE_STOP:
            pContext->isRunning = false;
            printf("视频捕获模块停止，处理了 %d 帧\n", pContext->frameCount);
            break;
            
        case IOC_EVTID_FRAME_REQUEST:
            if (pContext->isRunning) {
                // 模拟处理帧请求
                pContext->frameCount++;
                
                // 发布帧数据事件
                IOC_EvtDesc_T frameEvent = {
                    .EvtID = IOC_EVTID_FRAME_CAPTURED,
                    .DataLen = sizeof(uint32_t),
                    .pData = &pContext->frameCount
                };
                IOC_postEVT_inConlesMode(&frameEvent, NULL);
            }
            break;
            
        default:
            pContext->errorCount++;
            return IOC_RESULT_BUG;
    }
    
    pContext->lastHeartbeat = time(NULL);
    return IOC_RESULT_SUCCESS;
}

void SetupVideoCaptureModule() {
    static ModuleContext_T videoContext = {0};
    
    IOC_EvtID_T videoEvents[] = {
        IOC_EVTID_MODULE_START,
        IOC_EVTID_MODULE_STOP,
        IOC_EVTID_FRAME_REQUEST
    };
    
    IOC_SubEvtArgs_T videoSubArgs = {
        .CbProcEvt_F = VideoCaptureCallback,
        .pCbPrivData = &videoContext,
        .EvtNum = IOC_calcArrayElmtCnt(videoEvents),
        .pEvtIDs = videoEvents
    };
    
    IOC_Result_T result = IOC_subEVT_inConlesMode(&videoSubArgs);
    if (result != IOC_RESULT_SUCCESS) {
        printf("视频捕获模块订阅失败: %s\n", IOC_getResultStr(result));
    }
}
```

#### 2.2 错误处理和恢复

```c
// 带有错误处理的回调函数
static IOC_Result_T RobustCallback(IOC_EvtDesc_pT pEvtDesc, void *pCbPriv) {
    MyPrivateData_T *pPrivData = (MyPrivateData_T *)pCbPriv;
    
    // 参数验证
    if (!pEvtDesc || !pCbPriv) {
        printf("错误：回调函数收到空指针参数\n");
        return IOC_RESULT_BUG;
    }
    
    // 处理各种事件
    switch (pEvtDesc->EvtID) {
        case IOC_EVTID_TEST_KEEPALIVE:
            // 正常处理
            pPrivData->processedCount++;
            break;
            
        case IOC_EVTID_ERROR_RECOVERY:
            // 错误恢复逻辑
            printf("[%s] 执行错误恢复\n", pPrivData->moduleName);
            pPrivData->processedCount = 0; // 重置计数器
            break;
            
        case IOC_EVTID_SHUTDOWN:
            // 清理资源
            printf("[%s] 收到关闭信号，开始清理\n", pPrivData->moduleName);
            // 在这里可以设置标志位，主循环检查后进行清理
            break;
            
        default:
            printf("[%s] 警告：未处理的事件 ID: %d\n", 
                   pPrivData->moduleName, pEvtDesc->EvtID);
            // 对于未知事件，不返回错误，以保持系统稳定性
            break;
    }
    
    return IOC_RESULT_SUCCESS;
}
```

### 3. 取消订阅事件

当模块不再需要处理某些事件时，应及时取消订阅以释放系统资源。

```c
void UnsubscribeEvent() {
    // 注意：取消订阅时只需要提供回调函数和私有数据指针
    // IOC 会根据这两个参数找到对应的订阅并移除
    IOC_UnsubEvtArgs_T UnsubEvtArgs = {
        .CbProcEvt_F = MyCallback,
        .pCbPrivData = &privateData,  // 必须与订阅时的指针相同
    };

    IOC_Result_T Result = IOC_unsubEVT_inConlesMode(&UnsubEvtArgs);
    if (Result != IOC_RESULT_SUCCESS) {
        printf("取消订阅失败: %s\n", IOC_getResultStr(Result));
    } else {
        printf("成功取消事件订阅\n");
    }
}
```

### 4. 最佳实践

#### 4.1 模块生命周期管理

```c
typedef struct {
    bool initialized;
    bool subscribed;
    ModuleContext_T context;
} ModuleManager_T;

// 模块初始化
IOC_Result_T ModuleInit(ModuleManager_T *pMgr, const char *moduleName) {
    if (pMgr->initialized) {
        return IOC_RESULT_SUCCESS; // 已经初始化
    }
    
    // 初始化上下文
    memset(&pMgr->context, 0, sizeof(ModuleContext_T));
    strncpy(pMgr->context.moduleName, moduleName, sizeof(pMgr->context.moduleName) - 1);
    
    // 订阅事件
    IOC_EvtID_T events[] = {
        IOC_EVTID_MODULE_START,
        IOC_EVTID_MODULE_STOP,
        IOC_EVTID_TEST_KEEPALIVE
    };
    
    IOC_SubEvtArgs_T subArgs = {
        .CbProcEvt_F = ModuleCallback,
        .pCbPrivData = &pMgr->context,
        .EvtNum = IOC_calcArrayElmtCnt(events),
        .pEvtIDs = events
    };
    
    IOC_Result_T result = IOC_subEVT_inConlesMode(&subArgs);
    if (result == IOC_RESULT_SUCCESS) {
        pMgr->initialized = true;
        pMgr->subscribed = true;
        printf("模块 [%s] 初始化成功\n", moduleName);
    }
    
    return result;
}

// 模块清理
void ModuleCleanup(ModuleManager_T *pMgr) {
    if (pMgr->subscribed) {
        IOC_UnsubEvtArgs_T unsubArgs = {
            .CbProcEvt_F = ModuleCallback,
            .pCbPrivData = &pMgr->context
        };
        
        IOC_unsubEVT_inConlesMode(&unsubArgs);
        pMgr->subscribed = false;
    }
    
    if (pMgr->initialized) {
        printf("模块 [%s] 清理完成\n", pMgr->context.moduleName);
        pMgr->initialized = false;
    }
}
```

#### 4.2 常见错误和避免方法

```c
// ❌ 错误示例：重复订阅相同的回调和私有数据
void BadExample() {
    IOC_SubEvtArgs_T subArgs = {
        .CbProcEvt_F = MyCallback,
        .pCbPrivData = &privateData,
        .EvtNum = 1,
        .pEvtIDs = events
    };
    
    IOC_subEVT_inConlesMode(&subArgs); // 第一次订阅
    IOC_subEVT_inConlesMode(&subArgs); // ❌ 重复订阅会失败
}

// ✅ 正确示例：检查是否已订阅
bool isSubscribed = false;

void GoodExample() {
    if (!isSubscribed) {
        IOC_SubEvtArgs_T subArgs = {
            .CbProcEvt_F = MyCallback,
            .pCbPrivData = &privateData,
            .EvtNum = 1,
            .pEvtIDs = events
        };
        
        if (IOC_subEVT_inConlesMode(&subArgs) == IOC_RESULT_SUCCESS) {
            isSubscribed = true;
        }
    }
}
```

### 5. 完整的模块示例

```c
// 音频处理模块示例
typedef struct {
    uint32_t samplesProcessed;
    uint32_t bufferSize;
    bool isProcessing;
    char moduleName[32];
} AudioProcessor_T;

static IOC_Result_T AudioCallback(IOC_EvtDesc_pT pEvtDesc, void *pCbPriv) {
    AudioProcessor_T *pAudio = (AudioProcessor_T *)pCbPriv;
    
    switch (pEvtDesc->EvtID) {
        case IOC_EVTID_AUDIO_DATA_READY:
            if (pAudio->isProcessing) {
                // 处理音频数据
                pAudio->samplesProcessed++;
                printf("[%s] 处理音频样本 #%d\n", 
                       pAudio->moduleName, pAudio->samplesProcessed);
                
                // 处理完成后发布结果事件
                IOC_EvtDesc_T resultEvent = {
                    .EvtID = IOC_EVTID_AUDIO_PROCESSED,
                    .DataLen = sizeof(uint32_t),
                    .pData = &pAudio->samplesProcessed
                };
                IOC_postEVT_inConlesMode(&resultEvent, NULL);
            }
            break;
            
        case IOC_EVTID_MODULE_START:
            pAudio->isProcessing = true;
            printf("[%s] 音频处理模块启动\n", pAudio->moduleName);
            break;
            
        case IOC_EVTID_MODULE_STOP:
            pAudio->isProcessing = false;
            printf("[%s] 音频处理模块停止，共处理 %d 个样本\n", 
                   pAudio->moduleName, pAudio->samplesProcessed);
            break;
    }
    
    return IOC_RESULT_SUCCESS;
}

// 音频模块的完整生命周期
void AudioModuleExample() {
    // 初始化音频处理器
    static AudioProcessor_T audioProcessor = {
        .samplesProcessed = 0,
        .bufferSize = 1024,
        .isProcessing = false,
        .moduleName = "AudioProcessor"
    };
    
    // 订阅音频相关事件
    IOC_EvtID_T audioEvents[] = {
        IOC_EVTID_AUDIO_DATA_READY,
        IOC_EVTID_MODULE_START,
        IOC_EVTID_MODULE_STOP
    };
    
    IOC_SubEvtArgs_T audioSubArgs = {
        .CbProcEvt_F = AudioCallback,
        .pCbPrivData = &audioProcessor,
        .EvtNum = IOC_calcArrayElmtCnt(audioEvents),
        .pEvtIDs = audioEvents
    };
    
    IOC_Result_T result = IOC_subEVT_inConlesMode(&audioSubArgs);
    if (result != IOC_RESULT_SUCCESS) {
        printf("音频模块订阅失败: %s\n", IOC_getResultStr(result));
        return;
    }
    
    printf("音频模块初始化完成\n");
    
    // ... 模块运行期间 ...
    
    // 清理阶段：取消订阅
    IOC_UnsubEvtArgs_T audioUnsubArgs = {
        .CbProcEvt_F = AudioCallback,
        .pCbPrivData = &audioProcessor
    };
    
    result = IOC_unsubEVT_inConlesMode(&audioUnsubArgs);
    if (result != IOC_RESULT_SUCCESS) {
        printf("音频模块取消订阅失败: %s\n", IOC_getResultStr(result));
    } else {
        printf("音频模块清理完成\n");
    }
}
```

## asEvtProducer（事件生产者）

### 1. 基础事件发布

作为事件生产者，您可以发布事件通知其他模块。使用 `IOC_postEVT_inConlesMode` 函数来发布事件。

```c
// 基础事件发布
void PostBasicEvent() {
    IOC_EvtDesc_T EvtDesc = {
        .EvtID = IOC_EVTID_TEST_KEEPALIVE,
        .DataLen = 0,      // 无附加数据
        .pData = NULL      // 无附加数据
    };

    IOC_Result_T Result = IOC_postEVT_inConlesMode(&EvtDesc, NULL);
    if (Result != IOC_RESULT_SUCCESS) {
        printf("发布事件失败: %s\n", IOC_getResultStr(Result));
    } else {
        printf("成功发布 KeepAlive 事件\n");
    }
}

// 带数据的事件发布
void PostEventWithData() {
    // 准备要传递的数据
    typedef struct {
        uint32_t frameId;
        uint32_t timestamp;
        uint16_t width;
        uint16_t height;
    } FrameInfo_T;
    
    static FrameInfo_T frameInfo = {
        .frameId = 12345,
        .timestamp = 1640995200,  // Unix 时间戳
        .width = 1920,
        .height = 1080
    };
    
    IOC_EvtDesc_T EvtDesc = {
        .EvtID = IOC_EVTID_FRAME_CAPTURED,
        .DataLen = sizeof(FrameInfo_T),
        .pData = &frameInfo
    };

    IOC_Result_T Result = IOC_postEVT_inConlesMode(&EvtDesc, NULL);
    if (Result != IOC_RESULT_SUCCESS) {
        printf("发布帧事件失败: %s\n", IOC_getResultStr(Result));
    } else {
        printf("成功发布帧事件 [ID=%d, %dx%d]\n", 
               frameInfo.frameId, frameInfo.width, frameInfo.height);
    }
}
```

### 2. 高级事件发布模式

#### 2.1 批量事件发布

```c
// 批量发布事件（适用于需要快速发送多个相关事件的场景）
void PostBatchEvents() {
    const uint32_t eventCount = 10;
    
    for (uint32_t i = 0; i < eventCount; i++) {
        uint32_t sequenceId = i + 1;
        
        IOC_EvtDesc_T EvtDesc = {
            .EvtID = IOC_EVTID_SEQUENCE_EVENT,
            .DataLen = sizeof(uint32_t),
            .pData = &sequenceId
        };
        
        IOC_Result_T Result = IOC_postEVT_inConlesMode(&EvtDesc, NULL);
        if (Result != IOC_RESULT_SUCCESS) {
            printf("批量事件发布失败 [序号=%d]: %s\n", sequenceId, IOC_getResultStr(Result));
            
            // 处理队列满的情况
            if (Result == IOC_RESULT_TOO_MANY_QUEUING_EVTDESC) {
                printf("事件队列已满，等待处理...\n");
                // 强制处理积压的事件
                IOC_forceProcEVT();
                
                // 重试发布
                Result = IOC_postEVT_inConlesMode(&EvtDesc, NULL);
                if (Result != IOC_RESULT_SUCCESS) {
                    printf("重试后仍然失败，跳过事件 [序号=%d]\n", sequenceId);
                    continue;
                }
            }
        }
        
        printf("成功发布序列事件 [序号=%d]\n", sequenceId);
    }
}
```

#### 2.2 条件事件发布

```c
// 根据条件发布不同的事件
typedef struct {
    float temperature;
    float humidity;
    uint32_t sensorId;
} SensorData_T;

void PostSensorEvent(const SensorData_T *pSensorData) {
    if (!pSensorData) {
        printf("错误：传感器数据为空\n");
        return;
    }
    
    // 根据温度值发布不同的事件
    IOC_EvtID_T eventId;
    const char *eventType;
    
    if (pSensorData->temperature > 50.0f) {
        eventId = IOC_EVTID_TEMPERATURE_HIGH;
        eventType = "高温警告";
    } else if (pSensorData->temperature < 0.0f) {
        eventId = IOC_EVTID_TEMPERATURE_LOW;
        eventType = "低温警告";
    } else {
        eventId = IOC_EVTID_TEMPERATURE_NORMAL;
        eventType = "温度正常";
    }
    
    IOC_EvtDesc_T EvtDesc = {
        .EvtID = eventId,
        .DataLen = sizeof(SensorData_T),
        .pData = (void *)pSensorData
    };
    
    IOC_Result_T Result = IOC_postEVT_inConlesMode(&EvtDesc, NULL);
    if (Result == IOC_RESULT_SUCCESS) {
        printf("传感器事件 [%s]: 温度=%.1f°C, 湿度=%.1f%%, 传感器ID=%d\n",
               eventType, pSensorData->temperature, pSensorData->humidity, pSensorData->sensorId);
    } else {
        printf("传感器事件发布失败: %s\n", IOC_getResultStr(Result));
    }
}

// 使用示例
void SensorMonitorExample() {
    SensorData_T sensorReadings[] = {
        {25.5f, 60.2f, 1001},  // 正常温度
        {55.3f, 45.8f, 1002},  // 高温
        {-5.1f, 80.1f, 1003}   // 低温
    };
    
    for (size_t i = 0; i < IOC_calcArrayElmtCnt(sensorReadings); i++) {
        PostSensorEvent(&sensorReadings[i]);
    }
}
```

### 3. 事件处理控制

#### 3.1 强制事件处理

使用 `IOC_forceProcEVT` 函数强制处理所有挂起的事件。此函数会阻塞当前线程，直到所有事件都被处理完毕。

```c
void PostAndForceProcessEvent() {
    printf("开始发布事件序列...\n");
    
    // 发布多个事件
    for (int i = 0; i < 5; i++) {
        IOC_EvtDesc_T EvtDesc = {
            .EvtID = IOC_EVTID_TEST_KEEPALIVE,
            .DataLen = sizeof(int),
            .pData = &i
        };
        
        IOC_Result_T Result = IOC_postEVT_inConlesMode(&EvtDesc, NULL);
        if (Result != IOC_RESULT_SUCCESS) {
            printf("发布事件 %d 失败: %s\n", i, IOC_getResultStr(Result));
        } else {
            printf("已发布事件 %d\n", i);
        }
    }
    
    printf("强制处理所有挂起的事件...\n");
    // 强制处理事件 - 这是一个同步操作，会等待所有事件处理完成
    IOC_forceProcEVT();
    printf("所有事件处理完成\n");
}
```

#### 3.2 唤醒事件处理

使用 `IOC_wakeupProcEVT` 函数唤醒事件处理线程。与 `IOC_forceProcEVT` 不同，这是一个异步操作。

```c
void PostAndWakeupProcessEvent() {
    printf("发布事件并异步触发处理...\n");
    
    // 发布事件
    IOC_EvtDesc_T EvtDesc = {
        .EvtID = IOC_EVTID_TEST_KEEPALIVE,
        .DataLen = 0,
        .pData = NULL
    };
    
    IOC_Result_T Result = IOC_postEVT_inConlesMode(&EvtDesc, NULL);
    if (Result != IOC_RESULT_SUCCESS) {
        printf("发布事件失败: %s\n", IOC_getResultStr(Result));
        return;
    }
    
    printf("事件已发布，唤醒处理线程...\n");
    // 唤醒事件处理 - 这是异步操作，不会等待处理完成
    IOC_wakeupProcEVT();
    printf("处理线程已被唤醒，继续执行其他任务...\n");
    
    // 可以继续执行其他任务，事件会在后台异步处理
}
```

### 4. 高性能事件发布

#### 4.1 非阻塞事件发布

```c
#include "IOC_Option.h"  // 包含选项定义

// 非阻塞方式发布事件（推荐用于高频事件发布）
void PostNonBlockingEvent() {
    // 定义非阻塞选项
    IOC_Option_defineNonBlock(optNonBlock);
    
    IOC_EvtDesc_T EvtDesc = {
        .EvtID = IOC_EVTID_HIGH_FREQ_DATA,
        .DataLen = 0,
        .pData = NULL
    };
    
    IOC_Result_T Result = IOC_postEVT_inConlesMode(&EvtDesc, &optNonBlock);
    
    switch (Result) {
        case IOC_RESULT_SUCCESS:
            printf("非阻塞事件发布成功\n");
            break;
            
        case IOC_RESULT_TOO_MANY_QUEUING_EVTDESC:
            printf("警告：事件队列已满，事件被丢弃\n");
            // 在高频发布场景中，可以选择忽略这个错误或记录统计信息
            break;
            
        default:
            printf("非阻塞事件发布失败: %s\n", IOC_getResultStr(Result));
            break;
    }
}

// 高频数据发布示例
void HighFrequencyDataPublisher() {
    const uint32_t maxEvents = 1000;
    uint32_t successCount = 0;
    uint32_t droppedCount = 0;
    
    IOC_Option_defineNonBlock(optNonBlock);
    
    printf("开始高频事件发布 (%d 个事件)...\n", maxEvents);
    
    for (uint32_t i = 0; i < maxEvents; i++) {
        uint32_t data = i;
        IOC_EvtDesc_T EvtDesc = {
            .EvtID = IOC_EVTID_HIGH_FREQ_DATA,
            .DataLen = sizeof(uint32_t),
            .pData = &data
        };
        
        IOC_Result_T Result = IOC_postEVT_inConlesMode(&EvtDesc, &optNonBlock);
        
        if (Result == IOC_RESULT_SUCCESS) {
            successCount++;
        } else if (Result == IOC_RESULT_TOO_MANY_QUEUING_EVTDESC) {
            droppedCount++;
        } else {
            printf("发布事件 %d 失败: %s\n", i, IOC_getResultStr(Result));
        }
        
        // 可选：添加小延迟以模拟真实数据生成
        // usleep(100);  // 100 微秒
    }
    
    printf("高频发布完成: 成功=%d, 丢弃=%d, 总计=%d\n", 
           successCount, droppedCount, maxEvents);
}
```

#### 4.2 同步事件发布

```c
// 同步方式发布事件（确保事件被处理后再继续）
void PostSynchronousEvent() {
    // 定义同步选项
    IOC_Option_defineSync(optSync);
    
    printf("发布同步事件...\n");
    
    IOC_EvtDesc_T EvtDesc = {
        .EvtID = IOC_EVTID_CRITICAL_EVENT,
        .DataLen = 0,
        .pData = NULL
    };
    
    IOC_Result_T Result = IOC_postEVT_inConlesMode(&EvtDesc, &optSync);
    
    if (Result == IOC_RESULT_SUCCESS) {
        printf("同步事件已被完全处理\n");
    } else {
        printf("同步事件发布失败: %s\n", IOC_getResultStr(Result));
    }
}
```

### 5. 事件发布模式选择指南

| 使用场景     | 推荐方式                                       | 选项         | 特点                 |
| ------------ | ---------------------------------------------- | ------------ | -------------------- |
| 普通事件通知 | `IOC_postEVT_inConlesMode(&evt, NULL)`         | 默认（异步） | 平衡性能和可靠性     |
| 高频数据流   | `IOC_postEVT_inConlesMode(&evt, &optNonBlock)` | 非阻塞       | 高性能，允许丢失     |
| 关键事件     | `IOC_postEVT_inConlesMode(&evt, &optSync)`     | 同步         | 确保处理完成         |
| 批量处理后   | `IOC_forceProcEVT()`                           | 强制处理     | 立即处理所有挂起事件 |
| 后台异步     | `IOC_wakeupProcEVT()`                          | 唤醒处理     | 触发异步处理         |

### 6. 实际应用示例

#### 6.1 视频流处理系统

```c
// 视频帧生产者模块
typedef struct {
    uint32_t frameCount;
    uint32_t droppedFrames;
    bool isStreaming;
    uint32_t targetFPS;
} VideoProducer_T;

void VideoFrameGenerator(VideoProducer_T *pProducer) {
    if (!pProducer->isStreaming) {
        return;
    }
    
    // 模拟生成视频帧
    typedef struct {
        uint32_t frameId;
        uint64_t timestamp;
        uint32_t size;
    } VideoFrame_T;
    
    VideoFrame_T frame = {
        .frameId = ++pProducer->frameCount,
        .timestamp = getCurrentTimestamp(),  // 假设的时间戳函数
        .size = 1920 * 1080 * 3  // RGB 数据大小
    };
    
    IOC_EvtDesc_T frameEvent = {
        .EvtID = IOC_EVTID_VIDEO_FRAME_READY,
        .DataLen = sizeof(VideoFrame_T),
        .pData = &frame
    };
    
    // 使用非阻塞方式发布，允许在高负载时丢帧
    IOC_Option_defineNonBlock(optNonBlock);
    IOC_Result_T result = IOC_postEVT_inConlesMode(&frameEvent, &optNonBlock);
    
    if (result == IOC_RESULT_SUCCESS) {
        printf("视频帧 #%d 发布成功\n", frame.frameId);
    } else if (result == IOC_RESULT_TOO_MANY_QUEUING_EVTDESC) {
        pProducer->droppedFrames++;
        printf("视频帧 #%d 被丢弃（队列满）\n", frame.frameId);
    } else {
        printf("视频帧 #%d 发布失败: %s\n", frame.frameId, IOC_getResultStr(result));
    }
}

// 定期报告统计信息
void ReportVideoStats(const VideoProducer_T *pProducer) {
    IOC_EvtDesc_T statsEvent = {
        .EvtID = IOC_EVTID_VIDEO_STATS,
        .DataLen = sizeof(VideoProducer_T),
        .pData = (void *)pProducer
    };
    
    IOC_Result_T result = IOC_postEVT_inConlesMode(&statsEvent, NULL);
    if (result == IOC_RESULT_SUCCESS) {
        printf("视频统计信息已发布: 总帧数=%d, 丢帧数=%d\n", 
               pProducer->frameCount, pProducer->droppedFrames);
    }
}
```

#### 6.2 系统状态监控

```c
// 系统监控模块
typedef enum {
    SYS_STATE_IDLE,
    SYS_STATE_RUNNING,
    SYS_STATE_ERROR,
    SYS_STATE_SHUTDOWN
} SystemState_T;

void PublishSystemStateChange(SystemState_T oldState, SystemState_T newState) {
    typedef struct {
        SystemState_T oldState;
        SystemState_T newState;
        uint64_t timestamp;
    } StateChangeEvent_T;
    
    StateChangeEvent_T stateChange = {
        .oldState = oldState,
        .newState = newState,
        .timestamp = getCurrentTimestamp()
    };
    
    IOC_EvtDesc_T stateEvent = {
        .EvtID = IOC_EVTID_SYSTEM_STATE_CHANGED,
        .DataLen = sizeof(StateChangeEvent_T),
        .pData = &stateChange
    };
    
    // 系统状态变化是关键事件，使用同步方式确保被处理
    IOC_Option_defineSync(optSync);
    IOC_Result_T result = IOC_postEVT_inConlesMode(&stateEvent, &optSync);
    
    if (result == IOC_RESULT_SUCCESS) {
        printf("系统状态变化: %d -> %d (已确认处理)\n", oldState, newState);
    } else {
        printf("系统状态变化事件发布失败: %s\n", IOC_getResultStr(result));
    }
}

// 心跳事件发布
void PublishHeartbeat() {
    static uint32_t heartbeatCount = 0;
    heartbeatCount++;
    
    IOC_EvtDesc_T heartbeatEvent = {
        .EvtID = IOC_EVTID_SYSTEM_HEARTBEAT,
        .DataLen = sizeof(uint32_t),
        .pData = &heartbeatCount
    };
    
    // 心跳事件使用默认异步方式
    IOC_Result_T result = IOC_postEVT_inConlesMode(&heartbeatEvent, NULL);
    
    if (result == IOC_RESULT_SUCCESS) {
        printf("心跳 #%d 发布成功\n", heartbeatCount);
    } else {
        printf("心跳事件发布失败: %s\n", IOC_getResultStr(result));
    }
}
```

### 7. 最佳实践总结

#### 7.1 事件发布策略

```c
// ✅ 推荐的事件发布模式
void RecommendedEventPublishing() {
    // 1. 普通通知事件 - 使用默认异步方式
    IOC_EvtDesc_T normalEvent = {.EvtID = IOC_EVTID_NORMAL_NOTIFICATION};
    IOC_postEVT_inConlesMode(&normalEvent, NULL);
    
    // 2. 高频数据事件 - 使用非阻塞方式
    IOC_Option_defineNonBlock(optNonBlock);
    IOC_EvtDesc_T dataEvent = {.EvtID = IOC_EVTID_HIGH_FREQ_DATA};
    IOC_postEVT_inConlesMode(&dataEvent, &optNonBlock);
    
    // 3. 关键控制事件 - 使用同步方式确保被处理
    IOC_Option_defineSync(optSync);
    IOC_EvtDesc_T criticalEvent = {.EvtID = IOC_EVTID_CRITICAL_CONTROL};
    IOC_postEVT_inConlesMode(&criticalEvent, &optSync);
    
    // 4. 批量事件后 - 强制处理
    IOC_forceProcEVT();
}
```

#### 7.2 错误处理模式

```c
// 健壮的事件发布函数
IOC_Result_T RobustEventPublish(IOC_EvtID_T eventId, void *pData, size_t dataLen, bool isCritical) {
    IOC_EvtDesc_T event = {
        .EvtID = eventId,
        .DataLen = dataLen,
        .pData = pData
    };
    
    IOC_Result_T result;
    
    if (isCritical) {
        // 关键事件使用同步方式，并重试
        IOC_Option_defineSync(optSync);
        
        for (int retry = 0; retry < 3; retry++) {
            result = IOC_postEVT_inConlesMode(&event, &optSync);
            if (result == IOC_RESULT_SUCCESS) {
                break;
            }
            
            printf("关键事件发布失败，重试 %d/3: %s\n", retry + 1, IOC_getResultStr(result));
            
            if (result == IOC_RESULT_TOO_MANY_QUEUING_EVTDESC) {
                // 队列满时，强制处理后重试
                IOC_forceProcEVT();
            }
        }
    } else {
        // 普通事件使用默认方式
        result = IOC_postEVT_inConlesMode(&event, NULL);
        
        if (result == IOC_RESULT_TOO_MANY_QUEUING_EVTDESC) {
            // 可选：触发异步处理
            IOC_wakeupProcEVT();
        }
    }
    
    return result;
}
```

---

# API 参考手册

👉 **[完整 API 参考手册](README_RefAPIs.md)** 👈
