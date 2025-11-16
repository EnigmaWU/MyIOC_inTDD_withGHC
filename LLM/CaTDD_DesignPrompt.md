# CaTDD Design Prompt

**Short name**: CaTDD (Comment‑alive Test‑Driven Development)

- `CaTDD` is a LLM friendly TDD.
  - `Comment-alive` means detail design in comments, coexist with test code and production code.
  - `TDD` is same meaning as triditional TDD.
  - `EnigmaWU` named this method and practicing from 2023.10.

## Purpose
Turn design intent into executable tests by writing rich, structured comments first, then evolving them into unit tests and code. The test file serves as a living design document for humans and LLMs.

## Core Principles

### Design Philosophy
- **Improve Value**: Focus on tests that verify critical business value and user needs
- **Avoid Loss**: Prevent regressions, data corruption, and security vulnerabilities
- **Balance Skill vs Cost**: Match test complexity to team capability and project constraints

### Development Flow
- **Design before code**: Write comprehensive design in comments before implementation
- **Draft freely, then systematize**: Start with raw ideas, refine into structured US/AC/TC
- **TDD Red→Green cycle**: Write failing test first (RED), implement to pass (GREEN), refactor

### Status Tracking
- ⚪ **TODO/PLANNED**: Designed but not implemented yet
- 🔴 **RED/IMPLEMENTED**: Test exists and failing, waiting for production code
- 🟢 **GREEN/PASSED**: Test implemented and passing
- ⚠️ **ISSUES**: Known problem requiring attention

### Risk-Driven Prioritization
Prioritize test categories by: **Impact × Likelihood × Uncertainty**
- Score each: 1 (low) to 3 (high) = max 27 points
- High-risk items (≥18) move forward in priority

## Workflow

### Phase 1: Design & Planning

**Step 1: Define Coverage Strategy**
- Identify key dimensions for systematic coverage
- Examples:
  - Service Role × Client Role × Mode (Producer/Consumer × Callback/Pull)
  - Component State × Operation × Boundary (Init/Ready/Running × Start/Stop × Min/Max)
  - Concurrency × Resource Limits × Faults (Multi-thread × Buffer Full × Network Fail)

**Step 2: Freely Draft Ideas**
- Capture test ideas quickly without format constraints
- Use "FreelyDrafts" section in test file
- Focus on "what if" scenarios and intuitive cases
- Don't worry about structure yet

**Step 3: Build Coverage Matrix**
- Create systematic enumeration of test scenarios
- Use table format to ensure completeness
- Map scenarios to User Stories

### Phase 2: Structured Design

**Step 4: Write User Stories (US)**
- Express value from user/role perspective
- Format: "As a [role], I want [capability], so that [value]"
- Focus on business value and user needs
- At least 1 US, but typically 2-5 for a module

**Step 5: Define Acceptance Criteria (AC)**
- Make US testable with unambiguous conditions
- Format: "GIVEN [context], WHEN [action], THEN [result]"
- At least 1 AC per US, typically 2-4
- Each AC should be independently verifiable

**Step 6: Specify Test Cases (TC)**
- Detail concrete steps and expectations for each AC
- Naming: `verifyBehavior_byCondition_expectResult`
- At least 1 TC per AC, add more for edge cases
- Keep ≤3 key assertions per test (add separate tests if needed)

### Phase 3: Implementation

**Step 7: Prioritize & Track Status**
- Default order: Typical → Boundary → State → Misuse → Fault → Performance → Concurrency
- Adjust based on risk scoring (Impact × Likelihood × Uncertainty)
- Mark each TC with status: ⚪TODO → 🔴RED → 🟢GREEN

**Step 8: TDD Red→Green Cycle**
1. Write test first (should fail for missing feature)
2. Run test, confirm RED/failing
3. Implement minimal production code to pass
4. Run test, confirm GREEN/passing
5. Refactor both test and production code
6. Repeat for next test case

**Step 9: Refactor & Organize**
- Move mature, stable tests to category-specific files
- Extract common setup/teardown to fixtures
- Simplify test code while preserving coverage
- Update documentation and remove obsolete comments

