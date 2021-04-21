/*
  apod.c

  Astronomy Picture of the Day (APOD) viewer client
  for Ataris using #Fujinet.

  See the APOD web app (server)

  By Bill Kendrick <bill@newbreedsoftware.com>
  2021-03-27 - 2021-04-21
*/

#include <stdio.h>
#include <atari.h>
#include <string.h>
#include "version.h"
#include "globals.h"
#include "nsio.h"
#include "dli9.h"
#include "myprint.h"
#include "get_time.h"
#include "screen_helpers.h"
#include "colorbars.h"

/* In ColorView mode, we will have 3 display lists that
   we cycle through, each interleaving between three
   versions of the image (red, green, blue) */
extern unsigned char rgb_table[];

/* A block of space to store the graphics & display lists */
extern unsigned char scr_mem[];


/* Defaults that look good on my NTSC Atari 1200XL connected
   to a Commodore 1902 monitor with Tint knob at its default,
   and Color knob just below its default: */
#define DEFAULT_RGB_RED 0x20 /* 2 "orange" */
#define DEFAULT_RGB_GRN 0xC0 /* 12 "green" */
#define DEFAULT_RGB_BLU 0xB0 /* 11 "blue green" */

unsigned char last_day[13] = { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };


/**
 * Set up a basic Display List
 *
 * @param byte antic_mode
 */
void dlist_setup(unsigned char antic_mode) {
  unsigned char i;
  unsigned int gfx_ptr, dlist_idx;

  screen_off();

  gfx_ptr = (unsigned int) (scr_mem + SCR_OFFSET);

  dlist_idx = 0;

  dlist1[dlist_idx++] = DL_BLK8;
  dlist1[dlist_idx++] = DL_BLK8;
  dlist1[dlist_idx++] = DL_BLK8;

  /* Row 0 */
  dlist1[dlist_idx++] = DL_LMS(antic_mode);
  dlist1[dlist_idx++] = (gfx_ptr & 255);
  dlist1[dlist_idx++] = (gfx_ptr >> 8);

  for (i = 1; i <= 101; i++) {
    dlist1[dlist_idx++] = antic_mode;
  }

  /* Hitting 4K boundary! */
  i++;
  gfx_ptr = (unsigned int) (scr_mem + 4096);
  dlist1[dlist_idx++] = DL_LMS(antic_mode);
  dlist1[dlist_idx++] = (gfx_ptr & 255);
  dlist1[dlist_idx++] = (gfx_ptr >> 8);

  for (i = i; i <= 191; i++) {
    dlist1[dlist_idx++] = antic_mode;
  }

  dlist1[dlist_idx++] = DL_JVB;
  dlist1[dlist_idx++] = ((unsigned int) dlist1 & 255);
  dlist1[dlist_idx++] = ((unsigned int) dlist1 >> 8);

  screen_on();
}


/* Tracking which Display List is active */
unsigned char dlist_hi, dlist_lo;

/* Keep track of old VBI vector, so we can jump to it at
   the end of ours (see below), and restore it when we're done
   needing our VBI */
void * OLDVEC;

/* Keeping track of which RGB color we're showing
   (for fetching from the look-up table) */
unsigned char rgb_ctr;


/* VBI routine for flipping between our three
   Display Lists in RGB image modes */
#pragma optimize (push, off)
void VBLANKD9(void) {
  /* grab the current rgb color counter */
  asm("ldx %v", rgb_ctr);

  /* increment it; roll from 3 back to 0 */
  asm("inx");
  asm("cpx #3");
  asm("bcc %g", __vbi_ctr_set);

  asm("ldx #0");

__vbi_ctr_set:
  /* store the current rgb color counter back;
     also store it as a reference to our next display list */
  asm("stx %v", dli9_load_arg);
  asm("stx %v", rgb_ctr);

  /* display lists are 8K away from each other
     (tucked under screen memory); that's 32 (256 byte) pages,
     so we can shift left 5 times to multiply the rgb color counter
     by 32... then store it in the high byte of SDLST */
  asm("txa");
  asm("asl a");
  asm("asl a");
  asm("asl a");
  asm("asl a");
  asm("asl a");
  asm("adc %v", dlist_hi);
  asm("sta $d403");
  asm("lda %v", dlist_lo);
  asm("sta $d402");

  /* adjust end of the screen colors - set the last one to black color */
  asm("lda %v+187,x", rgb_table);
  asm("sta %v+190,x", rgb_table);
  asm("lda %v+188,x", rgb_table);
  asm("sta %v+191,x", rgb_table);
  asm("lda #0");
  asm("sta %v+192,x", rgb_table);

  /* start next screen with black color at top */
  asm("sta $d01a");

  asm("jmp (%v)", OLDVEC);
}


