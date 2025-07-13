///////////////////////////////////////////////////////////////////////////////////////////////////
// UT_DataBoundaryUS4_SharedData.cxx - US-4 Shared Test Data Implementation
// 📝 Purpose: Implementation of shared test data and utilities for US-4 error code coverage testing
// 🎯 Coverage: [@US-4] Shared test environment and utilities
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "UT_DataBoundaryUS4.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF US-4 SHARED DATA IMPLEMENTATION=================================================

// Global test configuration for US-4 error code coverage testing
__DatErrorCodeSharedTestData_T g_US4_SharedTestData = {.TestConfigs = {},
                                                       .ServiceID1 = 0,
                                                       .ServiceID2 = 0,
                                                       .SystemInitialized = false,
                                                       .ErrorCodeCounts = {},
                                                       .ObservedErrorCodes = {},
                                                       .CrossModeConsistency = true,
                                                       .ParameterPrecedenceValidated = false,
                                                       .DataSizePrecedenceValidated = false,
                                                       .TimeoutPrecedenceValidated = false};

//======>END OF US-4 SHARED DATA IMPLEMENTATION===================================================
