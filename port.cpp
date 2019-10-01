// port.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>

#include "debug.h"
#include "misc.h"
#include "mixer.h"
#include "alarmes.h"

/* retourne l'address d'un port */
/*int get_port_address(int board_number, int port_number)
{
  // port number is 0...7 (0...5 in case of ASLT) and board number 1 ... 26
  if(port_number < 8) 
    return (board_number * 8) + port_number;
  else 
    return 0x100 + (board_number * 8) + (port_number - 8);
}*/

/* retourne l'adresse d'un port en fonction du numero de carte, numero SBI et numero port */
port_address_t get_port_address(int board_number, int sbi_number, int port_number)
{
  port_address_t addr;
  
  if (sbi_number == 0) {
    addr.size = 1;
    addr.byte1 = ((board_number & 0xff) << 3)  | (port_number & 0xff);
	addr.byte2 = 0;
  }
  else {
    addr.size = 2;
    addr.byte1 = sbi_number & 0xff;
    addr.byte2 = ((board_number & 0xff) << 3) | (port_number & 0xff);
  }
 
  vega_log(INFO, "B %d SBI %d P %d ADDR size:%d %02X %02X", board_number, sbi_number, port_number, addr.size, addr.byte1, addr.byte2); 
  return addr;
}
int get_board_number_from_address(port_address_t addr)
{
  if (addr.size == 1)
    return (addr.byte1 >> 3) & 0x1f;
  
  return (addr.byte2 >> 3) & 0x1f;
}

int get_port_number_from_address(port_address_t addr)
{
  /* recupere la valeur des 3 derniers bits */
  if (addr.size == 1)
    return (addr.byte1 & 0x07);
  
  return (addr.byte2 & 0x07);
}

int get_sbi_number_from_address(port_address_t addr)
{
  if (addr.size == 1)
    return 0;
  
  return (addr.byte1 & 0x07);
}



port_address_t get_vega_port_address(vega_port_t* port)
{
  port_address_t addr;
  
  if (port->sbi_number == 0) {
    addr.size = 1;
    addr.byte1 = ((port->board_number & 0xff) << 3)  | (port->port_number & 0xff);
  }
  else {
    addr.size = 2;

    addr.byte1 = port->sbi_number & 0xff;
    addr.byte2 = ((port->board_number & 0xff) << 3) | (port->port_number & 0xff);
  }
  
  return addr;
}

