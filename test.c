#include "daGLiclock.h"
#include "CUnit/Basic.h"

void glBegin(GLenum mode) {
}

void glEnd(void) {
}

void glNormal3d(GLdouble nx, GLdouble ny, GLdouble nz) {
}

void glVertex3d(GLdouble x, GLdouble y, GLdouble z) {
}

static double mockAspect;

void glFrustum(GLdouble left, GLdouble right,
               GLdouble bottom, GLdouble top,
               GLdouble near_val, GLdouble far_val) {
    mockAspect = right / top;
}

int main() {
    int setUp(void) {
        return 0;
    }

    int tearDown(void) {
        return 0;
    }

    void testParseArgs(void) {
        CU_ASSERT_EQUAL(7, parse_args(3, (char *[]) {"daGLiclock", "-q", "7"}));
        CU_ASSERT_EQUAL(4, parse_args(3, (char *[]) {"daGLiclock", "-q", "low"}));
        CU_ASSERT_EQUAL(16, parse_args(3, (char *[]) {"daGLiclock", "-q", "high"}));
    }

    void testSetAspect(void) {
        set_aspect(3.14);
        CU_ASSERT_DOUBLE_EQUAL(3.14, mockAspect, 1e-4);
    }

    CU_initialize_registry();
    CU_pSuite s = CU_add_suite("dark", setUp, tearDown);

    if (s && CU_add_test(s, "parse_args", testParseArgs) && CU_add_test(s, "aspect", testSetAspect)) {
        CU_basic_set_mode(CU_BRM_VERBOSE);
        CU_basic_run_tests();
    }

    CU_cleanup_registry();
    return CU_get_error();
}