## Coverage Matrix Template

### Basic 2D Matrix
```
┌─────────────────┬─────────────┬──────────────────────────────┐
│ Dimension 1     │ Dimension 2 │ Key Scenarios                │
├─────────────────┼─────────────┼──────────────────────────────┤
│ Value A         │ Value X     │ US-1: Core happy path        │
│ Value A         │ Value Y     │ US-2: Boundary condition     │
│ Value B         │ Value X     │ US-3: Error handling         │
└─────────────────┴─────────────┴──────────────────────────────┘
```

### Full 3D Matrix
```
┌─────────────────┬─────────────┬─────────────┬──────────────────────────────┐
│ Dimension 1     │ Dimension 2 │ Dimension 3 │ Key Scenarios                │
├─────────────────┼─────────────┼─────────────┼──────────────────────────────┤
│ Service Role    │ Client Role │ Mode        │ US-X: Description            │
├─────────────────┼─────────────┼─────────────┼──────────────────────────────┤
│ EvtProducer     │ EvtConsumer │ Callback    │ US-1: Async event flow       │
│ EvtProducer     │ EvtConsumer │ Pull        │ US-2: Sync event flow        │
│ EvtConsumer     │ EvtProducer │ Callback    │ US-3: Reversed flow          │
└─────────────────┴─────────────┴─────────────┴──────────────────────────────┘
```

### Real-World Examples

**IOC Event System**
- Service Role: EvtProducer, EvtConsumer, Mixed
- Client Role: EvtConsumer, EvtProducer, Mixed
- Mode: Callback, Pull/Poll, Both

**State Machine Component**
- State: Init, Ready, Running, Stopped, Error
- Operation: Start, Stop, Pause, Resume, Reset
- Boundary: First call, Last call, Max transitions

**Concurrent Queue**
- Concurrency: Single-thread, Multi-thread, High-contention
- Resource: Empty, Partial, Full, Overflow
- Operation: Push, Pop, Peek, Clear

Status indicators
- GREEN/PASSED: implemented and verified
- RED/IMPLEMENTED: test exists and currently failing or pending behavior
- TODO/PLANNED: designed but not implemented
- ISSUES: known problem needing fix

## Test Classification Guide

### Default Priority Order
Typical → Boundary → Misuse → State → Fault → Performance → Concurrency → Capability → Robust → Demo/Example → Compatibility → Configuration

### Category Definitions

**1. Typical (Must-Have)** ⭐ *Core Functionality*
- **Purpose**: Verify main usage scenarios and happy paths
- **When**: First priority, fundamental behavior verification
- **Examples**:
  - IOC service registration and lookup
  - Event subscription and publishing
  - Command execution with expected response
  - Normal data flow through system

**2. Boundary (Important)** 🔲 *Edge Cases*
- **Purpose**: Test edge cases, parameter limits, and mode variations
- **When**: High priority, right after typical cases
- **Examples**:
  - Min/Max values (zero timeout, max string length)
  - Null/empty inputs (null pointer, empty array)
  - Block/NonBlock/Timeout modes
  - Buffer full/empty conditions

**3. Misuse (Hardening)** 🚫 *Error Prevention*
- **Purpose**: Test incorrect usage patterns and API abuse
- **When**: After core functionality, before advanced features
- **Examples**:
  - Wrong parameter order or types
  - Illegal state transitions (post before init)
  - Double-close, double-init scenarios
  - Operations on invalid handles

**4. State (Key)** 🔄 *Lifecycle Management*
- **Purpose**: Verify state machine transitions and object lifecycle
- **When**: Essential for stateful components, FSM verification
- **Examples**:
  - Service states: Init→Ready→Running→Stopped
  - Event lifecycle: Created→Queued→Processing→Completed
  - Connection states: Disconnected→Connecting→Connected→Closing

**5. Fault (Reliability)** ⚠️ *Error Handling*
- **Purpose**: Test error handling, failures, and recovery
- **When**: Critical for reliability requirements
- **Examples**:
  - Process crash recovery
  - Network failures and timeouts
  - Disk full, memory exhausted
  - External dependency unavailable

