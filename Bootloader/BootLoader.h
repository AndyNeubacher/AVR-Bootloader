/*
 * Bootloader.h
 *
 * Created: 13.01.2013 19:41:27
 *  Author: neubi
 */


#ifndef BOOTLOADER_H_
#define BOOTLOADER_H_

#define F_CPU                           12000000UL

#define BOOTLOADER_VERSION              "Bootloader V0.2"



#include <avr/io.h>
#include <util/atomic.h>
#include <avr/boot.h>
#include <avr/pgmspace.h>
#include <string.h>


typedef signed char                     SBYTE;
typedef unsigned char                   BYTE;
typedef signed int                      SWORD;
typedef unsigned int                    WORD;
typedef unsigned long                   LONG;


#define TRUE                            1
#define FALSE                           0

#define BOOTSIZE                        4096


#define EVENT_IDLE                      0x00
#define EVENT_1MS_TIMER                 0x01
#define EVENT_1SEC_TIMER                0x02
#define EVENT_UART_RX_MSG               0x04
#define EVENT_UART_TX_MSG               0x08
#define EVENT_UNUSED_1                  0x10
#define EVENT_UNUSED_2                  0x20
#define EVENT_UNUSED_3                  0x40
#define EVENT_UNUSED_4                  0x80

#define GetMutex()                      ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
#define ReleaseMutex()                  }
#define MutexFunc(func)                 ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {func}

#define SetEvent(event)                 MutexFunc(System.Event.Flags |= (event);)
#define SetEventMsg(event, msg)         MutexFunc(System.Event.Flags |= (event); System.Event.Msg = (msg);)
#define ClearEvent(event)               MutexFunc(System.Event.Flags &= ~(event);)
#define IsEventPending(event)           (System.Event.Flags & (event))



typedef struct
{
    WORD ms;
    WORD sec;
} t_time;

typedef struct
{
    volatile BYTE Flags;
    BYTE Msg;
} t_event;

typedef struct
{
    t_time  Time;
    t_event Event;
} t_system;


extern t_system System;


#define RESTART_TO_APPLICATION                      0
#define RESTART_TO_BOOTLOADER                       1

#define MSG_TYPE_BOOTLOADER                         'B'
#define MSG_TYPE_VERSION                            'v'

#define MSG_SUBTYPE_RESTART                         'q'
#define MSG_SUBTYPE_INVALID                         'E'
#define MSG_SUBTYPE_PROGRAM                         'p'
#define MSG_SUBTYPE_ERASE_CHIP                      'c'


#define DOWNLOAD_PARSER_START_SIGN                  ':'
#define DOWNLOAD_RECORD_TYPE_DATA                   0
#define DOWNLOAD_RECORD_TYPE_END_OF_FILE            1

#define DOWNLOAD_PARSER_RESULT_LINE_OK              0x00
#define DOWNLOAD_PARSER_RESULT_PROGRAM_PAGE         0x01
#define DOWNLOAD_PARSER_RESULT_END_OF_DOWNLOAD      0x02
#define DOWNLOAD_PARSER_RESULT_INVALID_CHKSUM       0x03
#define DOWNLOAD_PARSER_RESULT_INVALID_LINE         0x04
#define DOWNLOAD_PARSER_RESULT_INVALID_HEXFILE      0x05

#define FLASH_NOT_MODIFIED                          0x00
#define FLASH_MODIFIED                              0x01


#define DOWNLOAD_ASCII_HEXFILE_HEADERLEN            10      // bytecound + address + recordtype + checksum


typedef struct
{
    BYTE len;
    WORD page;
    BYTE status;
    BYTE data[SPM_PAGESIZE];
    BYTE first_page[SPM_PAGESIZE];
} t_flash_data;





BYTE ProcessDownload(BYTE* data);

void program_page(WORD page, BYTE *buf);

void FlashErase(void);

WORD hex2num (BYTE* ascii, BYTE num);

BYTE isApplicationValid(void);

void SystemRestart(BYTE target);

void Timer1Init(void);

void SystemInit(void);

void UartInit(void);




#endif /* BOOTLOADER_H_ */