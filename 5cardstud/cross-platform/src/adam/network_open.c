/**
 * Open Connection
 *
 * @param URL of form: N:PROTO://[HOSTNAME]:PORT/PATH/.../
 * @param open mode
 * @param translation mode (CR/LF to other line endings)
 * @return status
 */
#ifdef __ADAM__

#include <eos.h>
#include <string.h>
#include "network_utils.h"

#define READ_WRITE 12


struct
  {
    unsigned char cmd;
    unsigned char aux1;
    unsigned char aux2;
    unsigned char url[160];
  } co;

unsigned char network_open(char *url, unsigned char mode, unsigned char translation)
{
  memset(co,0,sizeof(co));
  co.cmd = 'O';
  co.aux1 = READ_WRITE;
  co.aux2 = translation;
  strcpy(co.url,"N:");
  strcat(co.url,url);

  return eos_write_character_device(NET_DEV,co,sizeof(co));
}

#endif