/**
 * Set the deffered VBI vector
 *
 * @param void * Addr the VBI routine's location
 */
void mySETVBV(void * Addr)
{
  rgb_ctr = 0;
  dlist_hi = (unsigned char) (((unsigned int) (scr_mem + DLIST_OFFSET)) >> 8);
  dlist_lo = (unsigned char) (((unsigned int) (scr_mem + DLIST_OFFSET)) & 255);

  OS.critic = 1;
  OS.vvblkd = Addr;
  OS.critic = 0;

  ANTIC.nmien = NMIEN_VBI;
}

/**
 * Activate our Display List Interrupt
 */
void dli_init(void * dli_func)
{
  rgb_ctr = 0;
  ANTIC.nmien = NMIEN_VBI;
  while (ANTIC.vcount < 124);
  OS.vdslst = (void *) dli_func;
  ANTIC.nmien = NMIEN_VBI | NMIEN_DLI;
}

/**
 * Deactivate our Display List Interrupt
 */
void dli_clear(void)
{
  ANTIC.nmien = NMIEN_VBI;
}

#pragma optimize (pop)


/**
 * Set up a color table of repeating Red, Green, and Blue hues
 */
void setup_rgb_table(void) {
  unsigned char *rgb_ptr;
  unsigned char i;

  rgb_ptr = rgb_table;
  for(i = 0; i < 64; i++)
  {
    *rgb_ptr++ = rgb_red;
    *rgb_ptr++ = rgb_grn;
    *rgb_ptr++ = rgb_blu;
  }
}


/**
 * Set up three Display Lists for ColorView 9 RGB mode
 * (utilizes DLI and VBI to flicker; see above)
 *
 * @param byte antic_mode
 */
void dlist_setup_rgb9(unsigned char antic_mode) {
  int l, i;
  unsigned int gfx_ptr /*, next_dlist */;
  unsigned int dlist_idx;
  unsigned char * dlist;

  screen_off();

  for (l = 0; l < 3; l++) {
    dlist = (scr_mem + (l * SCR_BLOCK_SIZE)) + DLIST_OFFSET;
    gfx_ptr = (unsigned int) (scr_mem + (l * SCR_BLOCK_SIZE)) + SCR_OFFSET;

    dlist_idx = 0;

    dlist[dlist_idx++] = DL_BLK8;
    dlist[dlist_idx++] = DL_BLK8;
    dlist[dlist_idx++] = DL_DLI(DL_BLK8); /* start with colors after this line */

    /* Row 0 */
    dlist[dlist_idx++] = DL_LMS(DL_DLI(antic_mode));
    dlist[dlist_idx++] = (gfx_ptr & 255);
    dlist[dlist_idx++] = (gfx_ptr >> 8);

    for (i = 1; i <= 101; i++) {
      dlist[dlist_idx++] = DL_DLI(antic_mode);
    }

    /* Hitting 4K boundary! */
    gfx_ptr += (102 * 40);
    dlist[dlist_idx++] = DL_LMS(DL_DLI(antic_mode));
    dlist[dlist_idx++] = (gfx_ptr & 255);
    dlist[dlist_idx++] = (gfx_ptr >> 8);

    for (i = 103; i <= 191; i++) {
      dlist[dlist_idx++] = DL_DLI(antic_mode);
    }

    dlist[dlist_idx++] = DL_JVB;
    dlist[dlist_idx++] = (((unsigned int) dlist) & 255);
    dlist[dlist_idx++] = (((unsigned int) dlist) >> 8);
  }

  setup_rgb_table();

  screen_on();
}

/**
 * Set up three Display Lists for ColorView 15 RGB mode
 * (utilizes DLI and VBI to flicker; see above)
 *
 * @param byte antic_mode
 */
