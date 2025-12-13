# UT_DataEdgeUS3 Test Design Documentation

## Test Case: verifyDatTimeoutEdge_byPrecisionTesting_expectAccurateTiming

### 🎯 PURPOSE
Verify that IOC DAT timeout operations exhibit precise and accurate timing behavior across various timeout values, ensuring the timeout mechanism is reliable for time-critical applications.

### 📋 TEST OBJECTIVE
Validate that timeout operations complete within acceptable variance from the requested timeout duration, ensuring system reliability and predictable behavior under different timing conditions.

### 🔧 TEST SETUP
1. Create bidirectional DAT services: sender and receiver pairs for both directions
2. Establish links for both send-side and receive-side timeout testing
3. Prepare buffer saturation mechanism for sendDAT timeout testing
4. Prepare empty queue mechanism for recvDAT timeout testing
5. Setup high-resolution timing measurement infrastructure
6. Define acceptable timing variance thresholds based on system characteristics

### 🧪 TEST EXECUTION STRATEGY

#### PHASE 1: IOC_recvDAT Precision Testing (Empty Queue Timeouts)
Test timeout values: 1ms, 2ms, 5ms, 10ms, 20ms, 50ms, 100ms, 200ms, 500ms, 1000ms

For each timeout value:
- a) Ensure receive queue is empty (no data available)
- b) Record start time using std::chrono::high_resolution_clock
- c) Call IOC_recvDAT with SyncTimeout option (expecting timeout)
- d) Record end time immediately after call returns
- e) Calculate actual elapsed time = end_time - start_time
- f) Verify result code is IOC_RESULT_TIMEOUT
- g) Assert timing accuracy within acceptable variance

#### PHASE 2: IOC_sendDAT Precision Testing (Buffer Full Timeouts)
Test timeout values: 1ms, 2ms, 5ms, 10ms, 20ms, 50ms, 100ms, 200ms, 500ms, 1000ms

For each timeout value:
- a) Saturate receiver buffer to create backpressure
- b) Record start time using std::chrono::high_resolution_clock
- c) Call IOC_sendDAT with ASyncTimeout option (expecting timeout due to full buffer)
- d) Record end time immediately after call returns
- e) Calculate actual elapsed time = end_time - start_time
- f) Verify result code is IOC_RESULT_TIMEOUT or IOC_RESULT_BUFFER_FULL
- g) Assert timing accuracy within acceptable variance

#### PHASE 3: Edge Case Precision Testing (Both Directions)
Test edge timeout values: 0ms (immediate), 1μs, 999μs, 1001μs, 9999μs

Special considerations:
- 0ms should return immediately with IOC_RESULT_TIMEOUT
- Sub-millisecond values should be handled appropriately
- Values just under/over millisecond boundaries
- Test both sendDAT and recvDAT edge cases

#### PHASE 4: Repeated Precision Testing (Statistical Validation)
For critical timeout values (1ms, 10ms, 100ms, 1000ms):
- Execute 10 iterations of each timeout for both sendDAT and recvDAT
- Calculate statistical metrics: mean, std deviation, min, max
- Verify consistent timing behavior across iterations
- Ensure no outliers exceed maximum acceptable variance
- Compare sendDAT vs recvDAT timeout precision consistency

#### PHASE 5: Bidirectional Timeout Testing
Test simultaneous timeout scenarios:
- Concurrent sendDAT timeout (buffer full) + recvDAT timeout (queue empty)
- Verify both operations timeout independently and accurately
- Test timing precision under concurrent timeout stress
- Validate system can handle multiple timeout operations simultaneously

#### PHASE 6: System Load Impact Testing
Simulate system load while testing timeout precision:
- Create background computational load (CPU-intensive loop)
- Test timeout precision under load for key values (10ms, 100ms)
- Test both sendDAT and recvDAT precision under load
- Verify timeout accuracy doesn't degrade significantly under load
- Test should demonstrate system resilience for both operations

### 📏 TIMING ACCURACY THRESHOLDS

**Acceptable Variance Definitions (Applied to Both sendDAT and recvDAT):**
- For timeouts ≤ 5ms:     ±2ms or ±50% (whichever is larger)
- For timeouts 6-50ms:    ±3ms or ±20% (whichever is larger)
- For timeouts 51-500ms:  ±10ms or ±5% (whichever is larger)
- For timeouts > 500ms:   ±20ms or ±3% (whichever is larger)

