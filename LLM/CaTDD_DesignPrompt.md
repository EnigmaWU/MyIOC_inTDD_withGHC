# CaTDD Design Prompt

**Short name**: CaTDD (Comment‑alive Test‑Driven Development)

- `CaTDD` is a LLM friendly TDD.
  - `Comment-alive` means:
    - Design details live IN the test and source file as structured comments
    - Comments evolve WITH the code (not separate docs that go stale)
    - Comments are first-class artifacts that LLMs can parse and update
    - US/AC/TC format bridges human intent and machine-executable tests
  - `TDD` is same meaning as traditional TDD.
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
- Default order:
  - P1: Typical → Boundary → Misuse → Fault
  - P2: State → Capability → Concurrency
  - P3: Performance → Robust → Compatibility → Configuration
  - P4: Demo/Example
- Adjust based on context (see Context-Specific Priority Adjustments)
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

### Priority-Based Framework

**Priority-1: Functional Testing**
```
P1-Functional = ValidFunc(Typical + Boundary) + InvalidFunc(Misuse + Fault)
```
- **ValidFunc**: Tests with valid inputs/states - verify correct behavior
  - Typical: Happy paths and core workflows
  - Boundary: Edge cases and parameter limits
- **InvalidFunc**: Tests with invalid inputs/states - verify error handling
  - Misuse: Incorrect API usage patterns
  - Fault: External failures and recovery

**Priority-2: Design-Oriented Testing**
- State → Capability → Concurrency

**Priority-3: Quality-Oriented Testing**
- Performance → Robust → Compatibility → Configuration

**Priority-4: Other-Addons Testing**
- Demo/Example

### Default Test Order

**P1-Functional**: ValidFunc + InvalidFunc
- ValidFunc: Typical → Boundary (prove it works right)
- InvalidFunc: Misuse → Fault (prove it fails right)

**P2-Design**: State → Capability → Concurrency

**P3-Quality**: Performance → Robust → Compatibility → Configuration

**P4-Addons**: Demo/Example

### Category Definitions

## Priority-1: Functional Testing

**Formula**: `P1 = ValidFunc(Typical + Boundary) + InvalidFunc(Misuse + Fault)`

Functional testing ensures the component behaves correctly for both valid and invalid inputs, covering the complete contract between the API and its users.

- **ValidFunc**: Proves the system works correctly when used properly
- **InvalidFunc**: Proves the system fails gracefully when used improperly or under adverse conditions

### ValidFunc: Valid Function Testing

Tests that verify correct behavior with **valid inputs and states**.

**1. Typical** ⭐ *Core Functionality*
- **Purpose**: Verify main usage scenarios and happy paths
- **When**: First priority, fundamental behavior verification
- **Examples**:
  - IOC service registration and lookup
  - Event subscription and publishing
  - Command execution with expected response
  - Normal data flow through system

**2. Boundary** 🔲 *Edge Cases & Limits*
- **Purpose**: Test edge cases, parameter limits, and mode variations
- **When**: High priority, right after typical cases
- **Examples**:
  - Min/Max values (zero timeout, max string length)
  - Null/empty inputs (null pointer, empty array)
  - Block/NonBlock/Timeout modes
  - Buffer full/empty conditions

### InvalidFunc: Invalid Function Testing

Tests that verify correct **error handling and recovery** with invalid inputs, wrong states, or adverse conditions.

**3. Misuse** 🚫 *Error Prevention*
- **Purpose**: Test incorrect usage patterns and API abuse
- **When**: After core functionality, before advanced features
- **Examples**:
  - Wrong parameter order or types
  - Illegal state transitions (post before init)
  - Double-close, double-init scenarios
  - Operations on invalid handles

**4. Fault** ⚠️ *Error Handling & Recovery*
- **Purpose**: Test error handling, failures, and recovery
- **When**: Critical for reliability requirements
- **Examples**:
  - Process crash recovery
  - Network failures and timeouts
  - Disk full, memory exhausted
  - External dependency unavailable

### Summary of Priority-1

