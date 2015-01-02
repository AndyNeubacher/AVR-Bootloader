/*
 * Bootloader.c
 *
 * Created: 13.01.2013 19:41:10
 *  Author: neubi
 */

#include "BootLoader.h"
#include "uart.h"



t_system        System;
t_flash_data    FlashData;




ISR(TIMER1_COMPA_vect)
{
    // update system-clock
    if(System.Time.ms++ > 999)
    {
        System.Time.ms = 0;
        System.Time.sec++;
        SetEvent(EVENT_1SEC_TIMER);
    }

    //SetEvent(EVENT_1MS_TIMER);
}


BYTE ProcessDownload(BYTE* data)
{
    BYTE i;
    BYTE len;
    BYTE record_type;
    BYTE checksum;
    WORD address;
    WORD flash_page;


    // is this really a HEX-line?
    if(data[0] != DOWNLOAD_PARSER_START_SIGN)
        return DOWNLOAD_PARSER_RESULT_INVALID_LINE;

    // calculate parameters of line
    len         = (BYTE)hex2num(&data[1], 2);
    address     = (WORD)hex2num(&data[3], 4);
    record_type = (BYTE)hex2num(&data[7], 2);
    flash_page  = address - (address % SPM_PAGESIZE);
    checksum    = 0x00;

    // calc checksum
    for(i=0; i<(len*2 + DOWNLOAD_ASCII_HEXFILE_HEADERLEN); i++)
        checksum += (BYTE)hex2num(&data[1+i++], 2);

    if(checksum != 0x00)
        return DOWNLOAD_PARSER_RESULT_INVALID_CHKSUM;


    // detect if data or end of line
    switch(record_type)
    {
        case DOWNLOAD_RECORD_TYPE_DATA:
            // new flashpage: store already buffered data to flash
            if((FlashData.len != 0x00) && (FlashData.page != flash_page))
                program_page(FlashData.page, &FlashData.data[0]);

            // copy new data to buffer
            if((FlashData.len + len) <= SPM_PAGESIZE)
            {
                for(i=0; i<len; i++)
                {
                    // store first page ... will be last page programmed -> to check if download was valid
                    if(flash_page == 0x0000)
                        FlashData.first_page[FlashData.len++] = (BYTE)hex2num(&data[9+(i*2)], 2);
                    else
                        FlashData.data[FlashData.len++] = (BYTE)hex2num(&data[9+(i*2)], 2);
                }                
                FlashData.page = flash_page;
            }            

            // flash data if page is full
            if(FlashData.len >= SPM_PAGESIZE)
                program_page(flash_page, &FlashData.data[0]);

            return DOWNLOAD_PARSER_RESULT_LINE_OK;
            break;

        case DOWNLOAD_RECORD_TYPE_END_OF_FILE:
            for(i=0; i<len; i++)
                FlashData.data[FlashData.len++] = (BYTE)hex2num(&data[9+(i*2)], 2);

            // store pending data to flash
            program_page(FlashData.page, &FlashData.data[0]);
            program_page(0x0000, &FlashData.first_page[0]);     // -> validate application

            return DOWNLOAD_PARSER_RESULT_END_OF_DOWNLOAD;
            break;

        default:
            return DOWNLOAD_PARSER_RESULT_INVALID_HEXFILE;
    }

    return DOWNLOAD_PARSER_RESULT_INVALID_LINE;
}


void program_page(WORD page, BYTE *buf)
{
    WORD i;


    GetMutex()
    {
        eeprom_busy_wait();

        boot_page_erase(page);
        boot_spm_busy_wait();       /* Wait until the memory is erased. */

        for (i=0; i<SPM_PAGESIZE; i+=2)
        {
            /* Set up little-endian word. */
            WORD w = *buf++;
            w += (*buf++) << 8;

            boot_page_fill(page + i, w);
        }

        boot_page_write(page);      /* Store buffer in flash page.      */
        boot_spm_busy_wait();       /* Wait until the memory is written.*/

        /* Reenable RWW-section again. We need this if we want to jump back */
        /* to the application after bootloading. */
        boot_rww_enable();
    }
    ReleaseMutex()

    FlashData.len    = 0;
    FlashData.status = FLASH_MODIFIED;
    memset(&FlashData.data, 0xFF, sizeof(FlashData.data));
}


void FlashErase(void)
{
    WORD i;

    GetMutex()
    {
        for(i = 0 ; i < (FLASHEND - (BOOTSIZE * 2)); i += SPM_PAGESIZE)
        {
            boot_page_erase(i);         // Erase the page
            boot_spm_busy_wait();       // Wait until finished.
        }
    }
    ReleaseMutex()
}


WORD hex2num(BYTE* ascii, BYTE num)
{
    BYTE  i;
    WORD val = 0;

    for (i=0; i<num; i++)
    {
        BYTE c = ascii[i];

        if (c >= '0' && c <= '9')       c -= '0';
        else if (c >= 'A' && c <= 'F')  c -= 'A' - 10;
        else if (c >= 'a' && c <= 'f')  c -= 'a' - 10;

        val = 16 * val + c;
    }

    return val;
}


BYTE isApplicationValid(void)
{
    if(pgm_read_byte(0x000E) != 0xFF)       // @0x000E = intvector of timer1 -> must not be 0xFF!
        return TRUE;
    
    return FALSE;
}