**Rationale:**
- Shorter timeouts have higher relative variance due to system overhead
- Longer timeouts should achieve better absolute precision
- Percentage-based thresholds account for natural timing variance
- Absolute minimums prevent unrealistic precision expectations
- Same thresholds apply to both send and receive operations for consistency

### ✅ SUCCESS CRITERIA

#### 1. RESULT CODE VALIDATION:
- All recvDAT timeout operations must return IOC_RESULT_TIMEOUT
- All sendDAT timeout operations must return IOC_RESULT_TIMEOUT or IOC_RESULT_BUFFER_FULL
- No unexpected error codes (e.g., IOC_RESULT_BUG, IOC_RESULT_INVALID)

#### 2. TIMING PRECISION VALIDATION:
- Actual elapsed time must fall within acceptable variance thresholds for both operations
- No timeout should complete significantly faster than requested
- No timeout should exceed maximum allowed overshoot
- sendDAT and recvDAT should demonstrate comparable timing precision

#### 3. CONSISTENCY VALIDATION:
- Repeated tests should show consistent timing behavior for both operations
- Standard deviation should remain within reasonable bounds
- No single iteration should be an extreme outlier
- Cross-validation: sendDAT precision should correlate with recvDAT precision

#### 4. BIDIRECTIONAL VALIDATION:
- Concurrent timeout operations should not interfere with each other
- System should handle multiple simultaneous timeouts gracefully
- Timing precision should remain consistent under concurrent load

#### 5. SYSTEM RESILIENCE VALIDATION:
- Timing accuracy should remain acceptable under moderate system load
- Performance degradation should be predictable and bounded
- Both sendDAT and recvDAT should degrade gracefully under load

### 🔍 SPECIFIC ASSERTIONS

#### For each recvDAT timeout test:
```cpp
// Basic result validation
ASSERT_EQ(IOC_RESULT_TIMEOUT, Result)
    << "recvDAT timeout operation should return IOC_RESULT_TIMEOUT";

// Timing lower bound (prevent premature completion)
ASSERT_GE(actualElapsedMs, requestedTimeoutMs * 0.8)
    << "recvDAT timeout should not complete significantly early";

// Timing upper bound (prevent excessive overshoot)
ASSERT_LE(actualElapsedMs, requestedTimeoutMs + maxAcceptableVarianceMs)
    << "recvDAT timeout should not exceed maximum acceptable overshoot";

// Precision validation
double timingError = abs(actualElapsedMs - requestedTimeoutMs);
double errorPercentage = (timingError / requestedTimeoutMs) * 100.0;
ASSERT_LE(errorPercentage, maxAcceptableErrorPercentage)
    << "recvDAT timing error percentage should be within acceptable bounds";
```

#### For each sendDAT timeout test:
```cpp
// Basic result validation (sendDAT can return TIMEOUT or BUFFER_FULL)
ASSERT_TRUE(Result == IOC_RESULT_TIMEOUT || Result == IOC_RESULT_BUFFER_FULL)
    << "sendDAT timeout operation should return IOC_RESULT_TIMEOUT or IOC_RESULT_BUFFER_FULL";

// Timing lower bound (prevent premature completion)
ASSERT_GE(actualElapsedMs, requestedTimeoutMs * 0.8)
    << "sendDAT timeout should not complete significantly early";

// Timing upper bound (prevent excessive overshoot)
ASSERT_LE(actualElapsedMs, requestedTimeoutMs + maxAcceptableVarianceMs)
    << "sendDAT timeout should not exceed maximum acceptable overshoot";

// Precision validation
double timingError = abs(actualElapsedMs - requestedTimeoutMs);
double errorPercentage = (timingError / requestedTimeoutMs) * 100.0;
ASSERT_LE(errorPercentage, maxAcceptableErrorPercentage)
    << "sendDAT timing error percentage should be within acceptable bounds";
```

#### For statistical validation:
```cpp
// Statistical consistency for recvDAT
ASSERT_LE(recvStandardDeviation, maxAcceptableStdDev)
    << "recvDAT timing consistency should be within acceptable bounds";

ASSERT_LE(recvMaxOutlierDeviation, maxAcceptableOutlierDeviation)
    << "No single recvDAT iteration should be an extreme outlier";

// Statistical consistency for sendDAT
ASSERT_LE(sendStandardDeviation, maxAcceptableStdDev)
    << "sendDAT timing consistency should be within acceptable bounds";

ASSERT_LE(sendMaxOutlierDeviation, maxAcceptableOutlierDeviation)
    << "No single sendDAT iteration should be an extreme outlier";

// Cross-operation consistency
double precisionDifference = abs(recvMeanPrecision - sendMeanPrecision);
ASSERT_LE(precisionDifference, maxAcceptablePrecisionDifference)
    << "sendDAT and recvDAT should have comparable timing precision";
```