✅ **Complete P1 Gate Requirements**:
- All ValidFunc tests GREEN (Typical + Boundary)
- All InvalidFunc tests GREEN (Misuse + Fault)
- Fast-Fail Six passing
- No critical functional bugs

**P1 ensures**: The API contract is fully tested - both success and failure paths.

---

## Priority-2: Design-Oriented Testing

Design-oriented testing validates architectural decisions, including state management, capacity planning, and concurrency models.

**5. State** 🔄 *Lifecycle & FSM*
- **Purpose**: Verify state machine transitions and object lifecycle
- **When**: Essential for stateful components, FSM verification
- **Examples**:
  - Service states: Init→Ready→Running→Stopped
  - Event lifecycle: Created→Queued→Processing→Completed
  - Connection states: Disconnected→Connecting→Connected→Closing

**6. Capability** 🏆 *Capacity & Limits*
- **Purpose**: Test maximum capacity and system limits
- **When**: After basic functionality, for capacity planning
- **Examples**:
  - Maximum concurrent connections
  - Queue/buffer capacity limits
  - Maximum message size
  - Resource pool exhaustion

**7. Concurrency** 🚀 *Thread Safety*
- **Purpose**: Test multi-threading, synchronization, and race conditions
- **When**: For concurrent components, high complexity
- **Examples**:
  - Parallel API calls from multiple threads
  - Shared resource access patterns
  - Race conditions and deadlock scenarios
  - Lock-free data structure validation

## Priority-3: Quality-Oriented Testing

Quality-oriented testing ensures the system meets non-functional requirements for performance, stability, and compatibility.

**8. Performance** ⚡ *Speed & Efficiency*
- **Purpose**: Measure execution time, throughput, and resource usage
- **When**: After functional tests, when SLOs exist
- **Examples**:
  - API call latency under load
  - Memory leak detection
  - CPU usage monitoring
  - Throughput benchmarks

**9. Robust** 🛡️ *Stability & Reliability*
- **Purpose**: Stress testing, repetition, and soak testing
- **When**: For stability verification, production readiness
- **Examples**:
  - Repeated operations (1000x)
  - Buffer overflow/underflow cycles
  - Long-running stress tests (24h+)
  - Resource exhaustion patterns

**10. Compatibility** 🔄 *Cross-Platform*
- **Purpose**: Test across different platforms, versions, configurations
- **When**: Multi-platform products, version upgrades
- **Examples**:
  - Windows/Linux/macOS variations
  - API version compatibility
  - Compiler differences
  - Legacy system integration

**11. Configuration** 🎛️ *Settings Validation*
- **Purpose**: Test different configuration scenarios
- **When**: Configurable systems, deployment variations
- **Examples**:
  - Debug vs Release builds
  - Different log levels
  - Feature flags on/off
  - Environment variable handling

## Priority-4: Other-Addons Testing

Optional tests that demonstrate features and provide documentation value but are not required for functional correctness.

**12. Demo/Example** 🎨 *Documentation & Tutorials*
- **Purpose**: End-to-end feature demonstrations
- **When**: For documentation, tutorials, showcases
- **Examples**:
  - Complete workflow demonstrations
  - Tutorial code examples
  - Integration scenarios
  - Best practice illustrations

## Context-Specific Priority Adjustments

### Quick Decision Matrix

**Default/Balanced Approach**
```
P1: Typical → Boundary → Misuse → Fault
P2: State → Capability → Concurrency
P3: Performance → Robust → Compatibility
```
*Rationale*: Standard functional-first approach for most components

**New Public API**
```
P1: Typical → Boundary → Misuse → Fault (complete P1 thoroughly)
P2: State → Capability → Concurrency
P3: Performance
```
*Rationale*: Ensure API contract correctness before advanced testing

**Stateful/FSM-Heavy Component**
```
P1: Typical → Boundary (basic functional)
P2: State (promote to early) → Capability → Concurrency
P1: Misuse → Fault (complete functional)
P3: Performance → Robust
```
*Rationale*: State transitions are architectural core, test after basic functionality

**Reliability-Critical Service**
```
P1: Typical → Boundary → Fault (promote) → Misuse
P2: State → Capability → Concurrency
P3: Robust (promote) → Performance → Compatibility
```
*Rationale*: Error handling and stability are paramount

