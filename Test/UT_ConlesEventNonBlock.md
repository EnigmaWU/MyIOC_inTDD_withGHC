# 非阻塞连续事件单元测试

## 用户故事

**US-1**: 作为一个事件生产者，当我调用 `IOC_postEVT_inConlesMode` 时，我希望在以下情况下立即返回而不等待：

- 在异步模式下，如果自动链接的内部事件描述队列已满。
- 在同步模式下，如果事件描述队列不为空。

这样我就能继续工作而不会意外阻塞。

## 验收标准

**AC-1@US-1**:  
给定事件生产者调用 `IOC_postEVT_inConlesMode`，当事件消费者的回调函数在异步模式下阻塞，导致 IOC 的事件描述队列已满时，事件生产者应立即返回而不等待，且发布的事件描述不会被 IOC 处理。

**AC-2@US-1**:  
给定事件生产者调用 `IOC_postEVT_inConlesMode`，当 IOC 的事件描述队列在同步模式下不为空时，事件生产者应立即返回而不等待，且发布的事件描述不会被 IOC 处理。

**AC-3@US-1**:  
给定事件消费者的回调函数可能意外阻塞，当多个事件生产者在异步或同步模式下调用 `IOC_postEVT_inConlesMode` 时，如果：

- 在异步模式下，IOC 的事件描述队列已满。
- 在同步模式下，事件描述队列不为空。

那么事件生产者应立即返回而不等待。

## 测试用例

**TC-1**: `verifyASyncNonblock_byPostOneMoreEVT_whenEvtDescQueueFull`

**目的**:  
根据 AC-1，验证当 IOC 的事件描述队列在异步模式下已满时，事件生产者调用 `IOC_postEVT_inConlesMode` 能够立即返回而不等待。

**步骤**:

1. 调用 `IOC_getCapability` 获取自动链接的事件描述队列深度，作为设置。
2. 使用回调函数 `__TC1_cbProcEvt` 调用 `IOC_subEVT(TEST_KEEPALIVE)`，作为设置。
3. 首次在异步模式下调用 `IOC_postEVT(TEST_KEEPALIVE)`，作为行为：
   - 等待 `__TC1_cbProcEvt` 被调用并阻塞，以避免进一步的事件处理。
   - 在异步模式下调用更多的 `IOC_postEVT(TEST_KEEPALIVE)`，以填满事件描述队列。
4. 再次在异步模式下调用 `IOC_postEVT(TEST_KEEPALIVE)`，作为验证：
   - 检查返回值是否为 `IOC_RESULT_TOO_MANY_QUEUING_EVTDESC`。
5. 调用 `IOC_unsubEVT(TEST_KEEPALIVE)`，作为清理。

**期望结果**:  
步骤 4 的返回值应为 `IOC_RESULT_TOO_MANY_QUEUING_EVTDESC`。

**备注**:  
在 `__TC1_cbProcEvt` 中递增 `KeepAliveCnt`，最终应等于队列深度。

---

**TC-2**: `verifySyncNonblock_byPostOneMoreEVT_whenEvtDescQueueNotEmpty`

**目的**:  
根据 AC-2，验证当 IOC 的事件描述队列在同步模式下不为空时，事件生产者调用 `IOC_postEVT_inConlesMode` 能够立即返回而不等待。

**步骤**:

1. 使用回调函数 `__TC2_cbProcEvt` 调用 `IOC_subEVT(TEST_KEEPALIVE)`，作为设置。
2. 首次在异步模式下调用 `IOC_postEVT(TEST_KEEPALIVE)`，作为行为：
   - 等待 `__TC2_cbProcEvt` 被调用并阻塞，以指示事件描述队列不为空。
3. 在同步模式下再次调用 `IOC_postEVT(TEST_KEEPALIVE)`，作为验证：
   - 检查返回值是否为 `IOC_RESULT_TOO_LONG_EMPTYING_EVTDESC_QUEUE`。
4. 调用 `IOC_unsubEVT(TEST_KEEPALIVE)`，作为清理。

**期望结果**:  
步骤 3 的返回值应为 `IOC_RESULT_TOO_LONG_EMPTYING_EVTDESC_QUEUE`。

---

**TC-3**: `verifyHybridNonblock_byAlternatelyCbProcEvtBlockedOrNot_withHighConcurrency`

**目的**:  
根据 AC-3，验证当事件消费者的回调函数可能阻塞时，在高并发情况下，事件生产者在调用 `IOC_postEVT_inConlesMode` 时，如果满足上述条件，能够立即返回而不等待。

**步骤**:

1. 使用回调函数 `__TC3_cbProcEvt` 调用 `IOC_subEVT(TEST_KEEPALIVE)` 和 `TEST_SLEEP_9US`，作为设置：
   - 在 `__TC3_cbProcEvt` 中：
     - 如果 `EvtID` 为 `TEST_KEEPALIVE`，则递增 `CbKeepAliveCnt`。
     - 如果 `EvtID` 为 `TEST_SLEEP_9US`，则睡眠 9 微秒并递增 `CbSleep9USCnt`。
2. 创建 `_TC3_MAX_N_ASYNC_THREADS` 个异步线程和 `_TC3_MAX_M_SYNC_THREADS` 个同步线程，作为设置。
3. 在每个异步线程中，调用 `_TC3_MAX_NN_EVENTS` 次异步模式的 `postEVT`，作为行为：
   - 默认发送 `TEST_KEEPALIVE` 事件，每 10000 次发送一次 `TEST_SLEEP_9US`。
   - 如果返回值为 `IOC_RESULT_SUCCESS`，则递增 `ASyncPostSuccessCnt`。
   - 如果返回值为 `IOC_RESULT_TOO_MANY_QUEUING_EVTDESC`，则递增 `ASyncPostNonBlockCnt`。
4. 在每个同步线程中，调用 `_TC3_MAX_MM_EVENTS` 次同步模式的 `postEVT`，重复步骤 3 的操作。
5. 检查并验证以下计数器：
   - `TotalASyncSuccessPostCnt`：所有异步线程的 `ASyncPostSuccessCnt` 之和。
   - `TotalSyncPostSuccessCnt`：所有同步线程的 `SyncPostSuccessCnt` 之和。
   - `TotalPostSuccessCnt`：`TotalASyncSuccessPostCnt` 与 `TotalSyncPostSuccessCnt` 之和。
   - `TotalPostSuccessCnt` 应等于 `CbKeepAliveCnt` 与 `CbSleep9USCnt` 之和。
6. 调用 `IOC_unsubEVT(TEST_KEEPALIVE)` 和 `TEST_SLEEP_9US`，作为清理。

**期望结果**:  
总的成功发布事件次数应与回调函数处理的事件次数相等。
