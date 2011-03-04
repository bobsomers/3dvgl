#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
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

void bomb(int err, const char *msg) {
    fprintf(stderr, "ERROR %d: %s\n", err, msg);
    exit(EXIT_FAILURE);
}


void initialize(libusb_device_handle *devh) {
    /*unsigned char bytes[][28] = {
        {0x42, 0x18, 0x03, 0x00}, // step 0 (4 bytes)
        {0x01, 0x00, 0x18, 0x00, 0x91, 0xED, 0xFE, 0xFF, 0x33, 0xD3, 0xFF, 0xFF,
            0xC6, 0xD7, 0xFF, 0xFF, 0x30, 0x28, 0x24, 0x22, 0x0A, 0x08, 0x05,
            0x04, 0x52, 0x79, 0xFE, 0xFF}, // step 1 (28 bytes)
        {0x01, 0x1C, 0x02, 0x00, 0x02, 0x00}, // step 2 (6 bytes)
        {0x01, 0x1E, 0x02, 0x00, 0xF0, 0x00}, // step 3 (6 bytes)
        {0x01, 0x1B, 0x01, 0x00, 0x07} // step 4 (5 bytes)
    };*/

    unsigned char step0_bytes[] = {0x42, 0x18, 0x03, 0x00};

    int bytes_sent = 0;
    int err = 0;

    // step 0 (4 bytes)
    err = libusb_bulk_transfer(devh, NVIDIA_CONTROL_EP, step0_bytes, 4, &bytes_sent, TIMEOUT_MS);
    if ((err != LIBUSB_SUCCESS) || (bytes_sent < 4)) {
        perror("libusb_bulk_transfer");
        bomb(err, "libusb_bulk_transfer() [step 0]");
    }

    /*

    // step 1 (28 bytes)
    if (libusb_bulk_transfer(devh, NVIDIA_USB_CONTROL_ENDPOINT, bytes[1], 28, &bytes_sent, 1000) != 0) {
        fprintf(stderr, "Error on initialization step 1.\n");
        return;
    }
    if (bytes_sent != 28) fprintf(stderr, "Bytes sent mismatch on intialization step 1.\n");

    // step 2 (6 bytes)
    if (libusb_bulk_transfer(devh, NVIDIA_USB_CONTROL_ENDPOINT, bytes[2], 6, &bytes_sent, 1000) != 0) {
        fprintf(stderr, "Error on initialization step 2.\n");
        return;
    }
    if (bytes_sent != 6) fprintf(stderr, "Bytes sent mismatch on intialization step 2.\n");

    // step 3 (6 bytes)
    if (libusb_bulk_transfer(devh, NVIDIA_USB_CONTROL_ENDPOINT, bytes[3], 6, &bytes_sent, 1000) != 0) {
        fprintf(stderr, "Error on initialization step 3.\n");
        return;
    }
    if (bytes_sent != 6) fprintf(stderr, "Bytes sent mismatch on intialization step 3.\n");

    // step 4 (5 bytes)
    if (libusb_bulk_transfer(devh, NVIDIA_USB_CONTROL_ENDPOINT, bytes[4], 5, &bytes_sent, 1000) != 0) {
        fprintf(stderr, "Error on initialization step 4.\n");
        return;
    }
    if (bytes_sent != 5) fprintf(stderr, "Bytes sent mismatch on intialization step 4.\n");

    */
}

int main(int argc, char *argv[]) {
    int result = 0;
    
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
