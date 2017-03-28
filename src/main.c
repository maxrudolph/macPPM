//
//  main.c
//  BuddyBox
//
//  Created by Nicholas Robinson on 12/23/12.
//  Copyright (c) 2012 Nicholas Robinson. All rights reserved.
//

/*
 Modifications to integrate with foohid (c) 2016 by albhm
 */

#include "BuddyBoxThread.h"

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <string.h>
#include <IOKit/IOKitLib.h>

static const unsigned int DEFAULT_SAMPLE_RATE = 124000;

static unsigned int running = 1;

  
// alternative joystick report descriptor:
unsigned char joy_reportdesc[] = {
0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
0x09, 0x04,        // Usage (Joystick)
0xA1, 0x01,        // Collection (Application)
0x15, 0x81,        //   Logical Minimum (-127)
0x25, 0x7F,        //   Logical Maximum (127)
0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
0x09, 0x01,        //   Usage (Pointer)
0xA1, 0x00,        //   Collection (Physical)
0x09, 0x30,        //     Usage (X)
0x09, 0x31,        //     Usage (Y)
0x09, 0x32,        //     Usage (Z)
0x09, 0x33,        //     Usage (Rx)
0x09, 0x34,        //     Usage (Ry)
0x75, 0x08,        //     Report Size (8)
0x95, 0x05,        //     Report Count (4) -> (5)
0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0xC0,              //   End Collection
0x05, 0x09,        //   Usage Page (Button)
0x19, 0x01,        //   Usage Minimum (0x01)
0x29, 0x08,        //   Usage Maximum (0x08)
0x15, 0x00,        //   Logical Minimum (0)
0x25, 0x01,        //   Logical Maximum (1)
0x75, 0x01,        //   Report Size (1)
0x95, 0x08,        //   Report Count (8)
0x55, 0x00,        //   Unit Exponent (0)
0x65, 0x00,        //   Unit (None)
0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0xC0              // End Collection

// 54 bytes
};
   /*
unsigned char joy_reportdesc[] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x04,                    // USAGE (Joystick)
    0xa1, 0x01,                    // COLLECTION (Application)
    0xa1, 0x00,                    //   COLLECTION (Physical)
 
    0x05, 0x01,                    //   USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    //     USAGE (X)
    0x09, 0x31,                    //     USAGE (Y)
    0x09, 0x32,                    //     USAGE
    0x09, 0x33,                    //     USAGE
    0x15, 0x81,                    //   LOGICAL_MINIMUM (-127)
    0x25, 0x7f,                    //   LOGICAL_MAXIMUM (127)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x04,                    //     REPORT_COUNT (4)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
   
    0x05, 0x09,                    //   USAGE_PAGE (Button)
    0x19, 0x01,                    //   USAGE_MINIMUM (Button 1)
    0x29, 0x08,                    //   USAGE_MAXIMUM (Button 8)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x08,                    //   REPORT_COUNT (8)
    0x55, 0x00,                    //   UNIT_EXPONENT (0)
    0x65, 0x00,                    //   UNIT (None)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0xc0,                          //   END_COLLECTION
    0xc0                           // END_COLLECTION
};
   */

struct joystick_report_t
{
    int8_t x;
    int8_t y;
    int8_t z;
    int8_t rx;
    int8_t ry;

    uint8_t buttons;
};

#define SERVICE_NAME "it_unbit_foohid"

#define FOOHID_CREATE 0  // create selector
#define FOOHID_DESTROY 1
#define FOOHID_SEND 2  // send selector

#define DEVICE_NAME "Foohid Virtual Joystick"
#define DEVICE_SN "SN 123456"


void intHandler(int sig) {
    running = 0;
}

void generateOutput(PASBuddyBox *pasBB)
{
    unsigned int i, outputChannelCount;
    
    outputChannelCount = getBuddyBoxThreadInputChannelCount(pasBB);
    setBuddyBoxThreadOutputChannelCount(pasBB, outputChannelCount);
    for (i = 0; i < outputChannelCount; i++)
        setBuddyBoxThreadOutputChannelValue(pasBB, i, getBuddyBoxThreadInputChannelValue(pasBB, i));
}

void displayInput(PASBuddyBox *pasBB)
{
    unsigned int i, inputChannelCount;
    
    inputChannelCount = getBuddyBoxThreadInputChannelCount(pasBB);
    for (i = 0; i < inputChannelCount; i++)
        printf("%f\t", getBuddyBoxThreadInputChannelValue(pasBB, i));
    printf("\n");
}

void initVirtualJoystick () {
    
}

