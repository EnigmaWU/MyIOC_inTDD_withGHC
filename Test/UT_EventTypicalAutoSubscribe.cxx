///////////////////////////////////////////////////////////////////////////////////////////////////
// Event Typical Auto-Subscribe (Conet) — Design Proposal
//
// Intent:
// - Propose and document auto-subscribe behavior for event consumers using IOC_ConnArgs_T::UsageArgs.pEvt.
// - If supported, IOC_connectService should internally call IOC_subEVT when Usage==EvtConsumer and pEvt is set.
// - This file is a design proposal; tests are skipped until framework support is implemented.
//
// API Contract:
//   - If IOC_ConnArgs_T::Usage == IOC_LinkUsageEvtConsumer and UsageArgs.pEvt != NULL,
//     then IOC_connectService MUST internally call IOC_subEVT(LinkID, UsageArgs.pEvt) after connect.
//   - This enables one-step connect+subscribe for event consumers.
//   - Service-side behavior is unchanged; clients can still call IOC_subEVT manually for advanced cases.
//
// Rationale:
//   - Simplifies typical event flows for clients.
//   - Reduces boilerplate and risk of missing subscription.
//   - Mirrors DAT/CMD auto-wiring via UsageArgs in SrvArgs/ConnArgs.
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "_UT_IOC_Common.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF DESIGN PROPOSAL=================================================================
/**
 * Design Proposal: IOC_ConnArgs_T::UsageArgs.pEvt auto-subscribe
 *
 * Motivation:
 *   - Enable clients to connect and subscribe to events in a single step.
 *   - Reduce manual IOC_subEVT calls and unify with DAT/CMD auto-wiring.
 *
 * API Contract:
 *   - If Usage == IOC_LinkUsageEvtConsumer and UsageArgs.pEvt != NULL,
 *     IOC_connectService MUST call IOC_subEVT(LinkID, UsageArgs.pEvt) after connect.
 *   - If UsageArgs.pEvt == NULL, no auto-subscribe occurs; manual IOC_subEVT is required.
 *   - Service-side event wiring remains explicit via IOC_subEVT or SrvArgs.UsageArgs.pEvt.
 *
 * Example:
 *   IOC_EvtUsageArgs_T evtArgs = { .CbProcEvt_F = MyCb, .pCbPrivData = &priv, .EvtNum = 1, .pEvtIDs = &evtID };
 *   IOC_ConnArgs_T connArgs = { .SrvURI = uri, .Usage = IOC_LinkUsageEvtConsumer };
 *   connArgs.UsageArgs.pEvt = &evtArgs;
 *   IOC_connectService(&linkID, &connArgs, NULL); // auto-subscribe occurs
 *
 * Status:
 *   - Not yet implemented in IOC core; tests below are skipped until supported.
 */
//======>END OF DESIGN PROPOSAL====================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES (skipped)============================================================

// [@AC-1,US-1] TC-1: Single client auto-subscribe
TEST(UT_EventTypicalAutoSubscribe, verifyAutoSubscribe_byConnArgsUsageArgsEvt_expectDelivered) {
    GTEST_SKIP()
        << "Design proposal: IOC_connectService should auto-subscribe if UsageArgs.pEvt is set (not implemented)";
}

// [@AC-2,US-1] TC-2: Multi-client isolation via auto-subscribe
TEST(UT_EventTypicalAutoSubscribe, verifyAutoSubscribe_multiClientIsolation_expectPerLinkDelivery) {
    GTEST_SKIP() << "Design proposal: multi-client auto-subscribe via UsageArgs.pEvt (not implemented)";
}

//======>END OF TEST CASES=========================================================================
