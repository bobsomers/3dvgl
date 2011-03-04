#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <libusb-1.0/libusb.h>

// usb configuration for NVIDIA 3D Vision stereo IR transmitter
#define NVIDIA_VID 0x955
#define NVIDIA_PID 0x7
#define NVIDIA_INTERFACE 0x0
#define NVIDIA_CONFIGURATION 0x1
#define NVIDIA_CONTROL_EP 0x2
#define NVIDIA_SYNC_EP 0x1

// usb timeout in milliseconds
#define TIMEOUT_MS 5000

// shutter wait time, in nanoseconds
#define SHUTTER_TIME 8333333

// number of shutter packets between each sync packet
#define SHUTTERS_PER_SYNC 14

// number of shutter packets to burst before starting to allow
// the glasses to recover the refresh frequency
#define FREQ_RECOVERY_PACKETS 32

typedef struct timespec timespec;

// are we running?
int running = 1;

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

void goodbye(int sig) {
    // try to shut down gracefully, but restore the
    // default signal handler... just in case
    running = 0;
    signal(SIGINT, SIG_DFL);
}

void shutter(libusb_device_handle *devh) {
    unsigned char shutter_packet[] = {0xAA, 0xFE, 0x00, 0x00,
                                      0x00, 0x00, 0xFF, 0xFF};
    unsigned char sync_packet[] = {0x42, 0x18, 0x03, 0x00};

    timespec shutter_start;
    timespec fps_start;
    timespec now;

    int shutter_count = 0;
    int frame_count = 0;
    int bytes_sent = 0;
    int err = 0;
    int left_right = 1;
    int initial = FREQ_RECOVERY_PACKETS;
            
    // bang!
    clock_gettime(CLOCK_MONOTONIC, &shutter_start);
    fps_start = shutter_start;
            
    // send out a sync packet
    err = libusb_bulk_transfer(devh, NVIDIA_CONTROL_EP, sync_packet, 4, &bytes_sent, TIMEOUT_MS);
    if ((err != LIBUSB_SUCCESS) || (bytes_sent < 4)) bomb(err, "libusb_bulk_transfer() [sync packet]");
    
    while (running) {
        clock_gettime(CLOCK_MONOTONIC, &now);

        if (nanotime_diff(shutter_start, now).tv_nsec > SHUTTER_TIME) {
            shutter_start = now;
            frame_count++;
            if (initial > 0) {
                initial--;
            } else {
                shutter_count++;
            }

            // send out a shutter packet
            if (initial <= 0) shutter_packet[1] = 0xFE | left_right;
            err = libusb_bulk_transfer(devh, NVIDIA_SYNC_EP, shutter_packet, 8, &bytes_sent, TIMEOUT_MS);
            if ((err != LIBUSB_SUCCESS) || (bytes_sent < 8)) bomb(err, "libusb_bulk_transfer() [shutter packet]");
            left_right = !left_right;
        }

        if (shutter_count >= SHUTTERS_PER_SYNC && initial <= 0) {
            shutter_count = 0;

            // send out a sync packet
            err = libusb_bulk_transfer(devh, NVIDIA_CONTROL_EP, sync_packet, 4, &bytes_sent, TIMEOUT_MS);
            if ((err != LIBUSB_SUCCESS) || (bytes_sent < 4)) bomb(err, "libusb_bulk_transfer() [sync packet]");
        }

        if (nanotime_diff(fps_start, now).tv_sec > 0) {
            fps_start = now;
            printf("%d %d\n", frame_count, shutter_count);
            frame_count = 0;
        }
    }
}

void initialize(libusb_device_handle *devh) {
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
    err = libusb_bulk_transfer(devh, NVIDIA_CONTROL_EP, step0_bytes, 4, &bytes_sent, TIMEOUT_MS);
    if ((err != LIBUSB_SUCCESS) || (bytes_sent < 4)) bomb(err, "libusb_bulk_transfer() [step 0]");

    // step 1 (28 bytes)
    err = libusb_bulk_transfer(devh, NVIDIA_CONTROL_EP, step1_bytes, 28, &bytes_sent, TIMEOUT_MS);
    if ((err != LIBUSB_SUCCESS) || (bytes_sent < 28)) bomb(err, "libusb_bulk_transfer() [step 1]");

    // step 2 (6 bytes)
    err = libusb_bulk_transfer(devh, NVIDIA_CONTROL_EP, step2_bytes, 6, &bytes_sent, TIMEOUT_MS);
    if ((err != LIBUSB_SUCCESS) || (bytes_sent < 6)) bomb(err, "libusb_bulk_transfer() [step 2]");

    // step 3 (6 bytes)
    err = libusb_bulk_transfer(devh, NVIDIA_CONTROL_EP, step3_bytes, 6, &bytes_sent, TIMEOUT_MS);
    if ((err != LIBUSB_SUCCESS) || (bytes_sent < 6)) bomb(err, "libusb_bulk_transfer() [step 3]");

    // step 4 (5 bytes)
    err = libusb_bulk_transfer(devh, NVIDIA_CONTROL_EP, step4_bytes, 5, &bytes_sent, TIMEOUT_MS);
    if ((err != LIBUSB_SUCCESS) || (bytes_sent < 5)) bomb(err, "libusb_bulk_transfer() [step 4]");
}

int main(int argc, char *argv[]) {
    int result = 0;
    
    // install our custom signal handler
    signal(SIGINT, goodbye);

    printf("=== IR transmitter tests ===\n");

    // initialize libusb using the default context
    result = libusb_init(NULL);
    if (result != LIBUSB_SUCCESS) bomb(result, "libusb_init()");

    // turn on libusb debug messages
    libusb_set_debug(NULL, 3);

    // discover and open the stereo controller in one go
    libusb_device_handle *devh = libusb_open_device_with_vid_pid(NULL, NVIDIA_VID, NVIDIA_PID);
    if (devh == NULL) bomb(-1, "libusb_open_device_with_vid_pid()");

    // set device config
    result = libusb_set_configuration(devh, NVIDIA_CONFIGURATION);
    if (result != LIBUSB_SUCCESS) bomb(result, "libusb_set_configuration()");

    // claim the interface
    result = libusb_claim_interface(devh, NVIDIA_INTERFACE);
    if (result != LIBUSB_SUCCESS) bomb(result, "libusb_claim_interface()");

    // attempt to initialize the device
    initialize(devh);

    // begin shuttering!
    shutter(devh);

    // release the interface
    result = libusb_release_interface(devh, NVIDIA_INTERFACE);
    if (result != LIBUSB_SUCCESS) bomb(result, "libusb_release_interface()");
    
    // close the device
    libusb_close(devh);

    // shutdown libusb
    libusb_exit(NULL);

    printf("Done.\n");
    return EXIT_SUCCESS;
};
