// __________________________________ CONTENTS ___________________________________
//
//    Common utils / includes / namespaces used for testing.
//    Reduces test boilerplate, should not be included anywhere else.
// _______________________________________________________________________________

// ___________________ TEST FRAMEWORK  ____________________

#define DOCTEST_CONFIG_VOID_CAST_EXPRESSIONS // makes 'CHECK_THROWS()' not give warning for discarding [[nodiscard]]
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN   // automatically creates 'main()' that runs tests
#include "external/doctest.h"
