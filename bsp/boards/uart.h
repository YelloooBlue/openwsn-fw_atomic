// #ifndef __UART_H
// #define __UART_H

// /**
// \addtogroup BSP
// \{
// \addtogroup uart
// \{

// \brief Cross-platform declaration "uart" bsp module.

// \author Thomas Watteyne <watteyne@eecs.berkeley.edu>, February 2012.
// */

// #include "stdint.h"
// #include "board.h"
 
// //=========================== define ==========================================

// #define XOFF            0x13
// #define XON             0x11
// #define XONXOFF_ESCAPE  0x12
// #define XONXOFF_MASK    0x10
// // XOFF            is transmitted as [XONXOFF_ESCAPE,           XOFF^XONXOFF_MASK]==[0x12,0x13^0x10]==[0x12,0x03]
// // XON             is transmitted as [XONXOFF_ESCAPE,            XON^XONXOFF_MASK]==[0x12,0x11^0x10]==[0x12,0x01]
// // XONXOFF_ESCAPE  is transmitted as [XONXOFF_ESCAPE, XONXOFF_ESCAPE^XONXOFF_MASK]==[0x12,0x12^0x10]==[0x12,0x02]

// //=========================== typedef =========================================

// typedef enum {
//    UART_EVENT_THRES,
//    UART_EVENT_OVERFLOW,
// } uart_event_t;

// typedef void    (*uart_tx_cbt)(void);
// typedef uint8_t (*uart_rx_cbt)(void);

// //=========================== variables =======================================

// //=========================== prototypes ======================================

// void    uart_init(void);
// void    uart_setCallbacks(uart_tx_cbt txCb, uart_rx_cbt rxCb);
// void    uart_enableInterrupts(void);
// void    uart_disableInterrupts(void);
// void    uart_clearRxInterrupts(void);
// void    uart_clearTxInterrupts(void);
// void    uart_setCTS(bool state);
// void    uart_writeByte(uint8_t byteToWrite);
// #if BOARD_FASTSIM_ENABLED
// void    uart_writeCircularBuffer_FASTSIM(uint8_t* buffer, uint16_t* outputBufIdxR, uint16_t* outputBufIdxW);
// #endif
// uint8_t uart_readByte(void);

// // interrupt handlers
// kick_scheduler_t uart_tx_isr(void);
// kick_scheduler_t uart_rx_isr(void);

// /**
// \}
// \}
// */

// #endif
