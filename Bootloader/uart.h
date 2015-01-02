/*
 * uart.h
 *
 * Created: 26.04.2012 15:42:17
 *  Author: neubi
 */


#ifndef UART_H_
#define UART_H_



#define UART_BUF_STATUS_EMPTY       0x00
#define UART_BUF_STATUS_FILLING     0x01
#define UART_BUF_STATUS_VALID       0x02

#define UART_TX_EN                  0x08
#define UART_RX_EN                  0x10
#define UART_RX_INT_EN              0x80
#define UART_URSEL                  0x80
#define UART_8BIT                   0x06

#define UartEnableInterrupt()       (UCSRB |= UART_RX_INT_EN)
#define UartDisableInterrupt()      (UCSRB &= ~UART_RX_INT_EN)



/* defines for protocol */
#define UART_SOT                    0x02
#define UART_EOT                    0x03

/* defines for send-receive buffer sizes */
#define UART_TX_MSG_BUFFER_LEN      32
#define UART_RX_MSG_BUFFER_LEN      64




// struct for 'user-rx-message'
typedef struct
{
    BYTE type;
    BYTE subtype;
    BYTE data[UART_RX_MSG_BUFFER_LEN];
} t_uart_rx_msg;


// struct for 'user-tx-message'
typedef struct
{
    BYTE type;
    BYTE subtype;
    BYTE data[UART_TX_MSG_BUFFER_LEN];
} t_uart_tx_msg;



/* strctures for RECEIVE */
typedef struct
{
   BYTE                 data[sizeof(t_uart_rx_msg)];
   BYTE                 status;
   BYTE                 len;
} t_serial_rx_buf;


/* structures for TRANSMIT */
typedef struct
{
    BYTE                data[sizeof(t_uart_tx_msg)];
    BYTE                status;
    BYTE                len;
} t_serial_tx_buf;



/* main UART struct */
typedef struct
{
    t_serial_rx_buf     Rx;
    t_serial_tx_buf     Tx;
} t_serial;

extern t_serial Uart;




void UartInitDataBuffer(void);

BYTE UartSendMessage(t_uart_tx_msg* msg, BYTE length);

void UartTransmitMessageQueue(void);

BYTE UartReceiveMessage(t_uart_rx_msg* msg);


#endif /* UART_H_ */
