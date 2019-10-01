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
#include "conf.h"
#include "mixer.h"
#include "alarmes.h"
#include "messages.h"


int get_sbi_number_from_address(port_address_t addr);


#define __DEBUG_TIMESLOT__
#ifdef __DEBUG_TIMESLOT__

typedef struct{
	int sc0_ts;
	int sc1_ts;
	int sc3_ts;
}cabling_t;
cabling_t array_connect_timeslot_to_suboutput[MAX_BOARD][SBI_BY_BOARD][AGA_PORTS_BY_BOARD]={0};
//cabling_t array_connect_timeslot_to_suboutput[24/*board*/][2/*sbi*/][8/*8 ports*/][3/*subchannel*/]={0};


typedef struct{
	int connected;
	int subchannel;
	vega_port_t* input_port ;
	vega_port_t* sub_output_port ;
}cablinput_t;
cablinput_t array_connect_input_to_sub_output[MAX_TIMESLOT]={0};

void dump_timeslots(FILE* fout, int connected_only)
{
	if ( NULL == fout ) return;
	int i=0,j=0,k=0;
	//printf("TS xxx ");printf(" 0 ->  No PORT  -> 7\n");

	//printf("PORT   ");	for ( j=0; j< MAX_PORT_BY_SBI ; j++) { printf(" %d ",j); }printf("\n");
	//printf("       ");	for ( j=0; j< MAX_PORT_BY_SBI ; j++) { printf(" | "); }printf("\n");
	
	fprintf(fout,"--- timeslot_to_suboutput ---\n");

	int B,SBI,P;
	for ( B=0;B<MAX_BOARD; B++){
		for ( SBI=0;SBI< SBI_BY_BOARD; SBI++){
			for (P=0;P< AGA_PORTS_BY_BOARD; P++){
				int ts0 = array_connect_timeslot_to_suboutput[B][SBI][P].sc0_ts;
				int ts1 = array_connect_timeslot_to_suboutput[B][SBI][P].sc1_ts;
				int ts3 = array_connect_timeslot_to_suboutput[B][SBI][P].sc3_ts;

				if ( ts0 ) fprintf(fout,"TS %03d ----------> B%02d - SBI%d - P%d - SC0\n", ts0, B, SBI, P );
				//else fprintf(fout,"TS     ----------> B%02d - SBI%d - P%d - SC0\n", ts0, B, SBI, P );
				if ( ts1 ) fprintf(fout,"TS %03d ----------> B%02d - SBI%d - P%d - SC1\n", ts1, B, SBI, P );
				//else fprintf(fout,"TS     ----------> B%02d - SBI%d - P%d - SC1\n", ts0, B, SBI, P );
				if ( ts3 ) fprintf(fout,"TS %03d ----------> B%02d - SBI%d - P%d - SC3\n", ts3, B, SBI, P );
				//else fprintf(fout,"TS     ----------> B%02d - SBI%d - P%d - SC3\n", ts0, B, SBI, P );
			}
		}
	}


		fprintf(fout,"--- input_to_sub_output ---\n");
	for (k=31;k<=200;k++)
	{ 
		if ( array_connect_input_to_sub_output[k].connected ) 
		{
			port_address_t adi = array_connect_input_to_sub_output[k].input_port->address;
			port_address_t ado = array_connect_input_to_sub_output[k].sub_output_port->address;

			fprintf(fout,"TS %03d <---------- B%02d - SBI%d - P%d - SC%d  ", 
				k, 
				get_board_number_from_address(adi),
				get_sbi_number_from_address(adi),
				get_port_number_from_address(adi),
				array_connect_input_to_sub_output[k].subchannel
				);

			fprintf(fout,"TS %03d ----------> B%02d - SBI%d - P%d\n", 
				k, 
				get_board_number_from_address(ado),
				get_sbi_number_from_address(ado),
				get_port_number_from_address(ado)
				);

		}
	}
}
#else
void dump_timeslots(FILE* fout, int connected_only){printf("dump_timeslots: not enabled in this version\n");}
#endif

int is_valid_timeslot(int ts) { return (ts>0 && ts<255) ;}
int mixer_update_input_connection_state(vega_port_t *p, int ts, int connected);
int mixer_update_sub_output_connection_state(vega_port_t *p, int number, int connected, int ts);

