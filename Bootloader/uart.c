/*
 * uart.c
 *
 * Created: 26.04.2012 15:42:03
 *  Author: neubi
 */


#include "Bootloader.h"
#include "uart.h"


t_serial Uart;





/*!
 *  flush UART buffers (Rx and Tx)
 *
 * @ param  none 
 * @ return none
 */
void UartInitDataBuffer(void)
{
    memset(&Uart.Rx, 0x00, sizeof(Uart.Rx));
    memset(&Uart.Tx, 0x00, sizeof(Uart.Tx));
}


/*!
*  UART receive interrupt -> handle received databytes
*
* @ param  none
* @ return none
*/
ISR(USART_RXC_vect)
{
    BYTE error=0;
    BYTE data=0;

    error = UCSRA;
    data = UDR;

    if(UCSRA & 0x18)                        // check overrun or frameerror ?
        while(UCSRA & 0x80) error = UDR;    // flush databuffer
    else
    {
        switch(data)
        {
            // start-of-text received
            case UART_SOT:
                    if(Uart.Rx.status != UART_BUF_STATUS_VALID)     // avoid overflow of received data
                    {
                        Uart.Rx.len = 0;       // 'flush' old data and re-start from idx=0
                        Uart.Rx.status = UART_BUF_STATUS_FILLING;
                    }
                break;

            // end-of-text received
            case UART_EOT:
                    if(Uart.Rx.status == UART_BUF_STATUS_FILLING)
                    {
                        if(Uart.Rx.len != 0)   // if we get many EOT in a row ...
                        {
                            Uart.Rx.status = UART_BUF_STATUS_VALID;
                            SetEvent(EVENT_UART_RX_MSG);
                        }
                        else
                            Uart.Rx.len = 0;   // 'flush' old data and re-start from idx=0
                    }
                break;

            // save msg-data to buffer
            default:
                    if((Uart.Rx.status == UART_BUF_STATUS_FILLING) && (Uart.Rx.len < sizeof(Uart.Rx.data)))
                        Uart.Rx.data[Uart.Rx.len++] = data;     // safe data
                break;
        }
    }
}


/**
*  send message via UART
*
* @ param  ptr to payload-data
* @ return TRUE on success, FALSE if failed (no empty buffer found)
*/
BYTE UartSendMessage(t_uart_tx_msg* msg, BYTE length)
{
    BYTE len = (sizeof(t_uart_tx_msg) - sizeof(msg->data)) + length;  // sizeof header + msglen


    if(Uart.Tx.status != UART_BUF_STATUS_EMPTY)             // pending msg in buffer
        return FALSE;

    if(len > sizeof(t_serial_tx_buf))                       // payload fits in buffer?
        return FALSE;

    memcpy((BYTE*)&Uart.Tx.data[0], (BYTE*)msg, len);       // copy msg to tx-buffer

    Uart.Tx.len     = len;
    Uart.Tx.status  = UART_BUF_STATUS_VALID;                // mark buffer has filled up and ready to transmit

    SetEvent(EVENT_UART_TX_MSG);                            // set event to trigger transmit
    return TRUE;
}


static inline void _uart_transmit_single_byte(BYTE data)
{
    while ((UCSRA & 0x20) == 0x00);
    UDR = data;                 // write data
}


/**
*  transmit next pending message
*
* @ param  none
* @ return none
*/
void UartTransmitMessageQueue(void)
{
    BYTE i;

    if(Uart.Tx.status == UART_BUF_STATUS_VALID)
    {
        _uart_transmit_single_byte(UART_SOT);               // transmit message
        for(i=0; i<Uart.Tx.len; i++)
            _uart_transmit_single_byte(Uart.Tx.data[i]);
        _uart_transmit_single_byte(UART_EOT);

        memset(&Uart.Tx, 0x00, sizeof(Uart.Tx));            // clear buffer
    }
}


/**
*  get next user-message from UART buffer
*
* @ param  ptr where next msg should be copied
* @ return TRUE if found pending message, FALSE if no new message found
*/
BYTE UartReceiveMessage(t_uart_rx_msg* msg)
{
    if(Uart.Rx.status == UART_BUF_STATUS_VALID)
    {
        // copy content
        memcpy((BYTE*)msg, (BYTE*)&Uart.Rx.data[0], Uart.Rx.len);
        memset(&Uart.Rx, 0x00, sizeof(Uart.Rx));
    
        return TRUE;
    }
    
    return FALSE;
}
