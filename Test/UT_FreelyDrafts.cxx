#include "_UT_IOC_Common.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//===TEMPLATE OF UT CASE===
/**
 * @[Name]: ${verifyBehivorX_byDoABC}
 * @[Purpose]: ${what to verify}
 * @[Steps]: ${how to do}
 *   1. ...
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
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO(@W): Freely start a new UT first from here, and then refine it later, just keep smooth mind and low resistance.