/* active ou desactive un S/N sur un port */
int __mixer_enable_disable__sn(vega_port_t *port, int sn_number, int enable)
{
  BYTE data[4];
  int i = 0;
  port_address_t addr = port->address;

  /*if (alarms_board_is_in_anomaly(alarms, get_board_number_from_address(addr)) == 0) {
    printf("la carte %d est en anomalie ! Impossible d'effectuer une operation __mixer_enable_disable__sn()\n",
	   get_board_number_from_address(addr));
    return -1;
  }*/

  if (enable == 0) 
    data[i++] = ICCMT_SWITCH_CLEAR;
  else
    data[i++] = ICCMT_SWITCH_SET;

 
  if (addr.size == 1) {
    data[i++] = addr.byte1;

  }
  else {
    data[i++] = addr.byte1;
    data[i++] = addr.byte2;

  }

  data[i++] = sn_number;

  if (vega_control_send_alphacom_msg(VEGA_URGENT,pAlphaComLink, data, i)) {
    return -1;
  }
  
  /* place la nouvelle valeur du port en memoire */
  if (sn_number == 1) 
    port->mixer.sn1 = enable == 0 ? 0 : 1;
  else if (sn_number == 2)
    port->mixer.sn2 = enable == 0 ? 0 : 1;
  else
    port->mixer.sn3 = enable == 0 ? 0 : 1;

  

  return 0;
};

/*const char * sub_channel_to_string(sub_channel_t sub)
{
  switch (sub) {
  case sub_channel_0:
    return "SC0";
    break;
  case sub_channel_1:
    return "SC1";
    break;
  case sub_channel_3:
    return "SC3";
    break;

  }
  return "invalid sub_channel";
}*/

int mixer_connect_input_to_sub_output(vega_port_t *input_port, vega_port_t *sub_output_port, sub_channel_t typechannel_out, int timeslot)
{ /* FAMOUS COMMAND 0x18 */
  
  BYTE msg[16];
  int i = 0;
  
  port_address_t addr_in = input_port->address;
  port_address_t addr_out = sub_output_port->address;

  /*if (alarms_board_is_in_anomaly(alarms, get_board_number_from_address(addr_in)) == 0) {

    vega_log(INFO, "IMPOSSIBLE CONN SC%d @ (P %d, SBI %d, B %d) <- TS%d <- (P %d, SBI %d, B %d) !!!",
	   (typechannel_out), 
	   sub_output_port->port_number, sub_output_port->sbi_number, sub_output_port->board_number,
	   timeslot,
	   input_port->port_number, input_port->sbi_number, input_port->board_number);
  
    printf("la carte %d est en anomalie ! Impossible d'effectuer une operation mixer_connect_input_to_sub_output()\n",
	   get_board_number_from_address(addr_in));
    return -1;
  }
  if (alarms_board_is_in_anomaly(alarms, get_board_number_from_address(addr_out)) == 0) {

    vega_log(INFO, "IMPOSSIBLE CONN SC%d @ (P %d, SBI %d, B %d) <- TS%d <- (P %d, SBI %d, B %d) !!!",
	   (typechannel_out), 
	   sub_output_port->port_number, sub_output_port->sbi_number, sub_output_port->board_number,
	   timeslot,
	   input_port->port_number, input_port->sbi_number, input_port->board_number);

    printf("la carte %d est en anomalie ! Impossible d'effectuer une operation mixer_connect_input_to_sub_output()\n",
	   get_board_number_from_address(addr_out));
    return -1;
  }*/


  msg[i++] = ICCMT_TS_2_UNITS_CONN;

  if (typechannel_out == sub_channel_0) 
    msg[i++] = 0x80;
  else if (typechannel_out == sub_channel_1)
    msg[i++] = 0x81;
  else if (typechannel_out == sub_channel_3)
    msg[i++] = 0x83;

  if (addr_in.size == 2) {
    msg[i++] = addr_in.byte1;
    msg[i++] = addr_in.byte2;
  }
  else {
    msg[i++] = addr_in.byte1;
    
  }
  
  /* place le timeslot */
  msg[i++] = timeslot;
 

  if (addr_out.size == 2) {
    msg[i++] = addr_out.byte1;
    msg[i++] = addr_out.byte2;
  }
  else {
    msg[i++] = addr_out.byte1;
 
  }
  
  msg[i] = 0x01 << get_port_number_from_address(addr_out);
  

  if (vega_control_send_alphacom_msg(VEGA_URGENT,pAlphaComLink, msg, i + 1) < 0)
    return -1;

  /* changer l'etat des mixers */
  vega_log(INFO, "CONN SC%d @ (P %d, SBI %d, B %d) <- TS%d <- (P %d, SBI %d, B %d)",
	   (typechannel_out), 
	   sub_output_port->port_number, sub_output_port->sbi_number, sub_output_port->board_number,
	   timeslot,
	   input_port->port_number, input_port->sbi_number, input_port->board_number);
	   
 /*vega_log(INFO, "addr_in(%d) %02X %02X addr_out(%d) %02X %02X", 
	 addr_in.size, addr_in.byte1, addr_in.byte2,
	 addr_out.size, addr_out.byte1, addr_out.byte2); */

  mixer_update_input_connection_state(input_port, timeslot, 1);
  mixer_update_sub_output_connection_state(sub_output_port, typechannel_out, 1, timeslot);
  
	
#ifdef __DEBUG_TIMESLOT__

  if ( timeslot>=0 && timeslot < MAX_TIMESLOT ) {
	array_connect_input_to_sub_output[timeslot].input_port = input_port; // store
	array_connect_input_to_sub_output[timeslot].sub_output_port = sub_output_port; // store
	array_connect_input_to_sub_output[timeslot].connected = 1; // store
	array_connect_input_to_sub_output[timeslot].subchannel = typechannel_out; // store
  }

  if ( timeslot>=0 && timeslot < MAX_TIMESLOT ) {
	  int B = get_board_number_from_address(sub_output_port->address);
	  int P = get_port_number_from_address(sub_output_port->address);
	  int SBI = get_sbi_number_from_address(sub_output_port->address);
	
	if ( sub_channel_0==typechannel_out  ){
		array_connect_timeslot_to_suboutput[B][SBI][P].sc0_ts = timeslot;
	}
	if ( sub_channel_1==typechannel_out  ){
		array_connect_timeslot_to_suboutput[B][SBI][P].sc1_ts = timeslot;
	}
	if ( sub_channel_3==typechannel_out  ){
		array_connect_timeslot_to_suboutput[B][SBI][P].sc3_ts = timeslot;
	}	  
  }
#endif

  return 0;
}

