struct point2d
{
      double x, y;
};

struct point3d
{
      double x, y, z;
};

struct bezier_point
{
      struct point2d point;
      struct point2d normal;
};

struct camera
{
      double x, y, z;
      double rotx, roty, rotz;
};

extern struct camera camera;
extern double spin_speed;
extern int slices;
extern int steps;

extern struct bezier_point *bezier_points;

extern GLuint lists;

extern int from[];
extern int to[];

void parse_args(int argc, char **argv);
void set_aspect(GLdouble aspect);
void draw_digit(int from_digit, int to_digit, double interpolation);
