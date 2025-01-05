#ifdef __ADAM__

/*
  Network functionality
*/

int getJsonResponse(char *url, char *buffer, int max_length)
{
  NetStatus stat;
  if (network_open(url,MODE_READ_WRITE,2) != ACK)
    return 0;

  network_set_channel_mode(MODE_JSON);
  network_parse_json();

  if (network_status(stat) != 0x80)
    return 0;

  if (eos_read_character_device(NET_DEV,buffer,max_length) != ACK)
    return 0;

  if (network_status(stat) != 0x80)
    return 0;

  return stat.rxBytesWaiting;
}

#endif /* __ADAM__ */