**High-Performance System (SLOs)**
```
P1: Typical → Boundary → Misuse
P3: Performance (promote to P2 level) → Robust
P2: State → Capability → Concurrency
P1: Fault (complete functional)
```
*Rationale*: Performance characteristics validated early, treat as design constraint

**Highly Concurrent Design**
```
P1: Typical → Boundary → Misuse
P2: Concurrency (promote to first P2) → State → Capability
P1: Fault (complete functional)
P3: Performance → Robust
```
*Rationale*: Thread safety is architectural foundation

**Data Processing Pipeline**
```
P1: Typical → Boundary → Fault → Misuse
P3: Performance (promote) → Robust (promote)
P2: State → Capability → Concurrency
```
*Rationale*: Data integrity and throughput are critical quality attributes

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

### Priority-Based Advancement Criteria

**Gate P1: Before Leaving Priority-1 (Functional Testing)**

Must complete: `ValidFunc(Typical + Boundary) + InvalidFunc(Misuse + Fault)`

- ✅ **ValidFunc complete** (system works correctly):
  - All Typical tests GREEN (80-90% core workflow coverage)
  - All Boundary tests GREEN (edge cases and limits validated)
- ✅ **InvalidFunc complete** (system fails gracefully):
  - All Misuse tests GREEN or documented (wrong usage handled)
  - All Fault tests GREEN or documented (error recovery verified)
- ✅ No critical correctness bugs
- ✅ Fast-Fail Six tests all passing
- ✅ Basic memory/resource leak checks clean

**Exit criteria**: Complete API contract tested - both success paths (ValidFunc) and failure paths (InvalidFunc).

**Gate P2: Before Priority-3 (Design-Oriented → Quality-Oriented)**
- ✅ State tests GREEN (if stateful component)
- ✅ Capability tests GREEN (limits characterized)
- ✅ Concurrency tests GREEN (if multi-threaded)
- ✅ No known deadlock or race conditions
- ✅ ThreadSanitizer/AddressSanitizer clean
- ✅ Architecture validated against design requirements

**Gate P3: Before Priority-4 or Release (Quality-Oriented Testing)**
- ✅ Performance tests GREEN (SLOs met if defined)
- ✅ Robust tests GREEN (stress/soak tests passing)
- ✅ Compatibility tests GREEN (if multi-platform)
- ✅ Configuration tests GREEN (if configurable)
- ✅ Production readiness criteria met

**Optional Gate P4: Documentation Complete**
- ✅ Demo/Example tests GREEN
- ✅ Tutorial code validated
- ✅ Best practices documented

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

## Agent Workflow Checklist

### For LLM/Agent: Step-by-Step Execution Guide

When asked to implement tests for a component, follow this structured workflow. Each phase has clear deliverables and checkpoint opportunities for human review.

### Phase 1: Understanding (Read-Only Analysis)

**Objective**: Gather sufficient context to design appropriate tests

- ☐ **Read component interface files**
  - Locate and read header files (.h) for the component
  - Identify public APIs, data structures, and constants
  - Note function signatures, parameters, and return types

- ☐ **Study existing related tests**
  - Search for existing UT_*.cxx files in Test/ directory
  - Review similar test patterns and naming conventions
  - Identify reusable test fixtures or helper functions

- ☐ **Identify dependencies and constraints**
  - Check CMakeLists.txt for dependencies
  - Review README_*.md files for design documentation
  - Note any special build requirements or configurations

- ☐ **Clarify ambiguities with human**
  - If API behavior is unclear, ask specific questions
  - If requirements are ambiguous, propose alternatives
  - If context is insufficient, request specific files/docs

**Checkpoint 1**: Present understanding summary to human:

```text
"I've analyzed [component]. It provides [key capabilities].
Key APIs: [list 3-5 main functions]
Dependencies: [list main dependencies]
Unclear aspects: [list questions if any]
Ready to proceed with test design?"
```

### Phase 2: Design (Comment Writing - No Code Yet)

