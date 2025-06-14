# UT_FreelyDrafts 使用指南

## 🎯 文件用途
`UT_FreelyDrafts.cxx` 是一个**测试开发工作台**，用于：
- 💭 快速记录测试想法
- 🏗️ 从草稿演进到正式测试
- 📚 提供测试开发模板
- 🔧 实验新的测试方法

## 🚀 快速开始

### 1. 自由草稿阶段
```cpp
// 在文件末尾直接写下你的想法
TEST(UT_FreelyDrafts, myQuickIdea) {
    // 任何想法都可以先写在这里
    printf("Testing my idea about...\n");
    // 不用担心格式，先把想法记录下来
}
```

### 2. 结构化阶段
当想法变得清晰时，使用提供的模板：
```cpp
TEST(UT_MyModule, verifyBehavior_byAction_expectResult) {
    //===SETUP===
    // 准备测试环境
    
    //===BEHAVIOR===
    // 执行被测试的行为
    
    //===VERIFY===
    // 验证结果
    
    //===CLEANUP===
    // 清理资源
}
```

## 📋 测试分类指南

| 分类 | 用途 | 示例 |
|------|------|------|
| **FreelyDrafts** | 自由想法记录 | 任何临时想法 |
| **Typical** | 典型用例 | IOC基本使用流程 |
| **Boundary** | 边界条件 | 参数边界值测试 |
| **State** | 状态转换 | FSM状态验证 |
| **Performance** | 性能测试 | API调用速度测试 |
| **Concurrency** | 并发测试 | 多线程安全性 |
| **Robust** | 鲁棒性 | 重复达到容量限制 |
| **Fault** | 故障处理 | 进程崩溃恢复 |
| **Misuse** | 误用检测 | 错误调用顺序 |

## 🔄 开发流程

1. **💭 想法阶段**: 在文件末尾自由记录想法
2. **📝 分析阶段**: 将想法转换为User Story
3. **✅ 验收标准**: 定义Acceptance Criteria  
4. **🧪 测试用例**: 编写具体的Test Cases
5. **⚡ 实现**: 编写测试代码
6. **🔧 重构**: 移动到专门的测试文件

## 💡 最佳实践

### ✅ 推荐做法
- 先写想法，后整理格式
- 使用描述性的测试名称
- 保持每个测试案例的独立性
- 定期将成熟的测试移动到专门文件

### ❌ 避免事项
- 不要在草稿阶段纠结格式
- 避免过于复杂的测试逻辑
- 不要让FreelyDrafts文件过于庞大

## 🎯 命名约定

### 测试用例命名
```
verify[被测行为]_by[测试动作]_expect[期望结果]
```

示例：
- `verifyServiceRegistration_byValidInput_expectSuccess`
- `verifyEventHandling_byNullPointer_expectFailure`

### 测试套件命名
```
UT_[模块名]_[测试类别]
```

示例：
- `UT_IOCService_Typical`
- `UT_EventQueue_Boundary`

## 📚 相关文件
- `_UT_IOC_Common.h`: 通用测试工具
- `UT_*Typical.cxx`: 典型用例测试
- `UT_*Boundary.cxx`: 边界条件测试
