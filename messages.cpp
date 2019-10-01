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

#include "misc.h"
#include "messages.h"

const char *msgid_tostring(BYTE id)
{
  switch (id) {
  case ICCMT_CC_GET_SW_VSN :
    return "REQUEST_GET_MODE";
    break;
  case ICCMT_CC_SW_VSN:
    return "RESPONSE_GET_MODE";
    break;
  case ICCMT_CC_SET_MODE:
    return "SET_SLAVE_MODE";
    break;
  case    ICCMT_CC_PING:
    return "PING";
    break;
  case  ICCMT_CC_PONG:
    return "PONG";
  case ICCMT_TS_2_UNITS_CONN:
    return "CONNECT INPUT TO SUBCHANNEL TIMESLOT";
    break;
  case ICCMT_TS_2_SC0_DISC:
    return "DISCONNECT  SUBCHANNEL 0  TO TIMESLOT";
    break;
  case ICCMT_TS_2_SC1_DISC:
    return "DISCONNECT  SUBCHANNEL 1  TO TIMESLOT";
    break;
  case ICCMT_TS_2_SC3_DISC:
    return "DISCONNNECT SUBCHANNEL 3  TO TIMESLOT";
    break;
  case ICCMT_TS_2_SC0_CONN:
    return "CONNECT SUBCHANNEL 0 TO TIMESLOT";
    break;
  case ICCMT_TS_2_SC1_CONN:
    return "CONNECT SUBCHANNEL 1 TO TIMESLOT";
    break;
  case ICCMT_TS_2_SC3_CONN:
    return "CONNECT SUBCHANNL 3 TO TIMESLOT";
    break;



  case ICCMT_SWITCH_SET:
    return "ENABLE S/N";
    break;
  case ICCMT_SWITCH_CLEAR:
    return "CLEAR S/N";
    break;
  default:
    return "unknown";
  }
}

const char *dump_message_buffer(BYTE * buf, int size)
{
  tcp_msg_t msg;
  memcpy(msg.data, buf, size);
  msg.size = size;
  return dump_message(&msg);
}

