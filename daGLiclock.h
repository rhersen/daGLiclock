#include <GL/gl.h>

struct point2d {
    double x, y;
};

struct point3d {
    double x, y, z;
};

struct bezier_point {
    struct point2d point;
    struct point2d normal;
};

int parse_args(int argc, char **argv);
void set_aspect(GLdouble aspect);
void draw_digit(int from_digit, int to_digit, double interpolation, int slices);