void dlist_setup_rgb15(unsigned char antic_mode) {
  int l, i;
  unsigned int gfx_ptr /*, next_dlist */;
  unsigned int dlist_idx;
  unsigned char * dlist;

  screen_off();

  for (l = 0; l < 3; l++) {
    dlist = (scr_mem + (l * SCR_BLOCK_SIZE)) + DLIST_OFFSET;
    gfx_ptr = (unsigned int) (scr_mem + (l * SCR_BLOCK_SIZE)) + SCR_OFFSET;

    dlist_idx = 0;

    dlist[dlist_idx++] = DL_BLK8;
    dlist[dlist_idx++] = DL_BLK8;
    dlist[dlist_idx++] = DL_DLI(DL_BLK8); /* start with colors after this line */

    /* Row 0 */
    dlist[dlist_idx++] = DL_LMS(DL_DLI(antic_mode));
    dlist[dlist_idx++] = (gfx_ptr & 255);
    dlist[dlist_idx++] = (gfx_ptr >> 8);

    for (i = 1; i <= 101; i++) {
      if (i % 3 == 0) {
        dlist[dlist_idx++] = DL_DLI(antic_mode);
      } else {
        dlist[dlist_idx++] = DL_DLI(antic_mode);
      }
    }

    /* Hitting 4K boundary! */
    gfx_ptr += (102 * 40);
    dlist[dlist_idx++] = DL_LMS(DL_DLI(antic_mode));
    dlist[dlist_idx++] = (gfx_ptr & 255);
    dlist[dlist_idx++] = (gfx_ptr >> 8);

    for (i = 103; i <= 191; i++) {
      if (i % 3 == 0) {
        dlist[dlist_idx++] = DL_DLI(antic_mode);
      } else {
        dlist[dlist_idx++] = DL_DLI(antic_mode);
      }
    }

    dlist[dlist_idx++] = DL_DLI(DL_BLK1);

    dlist[dlist_idx++] = DL_JVB;
    dlist[dlist_idx++] = (/*next_*/((unsigned int) dlist) & 255);
    dlist[dlist_idx++] = (/*next_*/((unsigned int) dlist) >> 8);
  }

  setup_rgb_table();

  screen_on();
}

/**
 * Set up display list for title/menu screen
 */
void dlist_setup_menu() {
  int dl_idx;

  screen_off();

  dlist1[0] = DL_BLK1;
  dlist1[1] = DL_BLK1;
  dlist1[2] = DL_BLK8;

  dlist1[3] = DL_LMS(DL_GRAPHICS2);
  dlist1[4] = ((unsigned int) scr_mem) & 255;
  dlist1[5] = ((unsigned int) scr_mem) >> 8;
  dlist1[6] = DL_GRAPHICS2;

  dlist1[7] = DL_BLK2;
  dlist1[8] = DL_GRAPHICS1;
  dlist1[9] = DL_GRAPHICS0;

  dlist1[10] = DL_BLK2;
  dlist1[11] = DL_GRAPHICS2;

  for (dl_idx = 12; dl_idx < 35; dl_idx++) {
    dlist1[dl_idx] = DL_GRAPHICS1;
  }

  dlist1[18] = DL_BLK2;
  dlist1[19] = DL_GRAPHICS2;

  dlist1[25] = DL_BLK2;
  dlist1[26] = DL_GRAPHICS2;

  dlist1[36] = DL_JVB;
  dlist1[37] = ((unsigned int) dlist1) & 255;
  dlist1[38] = ((unsigned int) dlist1) >> 8;

  OS.color4 = 0x00;
  OS.color0 = 0x8F;
  OS.color1 = 0x0A;
  OS.color2 = 0x80;
  OS.color3 = 0x88;

  screen_on();
}


/* The various graphics modes, the keypresses to
   choose them, and the argument to send to the
   web app via "?mode=". */
enum {
  CHOICE_NONE,
  CHOICE_HIRES_MONO,
  CHOICE_MEDRES_COLOR,
  CHOICE_LOWRES_GREY,
  CHOICE_LOWRES_RGB,
  CHOICE_MEDRES_RGB,
  NUM_CHOICES
};

unsigned char choice_keys[NUM_CHOICES] = {
  KEY_NONE,
  KEY_A,
  KEY_B,
  KEY_C,
  KEY_D,
  KEY_E
};

char * modes[NUM_CHOICES] = {
  "",
  "8",
  "15",
  "9",
  "rgb9",
  "rgb15"
};


/* The various sample options, and keys to select them */
enum {
  SAMPLE_NONE,
  SAMPLE_1,
  SAMPLE_2,
  SAMPLE_3,
  SAMPLE_4,
  SAMPLE_COLORBARS,
  NUM_SAMPLES
};