**Objective**: Create comprehensive test design in structured comments

- ☐ **Fill OVERVIEW section**
  - WHAT: Describe the component being tested
  - WHERE: Identify the module/subsystem location
  - WHY: State the quality attributes to verify
  - Define clear scope (in-scope vs out-of-scope)

- ☐ **Define Coverage Matrix dimensions**
  - Identify 2-3 key dimensions for systematic coverage
  - Create table showing dimension combinations
  - Map each combination to potential User Stories
  - Example: Service Role × Client Role × Mode

- ☐ **Write User Stories (2-5 typically)**
  - Use format: "As a [role], I want [capability], so that [value]"
  - Focus on user/business value, not implementation
  - Ensure each story is independently valuable
  - Cover both success scenarios and error handling

- ☐ **Write Acceptance Criteria (2-4 per US)**
  - Use format: "GIVEN [context], WHEN [action], THEN [result]"
  - Make each AC independently testable
  - Include both functional and non-functional criteria
  - Be specific about expected behaviors and error codes

- ☐ **Detail Test Cases (1+ per AC)**
  - Name: verifyBehavior_byCondition_expectResult
  - Purpose: Why this test matters
  - Brief: What the test does in simple terms
  - Steps: Detailed execution steps (for complex tests)
  - Expect: How to verify success
  - Notes: Dependencies, gotchas, special setup

- ☐ **Populate TODO tracking section**
  - List all planned test cases
  - Mark initial status as ⚪ TODO/PLANNED
  - Add priority indicators (P1/P2/P3/P4)
  - Note any dependencies or blockers

**Checkpoint 2**: Present design for human approval:

```text
"Test design complete for [component]:
- Coverage: [X] User Stories, [Y] Acceptance Criteria, [Z] Test Cases
- Priority distribution: P1=[count], P2=[count], P3=[count]
- Key scenarios covered: [list 3-5 main scenarios]
- Estimated implementation effort: [rough estimate]

Shall I proceed with implementation?"
```

### Phase 3: Implementation (TDD Red→Green Cycle)

**Objective**: Implement tests following strict TDD discipline

#### 3A: Fast-Fail Six (Quick Validation)

- ☐ **Implement Fast-Fail Six tests first**
  - Test 1: Null/Empty input handling
  - Test 2: Zero/Negative timeout
  - Test 3: Duplicate registration/subscription
  - Test 4: Illegal call sequence (before init, after cleanup)
  - Test 5: Buffer full/empty boundaries
  - Test 6: Double-close/re-init idempotency
  - Mark each as 🔴 RED in TODO section

- ☐ **Run Fast-Fail Six tests**
  - Confirm all tests compile
  - Confirm all tests FAIL (RED) as expected
  - If any test passes unexpectedly, investigate why

#### 3B: P1 Functional Testing (ValidFunc)

- ☐ **Implement P1 Typical tests**
  - Write test code with clear 4-phase structure (SETUP/BEHAVIOR/VERIFY/CLEANUP)
  - Keep ≤3 key assertions per test
  - Add printf("🎯 BEHAVIOR: ...") for visibility
  - Mark as 🔴 RED/IMPLEMENTED in TODO section

- ☐ **Run Typical tests → confirm RED**
  - Tests should fail because production code is missing
  - Verify failure messages are clear and helpful
  - Document any unexpected failures

- ☐ **Implement minimal production code**
  - Write just enough code to make current test pass
  - Don't over-engineer or implement untested features
  - Follow existing code style and patterns

- ☐ **Run Typical tests → confirm GREEN**
  - All implemented tests should now pass
  - Update TODO section: 🔴 → 🟢 GREEN/PASSED
  - Commit changes with clear message

- ☐ **Implement P1 Boundary tests**
  - Follow same RED→GREEN cycle
  - Test edge cases: min/max values, null/empty, limits
  - Update TODO section as tests pass

#### 3C: P1 Functional Testing (InvalidFunc)

- ☐ **Implement P1 Misuse tests**
  - Test incorrect API usage patterns
  - Verify proper error codes returned
  - Ensure system doesn't crash or corrupt state
  - Follow RED→GREEN cycle