/*int board_display_busy(int board_number)
{
  // placer la carte en mode BUSY 
  BYTE msg[2];
  msg[0] = ICCMT_BOARD_SET_BUSY;
  msg[1] = board_number;
  
  if (vega_control_send_alphacom_msg(VEGA_URGENT,pAlphaComLink, msg, 2) < 0)
    return -1;
  return 0;
}*/

/* connect une sous sortie vers un timeslot */
int mixer_connect_timeslot_to_sub_output(vega_port_t *sub_output_port, int typechannel, int timeslot, int initial)
{ /* FAMOUS COMMAND 0x03 or 0x04 or 0x06 */

 

	if ( sub_channel_0==typechannel  && sub_output_port->mixer.sub_channel0.connected ){
		vega_log(INFO, "BUG CONN TS%d -> suboutput SC%d P%d SBI%d B%d (deja TS%d)",
			   timeslot,
			   typechannel, 
			   sub_output_port->port_number, sub_output_port->sbi_number, sub_output_port->board_number,
			   sub_output_port->mixer.sub_channel0.tslot
			   );

		mixer_disconnect_timeslot_to_sub_output(sub_output_port, sub_channel_0);
	}
	if ( sub_channel_1==typechannel && sub_output_port->mixer.sub_channel1.connected ){

		vega_log(INFO, "BUG CONN TS%d -> suboutput SC%d P%d SBI%d B%d (deja TS%d)",
			   timeslot,
			   typechannel, 
			   sub_output_port->port_number, sub_output_port->sbi_number, sub_output_port->board_number,
			   sub_output_port->mixer.sub_channel1.tslot
			   );

		mixer_disconnect_timeslot_to_sub_output(sub_output_port, sub_channel_1);
	}
	if ( sub_channel_3==typechannel && sub_output_port->mixer.sub_channel3.connected ){
		vega_log(INFO, "BUG CONN TS%d -> suboutput SC%d P%d SBI%d B%d (deja TS%d)",
			   timeslot,
			   typechannel, 
			   sub_output_port->port_number, sub_output_port->sbi_number, sub_output_port->board_number,
			   sub_output_port->mixer.sub_channel3.tslot
			   );
		mixer_disconnect_timeslot_to_sub_output(sub_output_port, sub_channel_3);
	}

  BYTE msg[16];
  int i = 0;
 
  port_address_t addr = sub_output_port->address;

  /*if (alarms_board_is_in_anomaly(alarms, get_board_number_from_address(addr)) == 0) {
    vega_log(INFO, "IMPOSSIBLE CONN TS%d -> suboutput SC%d P%d SBI%d B%d",
	     timeslot,
	     typechannel, 
	     sub_output_port->port_number, sub_output_port->sbi_number, sub_output_port->board_number);
    printf("la carte %d est en anomalie ! Impossible d'effectuer une operation mixer_connect_input_to_sub_output()\n",
	   get_board_number_from_address(addr));    
    return -1;
  }*/
  
  if (typechannel == sub_channel_0) 
    msg[i++] = ICCMT_TS_2_SC0_CONN;
  else if (typechannel == sub_channel_1)
    msg[i++] = ICCMT_TS_2_SC1_CONN;
  else if (typechannel == sub_channel_3)
    msg[i++] = ICCMT_TS_2_SC3_CONN;

  if (addr.size == 2) {
    msg[i++] = addr.byte1;
    msg[i] = addr.byte2;
  }
  else
    msg[i] = addr.byte1;
  i++;
  msg[i] = timeslot;
  /*if (initial == 0) {
    int board_number = get_board_number_from_address(addr);
    board_display_busy(board_number);
  }*/
  
  if (vega_control_send_alphacom_msg(VEGA_URGENT,pAlphaComLink, msg, i + 1) < 0)
    return -1;
  
  vega_log(INFO, "CONN TS%d -> suboutput SC%d P%d SBI%d B%d",
	   timeslot,
	   typechannel, 
	   sub_output_port->port_number, sub_output_port->sbi_number, sub_output_port->board_number);

  //vega_log(INFO, "addr(%d) %02X %02X ", addr.size, addr.byte1, addr.byte2); 

  mixer_update_sub_output_connection_state(sub_output_port, typechannel, 1, timeslot);

#ifdef __DEBUG_TIMESLOT__

  if ( timeslot>=0 && timeslot < MAX_TIMESLOT ) {
	  int B = get_board_number_from_address(addr);
	  int P = get_port_number_from_address(addr);
	  int SBI = get_sbi_number_from_address(addr);
	
	if ( sub_channel_0==typechannel  ){
		array_connect_timeslot_to_suboutput[B][SBI][P].sc0_ts = timeslot;
	}
	if ( sub_channel_1==typechannel  ){
		array_connect_timeslot_to_suboutput[B][SBI][P].sc1_ts = timeslot;
	}
	if ( sub_channel_3==typechannel  ){
		array_connect_timeslot_to_suboutput[B][SBI][P].sc3_ts = timeslot;
	}	  

  }
#endif

  return 0;
}