unsigned char sample_keys[NUM_SAMPLES] = {
  KEY_0,
  KEY_1,
  KEY_2,
  KEY_3,
  KEY_4,
  KEY_5
};


/* The base URL for the web app */
char * default_baseurl = "N:HTTP://billsgames.com/fujinet/apod/index.php";
char * baseurl;

/* Space to store the composed URL */
char url[255];

void handle_rgb_keypress(unsigned char k) {
  if (k == KEY_R) {
    rgb_red += 16;
  } else if (k == KEY_G) {
    rgb_grn += 16;
  } else if (k == KEY_B) {
    rgb_blu += 16;
  } else if (k == (KEY_R | KEY_SHIFT)) {
    rgb_red -= 16;
  } else if (k == (KEY_G | KEY_SHIFT)) {
    rgb_grn -= 16;
  } else if (k == (KEY_B | KEY_SHIFT)) {
    rgb_blu -= 16;
  } else if (k == KEY_X) {
    rgb_red = DEFAULT_RGB_RED;
    rgb_grn = DEFAULT_RGB_GRN;
    rgb_blu = DEFAULT_RGB_BLU;
  }
}

/**
 * Display the chosen date (or "current") on the menu
 */
void show_chosen_date() {
  char str[20];

  myprint(scr_mem, 2, 15, "                  ");
  if (pick_day != 0) {
    sprintf(str, "20%02d-%02d-%02d", pick_yr, pick_mo, pick_day);
    myprint(scr_mem, 2, 15, str);
  } else {
    if (cur_yr == 99) {
      myprint(scr_mem, 2, 15, "[CTRL-T] get time");
    } else {
      myprint(scr_mem, 2, 15, "current");
    }
  }
}

/**
 * Pick the current date (as fetched from #FujiNet's APETIME)
 */
void pick_today() {
  pick_yr = cur_yr;
  pick_mo = cur_mo;
  pick_day = cur_day;
}

void show_sample_choice(char sample) {
  char tmp_str[2];

  if (sample) {
    tmp_str[0] = sample + '0';
    tmp_str[1] = '\0';
    myprint(scr_mem, 19, 17, tmp_str);
  } else {
    myprint(scr_mem, 19, 17, " ");
  }
}

