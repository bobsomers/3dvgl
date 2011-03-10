#include "main.h"

// window width and height
int GW = 800;
int GH = 600;

// frame counter
int frame_count = 0;

// are we running?
int running = 1;

// left/right image tracking
int left_right = 0;

// usb emitter device handle
libusb_device_handle *emitter = NULL;

void bomb(int err, const char *msg) {
    fprintf(stderr, "ERROR %d: %s\n", err, msg);
    exit(EXIT_FAILURE);
}

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

void init_emitter() {
    // initialize libusb
    int result = libusb_init(NULL);
    if (result != LIBUSB_SUCCESS) bomb(result, "libusb_init()");

    // turn on libusb debug messages
    libusb_set_debug(NULL, 3);

    // discover and open the stereo controller in one go
    emitter = libusb_open_device_with_vid_pid(NULL, NVIDIA_VID, NVIDIA_PID);
    if (emitter == NULL) bomb(-1, "libusb_open_device_with_vid_pid()");

    // set device config
    result = libusb_set_configuration(emitter, NVIDIA_CONFIGURATION);
    if (result != LIBUSB_SUCCESS) bomb(result, "libusb_set_configuration()");

    // claim the interface
    result = libusb_claim_interface(emitter, NVIDIA_INTERFACE);
    if (result != LIBUSB_SUCCESS) bomb(result, "libusb_claim_interface()");
    
    // byte streams derived from usb packet traces
    unsigned char step0_bytes[] = {0x42, 0x18, 0x03, 0x00};
    unsigned char step1_bytes[] = {0x01, 0x00, 0x18, 0x00,
                                   0x91, 0xED, 0xFE, 0xFF,
                                   0x33, 0xD3, 0xFF, 0xFF,
                                   0xC6, 0xD7, 0xFF, 0xFF,
                                   0x30, 0x28, 0x24, 0x22,
                                   0x0A, 0x08, 0x05, 0x04,
                                   0x52, 0x79, 0xFE, 0xFF};
    unsigned char step2_bytes[] = {0x01, 0x1C, 0x02, 0x00,
                                   0x02, 0x00};
    unsigned char step3_bytes[] = {0x01, 0x1E, 0x02, 0x00,
                                   0xF0, 0x00};
    unsigned char step4_bytes[] = {0x01, 0x1B, 0x01, 0x00,
                                   0x07};

    int bytes_sent = 0;
    int err = 0;
    
    // step 0 (4 bytes)
    err = libusb_bulk_transfer(emitter, NVIDIA_CONTROL_EP, step0_bytes, 4, &bytes_sent, TIMEOUT_MS);
    if ((err != LIBUSB_SUCCESS) || (bytes_sent < 4)) bomb(err, "libusb_bulk_transfer() [step 0]");

    // step 1 (28 bytes)
    err = libusb_bulk_transfer(emitter, NVIDIA_CONTROL_EP, step1_bytes, 28, &bytes_sent, TIMEOUT_MS);
    if ((err != LIBUSB_SUCCESS) || (bytes_sent < 28)) bomb(err, "libusb_bulk_transfer() [step 1]");

    // step 2 (6 bytes)
    err = libusb_bulk_transfer(emitter, NVIDIA_CONTROL_EP, step2_bytes, 6, &bytes_sent, TIMEOUT_MS);
    if ((err != LIBUSB_SUCCESS) || (bytes_sent < 6)) bomb(err, "libusb_bulk_transfer() [step 2]");

    // step 3 (6 bytes)
    err = libusb_bulk_transfer(emitter, NVIDIA_CONTROL_EP, step3_bytes, 6, &bytes_sent, TIMEOUT_MS);
    if ((err != LIBUSB_SUCCESS) || (bytes_sent < 6)) bomb(err, "libusb_bulk_transfer() [step 3]");

    // step 4 (5 bytes)
    err = libusb_bulk_transfer(emitter, NVIDIA_CONTROL_EP, step4_bytes, 5, &bytes_sent, TIMEOUT_MS);
    if ((err != LIBUSB_SUCCESS) || (bytes_sent < 5)) bomb(err, "libusb_bulk_transfer() [step 4]");
}

void cleanup_emitter() {
    // release the interface
    int result = libusb_release_interface(emitter, NVIDIA_INTERFACE);
    if (result != LIBUSB_SUCCESS) bomb(result, "libusb_release_interface()");
    
    // close the device
    libusb_close(emitter);

    // shutdown libusb
    libusb_exit(NULL);
}

void shutter() {
    unsigned char shutter_packet[] = {0xAA, 0xFE, 0x00, 0x00,
                                      0x00, 0x00, 0xFF, 0xFF};
    unsigned char sync_packet[] = {0x42, 0x18, 0x03, 0x00};

    static int num_shutters = 0;
    
    int bytes_sent = 0;
    int err = 0;
    
    if (num_shutters >= SHUTTERS_PER_SYNC) {
        num_shutters = 0;
        
        // send out a sync packet
        err = libusb_bulk_transfer(emitter, NVIDIA_CONTROL_EP, sync_packet, 4, &bytes_sent, TIMEOUT_MS);
        if ((err != LIBUSB_SUCCESS) || (bytes_sent < 4)) bomb(err, "libusb_bulk_transfer() [sync packet]");
    }
    
    // send out a shutter packet
    shutter_packet[1] = 0xFE | left_right;
    err = libusb_bulk_transfer(emitter, NVIDIA_SYNC_EP, shutter_packet, 8, &bytes_sent, TIMEOUT_MS);
    if ((err != LIBUSB_SUCCESS) || (bytes_sent < 8)) bomb(err, "libusb_bulk_transfer() [shutter packet]");
    num_shutters++;
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
    static uint64_t samples[1000];
    static timespec cycle_start = {0, 0};
    static timespec cycle_end = {0, 0};
    static int sample_i = 0;

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
    shutter();
    frame_count++;
    left_right = !left_right;
    
    cycle_start = cycle_end;
    clock_gettime(CLOCK_MONOTONIC, &cycle_end);
    if (sample_i < 1000) {
        samples[sample_i] = nanotime_diff(cycle_start, cycle_end).tv_nsec;
        sample_i++;
    } else if (sample_i == 1000) {
        int i = 0;
        for (i = 0; i < 1000; i++) {
            printf("sample[%d] = %lu\n", i, samples[i]);
        }
    }
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
            break;
        case 'f': case 'F': // force redisplay
            glutPostRedisplay();
            break;
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

    // enter full screen
    glutGameModeString("1920x1080:32@120");
    glutEnterGameMode();

    // glut callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);

    // opengl base state
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_FLAT);

    // 3d vision emitter initialization
    init_emitter();

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
    
    cleanup_emitter();
    
    printf("Done.\n");
    return EXIT_SUCCESS;
}
