
///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF OVERVIEW OF THIS UT FILE==========================================================
/**
 * @brief ${What's this submodule for?}
 *  ${Relationship with module or other submodules}
 *  ${What's the key point or concept of this submodule?}
 *
 *  ${What's the functionality/interface/data structure/operation/... of this submodule?}
 *
 */
//======END OF OVERVIEW OF THIS UT FILE============================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF UNIT TESTING DESIGN===============================================================
/**
 * At least one User Story(a.k.a US),
 *    and at least one Acceptance Criteria(a.k.a AC) for each US,
 *      and at least one Test Case(a.k.a TC) for each AC.
 *
 * US takes VALUE from USR perspective.
 * AC clear CONDITIONS may relate to the USR.
 * TC details each condition's STEPS to verify.
 */
//-------------------------------------------------------------------------------------------------
/**
 * @brief 【User Story】
 *
 *  US-1: As a ..., I WANT to ..., SO THAT ...
 *
 */

//-------------------------------------------------------------------------------------------------
/**
 * @brief 【Acceptance Criteria】
 *
 * [@US-n]
 * AC-1: GIVEN ..., WHEN ..., THEN ...
 *
 */

//-------------------------------------------------------------------------------------------------
/**
 * @brief 【Test Cases】
 *
 * [@AC-n]
 * TC-1: verifyBehivorX_byDoABC
 *
 */

//======END OF UNIT TESTING DESIGN=================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF UNIT TESTING IMPLEMENTATION=======================================================
#include "_UT_IOC_Common.h"

//===TEMPLATE OF UT CASE===
/**
 * @[Name]: ${verifyBehivorX_byDoABC}
 * @[Purpose]: ${according to what in SPEC, and why to verify in this way}
 * @[Steps]: ${how to do}
 *   1) do ..., with ..., as SETUP
 *   2) do ..., with ..., as BEHAVIOR
 *   3) do ..., with ..., as VERIFY
 *   4) do ..., with ..., as CLEANUP
 * @[Expect]: ${how to verify}
 * @[Notes]:
 */
TEST(UT_NameOfCategory, CaseNN_verifyBehivorX_byDoABC) {
  //===SETUP===
  // 1. ...

  //===BEHAVIOR===
  //@VerifyPoint xN(each case MAY have many 'ASSERT_XYZ' check points)

  //===VERIFY===
  //@KeyVerifyPoint<=3(each case SHOULD has less than 3 key 'ASSERT_XYZ' verify points)

  //===CLEANUP===
}

//---------------------------------------------------------------------------------------------------------------------
class UT_NameofCategoryFixture : public ::testing::Test {
 protected:
  void SetUp() override {
    // 1. ...
  }

  void TearDown() override {
    // 1. ...
  }
};

TEST_F(UT_NameofCategoryFixture, CaseNN_verifyBehivorX_byDoABC) {
  //===BEHAVIOR===
  //@VerifyPoint xN(each case MAY have many 'ASSERT_XYZ' check points)

  //===VERIFY===
  //@KeyVerifyPoint<=3(each case SHOULD has less than 3 key 'ASSERT_XYZ' verify points)
}

//======END OF UNIT TESTING IMPLEMENTATION=========================================================
///////////////////////////////////////////////////////////////////////////////////////////////////

// TODO(@W): Freely start a new UT just from here, if you have a idea in mind,
//  and then refine it later, JUST keep smooth mind and low resistance.
