#include "../../libs/abi.h"

#define AVC64ADR 0xfffd
#define DEBUG kernel_service(SYS_PRINT, (unsigned)"DEBUG", 5, 0, 0)

void print_char(unsigned char x, unsigned char y, unsigned char fg, unsigned char bg, unsigned char c)
{
    wemem(AVC64ADR, 0x0300 + x);
    wemem(AVC64ADR, 0x0400 + y);
    wemem(AVC64ADR, 0x0600 + c);
    wemem(AVC64ADR, 0x0700 + fg); // set D3 (fg color) to black (00)
    wemem(AVC64ADR, 0x0800 + bg); // set D4 (bg color) to white (FF)
    wemem(AVC64ADR, 0x0100);
}

void header(char *msg, unsigned len)
{
    // using the AVC64 video card in texture mode
    // In D1 and D2 you store the texture ID (in this case ascii char code)
    // D3 for foreground color
    // D4 for background color
    // CX for X pos
    // CY pos Y pos
    // AOS0.2 supports 8x8 ascii charmap, so coordinates will be
    // Y=0, X=0 for first char
    // Y=0, X=8 for the next one, and so on
    wemem(AVC64ADR, 0x0700); // set D3 (fg color) to black (00)
    wemem(AVC64ADR, 0x08FF); // set D4 (bg color) to white (FF)
    wemem(AVC64ADR, 0x0400); // set CY = 0
    wemem(AVC64ADR, 0x0500); // ascii uses only 1 byte, so D1 is always 0
    unsigned i = 0;
    for (; i < len; i++)
    {
        wemem(AVC64ADR, 0x0300 + (i << 3)); // set CX = i * 8
        wemem(AVC64ADR, 0x0600 + msg[i]);   // set D2 = ascci char code
        wemem(AVC64ADR, 0x0100);            // print
    }
    wemem(AVC64ADR, 0x0620); // set D2 = space char
    for (; i < 32; i++)
    {
        wemem(AVC64ADR, 0x0300 + (i << 3)); // set CX = i * 8
        wemem(AVC64ADR, 0x0100);
    }
    // reset color schema
    wemem(AVC64ADR, 0x07FF); // set D3 (fg color) to white (FF)
    wemem(AVC64ADR, 0x0800); // set D4 (bg color) to black (00)
    // print a new line
    kernel_service(SYS_PRINT, (unsigned)"\n", 1, 0, 0);
}