#### For bidirectional concurrent testing:
```cpp
// Concurrent timeout validation
ASSERT_EQ(IOC_RESULT_TIMEOUT, concurrentRecvResult)
    << "Concurrent recvDAT should timeout independently";

ASSERT_TRUE(concurrentSendResult == IOC_RESULT_TIMEOUT ||
            concurrentSendResult == IOC_RESULT_BUFFER_FULL)
    << "Concurrent sendDAT should timeout independently";

// Timing independence validation
ASSERT_LT(abs(concurrentRecvTime - expectedRecvTimeout), maxConcurrentVariance)
    << "Concurrent recvDAT timing should not be affected by concurrent sendDAT";

ASSERT_LT(abs(concurrentSendTime - expectedSendTimeout), maxConcurrentVariance)
    << "Concurrent sendDAT timing should not be affected by concurrent recvDAT";
```

### 🛠️ IMPLEMENTATION DETAILS

#### Helper Functions Needed:
- `calculateTimingVariance(requestedMs)` -> returns acceptable variance in ms
- `measureRecvTimeoutPrecision(timeoutMs, iterations)` -> returns recvDAT timing statistics
- `measureSendTimeoutPrecision(timeoutMs, iterations)` -> returns sendDAT timing statistics
- `saturateReceiverBuffer(linkID)` -> fills buffer to create sendDAT timeout conditions
- `drainReceiverBuffer(linkID)` -> empties buffer to create recvDAT timeout conditions
- `createSystemLoad()` -> creates background computational load
- `validateTimingStatistics(recvStats, sendStats)` -> validates statistical requirements
- `compareBidirectionalPrecision(recvStats, sendStats)` -> compares send vs recv precision

#### Data Structures:
```cpp
struct TimingStats {
    double meanMs;
    double stdDeviationMs;
    double minMs;
    double maxMs;
    std::vector<double> measurements;
    IOC_Result_T expectedResult;  // TIMEOUT for recv, TIMEOUT/BUFFER_FULL for send
};

struct BidirectionalTimingStats {
    TimingStats recvStats;
    TimingStats sendStats;
    double precisionCorrelation;  // How similar recv vs send precision is
    bool concurrentTestPassed;    // Whether concurrent timeout test passed
};
```

#### Test Configuration:
- Use high-resolution timing: std::chrono::high_resolution_clock
- Pre-warm system by running a few dummy timeout operations for both directions
- Account for system timer resolution limitations
- Log detailed timing information for debugging both sendDAT and recvDAT
- Setup buffer management for realistic timeout conditions

#### Buffer Management Strategy:
```cpp
// For sendDAT timeout testing:
1. Query system buffer capacity via IOC_getCapability()
2. Fill buffer to near capacity using ASyncNonBlock sends
3. Verify buffer pressure exists (subsequent sends return BUFFER_FULL)
4. Test ASyncTimeout sendDAT operations for timing precision
5. Cleanup: drain buffer between test iterations

// For recvDAT timeout testing:
1. Ensure no data is queued (drain any existing data)
2. Verify empty state (SyncNonBlock recv returns NO_DATA)
3. Test SyncTimeout recvDAT operations for timing precision
4. Cleanup: ensure clean state between iterations
```

### 🚨 POTENTIAL CHALLENGES & MITIGATIONS

#### 1. SYSTEM TIMER RESOLUTION:
- **Challenge:** System may not support sub-millisecond precision
- **Mitigation:** Test timer resolution first, adjust expectations accordingly

#### 2. SYSTEM LOAD INTERFERENCE:
- **Challenge:** Background processes may affect timing accuracy
- **Mitigation:** Run tests multiple times, use statistical validation

#### 3. COMPILER/OS OPTIMIZATION:
- **Challenge:** Compiler might optimize away timing-critical code
- **Mitigation:** Use volatile variables, barrier instructions

#### 4. THREADING/SCHEDULING:
- **Challenge:** Thread scheduling may introduce timing variance
- **Mitigation:** Account for scheduling overhead in variance calculations

#### 5. BUFFER STATE MANAGEMENT:
- **Challenge:** Creating consistent buffer full/empty conditions
- **Mitigation:** Robust buffer saturation/draining mechanisms with verification