**6. Performance (As Needed)** ⚡ *Speed & Efficiency*
- **Purpose**: Measure execution time, throughput, and resource usage
- **When**: After functional tests, when SLOs exist
- **Examples**:
  - API call latency under load
  - Memory leak detection
  - CPU usage monitoring
  - Throughput benchmarks

**7. Concurrency (Complex)** 🚀 *Thread Safety*
- **Purpose**: Test multi-threading, synchronization, and race conditions
- **When**: For concurrent components, high complexity
- **Examples**:
  - Parallel API calls from multiple threads
  - Shared resource access patterns
  - Race conditions and deadlock scenarios
  - Lock-free data structure validation

**8. Capability (Limits)** 🏆 *Capacity Testing*
- **Purpose**: Test maximum capacity and system limits
- **When**: After basic functionality, for capacity planning
- **Examples**:
  - Maximum concurrent connections
  - Queue/buffer capacity limits
  - Maximum message size
  - Resource pool exhaustion

**9. Robust (Stability)** 🛡️ *Long-Term Reliability*
- **Purpose**: Stress testing, repetition, and soak testing
- **When**: For stability verification, production readiness
- **Examples**:
  - Repeated operations (1000x)
  - Buffer overflow/underflow cycles
  - Long-running stress tests (24h+)
  - Resource exhaustion patterns

**10. Demo/Example** 🎨 *Documentation*
- **Purpose**: End-to-end feature demonstrations
- **When**: For documentation, tutorials, showcases
- **Examples**:
  - Complete workflow demonstrations
  - Tutorial code examples
  - Integration scenarios
  - Best practice illustrations

**11. Compatibility** 🔄 *Cross-Platform*
- **Purpose**: Test across different platforms, versions, configurations
- **When**: Multi-platform products, version upgrades
- **Examples**:
  - Windows/Linux/macOS variations
  - API version compatibility
  - Compiler differences
  - Legacy system integration

**12. Configuration** 🎛️ *Settings Validation*
- **Purpose**: Test different configuration scenarios
- **When**: Configurable systems, deployment variations
- **Examples**:
  - Debug vs Release builds
  - Different log levels
  - Feature flags on/off
  - Environment variable handling

## Context-Specific Priority Adjustments

### Quick Decision Matrix

**New Public API**
```
Typical → Boundary → Misuse → State → Fault → Performance → Concurrency
```
*Rationale*: Focus on correct usage patterns and preventing API misuse early

**Stateful/FSM-Heavy Component**
```
Typical → Boundary → State → Misuse → Fault → Concurrency → Performance
```
*Rationale*: State transitions are critical; verify lifecycle thoroughly

**Reliability-Critical Service**
```
Typical → Boundary → Fault → State → Misuse → Concurrency → Performance
```
*Rationale*: Error handling and recovery are paramount for uptime

**Throughput/Latency SLO Requirements**
```
Typical → Boundary → Performance → State → Concurrency → Fault → Capability
```
*Rationale*: Performance characteristics must be validated early

**Highly Concurrent Design**
```
Typical → Boundary → Concurrency → State → Fault → Performance → Capability
```
*Rationale*: Thread safety and race conditions are highest risk

**Data Processing Pipeline**
```
Typical → Boundary → Fault → Performance → Robust → State → Concurrency
```
*Rationale*: Data integrity and throughput are critical

### Risk-Based Priority Adjustment

**Scoring Formula**
```
Risk Score = Impact × Likelihood × Uncertainty
  Impact:      1 (low) → 3 (critical)
  Likelihood:  1 (rare) → 3 (frequent)
  Uncertainty: 1 (known) → 3 (unknown)
```

**Priority Rules**
- Score ≥ 18: Move category immediately after Boundary
- Score 12-17: Move up 2 positions from default
- Score 9-11: Move up 1 position from default
- Score ≤ 8: Keep default position

