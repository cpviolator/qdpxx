#ifndef TIME_MAT_EQ_HERM_MAT_DOUBLE
#define TIME_MAT_EQ_HERM_MAT_DOUBLE


#ifndef UNITTEST_H
#include "unittest.h"
#endif

#include "scalarsite_sse/sse_linalg_mm_su3_double.h"

class timeMeqHM_QDP  : public TestFixture { public: void run(void); };
class timeMeqHM      : public TestFixture { public: void run(void); };

class timeMPeqaHM_QDP  : public TestFixture { public: void run(void); };
class timeMPeqaHM      : public TestFixture { public: void run(void); };


#endif