/* deconnexion d'une sous sortie d'un timeslot */
int mixer_disconnect_timeslot_to_sub_output(vega_port_t *sub_output_port, int typechannel)
{ /* FAMOUS COMMAND 0x09 0x0A or 0x0C */
  
	BYTE msg[16];
	int i = 0;
    port_address_t addr = sub_output_port->address;

	if ( sub_channel_0==typechannel  ){
		if ( !sub_output_port->mixer.sub_channel0.connected ){ // faut il deconnecter avant ?
				vega_log(ERROR,"BUG mixer_disconnect_timeslot_to_sub_output SC%d NOT CONNECTED !!!",typechannel);
				return -1;
		}
	}
	if ( sub_channel_1==typechannel  ){
		if ( !sub_output_port->mixer.sub_channel1.connected ){ // faut il deconnecter avant ?
				vega_log(ERROR,"BUG mixer_disconnect_timeslot_to_sub_output SC%d NOT CONNECTED !!!",typechannel);
				return -1;
		}
	}
	if ( sub_channel_3==typechannel  ){
		if ( !sub_output_port->mixer.sub_channel3.connected ){ // faut il deconnecter avant ?
				vega_log(ERROR,"BUG mixer_disconnect_timeslot_to_sub_output SC%d NOT CONNECTED !!!",typechannel);
				return -1;
		}
	}

#ifdef __DEBUG_TIMESLOT__

	{	// check that is has been connected before !!!!
		int B = get_board_number_from_address(addr);	  
		int P = get_port_number_from_address(addr);
		int SBI = get_sbi_number_from_address(addr);

		if ( sub_channel_0==typechannel  ){
			if ( INVALID_TIMESLOT == array_connect_timeslot_to_suboutput[B][SBI][P].sc0_ts ) {
				vega_log(ERROR,"BUG mixer_disconnect_timeslot_to_sub_output SC%d NOT CONNECTED !!!",typechannel);
				//return 0;
			}
		}
		else if ( sub_channel_1==typechannel  ){
			if ( INVALID_TIMESLOT ==array_connect_timeslot_to_suboutput[B][SBI][P].sc1_ts ) {
				vega_log(ERROR,"BUG mixer_disconnect_timeslot_to_sub_output SC%d NOT CONNECTED !!!",typechannel);
				//return 0;
			}
		}
		else if ( sub_channel_3==typechannel  ){	
			if ( INVALID_TIMESLOT ==array_connect_timeslot_to_suboutput[B][SBI][P].sc3_ts ) {
				//vega_log(ERROR,"BUG mixer_disconnect_timeslot_to_sub_output SC%d NOT CONNECTED !!!",typechannel);
				return 0;
			}
		}	  
	}
#endif
 

	if (typechannel == sub_channel_0) 
		msg[i++] = ICCMT_TS_2_SC0_DISC;
	else if (typechannel == sub_channel_1)
		msg[i++] = ICCMT_TS_2_SC1_DISC;
	else if (typechannel == sub_channel_3)
		msg[i++] = ICCMT_TS_2_SC3_DISC;

	if (addr.size == 2) {
		msg[i++] = addr.byte1;
		msg[i] = addr.byte2;
	}
	else
		msg[i] = addr.byte1;

	int timeslot=0;
	if (typechannel == sub_channel_0)
		timeslot = sub_output_port->mixer.sub_channel0.tslot;
	else if (typechannel == sub_channel_3)
		timeslot = sub_output_port->mixer.sub_channel3.tslot;
	else if (typechannel == sub_channel_1)
		timeslot = sub_output_port->mixer.sub_channel1.tslot;
	else 
		timeslot = -1;

	
	/*if (alarms_board_is_in_anomaly(alarms, get_board_number_from_address(addr)) == 0) {
	  
	  vega_log(INFO, "DISC TS%d -> suboutput SC%d @ P%d SBI%d B%d ",
		   timeslot,
		   typechannel, 
		   sub_output_port->port_number, sub_output_port->sbi_number, sub_output_port->board_number);
	  
	  printf("la carte %d est en anomalie ! Impossible d'effectuer une operation mixer_disconnect_timeslot_to_sub_output()\n",
		 get_board_number_from_address(addr));    

	  return -1;
	}*/
    
  
	if (vega_control_send_alphacom_msg(VEGA_URGENT,pAlphaComLink, msg, i + 1) < 0)
		return -1;




	vega_log(INFO, "DISC TS%d -> suboutput SC%d @ P%d SBI%d B%d ",
		timeslot,
	   typechannel, 
	   sub_output_port->port_number, sub_output_port->sbi_number, sub_output_port->board_number);

  
	//vega_log(INFO, "addr(%d) %02X %02X ", addr.size, addr.byte1, addr.byte2); 
	mixer_update_sub_output_connection_state(sub_output_port, typechannel, 0, -1);
  
#ifdef __DEBUG_TIMESLOT__
	if ( timeslot>=0 && timeslot < MAX_TIMESLOT ) 
	{
		int B = get_board_number_from_address(addr);
		int P = get_port_number_from_address(addr);	  
		int SBI = get_sbi_number_from_address(addr);	

		if ( sub_channel_0==typechannel  ){
			array_connect_timeslot_to_suboutput[B][SBI][P].sc0_ts = INVALID_TIMESLOT;
		}
		if ( sub_channel_1==typechannel  ){
			array_connect_timeslot_to_suboutput[B][SBI][P].sc1_ts = INVALID_TIMESLOT;
		}
		if ( sub_channel_3==typechannel  ){	
			array_connect_timeslot_to_suboutput[B][SBI][P].sc3_ts = INVALID_TIMESLOT;
		}	  
  
	}
#endif
	
	return 0;
}



