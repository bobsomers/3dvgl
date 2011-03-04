#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>

#include <GL/freeglut.h>
#include <libusb-1.0/libusb.h>

typedef struct timespec timespec;

// window width and height
int GW = 800;
int GH = 600;

// frame counter
int frame_count = 0;

// are we running?
int running = 1;

// left/right image tracking
int left_right = 0;

timespec nanotime_diff(timespec start, timespec end) {
    timespec temp;

    if ((end.tv_nsec - start.tv_nsec) < 0) {
        temp.tv_sec = end.tv_sec - start.tv_sec - 1;
        temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec - start.tv_sec;
        temp.tv_nsec = end.tv_nsec - start.tv_nsec;
    }

    return temp;
}

void camera() {
    // project the camera on the projection stack (bad... i know)
    glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(60.0f, (float)GW / GH, 1.0f, 100.0f);
        gluLookAt(2.0f, 4.0f, 10.0f,
                  0.0f, 0.0f, 0.0f,
                  0.0f, 1.0f, 0.0f);
    glMatrixMode(GL_MODELVIEW);
}

void draw_axes() {
    #define SIZE 3.0f

    glBegin(GL_LINES);
        glColor3f(1.0f, 0.0f, 0.0f);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(SIZE, 0.0f, 0.0f);
        glColor3f(0.0f, 1.0f, 0.0f);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(0.0f, SIZE, 0.0f);
        glColor3f(0.0f, 0.0f, 1.0f);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(0.0f, 0.0f, SIZE);
    glEnd();

    #undef SIZE
}

void display() {
    // clear color and depth buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // project the camera
    camera();

    // clear the modelview stack
    glLoadIdentity();

    // draw the axes
    draw_axes();

    if (left_right == 0) {
        glPushMatrix();
            glTranslatef(-3.0f, 1.0f, 2.0f);
            glColor3f(0.0f, 0.0f, 1.0f);
            glutSolidCube(2.0f);
        glPopMatrix();
    } else {
        glPushMatrix();
            glTranslatef(3.0f, 2.0f, 1.0f);
            glColor3f(1.0f, 0.0f, 0.0f);
            glutSolidSphere(1.0f, 20, 20);
        glPopMatrix();
    }
    
    // swap buffers (TODO: voodoo here)
    glutSwapBuffers();
    frame_count++;
    left_right = !left_right;
}

void reshape(int w, int h) {
    GW = w;
    GH = h;

    glViewport(0, 0, GW, GH);
    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 'q': case 'Q': // quit
           running = 0; 
    }
}

int main(int argc, char *argv[]) {
    timespec fps_timer;
    timespec now;

    // initialize glut
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(GW, GH);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("A/B Test");

    // glut callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);

    // opengl base state
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_FLAT);

    // fps timer... bang!
    clock_gettime(CLOCK_MONOTONIC, &fps_timer);

    // main loop
    while (running) {
        clock_gettime(CLOCK_MONOTONIC, &now);

        // force a redraw
        glutPostRedisplay();

        // process glut events
        glutMainLoopEvent();

        // check for fps print time
        if (nanotime_diff(fps_timer, now).tv_sec > 0) {
            fps_timer = now;
            printf("%d\n", frame_count);
            frame_count = 0;
        }
    }
    
    printf("Done.\n");
    return EXIT_SUCCESS;
}
