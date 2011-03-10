#include "usb_thread.h"

uint16_t UsbThread::VENDOR_ID = 0x0955;
uint16_t UsbThread::PRODUCT_ID = 0x0007;
uint8_t UsbThread::INTERFACE = 0x0;
uint8_t UsbThread::CONFIGURATION = 0x1;
uint8_t UsbThread::CONTROL_ENDPOINT = 0x2;
uint8_t UsbThread::SYNC_ENDPOINT = 0x1;
uint32_t UsbThread::TIMEOUT_MS = 5000;
uint64_t UsbThread::SHUTTER_TIME_NS = 8333333;
uint32_t UsbThread::SHUTTERS_PER_SYNC = 14;
uint32_t UsbThread::FREQ_RECOVERY_PACKETS = 32;

UsbThread::UsbThread() :
        _dev(NULL) {
    // nothing
}

UsbThread::~UsbThread() {
    if (_dev != NULL) {
        Cleanup();
    }
}

void UsbThread::Run() {
    
}