const char *dump_message(tcp_msg_t *pmsg)
{
  char desc[1024] = {0};

  /* afficher le type */
  sprintf(desc, "msg :\n");
  sprintf(desc + strlen(desc), "\ttype : %s\n", msgid_tostring(pmsg->data[0]));
  sprintf(desc + strlen(desc), "\tsize : %d\n", pmsg->size);
  
  int l ;
/*   printf("dump hexa :\n"); */
/*   for (l = 0; l < pmsg->size ; l++) { */
/*     printf("0x%2.2X ", pmsg->data[l] & 0xff); */
/*   } */
/*   printf("\n"); */
  BYTE *buf = pmsg->data;
  int idx;
  switch (buf[0]) {
    
  case ICCMT_SWITCH_SET:
    {
      




    }
    break;
  case ICCMT_SWITCH_CLEAR:
    {


    }
    break;
  case ICCMT_TS_2_UNITS_CONN:
    /*   return "CONNECT INPUT TO SUBCHANNEL TIMESLOT"; */
    {
      idx = 1;
      if (buf[idx] == 0x80) 
	sprintf(desc + strlen(desc), "\t -> sub channel 0\n");
      else if (buf[idx] == 0x81)
	sprintf(desc + strlen(desc), "\t -> sub channel 1\n");
      else if (buf[idx] == 0x83)
	sprintf(desc + strlen(desc), "\t -> sub channel 3\n");
      idx ++;
      sprintf(desc + strlen(desc), "\t ----\n");
      if ((((buf[idx] & 0xff) >> 3) & 0x1f) == 0) {

	/* adresse sur 2 bytes */
	idx ++; 		/* passe  à 3 */
	sprintf(desc + strlen(desc), "\t -> input board number %u\n", (buf[idx] >> 3) & 0x1f);
	sprintf(desc + strlen(desc), "\t -> input port number %u\n", (buf[idx] & 0x07) & 0xff);
	sprintf(desc + strlen(desc), "\t -> input sbi number %u\n", (buf[idx-1] & 0x07) & 0xff);
      }
      else {

	sprintf(desc + strlen(desc), "\t -> input board number %u\n", (buf[idx] >> 3) & 0x1f);
	sprintf(desc + strlen(desc), "\t -> input port number %u\n", (buf[idx] & 0x07) & 0xff);
	sprintf(desc + strlen(desc), "\t -> input sbi number 0\n");
      }
      idx ++;
      sprintf(desc + strlen(desc), "\t -> timeslot %u\n", buf[idx] & 0xff);	   
      idx ++;

      if ((((buf[idx] & 0xff) >> 3) & 0x1f) == 0) {
	/* adresse sur 2 bytes */

	idx ++; 	       
	sprintf(desc + strlen(desc), "\t -> output board number %u\n", (buf[idx] >> 3) & 0x1f);
	sprintf(desc + strlen(desc), "\t -> output port number %u\n", (buf[idx] & 0x07) & 0xff);
	sprintf(desc + strlen(desc), "\t -> output sbi number %u\n", (buf[idx-1] & 0x07) & 0xff);
      }
      else {
	sprintf(desc + strlen(desc), "\t -> output board number %u\n", (buf[idx] >> 3) & 0x1f);
	sprintf(desc + strlen(desc), "\t -> output port number %u\n", (buf[idx] & 0x07) & 0xff);
	sprintf(desc + strlen(desc), "\t -> output sbi number 0\n");
      }
      idx ++ ;
      int k;
      for (k = 0; k < 8; k ++) {
	if (buf[idx] >> k == 1) {
	  sprintf(desc + strlen(desc), "\t -> output port number %d\n", k);
	  break;
	}
      }
    }
    break;
  case ICCMT_TS_2_SC0_DISC:
  case ICCMT_TS_2_SC1_DISC:
  case ICCMT_TS_2_SC3_DISC:

    {

      idx = 0;
      if (buf[idx] == ICCMT_TS_2_SC0_DISC)
	sprintf(desc + strlen(desc), "\t -> sub channel 0\n");
      else if  (buf[idx] == ICCMT_TS_2_SC1_DISC)
	sprintf(desc + strlen(desc), "\t -> sub channel 1\n");
      else if  (buf[idx] == ICCMT_TS_2_SC3_DISC)
	sprintf(desc + strlen(desc), "\t -> sub channel 3\n");
      idx ++;
      if (((buf[idx] & 0xff >> 3) & 0x1f) == 0) {
	/* adresse sur 2 bytes */
	idx ++; 		/* passe  à 3 */
	sprintf(desc + strlen(desc), "\t -> board number %d\n", (buf[idx] >> 3) & 0xff);
	sprintf(desc + strlen(desc), "\t -> port number %d\n", (buf[idx] & 0x07) & 0xff);
	sprintf(desc + strlen(desc), "\t -> sbi number %d\n", (buf[idx-1] & 0x07) & 0xff );
      }
      else {
	sprintf(desc + strlen(desc), "\t -> board number %d\n", (buf[idx] >> 3) & 0xff);
	sprintf(desc + strlen(desc), "\t -> port number %d\n", (buf[idx] & 0x07) & 0xff);
	sprintf(desc + strlen(desc), "\t -> sbi number 0\n");
      }
    
    }
    break;
  case ICCMT_TS_2_SC0_CONN:
  case ICCMT_TS_2_SC1_CONN:
  case ICCMT_TS_2_SC3_CONN:  
    {

      idx = 0;
      if (buf[idx] == ICCMT_TS_2_SC0_CONN)
	sprintf(desc + strlen(desc), "\t -> sub channel 0\n");
      else if  (buf[idx] == ICCMT_TS_2_SC1_CONN)
	sprintf(desc + strlen(desc), "\t -> sub channel 1\n");
      else if  (buf[idx] == ICCMT_TS_2_SC3_CONN)
	sprintf(desc + strlen(desc), "\t -> sub channel 3\n");
      idx ++;
      if ((((buf[idx] & 0xff) >> 3) & 0x1f) == 0) {
	/* adresse sur 2 bytes */
	idx ++; 		/* passe  à 3 */
	sprintf(desc + strlen(desc), "\t -> board number %d\n", (buf[idx] >> 3) & 0xff);
	sprintf(desc + strlen(desc), "\t -> port number %d\n", (buf[idx] & 0x07) & 0xff);
	sprintf(desc + strlen(desc), "\t -> sbi number %d\n", (buf[idx-1] & 0x07) & 0xff);
      }
      else {
	sprintf(desc + strlen(desc), "\t -> board number %d\n", (buf[idx] >> 3) & 0xff);
	sprintf(desc + strlen(desc), "\t -> port number %d\n", (buf[idx] & 0x07) & 0xff );
	sprintf(desc + strlen(desc), "\t -> sbi number 0\n");
      }
      idx++;
      sprintf(desc + strlen(desc), "\t -> timeslot %d\n", buf[idx] & 0xff);	   
    }
    break;
  case ICCMT_BP_MSG_ICCO_UNIT:
    {
/*       /\* afficher le numéro du port *\/ */
/*       sprintf(desc + strlen(desc), "\t -> board number %d\n", buf[1] & 0xff); */
/*       sprintf(desc + strlen(desc), "\t -> input number %d\n", buf[5] & 0xff); */
    }
    break;
  default:
    break;

  }



  
  return strdup(desc);
}

const char *dump_message_serialized(BYTE *buf, int size)
{
  char desc[1024] = {0};
  int idx;
  /* afficher le type */
  sprintf(desc, "msg :\n");
  sprintf(desc + strlen(desc),"\t@    : %p\n", buf);
  if ((buf[0] == EOMSG && buf[1] == 0)
      || (buf[0] == EOMSG && buf[1] == 1)) {
    sprintf(desc + strlen(desc), "\ttype : %s\n", msgid_tostring(buf[2]));
    idx = 2;
  }
  else {
    sprintf(desc + strlen(desc), "\ttype : %s\n", msgid_tostring(buf[1]));
    idx = 1;
  }

  sprintf(desc + strlen(desc), "\tsize : %d\n", size);
  
  int i ; 
  sprintf(desc + strlen(desc), "hexa :\n");
  for (i  = 0; i < size; i ++) 
    sprintf(desc + strlen(desc), "0x%2.2x ", buf[i] & 0xff);

	   
  
  return strdup(desc);
}
