#ifdef __ADAM__
#include "graphics.h"
#include <video/tms99x8.h>
#include <conio.h>
#include <sys/ioctl.h>

//defines for tile locations
#define POINTER_TILE 216
#define CHIP_TILE 1
#define UL_CORNER_TILE 144
#define HORIZONTAL_TILE 145
#define UR_CORNER_TILE 146
#define LL_CORNER_TILE 147
#define LR_CORNER_TILE 148
#define VERTICAL_TILE 149

unsigned char pointerLocation_x = 255;
unsigned char pointerLocation_y = 255;

void enableDoubleBuffer()
{
}

void disableDoubleBuffer()
{
}

void drawBuffer()
{
    printf("%s",tempBuffer);
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
}

void drawText(unsigned char x, unsigned char y, char* s)
{
    gotoxy(x,y);
    cputs(s);
}

void drawCard(unsigned char x, unsigned char y, unsigned char partial, const char* s)
{
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
        cputc(0);
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
      gotoxy(9,11);
      printf("FIVE CARD STUD");
}

unsigned char cycleNextColor()
{
}

void initGraphics()
{
    //set up the vdp vram with tile data, both tile and color... change to mode 1 first thing.
    //change background to medium green...
    int mode = 1;
    console_ioctl(IOCTL_GENCON_SET_MODE, &mode);
    vdp_color(VDP_INK_WHITE,VDP_INK_MEDIUM_GREEN,VDP_INK_MEDIUM_GREEN);
    vdp_vwrite(tiles_ch,0,sizeof(tiles_ch));
    vdp_vwrite(tiles_cc,0x2000,sizeof(tiles_cc));
    clrscr();
}

void waitvsync()
{
}

#endif /* __ADAM__ */