/* The program!  FIXME: Split into functions! */
void main(void) {
  unsigned char keypress, choice;
  int i, size;
  unsigned short data_len, data_read;
  char str[20];
  unsigned char sample = 0, done, k, date_chg;

  set_screen_and_dlist_pointers();

  /* Set the defaults for the RGB table */
  rgb_red = DEFAULT_RGB_RED;
  rgb_grn = DEFAULT_RGB_GRN;
  rgb_blu = DEFAULT_RGB_BLU;

  /* Get the current date/time, if we can */
  get_time();

  do {
    /* Prompt user for the preferred viewing mode */
    dlist_setup_menu();

                  /*--------------------*/
    myprint(scr_mem, 1,  0, "#FUJINET Astronomy");
    myprint(scr_mem, 1,  1, "Picture Of the Day");

                  /*--------------------*/
    myprint(scr_mem, 1,  2, "bill kendrick 2021");
    myprint(scr_mem, 0,  3, "with help from apc");
    myprint(scr_mem, 10 - strlen(VERSION) / 2, 4, VERSION);
 
                  /*--------------------*/
    myprint(scr_mem, 0,  5, "________HOW_________"); 
    myprint(scr_mem, 0,  6, "[A] hi-res mono");
    myprint(scr_mem, 0,  7, "[B] med-res 4 color");
    myprint(scr_mem, 0,  8, "[C] lo-res 16 shade");
    myprint(scr_mem, 0,  9, "[D]*lo-res 4K color");
    myprint(scr_mem, 0, 10, "[E]*med-res 64 color");
    myprint(scr_mem, 0, 11, "[F]*lo-res 256 color");

                  /*--------------------*/
    myprint(scr_mem, 0, 12, "________WHAT________");
    myprint(scr_mem, 0, 13, "[0] get apod");
    myprint(scr_mem, 0, 14, "[<=>] change date");
    show_chosen_date();
    myprint(scr_mem, 0, 16, "[1-4] get samples");
    myprint(scr_mem, 0, 17, "[5] color bars");
    show_sample_choice(sample);
  
                  /*--------------------*/
    myprint(scr_mem, 0, 18, "___*WHILE_VIEWING___");
    sprintf(str, "[R]=%02d [G]=%02d [B]=%02d", rgb_red >> 4, rgb_grn >> 4, rgb_blu >> 4);
    myprint(scr_mem, 0, 19, str);
    myprint(scr_mem, 0, 20, "[X] rbg defaults");
    myprint(scr_mem, 0, 21, "[ESC] return to menu");

    baseurl = default_baseurl;
  
    /* Accept a choice */
    choice = CHOICE_NONE;
    OS.ch = KEY_NONE;
    do {
      while (OS.ch == KEY_NONE) { }
      keypress = OS.ch;
      OS.ch = KEY_NONE;
  
      for (i = 0; i < NUM_CHOICES; i++) {
        if (choice_keys[i] == keypress) {
          choice = i;
        }
      }

      for (i = 0; i < NUM_SAMPLES; i++) {
        if (sample_keys[i] == keypress) {
          sample = i;
          show_sample_choice(sample);
        }
      }

      date_chg = 0;
      if (keypress == KEY_LESSTHAN) {
        /* [<] - Prev day */
        if (pick_day == 0) {
          pick_today();
        }
        if (pick_day > 1) {
          pick_day--;
        } else {
          if (pick_mo > 1) {
            pick_mo--;
          } else {
            pick_yr--;
            pick_mo = 12;
          }
          pick_day = last_day[pick_mo];
        }
        date_chg = 1;
      } else if (keypress == (KEY_LESSTHAN | KEY_SHIFT)) {
        /* [Shift]+[<] - Prev month */
        if (pick_mo > 1) {
          pick_mo--;
        } else {
          pick_yr--;
          pick_mo = 12;
        }
        date_chg = 1;
      } else if (keypress == (KEY_LESSTHAN | KEY_CTRL)) {
        /* [Ctrl]+[<] - Prev year */
        pick_yr--;
        date_chg = 1;
      } else if (keypress == KEY_GREATERTHAN) {
        /* [>] - Next day */
        if (pick_day == 0) {
          pick_today();
        }
        if (pick_day < last_day[pick_mo]) {
          pick_day++;
        } else {
          pick_day = 1;
          if (pick_mo < 12) {
            pick_mo++;
          } else {
            pick_mo = 1;
            pick_yr++;
          }
        }
        date_chg = 1;
      } else if (keypress == (KEY_GREATERTHAN | KEY_SHIFT)) {
        /* [Shift]+[>] - Next month */
        if (pick_mo < 12) {
          pick_mo++;
        } else {
          pick_mo = 1;
          pick_yr++;
        }
        date_chg = 1;
      } else if (keypress == (KEY_GREATERTHAN | KEY_CTRL)) {
        /* [Ctrl]+[>] - Next year */
        pick_yr++;
        date_chg = 1;
      } else if (keypress == KEY_EQUALS) {
        /* [=] - Choose current day (server-side) */
        pick_day = 0;
        date_chg = 1;
      } else if (keypress == (KEY_T | KEY_CTRL)) {
        /* [Ctrl]+[T] - Try to fetch time from #FujiNet APETIME again */
        get_time();
        pick_yr = cur_yr;
        pick_mo = cur_mo;
        pick_day = cur_day;
        date_chg = 1;
      }

      if (date_chg) {
        if (pick_day > last_day[pick_mo]) {
          pick_day = last_day[pick_mo];
        }
        if (pick_yr > cur_yr ||
            (pick_yr == cur_yr && pick_mo > cur_mo) ||
            (pick_yr == cur_yr && pick_mo == cur_mo && pick_day > cur_day)) {
          pick_today();
        }
        show_chosen_date();
      }
    } while (choice == CHOICE_NONE);

    /* Set up the display, based on the choice */
    size = 7680;

    OLDVEC = OS.vvblkd;

    if (choice == CHOICE_HIRES_MONO) {
      dlist_setup(DL_GRAPHICS8);
      OS.color4 = 128; /* Border (no reason) */
      OS.color2 = 0; /* Background */
      OS.color1 = 15; /* Foreground */
    } else if (choice == CHOICE_LOWRES_GREY) {
      dlist_setup(DL_GRAPHICS8);
      OS.gprior = 64;
      OS.color4 = 0; /* Greyscale */
    } else if (choice == CHOICE_MEDRES_COLOR) {
      dlist_setup(DL_GRAPHICS15);
      OS.color4 = 0; /* Background (black) */
      OS.color0 = 4; /* Dark foreground */
      OS.color1 = 8; /* Medium foreground */
      OS.color2 = 14; /* Light foreground */
    } else if (choice == CHOICE_LOWRES_RGB) {
      size = 7680 * 3;
      dlist_setup_rgb9(DL_GRAPHICS8);
      OS.gprior = 64;
    } else if (choice == CHOICE_MEDRES_RGB) {
      size = 7680 * 3;
      dlist_setup_rgb15(DL_GRAPHICS15);
    }

    wait_for_vblank();
 
    /* Load the data! */
    if (sample == SAMPLE_COLORBARS) {
      render_colorbars();
    } else {
      if (pick_day == 0) {
        snprintf(url, sizeof(url), "%s?mode=%s&sample=%d", baseurl, modes[choice], sample);
      } else {
        snprintf(url, sizeof(url), "%s?mode=%s&sample=%d&date=%02d%02d%02d", baseurl, modes[choice], sample, pick_yr, pick_mo, pick_day);
      }
   
      nopen(1 /* unit 1 */, url, 4 /* read */);
      /* FIXME: Check for error */
  
      if (size == 7680) {
        /* Single screen image to load */
        nread(1, scr_mem1, (unsigned short) size);
      } else {
        /* Multiple screen images to load... */

        OS.color4 = rgb_table[0];
        wait_for_vblank();

        for(data_read = 0; data_read < 7680; data_read += data_len)
        {
          nstatus(1);
          data_len=(OS.dvstat[1] << 8) + OS.dvstat[0];
          if (data_len == 0) break;
          if (data_read + data_len > 7680) data_len = 7680 - data_read;
          nread(1, scr_mem1 + data_read, data_len);
        }

        OS.sdmctl = 0;
        wait_for_vblank();
        OS.sdlst = dlist2;
        OS.color4 = rgb_table[1];
        wait_for_vblank();
        OS.sdmctl = DMACTL_PLAYFIELD_NORMAL | DMACTL_DMA_FETCH;
        wait_for_vblank();

        for(data_read = 0; data_read < 7680; data_read += data_len)
        {
          nstatus(1);
          data_len=(OS.dvstat[1] << 8) + OS.dvstat[0];
          if (data_len == 0) break;
          if (data_read + data_len > 7680) data_len = 7680 - data_read;
          nread(1, scr_mem2 + data_read, data_len);
        }

        OS.sdmctl = 0;
        wait_for_vblank();
        OS.sdlst = dlist3;
        OS.color4 = rgb_table[2];
        wait_for_vblank();
        OS.sdmctl = DMACTL_PLAYFIELD_NORMAL | DMACTL_DMA_FETCH;
        wait_for_vblank();

        for(data_read = 0; data_read < 7680; data_read += data_len)
        {
          nstatus(1);
          data_len=(OS.dvstat[1] << 8) + OS.dvstat[0];
          if (data_len == 0) break;
          if (data_read + data_len > 7680) data_len = 7680 - data_read;
          nread(1, scr_mem3 + data_read, data_len);
        }

        OS.sdlst = dlist1;
      }

      if (choice == CHOICE_MEDRES_COLOR) {
        nread(1, (char *) 712, 1);
        nread(1, (char *) 708, 3);
      }

      nclose(1 /* unit 1 */);
    }

    if (choice == CHOICE_LOWRES_RGB) {
      mySETVBV((void *) VBLANKD9);
      dli_init(dli9);
    } else if (choice == CHOICE_MEDRES_RGB) {
      // mySETVBV((void *) VBLANKD); /* FIXME */
      // dli_init(dli); /* FIXME */
    }


    /* Accept keypresses while viewing */
    OS.ch = KEY_NONE;
    done = 0;
    do {
      k = OS.ch;
      if (k == KEY_ESC) {
        done = 1;
      } else if ((k & 0x3F) == KEY_R || (k & 0x3F) == KEY_G || (k & 0x3F) == KEY_B || k == KEY_X) {
        /* [R], [G], or [B] key, with our without [Shift] */
        handle_rgb_keypress(k);
        setup_rgb_table();
        OS.ch = KEY_NONE;
      }
    } while (!done);
    OS.ch = KEY_NONE;

    if (choice == CHOICE_LOWRES_RGB ||
        choice == CHOICE_MEDRES_RGB) {
      dli_clear();  
      mySETVBV((void *) OLDVEC);
    }
    OS.gprior = 0;
  } while(1);
}