int main(int argc, const char * argv[])
{   int testmode = 0;
    
    signal(SIGKILL, intHandler);
    signal(SIGINT, intHandler);
    
    PASBuddyBox pasBB;
    pasBB.sampleRate = DEFAULT_SAMPLE_RATE;
 	
    
    int opt=0;
    while ((opt = getopt(argc, (char * const *) argv, "tc")) != -1) {
        switch (opt) {
        case 't': testmode = 1; break;
        case 'c': pasBB.deviceChannel = 0; break;
        default:
            fprintf(stderr, "Usage: %s [-tc] [sample rate]\n", argv[0]);
            exit(1);
        }
    }
    
    if (optind < argc)
    	pasBB.sampleRate = (unsigned int) strtol(argv[optind], NULL, 0);
    
    printf ("Usage: macPPM [-t(est) -c(hannel)] [sample rate] \n current sample rate: %d   \n\n", pasBB.sampleRate);
    initializeBuddyBoxThread(&pasBB, testmode);
    printf ("\n"); // to set output apart from the deprecation warning of libportaudio
    
    //////// init joystick //////
    io_iterator_t iterator;
    io_service_t service;
    io_connect_t connect;
    
    // Get a reference to the IOService
    kern_return_t ret = IOServiceGetMatchingServices(kIOMasterPortDefault, IOServiceMatching(SERVICE_NAME), &iterator);
    
    if (ret != KERN_SUCCESS) {
        printf("Unable to access IOService.\n");
        exit(1);
    }
    
    // Iterate till success
    int found = 0;
    while ((service = IOIteratorNext(iterator)) != IO_OBJECT_NULL) {
        ret = IOServiceOpen(service, mach_task_self(), 0, &connect);
        
        if (ret == KERN_SUCCESS) {
            found = 1;
            break;
        }
        
        IOObjectRelease(service);
    }
    IOObjectRelease(iterator);
    
    if (!found) {
        printf("Unable to open IOService.\n");
        exit(1);
    }
    
    
    //printf("size rep desc: %d .\n", sizeof(joy_reportdesc));
    // Fill up the input arguments.
    uint32_t input_count = 8;
    uint64_t input[input_count];
    input[0] = (uint64_t) strdup(DEVICE_NAME);  // device name
    input[1] = strlen((char *)input[0]);  // name length
    input[2] = (uint64_t) joy_reportdesc;  // report descriptor
    input[3] = sizeof(joy_reportdesc);  // report descriptor len
    input[4] = (uint64_t) strdup(DEVICE_SN);  // serial number
    input[5] = strlen((char *)input[4]);  // serial number len
    input[6] = (uint64_t) 2;  // vendor ID
    input[7] = (uint64_t) 3;  // device ID
    
    ret = IOConnectCallScalarMethod(connect, FOOHID_CREATE, input, input_count, NULL, 0);
    if (ret != KERN_SUCCESS) {
        printf("Unable to create HID device - hope that it exists already. \n");
       // exit(1);
    }
    
    // Arguments to be passed through the HID message.
    struct joystick_report_t joy;
    uint32_t send_count = 4;
    uint64_t send[send_count];
    send[0] = (uint64_t)input[0];  // device name
    send[1] = strlen((char *)input[0]);  // name length
    send[2] = (uint64_t) &joy;  // mouse struct
    send[3] = sizeof(struct joystick_report_t);  // mouse struct len
    
    /////////////end init joystick////////////
    
    while(running)
    {   
        startBuddyBoxThread(&pasBB);
        
        while(running && isBuddyBoxThreadRunning(&pasBB))
        {
            generateOutput(&pasBB);
            
            if (1 || isBuddyBoxThreadCalibrated(&pasBB)) {
                if (testmode) {
                  displayInput(&pasBB);
                
                  unsigned int i, inputChannelCount;
		  
                  inputChannelCount = 8;//getBuddyBoxThreadInputChannelCount(&pasBB);
                  for (i = 0; i < inputChannelCount; i++)
                      printf("%f\t", getBuddyBoxThreadInputChannelValue(&pasBB, i));
                  printf("\n");
                }
                // This is for a NineEagles J6 Pro Transmitter (6 channels); may need to adjust for other models
                //joy.buttons = (getBuddyBoxThreadInputChannelValue(&pasBB, 4)>0.5) ? 1:0;
                joy.x = (getBuddyBoxThreadInputChannelValue(&pasBB, 0)-0.5) * 255;
                joy.y = (getBuddyBoxThreadInputChannelValue(&pasBB, 1)-0.5) * 255;
                joy.rx = (getBuddyBoxThreadInputChannelValue(&pasBB, 3)-0.5) * 255;
                joy.ry = (getBuddyBoxThreadInputChannelValue(&pasBB, 5)-0.5) * 255;
      		joy.z =  (getBuddyBoxThreadInputChannelValue(&pasBB, 2)-0.5) * 255;

                ret = IOConnectCallScalarMethod(connect, FOOHID_SEND, send, send_count, NULL, 0);
                if (ret != KERN_SUCCESS) {
                    printf("Unable to send message to HID device.\n");
                }
            }
                
            
            usleep(100);
        }
        
        stopBuddyBoxThread(&pasBB);
        
        joinBuddyBoxThread(&pasBB);
    }
    
    cleanupBuddyBoxThread(&pasBB);
    
    ret = IOConnectCallScalarMethod(connect, FOOHID_DESTROY, input,2,NULL,0);
    if (ret != KERN_SUCCESS) {
        printf("Unable to destroy HID device. \n");
    }
    
    printf("Program Halted...\n");
}