#### 6. CONCURRENT TIMEOUT INTERFERENCE:
- **Challenge:** Multiple timeout operations might interfere with each other
- **Mitigation:** Test concurrent operations separately, validate independence

#### 7. SEND VS RECV TIMING DIFFERENCES:
- **Challenge:** sendDAT and recvDAT might have inherently different timing characteristics
- **Mitigation:** Allow for reasonable variance between operations, focus on consistency

#### 8. BUFFER PRESSURE VARIABILITY:
- **Challenge:** Buffer full conditions might be inconsistent across test runs
- **Mitigation:** Multiple buffer saturation strategies, statistical validation

### 📊 EXPECTED OUTPUT FORMAT
```
🧪 Testing recvDAT timeout precision for 1ms...
⏱️ Requested: 1000μs, Actual: 1150μs, Error: +15.0%, Status: ✓ PASS

🧪 Testing sendDAT timeout precision for 1ms...
⏱️ Requested: 1000μs, Actual: 1080μs, Error: +8.0%, Status: ✓ PASS

🧪 Testing recvDAT timeout precision for 10ms...
⏱️ Requested: 10000μs, Actual: 10250μs, Error: +2.5%, Status: ✓ PASS

🧪 Testing sendDAT timeout precision for 10ms...
⏱️ Requested: 10000μs, Actual: 10180μs, Error: +1.8%, Status: ✓ PASS

📊 Statistical validation for recvDAT 100ms (10 iterations):
📈 Mean: 100.3ms, StdDev: 0.8ms, Min: 99.1ms, Max: 101.7ms

📊 Statistical validation for sendDAT 100ms (10 iterations):
📈 Mean: 100.1ms, StdDev: 0.9ms, Min: 98.9ms, Max: 101.5ms

🔄 Bidirectional precision comparison:
📈 recvDAT avg precision: 98.2%, sendDAT avg precision: 98.5%
📈 Precision correlation: 0.95 (excellent)

🧪 Concurrent timeout test (10ms recv + 15ms send):
⏱️ recvDAT: 10.1ms, sendDAT: 15.2ms - both within tolerance

✅ All timing requirements met for both operations
```

### 🏁 CLEANUP REQUIREMENTS
- Close all test links and services (both send and receive directions)
- Stop any background load generation
- Drain all buffers and reset queue states
- Reset system state for subsequent tests
- Report comprehensive timing analysis summary for both operations
- Archive timing data for performance regression analysis

### 📝 NOTES
- This test may need adjustment based on target platform characteristics
- Consider making timing thresholds configurable for different environments
- May need platform-specific implementations for optimal precision
- Test results should be logged for performance regression analysis
- sendDAT and recvDAT precision may have different characteristics - document differences
- Buffer management is critical for realistic sendDAT timeout testing
- Statistical validation helps account for system timing variance
- Bidirectional testing provides comprehensive timeout mechanism validation

## Implementation Status

### ✅ Completed Implementation
- **recvDAT Timeout Precision Testing**: Fully implemented and working correctly
  - Polling mode configuration fixed (NULL callback enables timeout support)
  - All timeout values (2ms, 5ms, 10ms, 20ms, 50ms, 100ms) tested successfully
  - Average precision error: ~14-19% (within acceptable thresholds)
  - Proper result codes returned (IOC_RESULT_TIMEOUT)

### ⚠️ Simplified Implementation 
- **sendDAT Timeout Precision Testing**: Simplified due to architectural limitations
  - Fast receiver callback processing prevents reliable buffer saturation
  - Test validates basic timeout behavior (non-hanging) rather than precise timing
  - Architecture limitation documented and acknowledged in test output

### 🧪 Working Features
- **Statistical Validation**: Implemented with realistic variance thresholds
- **Concurrent Timeout Testing**: Successfully validates independent timeout operations
- **Zero Timeout Testing**: Validates immediate return behavior
- **Blocking Mode Transitions**: Tests different blocking modes correctly

### 🔧 Key Technical Insights
1. **Polling Mode Requirement**: recvDAT timeout testing requires NULL callback to enable polling mode
2. **Buffer Saturation Challenge**: sendDAT timeout testing is difficult due to efficient receiver processing
3. **Timing Variance**: System-level timing requires realistic variance thresholds (not ultra-precise expectations)
4. **Service Resource Limits**: Creating too many services can hit system allocation limits