- ☐ **Implement P1 Fault tests**
  - Test error handling and recovery
  - Simulate external failures (network, disk, memory)
  - Verify graceful degradation
  - Follow RED→GREEN cycle

**Gate P1 Checkpoint**: Before proceeding to P2:

```text
✅ All P1 ValidFunc tests GREEN (Typical + Boundary)
✅ All P1 InvalidFunc tests GREEN (Misuse + Fault)
✅ Fast-Fail Six tests all passing
✅ Code coverage ≥80% for tested modules
✅ No memory leaks (run with sanitizers)
✅ No critical functional bugs

P1 Complete. Proceed to P2? [Yes/No]
```

#### 3D: P2 Design-Oriented Testing (If Applicable)

- ☐ **Implement State tests** (if stateful component)
  - Verify state machine transitions
  - Test lifecycle: Init→Ready→Running→Stopped→Cleanup
  - Ensure invalid transitions are rejected

- ☐ **Implement Capability tests** (for capacity planning)
  - Test maximum concurrent operations
  - Test queue/buffer capacity limits
  - Document actual limits discovered

- ☐ **Implement Concurrency tests** (if multi-threaded)
  - Test parallel access from multiple threads
  - Run with ThreadSanitizer enabled
  - Test race conditions and synchronization

**Gate P2 Checkpoint**: Architecture validated, concurrency safe

#### 3E: P3 Quality-Oriented Testing (If Required)

- ☐ **Implement Performance tests** (if SLOs exist)
  - Benchmark latency, throughput, memory usage
  - Compare against SLO targets
  - Document actual performance characteristics

- ☐ **Implement Robust tests** (for production readiness)
  - Stress tests: high load, sustained operation
  - Soak tests: long-running (24h+)
  - Resource exhaustion scenarios

- ☐ **Implement Compatibility tests** (if multi-platform)
  - Test on different OS platforms
  - Test with different compiler versions
  - Test API version compatibility

**Gate P3 Checkpoint**: Production ready

### Phase 4: Finalization and Documentation

- ☐ **Refactor tests for clarity**
  - Extract common setup/teardown to fixtures
  - Remove duplicate code
  - Simplify test logic while preserving coverage

- ☐ **Update documentation**
  - Ensure all comments reflect actual implementation
  - Remove obsolete TODO items
  - Document any known limitations or issues

- ☐ **Final status update**
  - Mark all completed tests as 🟢 GREEN
  - Document any ⚠️ ISSUES or 🚫 BLOCKED items
  - Provide summary of coverage achieved

**Final Checkpoint**: Present completion report:

```text
"Testing complete for [component]:
✅ Tests implemented: [count] ([P1/P2/P3 breakdown])
✅ Test coverage: [percentage]%
✅ All tests passing: [Yes/No]
⚠️ Known issues: [list if any]
🚫 Blocked items: [list if any]

Next steps: [recommendations]"
```

### Workflow Tips for Agents

**DO:**

- ✅ Ask clarifying questions early (Phase 1)
- ✅ Wait for human approval at checkpoints
- ✅ Update TODO section immediately after each test
- ✅ Follow strict RED→GREEN discipline (never skip RED phase)
- ✅ Commit after each GREEN achievement
- ✅ Run tests frequently, report failures immediately

**DON'T:**

- ❌ Skip directly to implementation without design
- ❌ Implement production code before writing tests
- ❌ Let tests stay RED without addressing them
- ❌ Batch multiple features into one test
- ❌ Guess requirements - ask instead
- ❌ Implement P2/P3 before completing P1

## Agent Troubleshooting Guide

### Common Issues and Resolution Strategies

When you encounter problems during test implementation, follow these systematic troubleshooting steps.

### Issue 1: Test Compilation Fails

**Symptoms:**

- Compiler errors about missing types, functions, or headers
- Linker errors about undefined references
- Syntax errors in test code

**Resolution Steps:**

1. **Check #include statements**
   ```cpp
   // ❌ WRONG: Guessing header paths
   #include "IOC_Service.h"
   
   // ✅ CORRECT: Verify actual file structure
   #include "IOC/IOC_Service.h"  // Check workspace structure
   ```
   - Use `file_search` to locate actual header files
   - Check existing test files for correct include patterns

