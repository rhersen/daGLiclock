#include <time.h>
#include <GL/gl.h>
#include "SDL.h"
#include "daGLiclock.h"

struct camera {
    double x, y, z;
    double rotx, roty, rotz;
};

int main(int argc, char **argv)
{
    double interpolation = 0;
    int framecount = 0;
    int i;
    int width = 800;
    int height = 288;
    struct camera camera = { 0, 0, 9, 0, 0 };
    double spin_speed = 1;
    int from[6] = {0};
    int to[6] = {0};

    parse_args(argc, argv);

    steps = 4 * slices;

    if (SDL_Init(SDL_INIT_VIDEO) < 0 || !SDL_SetVideoMode(width, height, 0, SDL_OPENGL)) {
        return 1;
    }

    SDL_WM_SetCaption("Dali Clock", NULL);
    glViewport(0, 0, width, height);
    glClearDepth(1.0);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    set_aspect(1.0 * width / height);
    glMatrixMode(GL_MODELVIEW);

    {
#define light 0.6
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

    GLuint lists = glGenLists(10);

    for (i = 0; i < 10; i++) {
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

        if (gettimeofday(&t, 0) == 0) {
            int new = t.tv_sec % 10;

            if (new != to[0]) {
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

        for (pos = 0; pos < 6; pos++) {
            glPushMatrix();

            glTranslatef(7.5 - 3 * pos, 0, 0);

            if (from[pos] == to[pos]) {
                glCallList(lists + from[pos]);
            } else {
                draw_digit(from[pos], to[pos], interpolation);
            }

            glPopMatrix();
        }

        SDL_GL_SwapBuffers();
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                SDL_Quit();
                return 0;
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                spin_speed = 0;
            } else if (event.type == SDL_MOUSEMOTION) {
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
