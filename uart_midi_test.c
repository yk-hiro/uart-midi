/**
 * UART MIDI I/F test programe  (10.Jul.2024) 
 * Copyrught (c) 2024 id:yk-hiro(on Hatena) 
 * 
 * portion of this program by Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "pico/util/datetime.h"

#include "uart_midi.h"

#define mPrint_time  printf( "%08d: " , time_us_32() / 100 )


int main(void) {

    stdio_init_all();
    // In a default system, printf will also output via the default UART
    sleep_ms( 2000 );   // Picoリセット後にターミナルコンソールのCOM再接続時間を待つため（ざっくり２秒を設定）

    printf( "\x1b[2J" );  printf( "\x1b[1;1H" );  printf( "\x1b[?25l" );   // 画面クリア,カーソルを1行目1桁目に移動,カーソルoff

    UART_Init();

    printf( "\nStart UART MIDI Test\n" );
    mPrint_time;

    while (true) {      // 無限ループ ： 時間表示しながら受信データを待つ

        wchar_t  midi_message_byte;     // 受信したMIDIデータを記録する変数

        _debug_midi_tx();   // デバッグ用にダミーのMIDI送信を行う

        if ( uart_is_readable_within_us( MIDI_UART_ID , 0 ) ) {
            // 受信データがあるとき
            do {

                midi_message_byte = uart_getc(MIDI_UART_ID);    // 1byte 受信FIFOから読み出し
                //This function will block until the character has been read

                // uart_putc(MIDI_UART_ID ,midi_message_byte);  // ローカルループバックテスト時はコメントにしておく     
                // MIDIの送信FIFOに書き込み
                // 送信FIFOの空きチェックは割愛、受信と送信は同じbit rateなので...
    
                if ( (midi_message_byte & 0x80) == 0x80 ) {     // bit8 = 1 -> MIDI Status Byteの場合
                    printf( "\n" );
                    mPrint_time;
                }
            
                printf( "%02X " , midi_message_byte );  // CONSOLE(stdout)にMIDIデータを出力

            } while ( uart_is_readable_within_us( MIDI_UART_ID , 0 ) );     // 受信データがなくなるまでループする

            printf( "\n" );
        }  
        
        printf( "\r" );
        mPrint_time;  // １ループごとに時間表示
            
    }

    uart_deinit(MIDI_UART_ID);      // 一回も通らないが忘れないように入れておく

}


void UART_Init(void) {
    // Raspberry Pi Pico C/C++ SDKマニュアル P.180 の 4.1.23. hardware_uart 参照
    uint baudrate;
    uint32_t *pad_cntl_reg_adrs = 0;

    // Set up our UART with the required speed.
    // Set the TX and RX pins by using the function select on the GPIO

    baudrate = uart_init(MIDI_UART_ID, MIDI_BAUD_RATE);
    uart_set_format( MIDI_UART_ID , 8 , 1 , UART_PARITY_NONE );

    gpio_set_function(MIDI_UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(MIDI_UART_RX_PIN, GPIO_FUNC_UART);
    // 受信入力ピンのプルアップ・ダウンは off にする（外部回路で対応している）
    gpio_set_pulls( MIDI_UART_RX_PIN , false , false );

    printf( "MIDI baudrate : %d\n",baudrate);

    // 出力バッファの電流制限値を変更する
    printf( "MIDI TX Pin(%02d) Pad Control register address %08x\n" , 
        MIDI_UART_TX_PIN , 
        pad_cntl_reg_adrs = (uint32_t *) (PADS_BANK0_BASE + 0x04 + MIDI_UART_TX_PIN * 0x04 ) );
    printf( " Current Pad control register value : %08x \n" , *pad_cntl_reg_adrs );
    *pad_cntl_reg_adrs = 0x00000064;
    printf( " New pad control register value     : %08x \n" , *pad_cntl_reg_adrs );

    //RP2040 Datasheet  2.19.6.3. Pad Control - User Bank  (p.299)
    //
    //The User Bank Pad Control registers start at a base address of 0x4001c000 (defined as PADS_BANK0_BASE in SDK).
    // PADS_BANK0: GPIO0, GPIO1, …, GPIO28, GPIO29 Registers
    // Offsets:    0x04,  0x08, …,  0x74,   0x78
    //
    //[Pad control register]
    // |Bits| Name     | Description                                                            |Type|Reset|
    // |----|----------|------------------------------------------------------------------------|----|-----| 
    // |31:8|(Reserved)|    -                                                                   | -  |  -  |
    // |7   | OD       |Output disable.                                                         | RW | 0x0 |
    // |    |          | Has priority over output enable from peripherals                       |    |     |
    // |6   | IE       |Input enable                                                            | RW | 0x1 |
    // |5:4 | DRIVE    |Drive strength. 0x0(b00):2mA  0x1(b01):4mA  0x2(b10):8mA  0x3(b11):12mA | RW | 0x1 |
    // |3   | PUE      |Pull up enable                                                          | RW | 0x0 |
    // |2   | PDE      |Pull down enable                                                        | RW | 0x1 |
    // |1   | SCHMITT  |Enable schmitt trigger                                                  | RW | 0x1 |
    // |0   | SLEWFAST |Slew rate control. 1 = Fast, 0 = Slow                                   | RW | 0x0 |
    //
    // default 0x00 00 00 56
    // 0x56 = 0101 0110
    //        |||| ||||-Slew rate control [slow]
    //        |||| |||-Enable schmitt trigger [enable]
    //        |||| ||-Pull down enable [pull-down on]
    //        |||| |-Pull up enable [pull-up off]
    //        ||||-Drive strength [01:4mA]
    //        ||-Input enable [on]
    //        |-Output disable [off]
    //
    // 変更：1) Drive strength [01:4mA] -> [10:8mA] に変更する
    //       2) pull-down->off
    // 0x64 = 0110 0100
    //
}


void _debug_midi_tx( void ) {   // debugのために MIDIの送信FIFOにダミーデータを書き込み

    static char midi_note_prev = 255;    // static宣言で 送信したノート番号を保存できる様にする

    char midi_note = 255;
    char in_key = ' ';

    if ( PICO_ERROR_TIMEOUT == ( in_key = getchar_timeout_us( 0 ) ) ) {
        // キーイン タイムアウト（= キーインなし）
        return;
    }
    
    // キー入力あり → キー判定 ノート番号のセット
    switch ( in_key  ) {
        case 'z' :  // B
            midi_note = 47;      // note number
            break;
        case 'x' :  // C
            midi_note = 48;
            break;
        case 'd' :  // C#
            midi_note = 49;
            break;
        case 'c' :  // D  
            midi_note = 50;
            break;
        case 'f' :  // D#
            midi_note = 51;
            break;
        case 'v' :  // E
            midi_note = 52;
            break;
        case 'b' :  // F
            midi_note = 53;
            break;
        case 'h' :  // F#
            midi_note = 54;
            break;
        case 'n' :  // G
            midi_note = 55;
            break;
        case 'j' :  // G#
            midi_note = 56;
            break;
        case 'm' :  // A
            midi_note = 57;
            break;
        case 'k' :  // A#
            midi_note = 58;
            break;
        case ',' :  // B
            midi_note = 59;
            break;
        case '.' :  // C
            midi_note = 60;
            break;
        case ' ' :  // スペースキー入力で（前回のノート番号があれば）ノートオフ送信 -> returnする
            if ( midi_note_prev != 255 ) {
                uart_putc(MIDI_UART_ID , 0x80 );            // note off
                uart_putc(MIDI_UART_ID , midi_note_prev );  // note number
                uart_putc(MIDI_UART_ID , 90 );              // off velocity
            }
            return;

        default :   // 該当するキーがないときは なにも送信せずreturnする

            return;

    }   // end switch

    // 次のノート発音の前に 前回のノート番号（があれば）ノートオフする
    if ( midi_note_prev != 255 ) {
        uart_putc(MIDI_UART_ID , 0x80 );            // note off
        uart_putc(MIDI_UART_ID , midi_note_prev );  // note number
        uart_putc(MIDI_UART_ID , 90 );              // off velocity
    }

    // ノートオン送信
    uart_putc(MIDI_UART_ID , 0x90 );        // note on
    uart_putc(MIDI_UART_ID , midi_note );   // note number
    uart_putc(MIDI_UART_ID , 90 );          // on velocity

    midi_note_prev = midi_note;     //発音したノート番号を保存しておく

    return;
}