2. **Verify function signatures**
   ```cpp
   // ❌ WRONG: Assuming parameter types
   IOC_Result_T result = IOC_registerService(serviceName);
   
   // ✅ CORRECT: Check header for actual signature
   IOC_Result_T result = IOC_registerService(serviceName, &serviceId);
   ```
   - Read header file to verify exact function signature
   - Check parameter order, types, and pointer usage
   - Verify return type matches your expectations

3. **Check for missing test utilities**
   ```cpp
   // If _UT_IOC_Common.h is missing functions
   // Check what other test files use
   ```
   - Search for similar tests that compile successfully
   - Use `grep_search` to find where utilities are defined
   - Verify CMakeLists.txt includes necessary test libraries

4. **Ask human for clarification**
   ```
   "I'm getting compilation error: [paste exact error]
   
   I've checked:
   - Header file at [path] shows signature: [signature]
   - Similar test at [file] uses: [pattern]
   
   Questions:
   - Is [API] the correct function to use for [purpose]?
   - Should I include [header] or [alternative header]?
   - Is there a test utility function for [task]?"
   ```

### Issue 2: Test Design Seems Incomplete or Wrong

**Symptoms:**

- Coverage matrix doesn't align with User Stories
- Test Cases don't actually verify Acceptance Criteria
- Uncertainty about what to test

**Resolution Steps:**

1. **Verify alignment: TC → AC → US**
   ```
   US-1: As a [role], I want [capability], so that [value]
         ↓ Does AC test this US?
   AC-1: GIVEN [context], WHEN [action], THEN [result]
         ↓ Does TC implement this AC?
   TC-1: verifyBehavior_byCondition_expectResult
   ```
   - Trace each TC back to its AC
   - Trace each AC back to its US
   - If disconnected, re-read the US and adjust

2. **Check coverage matrix completeness**
   ```
   Dimension 1: [A, B, C]
   Dimension 2: [X, Y, Z]
   
   Expected combinations: 3 × 3 = 9
   Actual User Stories: [count]
   
   Missing coverage: [list gaps]
   ```
   - Count expected vs actual scenarios
   - Identify untested combinations
   - Ask human: "Should I test [scenario] or is it out of scope?"

3. **Validate test expectations**
   ```
   "For scenario [describe scenario]:
   
   I'm planning to test:
   - Input: [specific input]
   - Expected result: [specific output/behavior]
   
   Questions:
   - Is this the correct expected behavior?
   - Should I test additional aspects like [X, Y, Z]?
   - Are there error codes I should verify?"
   ```

4. **Review Fast-Fail Six checklist**
   - Have you covered null/empty inputs?
   - Have you tested boundary conditions?
   - Have you verified error handling?
   - If any missing, add to test design

### Issue 3: Production Code Behavior Unclear

**Symptoms:**
- Don't know what the API should return in edge cases
- Unclear how errors should be reported
- Ambiguous state transitions or side effects

**Resolution Steps:**

1. **Search for similar patterns in codebase**
   ```
   Use grep_search to find:
   - "IOC_RESULT_" (error code patterns)
   - "GIVEN.*WHEN.*THEN" (similar AC examples)
   - Similar function names (naming patterns)
   ```
   - Look for consistent error handling patterns
   - Identify common return codes
   - Note how similar APIs behave

2. **Read component documentation**
   ```
   Check files:
   - README_Specification.md (API contracts)
   - README_ArchDesign.md (design intent)
   - Source/[Component].md (implementation notes)
   - Doc/*.md (design documents)
   ```
   - Look for explicit behavior specifications
   - Note design principles and constraints
   - Identify documented edge cases

3. **Examine existing tests**
   ```
   Search for tests of similar functionality:
   - What scenarios do they cover?
   - What assertions do they make?
   - What error codes do they expect?
   ```
   - Use existing tests as behavior specification
   - Follow established testing patterns
   - Maintain consistency with existing tests

