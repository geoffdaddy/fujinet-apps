#ifdef __ADAM__
#include "../platform-specific/graphics.h"
#include <video/tms99x8.h>
#include <conio.h>
#include <sys/ioctl.h>
#include <smartkeys.h>
#include "udgs.h"

//defines for tile locations
// #define POINTER_TILE 216
// #define CHIP_TILE 1
// #define UL_CORNER_TILE 144
// #define HORIZONTAL_TILE 145
// #define UR_CORNER_TILE 146
// #define LL_CORNER_TILE 147
// #define LR_CORNER_TILE 148
// #define VERTICAL_TILE 149

#define POINTER_TILE 0xbe
#define CHIP_TILE 0xbc
#define UL_CORNER_TILE 0xb6
#define HORIZONTAL_TILE 0xb7
#define UR_CORNER_TILE 0xb8
#define LL_CORNER_TILE 0xb9
#define LR_CORNER_TILE 0xba
#define VERTICAL_TILE 0xbb

unsigned char pointerLocation_x = 255;
unsigned char pointerLocation_y = 255;
extern unsigned long currentTime;

void enableDoubleBuffer()
{
}

void disableDoubleBuffer()
{
}

void drawBuffer()
{
    cputs(tempBuffer);
}

// Call to clear the screen to an empty table
void resetScreen()
{
}

void clearStatusBar()
{
}

void drawStatusTextAt(unsigned char x, char* s)
{
}

void drawStatusText(char* s)
{
}

void drawStatusTimer()
{
    gotoxy(0,23);
    cputs("timer goes here");
}

void drawText(unsigned char x, unsigned char y, char* s)
{
    // gotoxy(x,y);
    // cputs(s);
    smartkeys_puts(x*8,y*8,s);

}

void drawCard(unsigned char x, unsigned char y, unsigned char partial, const char* s)
{
    //cards in tile set are 5 tiles tall, and 2 tiles wide for a full card (3 at the end), 
    //and 1 tile wide for partial

    //draw the top of the card
    gotoxy(x,y++);
    if (partial == FULL_CARD )
        cputs("\x93\x95\x97");
    else if (partial == PARTIAL_LEFT)
        cputc(0x93);
    else 
        cputc(0x97);

    gotoxy(x,y++);
  // Card value row
  switch (s[0]) {
    case 't': cputs("\x99\x8a\x99"); break;
    case 'j': cputs("\x99\x8b\x99"); break;
    case 'q': cputs("\x99\x8c\x99"); break;
    case 'k': cputs("\x99\x8d\x99"); break;
    case 'a': cputs("\x99\x8e\x99"); break;
    case '?': cputs("\xb1\xb2\x99"); break;
    default:
        cputc(0x99); cputc(s[0] - 0x80); cputc(0x99);
  }

// blank middle row of card
    gotoxy(x,y++);
    if (s[0] == '?')
        cputs("\x\x\x99");
    else
        cputs("\x99\xa8\x99");

//   switch (s[1]) {
//     case 'h' : suit=0x0A0B; red=1; break;
//     case 'd' : suit=0x0C0D; red=1; break;
//     case 'c' : suit=0x0E0F; red=0; break;
//     case 's' : suit=0x1011; red=0; break;
//     default: suit=0x7B7C; red=0; break;
//   }

// bottom of card
    gotoxy(x,y++);
    cputs("\x94\x96\x98");
}

void drawChip(unsigned char x, unsigned char y)
{
    gotoxy(x,y);
    cputc(CHIP_TILE);
}

void drawPointer(unsigned char x, unsigned char y)
{
    if (!pointerLocation_x == 255 && !pointerLocation_y == 255) // not the first time..clear the old pointer location
    {
        gotoxy(pointerLocation_x,pointerLocation_y);
        cputc(0x20);
    }

    gotoxy(x,y);
    cputc(POINTER_TILE);
    pointerLocation_x = x;
    pointerLocation_y = y;
}

void drawLine(unsigned char x, unsigned char y, unsigned char w)
{
    unsigned char i;
    gotoxy(x,y);
    for (i=0; i<w; i++)
        cputc(HORIZONTAL_TILE);
}

void drawBox(unsigned char x, unsigned char y, unsigned char w, unsigned char h)
{
    unsigned char i;
    gotoxy(x,y++);
    //Draw the top
    cputc(UL_CORNER_TILE);
    for (i = 1; i <= w; i++)
        cputc(HORIZONTAL_TILE);
    cputc(UR_CORNER_TILE);

    //Draw the sides
    for (i = 0; i < h; i++)
    {
        gotoxy(x,y);
        cputc(VERTICAL_TILE);
        gotoxy(x+w+1,y++);
        cputc(VERTICAL_TILE);
    }
    gotoxy(x,y);
    //Draw the bottom
    cputc(LL_CORNER_TILE);
    for (i = 1; i <= w; i++)
        cputc(HORIZONTAL_TILE);
    cputc(LR_CORNER_TILE);
    //   printf("\x20\x20\x20\x20\x20\x9A\x9B\x9C\x20\x20\x20\x20\x20       FIVE CARD STUD");
    
}

void drawBorder()
{
}

void drawLogo()
{
    //   gotoxy(9,11);

    smartkeys_puts(80,88,"FIVE CARD STUD");
    //   printf("FIVE CARD STUD");
}

unsigned char cycleNextColor()
{
}

void counterInt(void)
{
    currentTime++;
}

void initGraphics()
{
    //set up the vdp vram with tile data, both tile and color... change to mode 1 first thing.
    //change background to medium green...
    // int mode = 1;
    // console_ioctl(IOCTL_GENCON_SET_MODE, &mode);
	smartkeys_set_mode();
    void *param = &udgs;
    console_ioctl(IOCTL_GENCON_SET_UDGS,&param);
    vdp_color(VDP_INK_WHITE,VDP_INK_MEDIUM_GREEN,VDP_INK_MEDIUM_GREEN);
    clrscr();

    // smartkeys_puts(64,64,"Here is an example of SmartKeys Output.");
    
    // cputs("\xa0\xa1 \xa2\xa3\xa4"); // BET

	// cputs(" NORMAL TEXT");

	// gotoxy(2,2);
	// textcolor(RED);
	// textbackground(WHITE);
	// cputs("\x92");
	
	// textcolor(BLACK);
	// cputs("\x8F");



    // vdp_vwrite(tiles_ch,0,sizeof(tiles_ch));
    // vdp_vwrite(tiles_cc,0x2000,sizeof(tiles_cc));
    clrscr();
    //admittedly a weird place to put this, but we are using 9918 graphics library
    // functions for this...
    add_raster_int(counterInt);
}

void waitvsync()
{
}

#endif /* __ADAM__ */
