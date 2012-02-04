/*
  This is how I compile this program:
  gcc daGLiclock.c -o daGLiclock -lGL `sdl-config --cflags --libs`
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "daGLiclock.h"

#define NPOINTS 10
#define NCTRL 2

static struct point2d points[][NPOINTS] = {
    {
        {0, -2},
        {-1, -2},
        {-1, 2},
        {0, 2},
        {1, 2},
        {1, 1},
        {0.8, 0},
        {0.6, -1},
        {0.6, -2},
        {0, -2},
    },
    {
        {-1, 1.7},
        {-0.6, 2},
        {-0.6, 2},
        {-0.5, 2},
        {0, 2},
        {0, 2},
        {0, 0},
        {0, -1},
        {0, -1},
        {0, -2},
    },
    {
        {-1, 1.8},
        {1, 3},
        {1, 1.5},
        {1, 1},
        {1, 0},
        {-2, -1},
        {-1, -2},
        {-0.9, -2.1},
        {-0.9, -2.1},
        {1, -2},
    },
    {
        {-1, 1.8},
        {1, 3},
        {1, 0.4},
        {0.25, 0.4},
        {-0.25, 0.4},
        {-0.25, -0.4},
        {0.25, -0.4},
        {1, -0.4},
        {1, -3},
        {-1, -1.8},
    },
    {
        {1, -1},
        {-1, -1},
        {-1, -1},
        {-1, -0.5},
        {-1, 0},
        {-0.5, 2},
        {0, 2},
        {0.5, 2},
        {0.5, 2},
        {0.5, -2},
    },
    {
        {1, 2},
        {-0.4, 2},
        {-0.4, 2},
        {-0.5, 2},
        {-1, 2},
        {-1, 0},
        {-0.5, 0},
        {1.5, 0},
        {1.5, -3},
        {-1, -1.8},
    },
    {
        {0, 2},
        {-0.6, 1},
        {-0.6, 1},
        {-0.8, 0},
        {-1, -1},
        {-1, -2},
        {0, -2},
        {1, -2},
        {1, 0.5},
        {-0.8, 0},
    },
    {
        {-1, 2},
        {0, 2},
        {0.8, 2},
        {0.9, 1.9},
        {1, 1.8},
        {1, 1.5},
        {1, 1},
        {1, 0.9},
        {1,  0.9},
        {-1, -2},
    },
    {
        {-1, 1},
        {-1, 2},
        {2, 2},
        {0, 0},
        {-2, -2},
        {1, -2},
        {1, -1},
        {1, 0},
        {-1, 0},
        {-1, 1},
    },
    {
        {0.8, 0},
        {-1, -0.5},
        {-1, 2},
        {0, 2},
        {1, 2},
        {1, 1},
        {0.8, 0},
        {0.6, -1},
        {0.6, -1},
        {0, -2},
    }
};

int steps;
double fatness = 0.2;

struct bezier_point *bezier_points;

void draw_digit(int from_digit, int to_digit, double interpolation, int slices) {
    void calc_bezier_points(struct bezier_point *r,
                            int from_digit, int to_digit, double interpolation,
                            int segment) {
        double bernstein(int i, int n, double t) {
            int binom(int n, int i) {
                int factorial(int n) {
                    if (n < 2) {
                        return 1;
                    } else {
                        return n * factorial(n - 1);
                    }
                }

                return factorial(n) / factorial(i) / factorial(n - i);
            }

            if (i < 0 || i > n) {
                return 0;
            }
            else {
                return binom(n, i) * pow(t, i) * pow(1 - t, n - i);
            }
        }

        struct point2d bezier(struct point2d ctrl_points[], double t) {
            struct point2d r = {0};
            int i;

            for (i = 0; i < NCTRL + 2; i++) {
                double bt = bernstein(i, NCTRL + 1, t);
                r.x += ctrl_points[i].x * bt;
                r.y += ctrl_points[i].y * bt;
            }

            return r;
        }

        struct point2d bezier_deriv(struct point2d ctrl_points[], double t) {
            struct point2d r = {0};
            int i;

            for (i = 0; i < NCTRL + 2; i++) {
                double d = (NCTRL + 1) * (bernstein(i - 1, NCTRL, t) - bernstein(i, NCTRL, t));
                r.x += ctrl_points[i].x * d;
                r.y += ctrl_points[i].y * d;
            }

            return r;
        }

        void normalize(struct point2d *v) {
            double length = sqrt(v->x * v->x + v->y * v->y);
            v->x /= length;
            v->y /= length;
        }

        void interpolate(struct point2d *r,
                         struct point2d *from,
                         struct point2d *to,
                         double interpolation) {
            double smooth(double x) {
                return 3 * x * x - 2 * x * x * x;
            }

            double smooth_interpolation = smooth(interpolation);
            int i;

            for (i = 0; i < NCTRL + 2; i++) {
                r[i].x = from[i].x * (1 - smooth_interpolation) + to[i].x * smooth_interpolation;
                r[i].y = from[i].y * (1 - smooth_interpolation) + to[i].y * smooth_interpolation;
            }
        }

        int step;

        for (step = 0; step <= steps; step++) {
            double t = 1.0 * step / steps;
            struct point2d interpolated[NCTRL + 2];

            interpolate(interpolated,
                        points[from_digit] + (NCTRL + 1) * segment,
                        points[to_digit] + (NCTRL + 1) * segment,
                        interpolation);

            r[step].point = bezier(interpolated, t);
            struct point2d tangent = bezier_deriv(interpolated, t);
            r[step].normal.x = tangent.y;
            r[step].normal.y = -tangent.x;

            normalize(&r[step].normal);
        }
    }

    void vertex(int from_digit, int to_digit, double interpolation, int step, int slice) {
        struct point3d normal;
        double x, y, z;

        if (step % 2) {
            normal.x = bezier_points[step].normal.x * sin(2 * M_PI * (slice - 0.5) / slices);
            normal.y = bezier_points[step].normal.y * sin(2 * M_PI * (slice - 0.5) / slices);
            normal.z = cos(2 * M_PI * (slice - 0.5) / slices);
        } else {
            normal.x = bezier_points[step].normal.x * sin(2 * M_PI * slice / slices);
            normal.y = bezier_points[step].normal.y * sin(2 * M_PI * slice / slices);
            normal.z = cos(2 * M_PI * slice / slices);
        }

        x = fatness * normal.x;
        y = fatness * normal.y;
        z = fatness * normal.z;

        glNormal3d(normal.x, normal.y, normal.z);
        glVertex3d(bezier_points[step].point.x + x, bezier_points[step].point.y + y, z);
    }

    int segment;

    for (segment = 0; segment < 3; segment++) {
        int i;

        calc_bezier_points(bezier_points, from_digit, to_digit, interpolation, segment);

        for (i = 0; i < steps - 1; i += 2) {
            int slice;

            glBegin(GL_TRIANGLE_STRIP);

            for (slice = 0; slice < slices; slice++) {
                vertex(from_digit, to_digit, interpolation, i + 1, slice);
                vertex(from_digit, to_digit, interpolation, i, slice);
            }

            vertex(from_digit, to_digit, interpolation, i + 1, 0);
            vertex(from_digit, to_digit, interpolation, i, 0);
            glEnd();

            glBegin(GL_TRIANGLE_STRIP);

            for (slice = 0; slice < slices; slice++) {
                vertex(from_digit, to_digit, interpolation, i + 2, slice);
                vertex(from_digit, to_digit, interpolation, i + 1, slice + 1);
            }

            vertex(from_digit, to_digit, interpolation, i + 2, 0);
            vertex(from_digit, to_digit, interpolation, i + 1, 1);
            glEnd();
        }
    }
}

int parse_args(int argc, char **argv) {
    if (argc == 1) {
        return 8;
    }

    if (argc == 3 && strcmp(argv[1], "-q") == 0) {
        int q;

        if (strcmp(argv[2], "lowest") == 0) {
            return 2;
        } else if (strcmp(argv[2], "low") == 0) {
            return 4;
        } else if (strcmp(argv[2], "standard") == 0) {
            return 8;
        } else if (strcmp(argv[2], "high") == 0) {
            return 16;
        } else if (sscanf(argv[2], "%d", &q) == 1) {
            return q;
        }
    }

    return -1;
}

void set_aspect(GLdouble aspect) {
    GLdouble y = 0.1 * (sqrt(2) - 1);
    GLdouble x = y * aspect;

    glFrustum(-x, x, -y, y, 0.1, 100);
}