/* Input disconnect: */

/* ·          icc_msg(1) = 0x02 (ICCMT_UNIT_2_TS_DISC) */

/* ·           if (input on SBI0) */
/* icc_msg(2)=bbbbbccc */
/* else */
/* icc_msg(2)=00000sss */
/* icc_msg(3)=bbbbbccc */
/* endif */

/* deconnexion d'une sous sortie d'un timeslot */
/* When an input is disconnected ALL outputs which are tapping that timeslot must also be disconnected, even the outputs on other cards. */

/*int mixer_disconnect_timeslot_to_input(vega_port_t *input_port)
{
  BYTE msg[16];
  int i = 0;
  
  port_address_t addr = input_port->address;
  
  msg[i++] = ICCMT_UNIT_2_TS_DISC;

  if (addr.size == 2) {
    msg[i++] = addr.byte1;
    msg[i++] = addr.byte2;
  }
  else
    msg[i++] = addr.byte1;
  
  if (vega_control_send_alphacom_msg(VEGA_URGENT,pAlphaComLink, msg, i ) < 0)
    return -1;


  vega_log(INFO, "disconnect input@ port %d, sbi %d, board number %d TIMESLOT %d", 
	   input_port->port_number,	input_port->sbi_number, input_port->board_number, input_port->mixer.input.tslot);
  

  // rechercher pour les confs ou les postes abonnés connectés, les timeslot utilisés dans les sous sorties sur
  //   ce timeslot. Si on trouve, on fait la deconnection.
   
  mixer_update_input_connection_state(input_port, -1, 0);
  

  return 0;
}*/