void SystemRestart(BYTE target)
{
    void (*start)(void) = (void*)0x7000;            // default to Bootloader

    if(target == RESTART_TO_APPLICATION)
    {
        //TODO validate if Application is OK!
        start = (void*)0x0000;      // reboot to Application

        asm("LDI R24,  0x01");      // set IVCE -> int vectors to application
        asm("OUT 0x3B, R24");
        asm("LDI R24,  0x00");      // clear IVSEL
        asm("OUT 0x3B, R24");
    }

    start();
}


void Timer1Init(void)
{
    OCR1A   =  (F_CPU / 1000) - 1;                  // compare value: 1/1000 of CPU frequency
    TCCR1B  = (1 << WGM12) | (1 << CS10);           // switch CTC Mode on, set prescaler to 1
    TIMSK   = 1 << OCIE1A;                          // OCIE1A: Interrupt by timer compare
}


void SystemInit(void)
{
    asm("LDI R24,  0x01");          // set IVCE -> int vectors to bootloader
    asm("OUT 0x3B, R24");
    asm("LDI R24,  0x02");          // set IVSEL
    asm("OUT 0x3B, R24");

    System.Time.ms      = 0;
    System.Time.sec     = 0;

    System.Event.Flags  = 0;
    System.Event.Msg    = 0;

    FlashData.len       = 0;
    FlashData.status    = FLASH_NOT_MODIFIED;
    memset(&FlashData.data, 0xFF, sizeof(FlashData.data));
    memset(&FlashData.first_page, 0xFF, sizeof(FlashData.first_page));
}


void UartInit(void)
{
    UCSRB = 0x00;                                   // disable uart
    UBRRL = 12;                                     // 115200 baud
    UBRRH = 0x00;
    UCSRA |= (1<<U2X);                              // enable fast mode to achieve the baudrates
    UCSRB = ((1<<RXCIE) | (1<<RXEN)  | (1<<TXEN));
    UCSRC = ((1<<URSEL) | (1<<UCSZ0) | (1<<UCSZ1));

    UartInitDataBuffer();
}


int main(void)
{
    BYTE result;
    BYTE stay_in_bootloader = FALSE;
    t_uart_rx_msg rx_msg;
    t_uart_tx_msg tx_msg;

    
    SystemInit();
    UartInit();
    Timer1Init();
    sei();
    
    // send startup-string
    tx_msg.type = MSG_TYPE_BOOTLOADER;
    tx_msg.subtype = MSG_TYPE_VERSION;
    memcpy(&tx_msg.data[0], BOOTLOADER_VERSION, sizeof(BOOTLOADER_VERSION)-1);
    UartSendMessage(&tx_msg, sizeof(BOOTLOADER_VERSION)-1);

    while(1)
    {
        if(IsEventPending(EVENT_1MS_TIMER))
        {
            ClearEvent(EVENT_1MS_TIMER);
        }
            
            
        if(IsEventPending(EVENT_1SEC_TIMER))
        {
            ClearEvent(EVENT_1SEC_TIMER);
                
            if((System.Time.sec > 2) && (stay_in_bootloader == FALSE))
            {
                stay_in_bootloader = TRUE;
                if(isApplicationValid() == TRUE)
                    SystemRestart(RESTART_TO_APPLICATION);
            }
        }


        if(IsEventPending(EVENT_UART_RX_MSG))
        {
            ClearEvent(EVENT_UART_RX_MSG);
            stay_in_bootloader = TRUE;                      // stay in bootloader if one msg received

            if(UartReceiveMessage(&rx_msg) == TRUE)         // loop till msg queue is empty
            {
                if(rx_msg.type == MSG_TYPE_VERSION)
                {
                    tx_msg.type = MSG_TYPE_BOOTLOADER;      // default answer back
                    tx_msg.subtype = MSG_TYPE_VERSION;
                    memcpy(&tx_msg.data[0], BOOTLOADER_VERSION, sizeof(BOOTLOADER_VERSION)-1);
                    UartSendMessage(&tx_msg, sizeof(BOOTLOADER_VERSION)-1);
                }                

                if(rx_msg.type != MSG_TYPE_BOOTLOADER)      // only accept bootloader-commands
                    continue;

                tx_msg.type = MSG_TYPE_BOOTLOADER;          // default answer back
                tx_msg.subtype = rx_msg.subtype;
                    
                switch(rx_msg.subtype)
                {
                    case MSG_SUBTYPE_PROGRAM:
                        result = ProcessDownload(&rx_msg.data[0]);
                           
                        // invalidate flashcontent on download-error
                        if((result > DOWNLOAD_PARSER_RESULT_END_OF_DOWNLOAD) && (FlashData.status == FLASH_MODIFIED))
                            FlashErase();
                            
                        tx_msg.data[0] = '0' + result;
                        UartSendMessage(&tx_msg, 1);
                    break;
                        
                    case MSG_SUBTYPE_ERASE_CHIP:
                        FlashErase();
                        UartSendMessage(&tx_msg, 0);
                    break;

                    case MSG_SUBTYPE_RESTART:
                        if(isApplicationValid() == TRUE)
                            SystemRestart(RESTART_TO_APPLICATION);
                        else
                            SystemRestart(RESTART_TO_BOOTLOADER);
                    break;

                    default:
                        tx_msg.subtype = MSG_SUBTYPE_INVALID;
                        UartSendMessage(&tx_msg, 0);
                    break;
                }
            }
        } // EVENT_UART_RX_MSG

            
        if(IsEventPending(EVENT_UART_TX_MSG))
        {
            ClearEvent(EVENT_UART_TX_MSG);
            UartTransmitMessageQueue();
        }
    }                    

    return 1;
}
