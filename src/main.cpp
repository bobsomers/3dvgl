#include <stdio.h>
#include <stdlib.h>

#include <GL/glut.h>

extern "C" {
    #include "nvstusb.h"
}

#include "stereo_helper.h"

// global width and height of the window
int GW = 800;
int GH = 600;

// context for dealing with the stereo usb IR emitter
struct nvstusb_context *nv_ctx = NULL;

void draw(int eye) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glBegin(GL_QUADS);
    if (eye) { // left eye
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex2f(-1.0f, -1.0f);
        glVertex2f(0.0f, -1.0f);
        glVertex2f(0.0f, 1.0f);
        glVertex2f(-1.0f, 1.0f);
        
    } else { // right eye
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex2f(0.0f, -1.0f);
        glVertex2f(1.0f, -1.0f);
        glVertex2f(1.0f, 1.0f);
        glVertex2f(0.0f, 1.0f);
    }
    glEnd();
}

void idle() {
    // which eye are we on? (1/0 for left/right)
    static int current_eye = 0;
    
    // TODO: updates 'n shit
 
    // draw the frame for the current eye
    draw(current_eye);
    
    // this replaces our traditional glutSwapBuffers call (let the usb emitter
    // code call it and keep track of things)
    nvstusb_swap(nv_ctx, (nvstusb_eye) current_eye, glutSwapBuffers);
    current_eye = (current_eye + 1) % 2;
    
    // get the status of the button/wheel on the emitter (you MUST do this,
    // otherwise the whole system will stall out after just a couple of frames)
    struct nvstusb_keys k;
    nvstusb_get_keys(nv_ctx, &k);
    
    if (k.toggled3D) printf("TOGGLE!\n");
    if (k.deltaWheel != 0) printf("DELTA WHEEL %d\n", k.deltaWheel);
    if (k.pressedDeltaWheel != 0) printf("PRESSED DELTA WHEEL %d\n", k.pressedDeltaWheel);
}

void keyboard(unsigned char key, int x, int y) {
    switch(key) {
        case 'q': case 'Q':
            exit(EXIT_SUCCESS);
            break;
    }
}

void reshape(int w, int h) {
    GW = w;
    GH = h;
    glViewport(0, 0, GW, GH);
}

int main(int argc, char *argv[]) {
    printf("Starting up the demo app!\n");
    
    // initialize glut
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    
    // initialize communications with the usb emitter
    nv_ctx = nvstusb_init();
    if (nv_ctx == NULL) {
        fprintf(stderr, "Could not initialize NVIDIA 3D Vision IR emitter!\n");
        exit(EXIT_FAILURE);
    }
    
    // auto-config the vsync rate
    stereo_helper_config_refresh_rate(nv_ctx);
    
    // create glut windows
    glutInitWindowSize(GW, GH);
    glutInitWindowPosition(500, 500);
    glutCreateWindow("NVIDIA 3D Vision OpenGL on Linux Demo");
    
    // set glut callbacks
    glutIdleFunc(idle);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
 
    // set up opengl state
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    
    // off we go!
    glutMainLoop();
    
    // clean up usb emitter
    nvstusb_deinit(nv_ctx);

    return EXIT_SUCCESS;
}
