# UT_ConlesEventMayBlock

## 概览

本单元测试文件旨在验证在 Conles 模式下，当事件可能阻塞时的行为。当事件队列在异步模式下已满或同步模式下非空时，事件生产者在调用 `IOC_postEVT_inConlesMode` 时可能需要等待，以确保事件能够被 IOC 正常处理。

## 用户故事

- **US-1**：作为事件生产者，当我调用 `IOC_postEVT_inConlesMode` 时，如果自动链接的内部事件描述队列在异步模式下已满或同步模式下非空，我希望等待一段时间，以确保发布的事件能被 IOC 处理。

## 验收标准

- **AC-1**：给定事件生产者调用 `IOC_postEVT_inConlesMode`，当 IOC 的事件描述队列在异步模式下已满，事件生产者将等待，直到队列有空间，并且发布的事件将在合理的短时间内被 IOC 处理。

- **AC-2**：给定事件生产者调用 `IOC_postEVT_inConlesMode`，当 IOC 的事件描述队列在同步模式下非空，事件生产者将等待，直到队列为空，并且发布的事件将被 IOC 处理。

- **AC-3**：在高负载场景下，事件生产者调用 `IOC_postEVT_inConlesMode` 时，当 IOC 的事件描述队列已满或非空，系统不会崩溃，并且发布的事件将在合理的时间内被 IOC 处理。

## 测试用例

- **TC-1.1**（对应 AC-1）：
  - **名称**：`verifyASyncBlock_byPostOneMoreEVT_whenEvtDescQueueFull`
  - **目的**：验证当 IOC 的事件描述队列在异步模式下已满时，事件生产者将等待。

- **TC-2.1**（对应 AC-2）：
  - **名称**：`verifySyncBlock_byPostOneMoreEVT_whenEvtDescQueueNotEmpty`
  - **目的**：验证当 IOC 的事件描述队列在同步模式下非空时，事件生产者将等待。
