# CaTDD Design Prompt

Short name: CaTDD (Comment‑alive Test‑Driven Development)

Purpose: Turn design intent into executable tests by writing rich, structured comments first, then evolving them into unit tests and code. Use the file as a living prompt for humans and LLMs.

Principles
- Improve Value, Avoid Lost, Balance Skill vs Cost
- Design before code; draft freely, then systematize
- Track status: GREEN/PASSED, RED/IMPLEMENTED, TODO/PLANNED, ISSUES
 - Risk-driven ordering: prioritize by impact × likelihood × uncertainty

Workflow
1) Design Principles: define coverage axes and scope
   - Service Role × Client Role × Mode
   - Component State × Operation × Boundary
   - Concurrency × Resource limits × Faults
2) Freely Drafts: capture ideas quickly; don’t block on format
3) Coverage Matrix: enumerate scenarios across axes
4) User Story (US): value from user perspective
5) Acceptance Criteria (AC): unambiguous, testable
6) Test Cases (TC): steps and expectations
7) Status Tracking: mark progress; prioritize Typical → Boundary → State → Fault → Performance → Concurrency → Others
8) Implement & Iterate: write tests first; make RED fail for missing feature, then implement prod code until GREEN
9) Refactor: move mature tests to specialized files

Coverage Matrix (template)
| Dimension 1  | Dimension 2  | Dimension 3 | Key Scenarios                  |
|--------------|--------------|-------------|--------------------------------|
| [Example]    | [Example]    | [Example]   | US-X: [Short description]      |

IOC example axes
- Service Role: EvtProducer, EvtConsumer, Mixed
- Client Role: EvtConsumer, EvtProducer, Mixed
- Mode: Callback, Pull/Poll, Both

Status indicators
- GREEN/PASSED: implemented and verified
- RED/IMPLEMENTED: test exists and currently failing or pending behavior
- TODO/PLANNED: designed but not implemented
- ISSUES: known problem needing fix

Classification guide (priority order)
- Default order: Typical → Boundary → Misuse → State → Fault → Performance → Concurrency → Capability → Robust → Demo/Example → Compatibility → Configuration

1) Typical (must): core workflows
2) Boundary (important): edge and limits (min/max/null, Block/NonBlock/Timeout)
3) Misuse (hardening): illegal sequences, bad params, wrong states
4) State (key): lifecycle and transitions
5) Fault (reliability): failures and recovery (deps, IO, timeouts)
6) Performance (as needed): latency, throughput, memory
7) Concurrency (complex): race/deadlock/safety
8) Capability (limits): capacity, queue/buffer maxima
9) Robust (stability): stress, repetition, soak
10) Demo/Example, 11) Compatibility, 12) Configuration

Priority varies by context (quick picks)
- New public API: Typical → Boundary → Misuse → State → Fault → Performance → Concurrency → Capability → Robust
- Stateful/FSM-heavy: Typical → Boundary → State → Misuse → Fault → Concurrency → Performance → Capability → Robust
- Reliability-critical service: Typical → Boundary → Fault → State → Misuse → Concurrency → Performance → Capability → Robust
- Throughput/latency SLOs: Typical → Boundary → Performance → State → Concurrency → Fault → Capability → Robust
- Highly concurrent design: Typical → Boundary → Concurrency → State → Fault → Performance → Capability → Robust

Risk scoring to reorder
- Score each category: Impact (1–3) × Likelihood (1–3) × Uncertainty (1–3)
- If score ≥ 18, pull category forward right after Boundary

Gates before advancing layers
- Before leaving Typical: happy-path coverage ~80–90%, no critical correctness bugs
- Before Performance: Boundary + Misuse green; basic leak/memory checks clean
- Before Concurrency: State tests green; no known deadlock-prone paths; sanitizers/static checks clean on core paths
- Before Robust: Capability/limits characterized; key fault handling verified

Fast-fail six (run early)
- Null/empty input handling
- Zero/negative timeout
- Duplicate registration/subscription
- Illegal call sequence (e.g., post-before-init)
- Buffer full/empty boundaries
- Double-close/re-init idempotency

Test naming
verifyBehavior_byCondition_expectResult

Test phase markers
- SETUP: prepare environment
- BEHAVIOR: execute action
- VERIFY: assert outcomes (≤ 3 key assertions per test)
- CLEANUP: release resources

US/AC/TC skeleton
- US-n: As a [role], I want [capability], so that [value].
- AC-n: GIVEN [context], WHEN [action], THEN [result].
- TC-n: @[Name] verifyX_byY_expectZ; @[Purpose]; @[Brief]; Steps; Expectations

Implementation tracking block (paste into test files)
///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TODO/IMPLEMENTATION TRACKING SECTION===========================================
// 🔴 IMPLEMENTATION STATUS TRACKING - Organized by Priority and Category
// 🥇 HIGH PRIORITY – Core
//   TODO: [@AC-1,US-1] TC-1: verifyCore_byBasic_expectSuccess – RED/IMPLEMENTED
// 🥈 MEDIUM – Boundary
//   TODO: [@AC-2,US-1] TC-1: verifyBoundary_byEdge_expectHandled – RED/IMPLEMENTED
// 🥉 LOW – Advanced
//   TODO: [@AC-3,US-2] TC-1: verifyAdvanced_byComplex_expectFull – RED/IMPLEMENTED
///////////////////////////////////////////////////////////////////////////////////////////////////

Usage
- Start with this prompt when creating a new test module
- Keep comments rich and structured; let tools or LLMs help expand to tests
- Move from draft → structured AC/TC → runnable tests → green