**Example Risk Assessment**
```
Concurrency in multi-threaded queue:
  Impact: 3 (data corruption)
  Likelihood: 3 (many threads)
  Uncertainty: 3 (complex interactions)
  Score: 27 → Test immediately after Boundary

Performance in batch processor:
  Impact: 2 (slower but functional)
  Likelihood: 2 (depends on load)
  Uncertainty: 2 (some benchmarks exist)
  Score: 8 → Keep default position
```

## Quality Gates

### Advancement Criteria

**Gate 1: Before Leaving Typical**
- ✅ Happy-path coverage: 80-90% of core workflows
- ✅ No critical correctness bugs
- ✅ All typical test cases GREEN
- ✅ Basic smoke tests passing

**Gate 2: Before Performance Testing**
- ✅ All Boundary tests GREEN
- ✅ All Misuse tests GREEN or documented
- ✅ Basic memory leak checks clean (valgrind/sanitizers)
- ✅ No known resource leaks in core paths

**Gate 3: Before Concurrency Testing**
- ✅ All State tests GREEN
- ✅ No known deadlock-prone paths
- ✅ ThreadSanitizer clean on core operations
- ✅ Lock ordering documented

**Gate 4: Before Robust/Stress Testing**
- ✅ Capability limits characterized
- ✅ Key fault handling verified
- ✅ Resource cleanup verified under errors
- ✅ Recovery paths tested

### Fast-Fail Six

Run these tests **early and often** to catch common issues quickly:

1. **Null/Empty Input Handling**
   ```c
   IOC_doOperation(NULL, ...)     → IOC_RESULT_INVALID_PARAM
   IOC_doOperation("", ...)       → IOC_RESULT_INVALID_PARAM
   IOC_doOperation(NULL, 0, ...)  → IOC_RESULT_INVALID_PARAM
   ```

2. **Zero/Negative Timeout**
   ```c
   IOC_wait(..., 0)      → IOC_RESULT_TIMEOUT (immediate)
   IOC_wait(..., -1)     → IOC_RESULT_INVALID_PARAM
   IOC_wait(..., UINT_MAX) → Handle overflow
   ```

3. **Duplicate Registration/Subscription**
   ```c
   IOC_register("service")   → IOC_RESULT_SUCCESS
   IOC_register("service")   → IOC_RESULT_ALREADY_EXISTS
   IOC_subscribe(event)      → IOC_RESULT_SUCCESS
   IOC_subscribe(event)      → IOC_RESULT_ALREADY_SUBSCRIBED
   ```

4. **Illegal Call Sequence**
   ```c
   IOC_post(...)           → IOC_RESULT_NOT_INITIALIZED (before init)
   IOC_init(...)
   IOC_post(...)           → IOC_RESULT_SUCCESS
   IOC_cleanup(...)
   IOC_post(...)           → IOC_RESULT_INVALID_STATE (after cleanup)
   ```

5. **Buffer Full/Empty Boundaries**
   ```c
   // Fill buffer to capacity
   for (i = 0; i < CAPACITY; i++)
     IOC_enqueue(...)      → IOC_RESULT_SUCCESS
   IOC_enqueue(...)        → IOC_RESULT_QUEUE_FULL
   
   // Empty buffer completely
   for (i = 0; i < CAPACITY; i++)
     IOC_dequeue(...)      → IOC_RESULT_SUCCESS
   IOC_dequeue(...)        → IOC_RESULT_QUEUE_EMPTY
   ```

6. **Double-Close/Re-Init Idempotency**
   ```c
   IOC_close(handle)       → IOC_RESULT_SUCCESS
   IOC_close(handle)       → IOC_RESULT_INVALID_HANDLE (or SUCCESS if idempotent)
   
   IOC_init(...)
   IOC_init(...)           → IOC_RESULT_ALREADY_INITIALIZED (or SUCCESS if idempotent)
   ```

## Test Structure Guidelines

### Test Naming Convention

**Format**: `verifyBehavior_byCondition_expectResult`

**Components**:
- `verifyBehavior`: What functionality is being tested
- `byCondition`: Under what circumstances/inputs
- `expectResult`: What outcome is expected

**Examples**:
```c
verifyServiceRegistration_byValidName_expectSuccess
verifyEventPost_byFullQueue_expectNonBlockReturn
verifyCommandExec_byMultipleClients_expectIsolatedExecution
verifyStateTransition_byInvalidSequence_expectError
```