/*on connecte une entrée à une timeslot : on utile sub channel 3 du 5 ieme port de l'AGA 24 comme sous sortie temporaire
 1 - mixer_connect_input_to_sub_output 
 2 - mixer_disconnect_timeslot_to_sub_output
*/
int mixer_connect_timeslot_to_input(vega_port_t *input_port,  int timeslot)
{
  static vega_port_t sub_output_port;

  /* port 5 de l'AGA 24 */
  sub_output_port.address = get_port_address(24, 0, 4);
  sub_output_port.board_number = 24;
  sub_output_port.sbi_number = 0;
  sub_output_port.port_number = 4;

  
  sub_channel_t subchannel_type = sub_channel_3;
  
  if (mixer_connect_input_to_sub_output(input_port, &sub_output_port, sub_channel_3, timeslot))
    return -1;
  
/*   vega_log(INFO, "connect input @ port %d, sbi %d, board %d TIMESLOT %d", */
/* 	   input_port->port_number,	input_port->sbi_number, input_port->board_number, timeslot); */


  /* on deconnecte la sous sortie 3 @ AGA 24 du timeslot timeslot */
  if (mixer_disconnect_timeslot_to_sub_output(&sub_output_port,  sub_channel_3) < 0)
    return -1;

  /* TODO : sauvegarder pour le input le timeslot connecté !! */
  mixer_update_input_connection_state(input_port, timeslot, 1);
  
  return 0;
}

/* retourne 0 si aucune sous entrée n'est connectée à un timeslot */
int mixer_all_disconnected(vega_physical_mixer_t *mixer)
{
	if (mixer->sub_channel1.connected == 0 && mixer->sub_channel0.connected == 0 && mixer->sub_channel3.connected == 0) return 0;
	return 1;
}


int mixer_update_input_connection_state(vega_port_t *p, int ts, int connected)
{
	p->mixer.input.connected = connected;
	p->mixer.input.tslot = ts;
	return 0;
};

int mixer_update_sub_output_connection_state(vega_port_t *p, int number, int connected, int ts)
{
  if (number == sub_channel_0) {
    p->mixer.sub_channel0.connected = connected;
    p->mixer.sub_channel0.tslot = ts;
  }
  if (number == sub_channel_1) {
    p->mixer.sub_channel1.connected = connected;
    p->mixer.sub_channel1.tslot = ts;
  }
  if (number == sub_channel_3) {
    p->mixer.sub_channel3.connected = connected;
    p->mixer.sub_channel3.tslot = ts;
  }
  return 0;
}