4. **Ask human with specific alternatives**
   ```
   "For API: IOC_doOperation(NULL, ...)
   
   Possible behaviors:
   A) Return IOC_RESULT_INVALID_PARAM immediately
   B) Return IOC_RESULT_NULL_POINTER with error log
   C) Assert/crash (defensive programming)
   
   Similar API IOC_otherOperation() returns [X].
   
   Which behavior is correct for IOC_doOperation?"
   ```
   - Present 2-3 concrete alternatives
   - Reference similar APIs or patterns
   - Make human's decision easy (not open-ended)

### Issue 4: Test Fails Unexpectedly

**Symptoms:**
- Test should pass but fails
- Error message unclear
- Assertion fails with unexpected value

**Resolution Steps:**

1. **Verify test setup is correct**
   ```cpp
   // Common setup mistakes:
   
   // ❌ WRONG: Forgot to initialize
   IOC_Result_T result = IOC_doOperation(...);
   
   // ✅ CORRECT: Initialize first
   IOC_init();
   IOC_Result_T result = IOC_doOperation(...);
   ```
   - Check initialization order
   - Verify all preconditions are met
   - Ensure resources are properly created

2. **Add diagnostic output**
   ```cpp
   printf("🔍 DEBUG: result=%d, expected=%d\n", result, IOC_RESULT_SUCCESS);
   printf("🔍 DEBUG: state=%d, value=%p\n", state, ptr);
   ```
   - Add printf statements before assertions
   - Print actual vs expected values
   - Show intermediate state

3. **Check test isolation**
   ```cpp
   // ❌ WRONG: State leaks between tests
   TEST(Suite, test1) {
       IOC_init();
       // ... test logic
       // Missing cleanup!
   }
   
   // ✅ CORRECT: Clean isolation
   TEST(Suite, test2) {
       IOC_init();
       // ... test logic
       IOC_cleanup();  // Clean up!
   }
   ```
   - Verify cleanup in previous tests
   - Check for global state pollution
   - Run single test in isolation to confirm

4. **Report findings to human**
   ```
   "Test failing: verifyX_byY_expectZ
   
   Expected: [value]
   Actual: [value]
   
   Setup:
   - [step 1]
   - [step 2]
   
   Diagnostic output:
   [paste relevant output]
   
   Checked:
   ✅ Test setup looks correct
   ✅ API signature matches header
   ⚠️ Unexpected: result is [X] not [Y]
   
   Questions:
   - Is the expected value wrong?
   - Is there a missing initialization step?
   - Should the API behave differently in this case?"
   ```

### Issue 5: Unable to Proceed / Blocked

**Symptoms:**
- Missing production code makes testing impossible
- Dependency not available or broken
- Requirement fundamentally unclear

**Resolution Steps:**

1. **Clearly state the blocker**
   ```
   "🚫 BLOCKED: Cannot implement TC-5
   
   Reason: API IOC_getCapability() does not exist yet
   
   Impact: Cannot test capacity-related scenarios (3 tests blocked)
   
   Workarounds considered:
   - Mock the API: Not appropriate for integration tests
   - Hard-code capacity: Breaks when capacity changes
   - Skip tests: Incomplete coverage
   
   Recommendation: Mark TC-5, TC-6, TC-7 as 🚫 BLOCKED and continue with other tests."
   ```

2. **Document in TODO section**
   ```cpp
   //   🚫 [@AC-3,US-2] TC-1: verifyCapacity_byMaxConnections_expectLimit
   //        - BLOCKED: IOC_getCapability() API not yet implemented
   //        - Depends on: Issue #123
   //        - Estimated effort: 2 hours (once unblocked)
   ```

3. **Propose concrete next steps**
   ```
   "To unblock:
   
   Option A: Implement IOC_getCapability() API first (estimated 4 hours)
   Option B: Continue with other P1 tests, defer capacity tests to P2
   Option C: Use hard-coded constant for now, add TODO to fix later
   
   My recommendation: Option B - complete P1 ValidFunc (Typical+Boundary) first.
   
   Proceed? [A/B/C]"
   ```