### Test Phase Structure

**4-Phase Pattern**: SETUP → BEHAVIOR → VERIFY → CLEANUP

```cpp
TEST(CategoryName, verifyBehavior_byCondition_expectResult) {
    //===SETUP===
    // 1. Initialize test environment
    // 2. Create necessary objects/resources
    // 3. Configure preconditions
    
    //===BEHAVIOR===
    printf("🎯 BEHAVIOR: verifyBehavior_byCondition_expectResult\n");
    // Execute the action being tested
    IOC_Result_T result = IOC_doSomething(...);
    
    //===VERIFY===
    // Validate outcomes (keep ≤3 key assertions)
    ASSERT_EQ(IOC_RESULT_SUCCESS, result);
    ASSERT_EQ(expectedValue, actualValue);
    ASSERT_TRUE(conditionMet);
    
    //===CLEANUP===
    // Release resources
    // Reset state
}
```

**Why ≤3 assertions?**
- Easier to identify what failed
- Better test isolation
- Clearer test purpose
- If you need more, create additional test cases

### US/AC/TC Contract

**User Story (US) Template**
```
US-n: As a [specific role/persona],
      I want [specific capability or feature],
      So that [concrete business value or benefit].
```

**Examples**:
```
US-1: As an event producer,
      I want to post events without blocking when the queue is full,
      So that my application remains responsive under high load.

US-2: As a service implementor,
      I want to receive commands via callback,
      So that I can process requests immediately without polling.
```

**Acceptance Criteria (AC) Template**
```
AC-n: GIVEN [initial context and preconditions],
      WHEN [specific trigger, action, or event],
      THEN [expected observable outcome or behavior].
```

**Examples**:
```
AC-1: GIVEN an event producer calling IOC_postEVT_inConlesMode,
      WHEN IOC's EvtDescQueue is full in ASyncMode,
      THEN the producer returns immediately with TOO_MANY_QUEUING_EVTDESC,
       AND the event is not queued.

AC-2: GIVEN a service with CbExecCmd_F registered,
      WHEN client sends PING command via IOC_execCMD,
      THEN callback executes synchronously and returns PONG result.
```

**Test Case (TC) Template**
```
TC-n:
  @[Name]: verifyBehavior_byCondition_expectResult
  @[Purpose]: Why this test matters and what it validates
  @[Brief]: What the test does in simple terms
  @[Steps]: Detailed execution steps (optional for complex tests)
    1) Step one
    2) Step two
    3) Step three
  @[Expect]: How to verify success
  @[Notes]: Additional context, gotchas, or dependencies
```

**Example**:
```
[@AC-1,US-1]
 TC-1:
   @[Name]: verifyASyncNonblock_byPostOneMoreEVT_whenEvtDescQueueFull
   @[Purpose]: Validate producer non-blocking behavior when queue is full
   @[Brief]: Fill queue to capacity, post one more event, verify immediate return
   @[Steps]:
     1) Get queue capacity via IOC_getCapability
     2) Subscribe with blocking callback to prevent queue drain
     3) Post events until queue is full
     4) Post one more event with NonBlock option
   @[Expect]: Step 4 returns IOC_RESULT_TOO_MANY_QUEUING_EVTDESC immediately
   @[Notes]: Callback intentionally blocks to keep queue full
```

## Implementation Tracking Template

Copy this block into your test files to track progress:

