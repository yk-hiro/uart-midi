/**
 * UART MIDI I/F test programe  (10.Jul.2024) 
 * Copyrught (c) 2024 id:yk-hiro(on Hatena blog) 
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define MIDI_UART_ID uart1
#define MIDI_BAUD_RATE 31250
#define MIDI_UART_TX_PIN 4
#define MIDI_UART_RX_PIN 5

void UART_Init(void);
void _debug_midi_tx( void );
