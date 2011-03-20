#include <stdio.h>
#include <stdlib.h>

#include <GL/glut.h>

extern "C" {
    #include "nvstusb.h"
}

#include "scene.h"
#include "stereo_helper.h"
#include "screenshot.h"

// global width and height of the window
int GW = 800;
int GH = 600;

// context for dealing with the stereo usb IR emitter
struct nvstusb_context *nv_ctx = NULL;

// 3D camera from stereo helper
StereoHelper::Camera cam;

// forces a particular eye to be displayed (for debugging)
// 0 = normal swapping, 1 = left always, 2 = right always
int force_eye = 0;

// controls whether or not the pulsar is rotating
bool rotation = true;

void draw(int eye) {
    static float angle = 0.0f;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    int show = 0;
    switch (force_eye) {
        case 0:
            show = eye;
            break;
        
        case 1:
            show = 1;
            break;
            
        case 2:
            show = 0;
            break;
    }
    
    // do the camera projection
    StereoHelper::ProjectCamera(cam, (float)GW / GH, show);
    
    // draw Paul Bourke's test scene "pulsar"
    if (rotation) angle += 1.0f;
    PaulBourke::MakeLighting();
    PaulBourke::MakeGeometry(angle);
}

void idle() {
    // which eye are we on? (1/0 for left/right)
    static int current_eye = 0;
 
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
    
    // the 3D button on the IR emitter controls toggling the rotation on and
    // off
    if (k.toggled3D) {
        rotation = !rotation;
        printf("Toggled rotation.\n");
    }
    
    // the wheel on the back adjusts the focal length of the camera (and
    // interoccular distance, since we want to maintain IOD = 1/30th of the
    // focal length)
    if (k.deltaWheel != 0) {
        cam.focal += k.deltaWheel;
        cam.iod = cam.focal / 30.0f;
        printf("Set camera focal length to %f.\n", cam.focal);
    }
    
    // you can also use k.pressedDeltaWheel, which reports the amount the wheel
    // moves while the 3D button is pressed
}

void keyboard(unsigned char key, int x, int y) {
    switch(key) {
        case 'q': case 'Q':
            exit(EXIT_SUCCESS);
            break;
            
        case 'c': case 'C': // switch camera type
            if (cam.type == StereoHelper::TOE_IN) {
                cam.type = StereoHelper::PARALLEL_AXIS_ASYMMETRIC;
                printf("Using parallel axis asymmetric frusta camera.\n");
            } else {
                cam.type = StereoHelper::TOE_IN;
                printf("Using toe-in stereo camera.\n");
            }
            break;
            
        case 'f': case 'F': // force eye
            force_eye = (force_eye + 1) % 3;
            if (force_eye == 0) {
                printf("Swapping eyes normally.\n");
            } else if (force_eye == 1) {
                printf("Forcing left eye always.\n");
            } else {
                printf("Forcing right eye always.\n");
            }
            break;
            
        case 's': case 'S': // take screenshot
            Screenshot::Screenshot(0, 0, GW, GH, "screenshot.tga");
            printf("Wrote frame buffer to screenshot.tga.\n");
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
    StereoHelper::ConfigRefreshRate(nv_ctx);
    
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
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
    Screenshot::Init();
    
    // set up our 3D camera (see stereohelper.h for more documentation)
    cam.type = StereoHelper::PARALLEL_AXIS_ASYMMETRIC;
    cam.eye = StereoHelper::Vec3(39.0f, 53.0f, 22.0f);
    cam.look = StereoHelper::Vec3(0.0f, 0.0f, 0.0f);
    cam.up = StereoHelper::Vec3(0.0f, 1.0f, 0.0f);
    cam.focal = 70.0f;
    cam.fov = 50.0f;
    cam.iod = cam.focal / 30.0f;
    cam.near = 1.0f;
    cam.far = 200.0f;
    
    // off we go!
    glutMainLoop();
    
    // clean up usb emitter
    nvstusb_deinit(nv_ctx);

    return EXIT_SUCCESS;
}
