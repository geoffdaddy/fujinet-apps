#ifdef __ADAM__
#include <eos.h>
/**
 * @brief Adam input routines
 * @author Thomas Cherryhomes, Geoff Oltmans
 * @license gpl v.3
 */

static GameControllerData cont;

unsigned char readJoystick()
{
  unsigned char temp = 0;
  eos_read_game_controller(0x01,&cont);
  temp = cont.joystick1;
  temp |= (cont.joystick1_button_left || cont.joystick1_button_right)<<4;

  return 0;
}

#endif /* __ADAM__ */
