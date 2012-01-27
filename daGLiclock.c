/*
  This is how I compile this program:
  gcc daGLiclock.c -o daGLiclock -lGL `sdl-config --cflags --libs`
*/

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include <GL/gl.h>
#include "SDL.h"

#define NPOINTS 10
#define NCTRL 2

static struct camera
{
      double x, y, z;
      double rotx, roty, rotz;
} camera = { 0, 0, 9, 0, 0 };

static double spin_speed = 0;

struct point2d
{
      double x, y;
};

struct point3d
{
      double x, y, z;
};

static struct point2d points[][NPOINTS] =
{
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

static int slices = 8;
static int steps;
static double fatness = 0.4;

static struct bezier_point
{
      struct point2d point;
      struct point2d normal;
} *bezier_points;

static int from[6] = {0};
static int to[6] = {0};

static GLuint lists;

static double smooth(double x) {
  return 3 * x * x - 2 * x * x * x;
}

static int factorial(int n)
{
   if (n < 2)
   {
      return 1;
   }
   else
   {
      return n * factorial(n - 1);
   }
}

static int binom(int n, int i)
{
   return factorial(n) / factorial(i) / factorial(n - i);
}

static double bernstein(int i, int n, double t)
{
   if (i < 0 || i > n)
   {
      return 0;
   }
   else
   {
      return binom(n, i) * pow(t, i) * pow(1 - t, n - i);
   }
}

static struct point2d bezier(struct point2d ctrl_points[], double t)
{
   struct point2d r = {0};
   int i;

   for (i = 0; i < NCTRL + 2; i++)
   {
      double bt = bernstein(i, NCTRL + 1, t);
      r.x += ctrl_points[i].x * bt;
      r.y += ctrl_points[i].y * bt;
   }

   return r;
}

static struct point2d bezier_deriv(struct point2d ctrl_points[], double t)
{
   struct point2d r = {0};
   int i;

   for (i = 0; i < NCTRL + 2; i++)
   {
      double d = (NCTRL + 1) * (bernstein(i - 1, NCTRL, t) - bernstein(i, NCTRL, t));
      r.x += ctrl_points[i].x * d;
      r.y += ctrl_points[i].y * d;
   }