4. **Continue with unblocked work**
   - Don't wait idle - work on other tests
   - Mark blocked items clearly in TODO
   - Provide regular status updates
   - Return to blocked items when unblocked

### Issue 6: Test Passes When It Should Fail (RED Phase)

**Symptoms:**
- New test passes immediately (should be RED)
- Production code already exists
- Test might not be testing what you think

**Resolution Steps:**

1. **Verify test is actually executing**
   ```cpp
   TEST(Suite, verifyNew_byCondition_expectResult) {
       printf("🎯 BEHAVIOR: This test IS running\n");
       
       // Temporarily make it fail to confirm
       ASSERT_TRUE(false) << "Intentional failure to verify test runs";
   }
   ```
   - Add printf to confirm execution
   - Add temporary failing assertion
   - Run test suite and verify output

2. **Check if feature already exists**
   ```
   "🤔 Test passed unexpectedly: verifyX_byY_expectZ
   
   This suggests the feature might already be implemented.
   
   Checked:
   - Production code at [file] line [X]
   - Function [name] appears to implement [behavior]
   
   Questions:
   - Is this feature already complete?
   - Should I proceed to next test?
   - Should I enhance this test with additional assertions?"
   ```

3. **Verify test assertions are meaningful**
   ```cpp
   // ❌ WEAK: Test doesn't verify much
   ASSERT_TRUE(result != NULL);
   
   // ✅ STRONG: Test verifies specific behavior
   ASSERT_EQ(IOC_RESULT_SUCCESS, result);
   ASSERT_STREQ("expected_value", actualValue);
   ASSERT_GT(count, 0);
   ```
   - Ensure assertions actually test the requirement
   - Don't just test "something happened"
   - Verify specific expected outcomes

4. **Update test design if needed**
   - Mark as 🟢 GREEN if feature is complete
   - Enhance test with additional assertions if too weak
   - Document why test passed (feature already implemented)

### General Troubleshooting Principles

**When stuck:**
- ✅ **DO**: Clearly state what's blocking you
- ✅ **DO**: Show what you've already tried
- ✅ **DO**: Propose 2-3 concrete alternatives
- ✅ **DO**: Ask specific, answerable questions
- ✅ **DO**: Continue with unblocked work while waiting

**Never:**
- ❌ **DON'T**: Guess requirements or make up expected behavior
- ❌ **DON'T**: Skip tests because they're hard
- ❌ **DON'T**: Silently proceed when fundamentally unclear
- ❌ **DON'T**: Wait idle - always have alternative work
- ❌ **DON'T**: Batch multiple unrelated questions together

**Question Quality Examples:**

```text
❌ POOR: "How should this work?"
   (Too vague, open-ended)

✅ GOOD: "Should IOC_register(NULL) return INVALID_PARAM or crash?"
   (Specific, binary choice)

❌ POOR: "The test fails."
   (No context, not actionable)

✅ GOOD: "Test fails at line 45: expected SUCCESS, got TIMEOUT.
         Setup: initialized IOC, registered service, called API.
         Is 5-second timeout too short for this operation?"
   (Context, diagnostic info, specific question)

❌ POOR: "I don't know what to test."
   (No investigation shown)

✅ GOOD: "I see 3 error codes: INVALID_PARAM, TIMEOUT, NOT_FOUND.
         Should I write separate tests for each error condition?"
   (Shows investigation, specific proposal)
```

### Quick Reference: Resolution Decision Tree

```text
Problem encountered
  ↓
[1] Compilation error?
  → Check headers/signatures → Search similar code → Ask human
  ↓
[2] Test design unclear?
  → Verify TC→AC→US → Check coverage matrix → Ask alternatives
  ↓
[3] Behavior unclear?
  → Search patterns → Read docs → Check similar tests → Ask with options
  ↓
[4] Test fails unexpectedly?
  → Check setup → Add diagnostics → Check isolation → Report findings
  ↓
[5] Blocked completely?
  → State blocker → Document in TODO → Propose options → Continue elsewhere
  ↓
[6] Test passes unexpectedly?
  → Verify execution → Check existing code → Strengthen assertions → Update status
```

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
