# IOC Error Precedence Improvement Proposal

## Current Problem
The IOC system has inconsistent error validation precedence that depends on:
- Operation type (sendDAT vs recvDAT)  
- Specific parameter values (UINT64_MAX behaves differently)
- Parameter combinations (NULL + zero size → data error instead of param error)

## Proposed Solution: LinkID > DatDescParams > Options

### Rationale
1. **Resource First**: Validate connection exists before processing data
2. **Data Second**: Validate data parameters on valid connections  
3. **Config Last**: Validate optional settings after core parameters

### Benefits
- ✅ Consistent behavior across all operations
- ✅ Logical validation order (resource → data → config)
- ✅ Performance optimization (fail fast on invalid connections)
- ✅ Security improvement (don't process data on invalid links)
- ✅ Simplified error handling for applications

### Implementation Impact
- **Breaking Change**: Applications relying on current precedence will need updates
- **Test Updates**: All boundary tests need precedence expectation updates
- **Documentation**: API docs need clear precedence specification

### Recommendation
Implement this change in next major version with:
1. Clear migration guide for existing applications
2. Comprehensive test coverage for new precedence order
3. Updated API documentation with precedence guarantees