```cpp
///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TODO/IMPLEMENTATION TRACKING SECTION=============================================
// 🔴 IMPLEMENTATION STATUS TRACKING - Organized by Priority and Category
//
// STATUS LEGEND:
//   ⚪ TODO/PLANNED:      Designed but not implemented
//   🔴 RED/IMPLEMENTED:   Test written and failing (need prod code)
//   🟢 GREEN/PASSED:      Test written and passing
//   ⚠️  ISSUES:           Known problem needing attention
//
// PRIORITY LEVELS:
//   🥇 HIGH:    Must-have for release (Typical, critical Boundary)
//   🥈 MEDIUM:  Important for quality (State, Misuse, most Boundary)
//   🥉 LOW:     Nice-to-have (Performance, advanced scenarios)
//
//=================================================================================================
// 🥇 HIGH PRIORITY – Core Functionality
//=================================================================================================
//   ⚪ [@AC-1,US-1] TC-1: verifyCore_byBasicOperation_expectSuccess
//   ⚪ [@AC-1,US-1] TC-2: verifyCore_byNullInput_expectError
//   🔴 [@AC-2,US-1] TC-1: verifyCore_byMaxCapacity_expectProperHandling – BLOCKED: need capacity API
//
//=================================================================================================
// 🥈 MEDIUM PRIORITY – Boundary & Error Handling
//=================================================================================================
//   ⚪ [@AC-3,US-1] TC-1: verifyBoundary_byEmptyQueue_expectEmptyResult
//   ⚪ [@AC-3,US-1] TC-2: verifyBoundary_byFullQueue_expectFullResult
//   ⚪ [@AC-4,US-2] TC-1: verifyMisuse_byDoubleInit_expectError
//
//=================================================================================================
// 🥉 LOW PRIORITY – Advanced Scenarios
//=================================================================================================
//   ⚪ [@AC-5,US-2] TC-1: verifyPerformance_byHighLoad_expectAcceptableLatency
//   ⚪ [@AC-6,US-3] TC-1: verifyConcurrency_byMultipleThreads_expectThreadSafe
//
///////////////////////////////////////////////////////////////////////////////////////////////////
//======>END OF TODO/IMPLEMENTATION TRACKING SECTION=============================================
```

## Usage Guide

### When Starting a New Test Module

1. **Copy the template** (`LLM/CaTDD_ImplTemplate.cxx`)
2. **Fill in OVERVIEW section**: What you're testing and why
3. **Freely draft ideas**: Capture test scenarios without format
4. **Define coverage matrix**: Identify test dimensions
5. **Write User Stories**: Express value from user perspective
6. **Define Acceptance Criteria**: Make stories testable
7. **Specify Test Cases**: Detail concrete tests
8. **Update TODO section**: Track implementation status

### TDD Red→Green Workflow

```
1. Pick next TODO test case
2. Write test implementation (mark 🔴 RED)
3. Run test → should FAIL
4. Implement minimal production code
5. Run test → should PASS (mark 🟢 GREEN)
6. Refactor if needed
7. Commit changes
8. Repeat for next test
```

### Organizing Tests

**Single File Strategy** (simpler projects)
- Keep all tests for a component in one file
- Use TEST suites to organize by category
- Good for components with <50 tests

**Multi-File Strategy** (larger projects)
- Start with `UT_ComponentFreelyDrafts.cxx` for exploration
- Move to category-specific files as tests mature:
  - `UT_ComponentTypical.cxx` - Core workflows
  - `UT_ComponentBoundary.cxx` - Edge cases
  - `UT_ComponentState.cxx` - State transitions
  - `UT_ComponentConcurrency.cxx` - Thread safety
  - etc.

### For LLM/AI Assistance

**Provide Context**:
- Share this prompt file
- Include relevant test files
- Reference production code interfaces
- Mention specific concerns or risks

**Request Structure**:
```
"Using CaTDD methodology, help me design tests for [component].
Key concerns: [list risk factors]
Coverage dimensions: [dimension 1] × [dimension 2] × [dimension 3]
Priority: [context-specific priority order]"
```

**Iterative Refinement**:
1. Start with high-level US/AC design
2. Review and adjust coverage
3. Expand to detailed TC specifications
4. Generate test implementation
5. Review and refactor

### Best Practices

✅ **DO**:
- Write tests before production code (TDD)
- Keep test comments updated as design evolves
- Use descriptive test names
- Limit assertions to ≤3 per test
- Track status in TODO section
- Run tests frequently

❌ **DON'T**:
- Skip test design (jumping to implementation)
- Write tests after production code
- Let comments become stale
- Cram too many assertions in one test
- Forget to mark tests as RED/GREEN
- Accumulate failing tests without addressing them
