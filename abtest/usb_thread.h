#ifndef __USBTHREAD_H__
#define __USBTHREAD_H__

#include <stdint.h>
#include <libusb-1.0/libusb.h>
#include "ting/Thread.hpp"

class UsbThread : public ting::Thread {
public:
	explicit UsbThread();
	~UsbThread();

    // entry point
    void Run();
    
private:
    libusb_device_handle *_dev;
    
    void Init();
    void Cleanup();
    
    static const uint16_t VENDOR_ID;
    static const uint16_t PRODUCT_ID;
    static const uint8_t INTERFACE;
    static const uint8_t CONFIGURATION;
    static const uint8_t CONTROL_ENDPOINT;
    static const uint8_t SYNC_ENDPOINT;
    static const uint32_t TIMEOUT_MS;
    static const uint64_t SHUTTER_TIME_NS;
    static const uint32_t SHUTTERS_PER_SYNC;
    static const uint32_t FREQ_RECOVERY_PACKETS;
};



#endif // __USBTHREAD_H__