   return r;
}

static void normalize(struct point2d *v) {
  double length = sqrt(v->x * v->x + v->y * v->y);
  v->x /= length;
  v->y /= length;
}

static void interpolate(struct point2d *r,
			struct point2d *from,
			struct point2d *to,
			double interpolation)
{
   double smooth_interpolation = smooth(interpolation);
   int i;

   for (i = 0; i < NCTRL + 2; i++)
   {
      r[i].x = from[i].x * (1 - smooth_interpolation) + to[i].x * smooth_interpolation;
      r[i].y = from[i].y * (1 - smooth_interpolation) + to[i].y * smooth_interpolation;
   }
}

static void calc_bezier_points(struct bezier_point *r,
			       int from_digit, int to_digit, double interpolation,
			       int segment)
{
   int step;

   for (step = 0; step <= steps; step++)
   {
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

static void vertex(int from_digit, int to_digit, double interpolation, int step, int slice)
{
   struct point3d normal;
   double x, y, z;

   if (step % 2)
   {
      normal.x = bezier_points[step].normal.x * sin(2 * M_PI * (slice - 0.5) / slices);
      normal.y = bezier_points[step].normal.y * sin(2 * M_PI * (slice - 0.5) / slices);
      normal.z = cos(2 * M_PI * (slice - 0.5) / slices);
   }
   else
   {
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

static void draw_digit(int from_digit, int to_digit, double interpolation)
{
   int segment;

   for (segment = 0; segment < 3; segment++)
   {
      int i;

      calc_bezier_points(bezier_points, from_digit, to_digit, interpolation, segment);

      for (i = 0; i < steps - 1; i += 2)
      {
	 int slice;

	 glBegin(GL_TRIANGLE_STRIP);

	 for (slice = 0; slice < slices; slice++)
	 {
	    vertex(from_digit, to_digit, interpolation, i + 1, slice);
	    vertex(from_digit, to_digit, interpolation, i, slice);
	 }

	 vertex(from_digit, to_digit, interpolation, i + 1, 0);
	 vertex(from_digit, to_digit, interpolation, i, 0);
	 glEnd();

	 glBegin(GL_TRIANGLE_STRIP);

	 for (slice = 0; slice < slices; slice++)
	 {
	    vertex(from_digit, to_digit, interpolation, i + 2, slice);
	    vertex(from_digit, to_digit, interpolation, i + 1, slice + 1);
	 }

	 vertex(from_digit, to_digit, interpolation, i + 2, 0);
	 vertex(from_digit, to_digit, interpolation, i + 1, 1);
	 glEnd();
      }
   }
}

void parse_args(int argc, char **argv)
{
   if (argc == 1)
   {
      return;
   }

   if (argc == 3 && strcmp(argv[1], "-q") == 0)
   {
      int q;

      if (strcmp(argv[2], "lowest") == 0)
      {
	 slices = 2;
	 return;
      }
      else if (strcmp(argv[2], "low") == 0)
      {
	 slices = 4;
	 return;
      }
      else if (strcmp(argv[2], "standard") == 0)
      {
	 slices = 8;
	 return;
      }
      else if (strcmp(argv[2], "high") == 0)
      {
	 slices = 16;
	 return;
      }
      else if (sscanf(argv[2], "%d", &q) == 1)
      {
	 slices = q;
	 return;
      }
   }

   fprintf(stderr,
	   "usage: %s [-q <quality>]\n"
	   "where quality is an integer >= 2 or any of the aliases\n"
	   "lowest, low, standard or high, which are translated to\n"
	   "2, 4, 8 and 16, respectively.\n",
	   argv[0]);

   exit(1);
}

static void set_aspect(GLdouble aspect)
{
   GLdouble y = 0.1 * (sqrt(2) - 1);
   GLdouble x = y * aspect;

   glFrustum(-x, x, -y, y, 0.1, 100);
}

int main(int argc, char **argv)
{
   double interpolation = 0;
   int framecount = 0;
   int i;
   int width = 800;
   int height = 288;

   parse_args(argc, argv);

   steps = 4 * slices;

   if (SDL_Init(SDL_INIT_VIDEO) < 0 || !SDL_SetVideoMode(width, height, 0, SDL_OPENGL))
   {
      return 1;
   }

   SDL_WM_SetCaption("Dali Clock", NULL);
   glViewport(0, 0, width, height);
   glClearDepth(1.0);		// Enables Clearing Of The Depth Buffer
   glDepthFunc(GL_LESS);	// The Type Of Depth Test To Do
   glEnable(GL_DEPTH_TEST);	// Enables Depth Testing
   glEnable(GL_CULL_FACE);
   glShadeModel(GL_SMOOTH);	// Enables Smooth Color Shading
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();		// reset The Projection Matrix
   set_aspect(1.0 * width / height);
   glMatrixMode(GL_MODELVIEW);

   {
      GLfloat light = 0.6;
      GLfloat pos[4] = {0.0, 40.0, 0.0, 1.0};
      GLfloat amb[4] = {light, light, light, 1.0};
      GLfloat dif[4] = {light, light, light, 1.0};
      GLfloat spc[4] = {1.0, 1.0, 1.0, 1.0};
      GLfloat ambient[4] = {0.247250, 0.199500, 0.074500, 1.000000};
      GLfloat diffuse[4] = {0.751640, 0.606480, 0.226480, 1.000000};
      GLfloat specular[4] = {0.628281, 0.555802, 0.366065, 1.000000};
      GLfloat shininess = 51.200001;

      glEnable(GL_LIGHTING);
      glEnable(GL_DEPTH_TEST);

/*       glEnable(GL_LIGHT0); */
      glLightfv(GL_LIGHT0, GL_POSITION, pos);
      glLightfv(GL_LIGHT0, GL_AMBIENT,  amb);
      glLightfv(GL_LIGHT0, GL_DIFFUSE,  dif);
      glLightfv(GL_LIGHT0, GL_SPECULAR, spc);

      glEnable(GL_LIGHT1);
      pos[2] = 30;
      glLightfv(GL_LIGHT1, GL_POSITION, pos);
      glLightfv(GL_LIGHT1, GL_AMBIENT,  amb);
      glLightfv(GL_LIGHT1, GL_DIFFUSE,  dif);
      glLightfv(GL_LIGHT1, GL_SPECULAR, spc);

/*       glEnable(GL_LIGHT2); */
      pos[0] = -10;
      glLightfv(GL_LIGHT2, GL_POSITION, pos);
      glLightfv(GL_LIGHT2, GL_AMBIENT,  amb);
      glLightfv(GL_LIGHT2, GL_DIFFUSE,  dif);
      glLightfv(GL_LIGHT2, GL_SPECULAR, spc);

      glMaterialfv(GL_FRONT, GL_AMBIENT,  ambient);
      glMaterialfv(GL_FRONT, GL_DIFFUSE,  diffuse);
      glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
      glMaterialf(GL_FRONT, GL_SHININESS, shininess);
   }

   bezier_points = malloc((1 + steps) * sizeof(struct bezier_point));

   lists = glGenLists(10);

   for (i = 0; i < 10; i++)
   {
      glNewList(lists + i, GL_COMPILE);
      draw_digit(i, i, 0);
      glEndList();
   }

   while (1) {
      int pos;
      SDL_Event event;
      struct timeval t;

      framecount++;

      glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
      glLoadIdentity();
      
      glRotatef(camera.rotz, 0, 0, 1);
      glTranslatef(0,0/* -camera.y */,-camera.z);
      glRotatef(camera.rotx, 1, 0, 0);
      glRotatef(camera.roty += spin_speed, 0, 1, 0);

      if (gettimeofday(&t, 0) == 0)
      {
	 int new = t.tv_sec % 10;

	 if (new != to[0])
	 {
	    time_t timep = time(0);
	    struct tm *tm = localtime(&timep);
	    from[0] = to[0];
	    to[0] = new;
	    from[1] = to[1];
	    to[1] = tm->tm_sec / 10;
	    from[2] = to[2];
	    to[2] = tm->tm_min % 10;
	    from[3] = to[3];
	    to[3] = tm->tm_min / 10;
	    from[4] = to[4];
	    to[4] = tm->tm_hour % 10;
	    from[5] = to[5];
	    to[5] = tm->tm_hour / 10;

	    printf("%d fps\n", framecount);
	    framecount = 0;
	 }

	 interpolation = t.tv_usec / 1e6;
      }

      for (pos = 0; pos < 6; pos++)
      {
	 glPushMatrix();

	 glTranslatef(7.5 - 3 * pos, 0, 0);

	 if (from[pos] == to[pos])
	 {
	    glCallList(lists + from[pos]);
	 }
	 else
	 {
	    draw_digit(from[pos], to[pos], interpolation);
	 }

	 glPopMatrix();
      }

      SDL_GL_SwapBuffers();
      while (SDL_PollEvent(&event))
      {
	 if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
	 {
	    SDL_Quit();
	    return 0;
	 }
	 else if (event.type == SDL_MOUSEBUTTONDOWN)
	 {
	    spin_speed = 0;
	 }
	 else if (event.type == SDL_MOUSEMOTION)
	 {
	    if (event.motion.state == (SDL_BUTTON(1) | SDL_BUTTON(3))) {
	       camera.z += event.motion.yrel / 16.0;
	    } else if (event.motion.state & SDL_BUTTON(1)) {
	       spin_speed += event.motion.xrel / 256.0;
	    } else if (event.motion.state & SDL_BUTTON(3)) {
	       camera.roty += event.motion.xrel / 4.0;
	       camera.rotx += event.motion.yrel / 4.0;
	    }

	 }
      }

      SDL_Delay(1);
   }
}
