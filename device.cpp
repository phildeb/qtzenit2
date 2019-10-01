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
#include "ini.h"
#include "mixer.h"
#include "messages.h"
#include "misc.h"

static pthread_mutex_t mutex_state_transitions;
static QLinkedList<event_t *> qlist_events;
static QMutex qlist_events_mutex;


QMap<int /*board number*/,bool /*is_good*/> map_boards;



bool CRM4::start_hearing_in_aslt(vega_conference_t* C)
{
	if ( this->has_hardware_error() ) return false;
	if ( C && C->has_hardware_error() ) { // pdl 20091005
		vega_log(INFO, "C%d HARDWARE ERROR !!!",C->number);
		device_line_printf(4,"C%d HARDWARE error....",C->number);
		return false;
	}

	CRM4 *d=this;
	int ts_value = -1; // todo: cas d'un directeur qui dirige plusieurs conferences !

	if( NULL==C->matrix_configuration ) C->matrix_configuration = &vega_conference_t::configuration_conferences[C->number-1];//config;

	if ( d == C->participant_director.device ){ 
		// branchement sur port AGA associe du ts de cette conference:
		ts_value = C->matrix_configuration->ts_start + 5; // +5 TS DIRECTOR
		vega_log(INFO,"start_hearing_in_aslt DIRECTOR in C%d TS %d",C->number, ts_value);// => uniquement vert, sinon rouge et vert");
	}else{
		ts_value = C->matrix_configuration->ts_start + 8;
		vega_log(INFO,"start_hearing_in_aslt SECONDARY in C%d  TS %d",C->number, ts_value);// => uniquement vert, sinon rouge et vert");
	}
	/* connecter le timeslot de la sous sortie 1 du mixer ASLT du poste vers le timeslot mix conf de la conf */
	if ( port_aslt.mixer.sub_channel1.connected ){ // faut il deconnecter avant ?
		mixer_disconnect_timeslot_to_sub_output(&port_aslt, sub_channel_1);
	}
	mixer_connect_timeslot_to_sub_output(&port_aslt, sub_channel_1, ts_value, 0);

	// todo: verifier qu'on n'ecoute pas la conference sur un subchannel de notre AGA associe
	return true;
}

bool CRM4::start_hearing_in_aslt_no_echo(vega_conference_t* C, int ts_OFFSET)
{
	if ( this->has_hardware_error() ) return false;
	//if( NULL==C->matrix_configuration ) C->matrix_configuration = &vega_conference_t::configuration_conferences[C->number-1];//config;
	if ( C && C->has_hardware_error() ) { // pdl 20091005
		vega_log(INFO, "C%d HARDWARE ERROR !!!",C->number);
		device_line_printf(4,"C%d HARDWARE error....",C->number);
		return -1;
	}

	int ts = C->matrix_configuration->ts_start + ts_OFFSET;
	if ( port_aslt.mixer.sub_channel1.connected ){ // faut il deconnecter avant ?
		mixer_disconnect_timeslot_to_sub_output(&port_aslt, sub_channel_1);
	}
	mixer_connect_timeslot_to_sub_output(&port_aslt, sub_channel_1, ts, 0);
	return true;
}

int CRM4::director_start_speaking(vega_conference_t *c)
{
	if ( this->has_hardware_error() ) return false;
	//if( NULL==c->matrix_configuration ) c->matrix_configuration = &vega_conference_t::configuration_conferences[c->number-1];//config;
	if ( c && c->has_hardware_error() ) { // pdl 20091005
		vega_log(INFO, "C%d HARDWARE ERROR !!!",c->number);
		device_line_printf(4,"C%d HARDWARE error....",c->number);
		return -1;
	}

	//int ts_value = FIRST_DEVICE_CRM4_INPUT_TS;
	//ts_value += port_aslt.port_number; /* on recupere le timeslot pour le micro du poste directeur */
	//ts_value += (get_board_number_from_address(port_aslt.address) - 1 ) * 6;
	{
		int ts_value=m_timeslot_micro;
		
		vega_log(INFO, "<<<< DIRECTOR D%d START speaking TS%d", number, ts_value);
	
		/* connecte la sous sortie 3 du mixer 8 vers le micro du directeur  (2nd poste secondaire)*/
		mixer_connect_timeslot_to_sub_output(&c->ports[7], sub_channel_1, ts_value, 0);
		/* connecte la sous sortie 3 du mixer 7 vers le micro du directeur  (1er poste secondaire)*/
		mixer_connect_timeslot_to_sub_output(&c->ports[6], sub_channel_1, ts_value, 0);
		/* connecte la sous sortie 0 */
		mixer_connect_timeslot_to_sub_output(&c->ports[1], sub_channel_0, ts_value, 0); 	// // access 4,5,6
      }
	  return 0;
}

int CRM4::director_stop_speaking(vega_conference_t *c)
{
	if ( this->has_hardware_error() ) return false;
	//if( NULL==c->matrix_configuration ) c->matrix_configuration = &vega_conference_t::configuration_conferences[c->number-1];//config;
	if ( c && c->has_hardware_error() ) { // pdl 20091005
		vega_log(INFO, "C%d HARDWARE ERROR !!!",c->number);
		device_line_printf(4,"C%d HARDWARE error....",c->number);
		return -1;
	}

	CRM4 *d=this;

	  {
		
		vega_log(INFO, "<<<< DIRECTOR D%d STOP speaking TS%d", d->number, m_timeslot_micro);

		/* deconnecte la sous sortie 3 du mixer 8 vers le micro du directeur  (2nd poste secondaire)*/
		mixer_disconnect_timeslot_to_sub_output(&c->ports[7], sub_channel_1);
		/* deconnecte la sous sortie 3 du mixer 7 vers le micro du directeur  (1er poste secondaire)*/
		mixer_disconnect_timeslot_to_sub_output(&c->ports[6], sub_channel_1);
		/* deconnecte la sous sortie 0 */
		mixer_disconnect_timeslot_to_sub_output(&c->ports[1], sub_channel_0); 	// // access 4,5,6
		
	  }
	  return 0;
}


int CRM4::secondary_start_stop_speaking(vega_conference_t *c, port_reservation_t res_type,  int enable)
{
	if ( this->has_hardware_error() ) return false;
	if ( NULL==c ) return -1;
	//if( NULL==c->matrix_configuration ) c->matrix_configuration = &vega_conference_t::configuration_conferences[c->number-1];//config;
	if ( c && c->has_hardware_error() ) { // pdl 20091005
		vega_log(INFO, "C%d HARDWARE ERROR !!!",c->number);
		device_line_printf(4,"C%d HARDWARE error....",c->number);
		return -1;
	}

	CRM4 *d=this;
	switch (res_type)
	{
  
	default:
		vega_log(INFO, "<<<< D%d UNHANDLED res type %d secondary_start_stop_speaking !!!", d->number, res_type);	  
		break;

	case PORT_RESERVED_SECONDARY1:
		dump_mixers();
		vega_log(INFO, "secondary_start_stop_speaking D%d (enable:%d) PORT_RESERVED_SECONDARY1", d->number,enable);
		if (enable== 1) 
		{
		
			//int ts_value = FIRST_DEVICE_CRM4_INPUT_TS	;
			//ts_value += d->port_aslt.port_number;
			//ts_value += (get_board_number_from_address(d->port_aslt.address) - 1 ) * 6;
			int ts_value = m_timeslot_micro;

			// 1-TIMESLOT of micro of 1st secondary that want to talk into conference !!!
			mixer_connect_timeslot_to_sub_output(&c->ports[1], sub_channel_1, ts_value, 0);	// access 4,5,6
			/* mous 2009 02 26 : connexion du ts du micro du poste secondaire 1 vers le sub channel 1 du mixer du directeur (sur le port 6 du schema)  */
			mixer_connect_timeslot_to_sub_output(&c->ports[5], sub_channel_1, ts_value, 0);	// send my microphone to the DIRECTOR
			mixer_connect_timeslot_to_sub_output(&c->ports[7], sub_channel_3, ts_value, 0);   // send my microphone to the other secondary

			// 2-brancher son port d'ecoute de notre conference ASLT sub_channel_1 !!!
			/* mous 2009 02 26 pouvoir ecouter sa propre conference pendant un appui micro  */
			if (stop_hearing_in_aslt(c)) { 
				start_hearing_in_aslt_no_echo(c, 6);
				//stop_hearing_in_aga(c);
			}else if (stop_hearing_in_aga(c)){ 
				start_hearing_in_aga_no_echo(c, 6);
				//stop_hearing_in_aslt(c);
			}


		}else {
			mixer_disconnect_timeslot_to_sub_output(&c->ports[1], sub_channel_1);
			mixer_disconnect_timeslot_to_sub_output(&c->ports[5], sub_channel_1);
			mixer_disconnect_timeslot_to_sub_output(&c->ports[7], sub_channel_3);// the other secondary

			if (stop_hearing_in_aslt(c)) { start_hearing_in_aslt(c);}
			else if (stop_hearing_in_aga(c)){ start_hearing_in_aga(c);}

		}

		dump_mixers();

		break;

  case PORT_RESERVED_SECONDARY2:

     
	  if (enable== 1)
	  {
		vega_log(INFO, "secondary_start_stop_speaking D%d (enable:%d) PORT_RESERVED_SECONDARY2", d->number,enable);

			/* on recupere le timeslot pour le micro du poste */
			int ts_value = m_timeslot_micro;

			mixer_connect_timeslot_to_sub_output(&c->ports[1], sub_channel_3, ts_value, 0);	// // access 4,5,6
			/* mous 2009 02 26 : connexion du ts du micro du poste secondaire 1 vers le sub channel 3 du mixer du directeur (sur le port 6 du schema)  */
			mixer_connect_timeslot_to_sub_output(&c->ports[5], sub_channel_3, ts_value, 0);	// send my microphone to the DIRECTOR
			mixer_connect_timeslot_to_sub_output(&c->ports[6], sub_channel_3, ts_value, 0);   // send my microphone to the other secondary


			if (stop_hearing_in_aslt(c)) { 
				start_hearing_in_aslt_no_echo(c, 7);
				stop_hearing_in_aga(c);
			}else if (stop_hearing_in_aga(c)){ 
				start_hearing_in_aga_no_echo(c, 7);
				stop_hearing_in_aslt(c);
			}

	  }else {
      	
			mixer_disconnect_timeslot_to_sub_output(&c->ports[1], sub_channel_3);
			mixer_disconnect_timeslot_to_sub_output(&c->ports[5], sub_channel_3);
			mixer_disconnect_timeslot_to_sub_output(&c->ports[6], sub_channel_3);// the other secondary

			if (stop_hearing_in_aslt(c)) { start_hearing_in_aslt(c);}
			else if (stop_hearing_in_aga(c)){ start_hearing_in_aga(c);}
	  }
	  break;

  case PORT_RESERVED_JUPITER2: 
 
	  if (enable == 1) 
	  {
			vega_log(INFO, "<<<< D%d SQUEEZE JUPITER ROOM IN CONFERENCE !!!", d->number);

			/* timeslot of CRM4 connected to CONF AGA 0 2 and 3 as if it was a jupiter ! */
			int ts_value = m_timeslot_micro;

			mixer_connect_timeslot_to_sub_output(&c->ports[0], sub_channel_3, ts_value, 0); // AGA mix 1,2,3
			mixer_connect_timeslot_to_sub_output(&c->ports[2], sub_channel_3, ts_value, 0); // AGA Radio1
			mixer_connect_timeslot_to_sub_output(&c->ports[3], sub_channel_3, ts_value, 0); // AGA Jup2

			// cf schema p48: on connecte l'input du mixer AGA CONF JUPITER2 vers le speaker du CRM4: 31 + 4 = 35 
			//start_hearing_in_aslt_no_echo(c, 4);
			if (stop_hearing_in_aslt(c)) { 
				start_hearing_in_aslt_no_echo(c, 4);
				stop_hearing_in_aga(c);
			}else if (stop_hearing_in_aga(c)){ 
				start_hearing_in_aga_no_echo(c, 4);
				stop_hearing_in_aslt(c);
			}


	  }else{	
			vega_log(INFO, "<<<< D%d FREE JUPITER ROOM IN CONFERENCE !!!", d->number);

			mixer_disconnect_timeslot_to_sub_output(&c->ports[0], sub_channel_3); // AGA mix 1,2,3
			mixer_disconnect_timeslot_to_sub_output(&c->ports[2], sub_channel_3); // AGA Radio1
			mixer_disconnect_timeslot_to_sub_output(&c->ports[3], sub_channel_3); // AGA Radio2

			if (stop_hearing_in_aslt(c)) { start_hearing_in_aslt(c);}
			else if (stop_hearing_in_aga(c)){ start_hearing_in_aga(c);}

	  }
	  break; 

  

  case PORT_RESERVED_RADIO1: 
		if (enable == 1) 
		{
			vega_log(INFO, "<<<< D%d SQUEEZE RADIO1 ROOM IN CONFERENCE !!!", d->number);

			/* timeslot of CRM4 connected to CONF AGA 0 3 and 4 as if it was radio1 ! */
			int ts_value = m_timeslot_micro;

			vega_log(INFO,"ENABLE PORT_RESERVED_RADIO1 D%d TS %d",d->number, ts_value);

			mixer_connect_timeslot_to_sub_output(&c->ports[0], sub_channel_0, ts_value, 0);
			mixer_connect_timeslot_to_sub_output(&c->ports[3], sub_channel_1, ts_value, 0);
			mixer_connect_timeslot_to_sub_output(&c->ports[4], sub_channel_1, ts_value, 0);

			//start_hearing_in_aslt_no_echo(c, 2);
			if (stop_hearing_in_aslt(c)) { 
				start_hearing_in_aslt_no_echo(c, 2);
				stop_hearing_in_aga(c);
			}else if (stop_hearing_in_aga(c)){ 
				start_hearing_in_aga_no_echo(c, 2);
				stop_hearing_in_aslt(c);
			}

		}else{	
		  
			vega_log(INFO, "<<<< D%d FREE RADIO1 ROOM IN CONFERENCE !!!", d->number);

			mixer_disconnect_timeslot_to_sub_output(&c->ports[0], sub_channel_0); // AGA mix 1,2,3
			mixer_disconnect_timeslot_to_sub_output(&c->ports[3], sub_channel_1); // AGA Radio1
			mixer_disconnect_timeslot_to_sub_output(&c->ports[4], sub_channel_1); // AGA Radio2

			if (stop_hearing_in_aslt(c)) { start_hearing_in_aslt(c);}
			else if (stop_hearing_in_aga(c)){ start_hearing_in_aga(c);}


		
		}
	  break;


  case PORT_RESERVED_RADIO2: 
	  if (enable == 1) 
	  {
			vega_log(INFO, "<<<< D%d SQUEEZE RADIO2 ROOM IN C%d !!!", d->number, c->number);
			/* timeslot of CRM4 connected to CONF AGA 0 2 and 4 as if it was radio2 ! */
			int ts_value = m_timeslot_micro;


			vega_log(INFO,"ENABLE PORT_RESERVED_RADIO2 D%d TS %d",d->number, ts_value);
			mixer_connect_timeslot_to_sub_output(&c->ports[0], sub_channel_1, ts_value, 0); // AGA mix 1,2,3
			mixer_connect_timeslot_to_sub_output(&c->ports[2], sub_channel_1, ts_value, 0); // AGA Radio1
			mixer_connect_timeslot_to_sub_output(&c->ports[4], sub_channel_3, ts_value, 0); // AGA Jup2

			//start_hearing_in_aslt_no_echo(c, 3);
			if (stop_hearing_in_aslt(c)) { 
				start_hearing_in_aslt_no_echo(c, 3);
				stop_hearing_in_aga(c);
			}else if (stop_hearing_in_aga(c)){ 
				start_hearing_in_aga_no_echo(c, 3);
				stop_hearing_in_aslt(c);
			}

	  }else{	
		  
			vega_log(INFO, "<<<< D%d FREE RADIO2 ROOM IN C%d !!!", d->number, c->number);

			mixer_disconnect_timeslot_to_sub_output(&c->ports[0], sub_channel_1); // AGA mix 1,2,3
			mixer_disconnect_timeslot_to_sub_output(&c->ports[2], sub_channel_1); // AGA Radio1
			mixer_disconnect_timeslot_to_sub_output(&c->ports[4], sub_channel_3); // AGA Radio2

			if (stop_hearing_in_aslt(c)) { start_hearing_in_aslt(c);}
			else if (stop_hearing_in_aga(c)){ start_hearing_in_aga(c);}

	  
	  }
	  break;

	}// switch
	return 0;
}

/*
void CRM4::avoid_hear_myself_in_aga_3conf( vega_conference_t *c)
{
	// est ce qu'un des timeslots de la conference est deja dans le mixeur 3 conf AGA associe ?
	if ( port_3confs.mixer.sub_channel0.connected )
	{
		if ( c->timeslot_belongs_t_conference( port_3confs.mixer.sub_channel0.tslot ) ){
			mixer_disconnect_timeslot_to_sub_output( &port_3confs, sub_channel_0);
		}
	}
	else if ( port_3confs.mixer.sub_channel1.connected )
	{
		if ( c->timeslot_belongs_t_conference( port_3confs.mixer.sub_channel1.tslot ) ){
			mixer_disconnect_timeslot_to_sub_output(&port_3confs, sub_channel_1);
		}
	}
	else if ( port_3confs.mixer.sub_channel3.connected )
	{
		if ( c->timeslot_belongs_t_conference( port_3confs.mixer.sub_channel3.tslot ) ){
			mixer_disconnect_timeslot_to_sub_output(&port_3confs, sub_channel_3);
		}		
	}
}
*/
bool CRM4::stop_hearing_in_aslt(vega_conference_t* c)
{
	if ( this->has_hardware_error() ) return false;
	//if( NULL==c->matrix_configuration ) c->matrix_configuration = &vega_conference_t::configuration_conferences[c->number-1];//config;
	if ( c && c->has_hardware_error() ) { // pdl 20091005
		vega_log(INFO, "C%d HARDWARE ERROR !!!",c->number);
		device_line_printf(4,"C%d HARDWARE error....",c->number);
		return false;
	}

	// verifier si le timeslot de la nouvelle conf n est pas deja sur un des 3 SC du portAGA associe ?
	if ( port_aslt.mixer.sub_channel1.connected &&
	   ( c->timeslot_belongs_t_conference( port_aslt.mixer.sub_channel1.tslot ) ) ){
		mixer_disconnect_timeslot_to_sub_output(&port_aslt, sub_channel_1);
		vega_log(INFO, "YES: stop_hearing_in_aslt: C%d",c->number);
		return true;
	}
	vega_log(INFO, "NO: stop_hearing_in_aslt: C%d",c->number);
	return false;
}

bool CRM4::stop_hearing_in_aga(vega_conference_t* c)
{
	if ( this->has_hardware_error() ) return false;
	//if( NULL==c->matrix_configuration ) c->matrix_configuration = &vega_conference_t::configuration_conferences[c->number-1];//config;
	if ( c && c->has_hardware_error() ) { // pdl 20091005
		vega_log(INFO, "C%d HARDWARE ERROR !!!",c->number);
		device_line_printf(4,"C%d HARDWARE error....",c->number);
		return false;
	}

	bool ret=false;
	// verifier si le timeslot de la nouvelle conf n est pas deja sur un des 3 SC du portAGA associe ?
	if ( port_3confs.mixer.sub_channel0.connected &&
	   ( c->timeslot_belongs_t_conference( port_3confs.mixer.sub_channel0.tslot ) ) ){
		mixer_disconnect_timeslot_to_sub_output(&port_3confs, sub_channel_0);
		vega_log(INFO, "YES: stop_hearing_in_aga SC0: C%d",c->number);
		ret = true;
	}else
		vega_log(INFO, "D%d: stop_hearing_in_aga SC0: C%d",number,c->number);
	if ( port_3confs.mixer.sub_channel1.connected &&
	   ( c->timeslot_belongs_t_conference( port_3confs.mixer.sub_channel1.tslot ) ) ){		
		mixer_disconnect_timeslot_to_sub_output(&port_3confs, sub_channel_1);
		vega_log(INFO, "YES: stop_hearing_in_aga SC1: C%d",c->number);
		ret = true;
	}else
		vega_log(INFO, "D%d: stop_hearing_in_aga SC1: C%d",number,c->number);

	if ( port_3confs.mixer.sub_channel3.connected &&
	   ( c->timeslot_belongs_t_conference( port_3confs.mixer.sub_channel3.tslot ) ) ){		
		mixer_disconnect_timeslot_to_sub_output(&port_3confs, sub_channel_3);
		vega_log(INFO, "YES: stop_hearing_in_aga SC3: C%d",c->number);
		ret = true;
	}else
		vega_log(INFO, "D%d: stop_hearing_in_aga SC3: C%d",number,c->number);

	//if ( false==ret ) vega_log(INFO, "NO: stop_hearing_in_aga : C%d",c->number);

	return ret;
}

bool CRM4::start_hearing_in_aga_no_echo(vega_conference_t* C, int ts_OFFSET)
{
	if ( this->has_hardware_error() ) return false;
	//if( NULL==C->matrix_configuration ) C->matrix_configuration = &vega_conference_t::configuration_conferences[C->number-1];//config;
	if ( C && C->has_hardware_error() ) { // pdl 20091005
		vega_log(INFO, "C%d HARDWARE ERROR !!!",C->number);
		device_line_printf(4,"C%d HARDWARE error....",C->number);
		return false;
	}

	int ts = C->matrix_configuration->ts_start + ts_OFFSET;
	vega_log(INFO, "start_hearing_in_aga_no_echo: C%d TS %d",C->number, ts);

	if (! port_3confs.mixer.sub_channel3.connected) {
		mixer_connect_timeslot_to_sub_output(&port_3confs, sub_channel_3, ts, 0);
	}else{
		if (!port_3confs.mixer.sub_channel1.connected) 
			mixer_connect_timeslot_to_sub_output(&port_3confs, sub_channel_1, ts, 0);
		else{
			if (!port_3confs.mixer.sub_channel0.connected) 
				mixer_connect_timeslot_to_sub_output(&port_3confs, sub_channel_0, ts, 0);
			else{
				vega_log(INFO, "start_hearing_in_aga_no_echo: NO AGA RESOURCE AVAILABLE %d for TS %d",number, ts);
			}
		}
	}
}

bool CRM4::start_hearing_in_aga(vega_conference_t* c)
{	// NORMALLY MUST BE already hearing a conference in its aslt mixer...
	if ( this->has_hardware_error() ) return false;
	if ( c && c->has_hardware_error() ) { // pdl 20091005
		vega_log(INFO, "C%d HARDWARE ERROR !!!",c->number);
		device_line_printf(4,"C%d HARDWARE error....",c->number);
		return false;
	}

	CRM4 *d=this;
	int ts_value = -1; // todo: cas d'un directeur qui dirige plusieurs conferences !
	if ( d == c->participant_director.device ) // a secondary may have initiated the conf where we are director !
	{
		ts_value = c->matrix_configuration->ts_start + 5; // +5 TS DIRECTOR
		vega_log(INFO,"start_hearing_in_aga DTOR in C%d TS %d",c->number, ts_value);// => uniquement vert, sinon rouge et vert");
	}else{
		ts_value = c->matrix_configuration->ts_start + 8;
		vega_log(INFO,"start_hearing_in_aga SECONDARY in C%d TS %d",c->number, ts_value);// => uniquement vert, sinon rouge et vert");
	}

	// verifier si le timeslot de la nouvelle conf n est pas deja sur un des 3 SC du portAGA associe ?
	if ( port_3confs.mixer.sub_channel0.connected &&
	   ( c->timeslot_belongs_t_conference( port_3confs.mixer.sub_channel0.tslot ) ) ){		
		vega_log(INFO, "start_hearing_in_aga: YET HEARING SC1 TS %d",number, ts_value);
	}
	else if ( port_3confs.mixer.sub_channel1.connected &&
	   ( c->timeslot_belongs_t_conference( port_3confs.mixer.sub_channel1.tslot ) ) ){		
		vega_log(INFO, "start_hearing_in_aga: YET HEARING SC1 TS %d",number, ts_value);
	}
	else if ( port_3confs.mixer.sub_channel3.connected &&
	   ( c->timeslot_belongs_t_conference( port_3confs.mixer.sub_channel3.tslot ) ) ){	
		vega_log(INFO, "start_hearing_in_aga: YET HEARING SC3 TS %d",number, ts_value);
	}
	else{
		// commence au hasard par le 3eme subchannel du port aga
		if (! port_3confs.mixer.sub_channel3.connected) {
			mixer_connect_timeslot_to_sub_output(&port_3confs, sub_channel_3, ts_value, 0);
		}else{
			if (!port_3confs.mixer.sub_channel1.connected) 
				mixer_connect_timeslot_to_sub_output(&port_3confs, sub_channel_1, ts_value, 0);
			else{
				if (!port_3confs.mixer.sub_channel0.connected) 
					mixer_connect_timeslot_to_sub_output(&port_3confs, sub_channel_0, ts_value, 0);
				else{
					vega_log(INFO, "start_hearing_in_aga: NO AGA RESOURCE AVAILABLE %d for TS %d",number, ts_value);
					vega_log(INFO, "start_hearing_in_aga: NO AGA RESOURCE AVAILABLE %d for TS %d",number, ts_value);
					vega_log(INFO, "start_hearing_in_aga: NO AGA RESOURCE AVAILABLE %d for TS %d",number, ts_value);
					vega_log(INFO, "start_hearing_in_aga: NO AGA RESOURCE AVAILABLE %d for TS %d",number, ts_value);
					vega_log(INFO, "start_hearing_in_aga: NO AGA RESOURCE AVAILABLE %d for TS %d",number, ts_value);
					vega_log(INFO, "start_hearing_in_aga: NO AGA RESOURCE AVAILABLE %d for TS %d",number, ts_value);
					vega_log(INFO, "start_hearing_in_aga: NO AGA RESOURCE AVAILABLE %d for TS %d",number, ts_value);
					vega_log(INFO, "start_hearing_in_aga: NO AGA RESOURCE AVAILABLE %d for TS %d",number, ts_value);
					vega_log(INFO, "start_hearing_in_aga: NO AGA RESOURCE AVAILABLE %d for TS %d",number, ts_value);
					vega_log(INFO, "start_hearing_in_aga: NO AGA RESOURCE AVAILABLE %d for TS %d",number, ts_value);
					return false;
				}
			}
		}
	}
	return true;
}

//device_create_CRM4
CRM4 *CRM4::create(int device_number, const char* a_name, int a_gain)
{
	CRM4 *d = NULL;	
	if ( is_valid_device_number(device_number) ) {
		d = new CRM4(device_number);
	}
	else
	{
	  vega_log(VEGA_LOG_CRITICAL, "cannot allocate device");
	  return NULL;
	}

	device_t::qlist_devices.append(d); // CREATE CRM4

	d->number = device_number;

	//if ( name ) strncpy(d->name, a_name, sizeof(d->name) -1);
	if ( a_name  && a_name[0] ) strncpy_only_digit_az_AZ(d->name, a_name, sizeof(d->name) -1);
	else snprintf(d->name, sizeof(d->name) -1, "CRM4-%02d",d->number);
	
	if (strlen(d->name) <= 1) snprintf(d->name, sizeof(d->name) -1, "CRM4-%02d",d->number);

	d->gain = a_gain;
	d->activating_conference = d->excluding_including_conference = d->self_excluding_including_conference = 0;

	gettimeofday(&d->_tstart_btn_micro, NULL);
	gettimeofday(&d->_tstart_btn_device, NULL);
	gettimeofday(&d->_tstart_btn_conf, NULL);


	vega_log(DEBUG, "##################### device_create D%d name:%s %p TS micro: %d", device_number, d->name, d, d->m_timeslot_micro);

	vega_alarm_log(EV_ALARM_DEVICE_ANOMALY,  d->number , d->number, "debut alarme station %s", d->name); 
	// simuler une alarme qui disparaitra lors de la detection de la station
	
	return d;
}

//device_create_RADIO
Radio* Radio::create(int device_number, const char* name, int gain)
{
	Radio *d = NULL;	
	if ( is_valid_device_number(device_number) ) {
		d = new Radio(device_number);
	}
	else
	{
	  vega_log(VEGA_LOG_CRITICAL, "cannot allocate device");
	  return NULL;
	}

	device_t::qlist_devices.append(d); // CREATE RADIO


  d->number = device_number;
  if ( name ) strncpy(d->name, name, sizeof(d->name) -1);
  else snprintf(d->name, sizeof(d->name) -1, "RADIO-%02d",d->number);


  vega_log(DEBUG, "##################### device_create D%d name:%s %p TS%d micro", device_number, d->name, d , d->m_timeslot_micro);
  return d;
}

bool CRM4::has_hardware_error()
{
	if ( false==map_boards[port_aslt.board_number] ) {
		vega_log(INFO, "CRM4 hardware ASLT error",number);
		return true;
	}
	if ( false==map_boards[port_3confs.board_number] ) {
		vega_log(INFO, "CRM4 hardware AGA error",number);
		return true;
	}
	return false;
}


bool Radio::has_hardware_error()
{
	if ( false==map_boards[port_aga_emission.board_number] ) {
		vega_log(INFO, "CRM4 hardware AGA emission error",number);
		return true;
	}
	if ( false==map_boards[port_aga_reception.board_number] ) {
		vega_log(INFO, "CRM4 hardware AGA reception error",number);
		return true;
	}
	return false;
}




/*
int CRM4::device_set_led_red_and_green(int key, T_LED_MODE mode_red, char status_red, T_LED_MODE mode_green, char status_green)
{
  int i = 0;
  BYTE msg[256];

  msg[i++] = 0x43;
  msg[i++] = 0x41;
  msg[i++] = port_aslt.address.byte1;
	/////////////////////////////////// RED /////////////////////////////////////
  {	//RED LED_MODE_SLOW_BLINK
	  unsigned short kis;
	  BYTE kis1, kis2;
	  BYTE tmp;

	  kis = 0x0000;
	  key = key -1;         // to start with 0
	  key = key & 0x03FF;	// to cut the 10 LSB
	  key = key << 6;
	  kis = kis | key;		//set the key bits to 1
	  // select the color
	  tmp = LED_COLOR_RED;
	  tmp = tmp << 3;		// go to the 11th bit
	  kis = kis | tmp;
	  // select the type of display
	  tmp = mode_red;//LED_MODE_SLOW_BLINK;
	  tmp = tmp << 1;
	  kis = kis | tmp;

	  tmp = status_red;
	  kis = kis | tmp;
	  kis1 = (unsigned char)(kis >> 8);
	  kis2 = (unsigned char)(kis & 0x00FF);

	  msg[i++] = kis1;
	  msg[i++] = kis2;
  }	  
 
  msg[i++] = 0x41;
  msg[i++] = port_aslt.address.byte1;
	///////////////////////////////////////// GREEN ////////////////////////////////////////
  {	//GREEN LED_MODE_SLOW_BLINK 
	  unsigned short kis;
	  BYTE kis1, kis2;
	  BYTE tmp;


	  kis = 0x0000;
	  key = key -1;         // to start with 0
	  key = key & 0x03FF;	// to cut the 10 LSB
	  key = key << 6;
	  kis = kis | key;		//set the key bits to 1
	  // select the color
	  tmp = LED_COLOR_GREEN;
	  tmp = tmp << 3;		// go to the 11th bit
	  kis = kis | tmp;
	  // select the type of display
	  tmp = mode_green;//LED_MODE_SLOW_BLINK;
	  tmp = tmp << 1;
	  kis = kis | tmp;

	  tmp = status_green;
	  kis = kis | tmp;
	  kis1 = (unsigned char)(kis >> 8);
	  kis2 = (unsigned char)(kis & 0x00FF);

	  msg[i++] = kis1;
	  msg[i++] = kis2;
  }	  

  return vega_control_send_alphacom_msg(VEGA_NOT_URGENT,pAlphaComLink, msg, i);
} */


int CRM4::device_set_led(int key, unsigned char color, char select, char status)
{
	if ( has_hardware_error() ) return -1;

  BYTE msg[256];
  int i = 0;
  msg[i++] = 0x43;
  msg[i++] = 0x41;
  msg[i++] = port_aslt.address.byte1;

  {
	  unsigned short kis;
	  BYTE kis1, kis2;
	  BYTE tmp;


	  kis = 0x0000;
	  key = key -1;         // to start with 0
	  key = key & 0x03FF;	// to cut the 10 LSB
	  key = key << 6;
	  kis = kis | key;		//set the key bits to 1
	  // select the color
	  tmp = color;
	  tmp = tmp << 3;		// go to the 11th bit
	  kis = kis | tmp;
	  // select the type of display
	  tmp = select;
	  tmp = tmp << 1;
	  kis = kis | tmp;

	  tmp = status;
	  kis = kis | tmp;
	  kis1 = (unsigned char)(kis >> 8);
	  kis2 = (unsigned char)(kis & 0x00FF);

	  msg[i++] = kis1;
	  msg[i++] = kis2;
  }
  vega_control_send_alphacom_msg(VEGA_NOT_URGENT,pAlphaComLink, msg, i);
} 

int CRM4::device_reset_all_leds()
{
	if ( has_hardware_error() ) return -1;

	BYTE msg[25];
  int i = 0;

  msg[i++] = 0x43;
  msg[i++] = 0x42;
  msg[i++] = port_aslt.address.byte1;
  msg[i++] = 0xF1;
  
  return vega_control_send_alphacom_msg(VEGA_URGENT,pAlphaComLink, msg, i);
}

/*
The backlight is handled as key 1023, indicator 0, status selector 0
 
KIS:
1111111111000001 (0xffc1) for ON
1111111111000000 (0xffc0) for OFF
 
Regards,
 
Hans	*/

int CRM4::device_set_backlight_on()
{				/* envoie option poste avec afficheur */
	if ( has_hardware_error() ) return -1;

int color=0;
int status=1;
int select=0;

  BYTE msg[256];
  int i = 0;
  msg[i++] = 0x43;
  msg[i++] = 0x41;
  msg[i++] = port_aslt.address.byte1;

  {
	  unsigned short kis;
	  BYTE kis1, kis2;
	  BYTE tmp;


	  kis = 0x0000;
	  int key = 1023;         
	  key = key & 0x03FF;	// to cut the 10 LSB
	  key = key << 6;
	  kis = kis | key;		//set the key bits to 1
	 
	  // select the color
	  tmp = color;
	  tmp = tmp << 3;		// go to the 11th bit
	  kis = kis | tmp;

	  // select the type of display
	  tmp = select;
	  tmp = tmp << 1;
	  kis = kis | tmp;

	  tmp = status;
	  kis = kis | tmp;
	  kis1 = (unsigned char)(kis >> 8);
	  kis2 = (unsigned char)(kis & 0x00FF);

	  msg[i++] = kis1;
	  msg[i++] = kis2;
  }
  vega_control_send_alphacom_msg(VEGA_NOT_URGENT,pAlphaComLink, msg, i);
} 

int CRM4::device_set_option_display_present()
{				/* envoie option poste avec afficheur */
	if ( has_hardware_error() ) return -1;

  BYTE msg[4];
  int i = 0;
  
  port_address_t addr = port_aslt.address;

  msg[i++] = ICCMT_ST_OPTIONS;
  msg[i++] = addr.byte1;	/* toujours premier byte pour une addresse sur carte ASLT (que un seul SBI) */
  msg[i++] = 0x32;
  msg[i++] = 0xF1;


  if (vega_control_send_alphacom_msg(VEGA_URGENT,pAlphaComLink, msg, i) < 0)
    return -1;
 
  return 0;
}



int CRM4::device_open_micro()

{
	if ( has_hardware_error() ) return -1;

    
      BYTE msg[4];
      int  i = 0;
      msg[i++] = ICCMT_ST_OPEN_MIC;

      port_address_t addr = port_aslt.address;
      if (addr.size == 1) 
			msg[i++] = addr.byte1;
      else {
			msg[i++] = addr.byte1;
			msg[i++] = addr.byte2;
      }
      
      if (vega_control_send_alphacom_msg(VEGA_URGENT,pAlphaComLink, msg, i) < 0) return -1;
    
    
  return 0;
}

int CRM4::device_close_micro()
{
	if ( has_hardware_error() ) return -1;

      BYTE msg[4];
      int  i = 0;
      msg[i++] = ICCMT_ST_SHUT_MIC;

      port_address_t addr = port_aslt.address;
      if (addr.size == 1) 
			msg[i++] = addr.byte1;
      else {
			msg[i++] = addr.byte1;
			msg[i++] = addr.byte2;
      }
      
      if (vega_control_send_alphacom_msg(VEGA_URGENT,pAlphaComLink, msg, i) < 0)
	return -1;

  return 0;
}


// return how many timeslot of conf are connected to ALST sc1 or associated AGA port_3confs
int CRM4::device_check_timselot_duplication(int confnum)
{

	int ts=-1;
	int nb_ts = 0;
	vega_conference_t *C = vega_conference_t::by_number(confnum);
	
	if ( NULL ==C ) return 0;

	if ( port_aslt.mixer.sub_channel1.connected  )
	{
		ts = port_aslt.mixer.sub_channel1.tslot ;
		if ( C->timeslot_belongs_t_conference(ts) ){
			nb_ts++;
		}

	}

	if ( port_3confs.mixer.sub_channel0.connected  ){
		ts = port_3confs.mixer.sub_channel0.tslot ;
		if ( C->timeslot_belongs_t_conference(ts) ){
			nb_ts++;
		}

	}	if ( port_3confs.mixer.sub_channel1.connected  ){
		ts = port_3confs.mixer.sub_channel1.tslot ;
		if ( C->timeslot_belongs_t_conference(ts) ){
			nb_ts++;
		}

	}
	if ( port_3confs.mixer.sub_channel3.connected  ){
		ts = port_3confs.mixer.sub_channel3.tslot ;
		if ( C->timeslot_belongs_t_conference(ts) ){
			nb_ts++;
		}

	}
	if ( 0==nb_ts ) {
		vega_log(INFO, "bizarre: NO TS belongs to C%d", C->number );
	}
	else if ( 1==nb_ts ) {
		//vega_log(INFO, "OK OK OK OK OK ONLY ONE TS %d belongs to C%d", ts, C->number );
	}
	else if ( nb_ts>1 ) {
		dump_mixers();
		vega_log(INFO, "ERROR MORE THAN ONE TS %d belongs to C%d\n", ts, C->number );
		vega_log(INFO, "ERROR MORE THAN ONE TS %d belongs to C%d\n", ts, C->number );
		vega_log(INFO, "ERROR MORE THAN ONE TS %d belongs to C%d\n", ts, C->number );
		fprintf(stderr,"ERROR MORE THAN ONE TS %d belongs to C%d\n", ts, C->number );
		fprintf(stderr,"ERROR MORE THAN ONE TS %d belongs to C%d\n", ts, C->number );
		fprintf(stderr,"ERROR MORE THAN ONE TS %d belongs to C%d\n", ts, C->number );
		//exit(-5);
	}
	return nb_ts;
}

int CRM4::device_enable_events()
{
	if ( has_hardware_error() ) return -1;
  BYTE msg[4];
  int  i = 0;
  port_address_t addr;

  
   
  msg[i++] = ICCMT_ST_EV_ON;
  addr = port_aslt.address;  

  if (addr.size == 1) 
    msg[i++] = addr.byte1;
  else {
    msg[i++] = addr.byte1;
    msg[i++] = addr.byte2;
  }
  
  if (vega_control_send_alphacom_msg(VEGA_URGENT,pAlphaComLink, msg, i) < 0)
    return -1;

  return 0;
}


int CRM4::device_change_gain( int value)
{
	if ( has_hardware_error() ) return -1;

  BYTE msg[4];

  
  int  i = 0;

  
  msg[i++] = ICCMT_ST_SET_VOLUME;
  port_address_t addr;
  
  
  addr = port_aslt.address;

  
  if (addr.size == 1) 
    msg[i++] = addr.byte1;
  else {
    msg[i++] = addr.byte1;
    msg[i++] = addr.byte2;
  }
  
  msg[i++] = value;
  
  if (vega_control_send_alphacom_msg(VEGA_NOT_URGENT,pAlphaComLink, msg, i) < 0)
    return -1;  

  return 0;
}


/*
  retourne 0 ou -1 respectivement si la carte numéro board_number est 
  utilisée par le device (notion  abonné).
*/

/*
int CRM4::device_is_board_used( int board_number)
{
 	CRM4*d = this;
 if (!d) {
    vega_log(VEGA_LOG_CRITICAL, "d == NULL");
    return -1;
  }
  
  if (d->type == TYPE_CRM4) {
    if (get_board_number_from_address(d->port_aslt.address) == board_number)
      return 0;
    if (get_board_number_from_address(d->port_3confs.address) == board_number)
      return 0;
  }
  else if (d->type == TYPE_JUP2) {
    if (get_board_number_from_address(d->device.jupiter2.port_aga_reception.address) == board_number)
      return 0;
    if (get_board_number_from_address(d->device.jupiter2.port_aga_emission.address) == board_number)
      return 0;
    
  }
  else if (d->type == TYPE_RADIO) {
    if (get_board_number_from_address(d->port_aga_reception.address) == board_number)
      return 0;
    if (get_board_number_from_address(d->port_aga_emission.address) == board_number)
      return 0;
  }
  return -1;
}*/

/* faire une fonction qui configure 1 ou n ports d'un device */



int  CRM4::device_display_msg(int line_number, char *message)
{
	if ( has_hardware_error() ) return -1;

  BYTE msg[35];	// this is the capacity of every one of the four lines + the commands
  char line_index[4] = {0x00, 0x40, 0x14, 0x54};
  int i = 0;
  

  port_address_t addr = port_aslt.address;

  msg[i++] = 0x43;
  msg[i++] = 0x33;
  msg[i++] = addr.byte1;
  msg[i++] = 0x00;
  msg[i++] = 0x13;
  msg[i++] = line_index[line_number - 1];


  unsigned int a = 0;
  for (a = 0; a < strlen(message); a++)
    msg[i++] = message[a];

  return vega_control_send_alphacom_msg(VEGA_NOT_URGENT,pAlphaComLink, msg, i);
}


int CRM4::device_reset_display()
{
  BYTE msg[8];	// this is the capacity of every one of the four lines + the commands
  int i = 0;

  msg[i++] = 0x43;
  msg[i++] = 0x33;
  msg[i++] = port_aslt.address.byte1;
  msg[i++] = 0xFF;
  msg[i++] = 0x10;


  return vega_control_send_alphacom_msg(VEGA_NOT_URGENT,pAlphaComLink, msg, i);
}

int CRM4::device_side_tone_off()
//int device_open_micro(device_t *d)
{
	if ( has_hardware_error() ) return -1;

	BYTE msg[4];
      int  i = 0;
      msg[i++] = ICCMT_ST_SIDE_OFF;

      port_address_t addr = port_aslt.address;
      if (addr.size == 1) 
		msg[i++] = addr.byte1;
      else {
		msg[i++] = addr.byte1;
		msg[i++] = addr.byte2;
      }
      
      if (vega_control_send_alphacom_msg(VEGA_NOT_URGENT,pAlphaComLink, msg, i) < 0)
		return -1;

  return 0;
}

/*int device_enter_conference(device_t *d, int new_conference_number, int old_conference_number)
{
  vega_conference_t *conf = NULL;


  if ((conf = vega_conference_t::by_number(new_conference_number)) == NULL) {
    vega_log(VEGA_LOG_ERROR, "conference number %d doesnt exists !", new_conference_number);
    return -1;
  }

  if (device_is_in(conf, d)) {
    vega_log(VEGA_LOG_ERROR, "conference number %d doesnt contains device %d !", new_conference_number, d->number);
    return -1;
  }
	// la conference conference_number contient bien le device d !
  d->current_conference = conf->number;

  device_display_member_of_conference(d, conf);

  // pdl 20090305
  if ( d!=conf->device_director ) {  // pas besoin d'entrer dans une conference lorsqu'on est director de celle ci 
	  // cela se fait des son activation , peut importe par qui !
	  vega_log(INFO, "ENTER conference %d AS SECONDARY ( current %d -> %d) !", new_conference_number,d->current_conference,old_conference_number);
	  if ( old_conference_number > 0 ){
		  // on etait deja cable sur une autre conference:
		  // 1 - liberer notre sous sortie 1 du mixer ASLT du poste

		  vega_log(INFO, "port_aslt.mixer.sub_channel0.connected=%d",d->port_aslt.mixer.sub_channel0.connected );
		  vega_log(INFO, "port_aslt.mixer.sub_channel0.tslot=%d",d->port_aslt.mixer.sub_channel0.tslot );
		  vega_log(INFO, "port_aslt.mixer.sub_channel1.connected=%d",d->port_aslt.mixer.sub_channel1.connected );
		  vega_log(INFO, "port_aslt.mixer.sub_channel1.tslot=%d",d->port_aslt.mixer.sub_channel1.tslot );
		  vega_log(INFO, "port_aslt.mixer.sub_channel3.connected=%d",d->port_aslt.mixer.sub_channel3.connected );
		  vega_log(INFO, "port_aslt.mixer.sub_channel3.tslot=%d",d->port_aslt.mixer.sub_channel3.tslot );

		  mixer_disconnect_timeslot_to_sub_output(&d->port_aslt, sub_channel_1);

		  // 2 - la conference ds laquelle on veut parler etait deja cablee ( car on l'entandait) sur notre mixeur AGA associe, la decabler !
	  }
	  // 1 - connecter le timeslot de la sous sortie 1 du mixer ASLT du poste vers le timeslot mix conf de la conf 
	  int ts_value = conf->matrix_configuration->ts_start + 8;
	  mixer_connect_timeslot_to_sub_output(&d->port_aslt, sub_channel_1, ts_value, 0);

  }else{


	  vega_log(INFO, "ENTER conference %d ( current %d ) AS MASTER (DEJA FAIT DES L'ACTIVATION!)", new_conference_number,d->current_conference);
	  // le master fait cela pour avoir l'affichage leds des particpants a sa conference probablement
		
  }      
  //vega_log(INFO, "device %d is entered in conference %d", d->number, conf->number);
  return 0;
}*/


/*
int device_quit_current_conference(device_t *d)
{
  if (d->current_conference == 0)
    return 0;

  vega_conference_t *conf = NULL;
  if ((conf = vega_conference_t::by_number(d->current_conference)) == NULL) {
    vega_log(VEGA_LOG_ERROR, "conference number %d doesnt exists !", d->current_conference);
    return -1;
  }else{
    vega_log(INFO, "QUIT conference number %d !", d->current_conference);
  }

  // on ne touche pas aux 2 leds de la conference qu'on quitte 
  // et on efface les devices de celle ci
  device_display_reset_all_devices_led(d);

  // pdl 20090305
  if ( d != conf->device_director ) {

	  vega_log(INFO, "SLAVE QUIT conference %d ( current %d) !", conf->number,d->current_conference);
	  // connecter le timeslot de la sous sortie 1 du mixer ASLT du poste vers le timeslot mix conf de la conf 
	  //int ts_value = conf->matrix_configuration->ts_start + 8;
	  //mixer_connect_timeslot_to_sub_output(&d->port_aslt, sub_channel_1, ts_value, 0);
	  printf(" DISC sous sortie 1 du TS mixer ASLT... \n");
		  vega_log(INFO, "port_aslt.mixer.sub_channel0.connected=%d",d->port_aslt.mixer.sub_channel0.connected );
		  vega_log(INFO, "port_aslt.mixer.sub_channel0.tslot=%d",d->port_aslt.mixer.sub_channel0.tslot );
		  vega_log(INFO, "port_aslt.mixer.sub_channel1.connected=%d",d->port_aslt.mixer.sub_channel1.connected );
		  vega_log(INFO, "port_aslt.mixer.sub_channel1.tslot=%d",d->port_aslt.mixer.sub_channel1.tslot );
		  vega_log(INFO, "port_aslt.mixer.sub_channel3.connected=%d",d->port_aslt.mixer.sub_channel3.connected );
		  vega_log(INFO, "port_aslt.mixer.sub_channel3.tslot=%d",d->port_aslt.mixer.sub_channel3.tslot );

	  mixer_disconnect_timeslot_to_sub_output(&d->port_aslt, sub_channel_1);

	  d->current_conference = 0;

  }else{
	  vega_log(INFO, "MASTER QUIT conference %d ( current %d) le directeur peut il quitter sa conference ?", conf->number,d->current_conference);
  }
  //device_display_conferences_state_all_devices();  
  return 0;
}*/



void CRM4::dump_mixers()
{
	CRM4*d =this;
	vega_log(INFO, "D%02d ASLT SC0 %d TS %03d - SC1 %d TS %03d - SC3 %d TS %03d      AGA  SC0 %d TS %03d - SC1 %d TS %03d - SC3 %d TS %03d",
	  d->number,
	  d->port_aslt.mixer.sub_channel0.connected,
	  d->port_aslt.mixer.sub_channel0.tslot ,
	  d->port_aslt.mixer.sub_channel1.connected,
	  d->port_aslt.mixer.sub_channel1.tslot  ,
	  d->port_aslt.mixer.sub_channel3.connected,
	  d->port_aslt.mixer.sub_channel3.tslot  ,
	  //);
	//vega_log(INFO, "D%02d    AGA  SC0 %d TS %03d * SC1 %d TS %03d * SC3 %d TS %03d",
	  //d->number,
	  d->port_3confs.mixer.sub_channel0.connected,
	  d->port_3confs.mixer.sub_channel0.tslot ,
	  d->port_3confs.mixer.sub_channel1.connected,
	  d->port_3confs.mixer.sub_channel1.tslot  ,
	  d->port_3confs.mixer.sub_channel3.connected,
	  d->port_3confs.mixer.sub_channel3.tslot  
	  );
}

void CRM4::fprint_mixers( FILE* fout)
{
	if ( NULL == fout) return;
	if ( nb_active_conf_where_device_is_active_or_dtor_excluded()>0  ) 
	{
		fprintf(fout,"D%02d aslt SC0 %d TS %03d - SC1 %d TS %03d - SC3 %d TS %03d\n"
					 "D%02d aga_ SC0 %d TS %03d - SC1 %d TS %03d - SC3 %d TS %03d",
		  number,
		  port_aslt.mixer.sub_channel0.connected,
		  port_aslt.mixer.sub_channel0.tslot ,
		  port_aslt.mixer.sub_channel1.connected,
		  port_aslt.mixer.sub_channel1.tslot  ,
		  port_aslt.mixer.sub_channel3.connected,
		  port_aslt.mixer.sub_channel3.tslot  ,
		  //);
		//vega_log(INFO, "D%02d    AGA  SC0 %d TS %03d * SC1 %d TS %03d * SC3 %d TS %03d",
		  number,
		  port_3confs.mixer.sub_channel0.connected,
		  port_3confs.mixer.sub_channel0.tslot ,
		  port_3confs.mixer.sub_channel1.connected,
		  port_3confs.mixer.sub_channel1.tslot  ,
		  port_3confs.mixer.sub_channel3.connected,
		  port_3confs.mixer.sub_channel3.tslot  
		  );
		fprintf(fout,"\n");
	}
}
	
void Radio::fprint_mixers( FILE* fout)
{
	if ( NULL == fout) return;
	if ( nb_active_conf_where_device_is_active_or_dtor_excluded()>0  ) {
		fprintf(fout,"D%02d agaR SC0 %d TS %03d * SC1 %d TS %03d * SC3 %d TS %03d\n"
					 "D%02d agaT SC0 %d TS %03d * SC1 %d TS %03d * SC3 %d TS %03d",
		  number,
		  port_aga_reception.mixer.sub_channel0.connected,
		  port_aga_reception.mixer.sub_channel0.tslot ,
		  port_aga_reception.mixer.sub_channel1.connected,
		  port_aga_reception.mixer.sub_channel1.tslot  ,
		  port_aga_reception.mixer.sub_channel3.connected,
		  port_aga_reception.mixer.sub_channel3.tslot  ,
		  number,
		  port_aga_emission.mixer.sub_channel0.connected,
		  port_aga_emission.mixer.sub_channel0.tslot ,
		  port_aga_emission.mixer.sub_channel1.connected,
		  port_aga_emission.mixer.sub_channel1.tslot  ,
		  port_aga_emission.mixer.sub_channel3.connected,
		  port_aga_emission.mixer.sub_channel3.tslot  
		  );
		fprintf(fout,"\n");
	}
}	

int BroadcastEvent(EVENT* ev, device_t* first_device)
{	
	vega_log(INFO, "BroadcastEvent %d first_device %p", ev->code, first_device);
	if ( first_device != NULL ) {
		first_device->ExecuteStateTransition(ev); // envoyer d'abord a ce device puis a tous les autres !
	}
	unsigned int k=0;
	foreach(device_t* d1,device_t::qlist_devices)
	{
		vega_log(INFO, "BroadcastEvent D%d", d1->number);
		if ( first_device != NULL ){
				if (first_device!=d1) 
					d1->ExecuteStateTransition(ev);
		}else
			d1->ExecuteStateTransition(ev);
		k++;
	}
	return k;
}

int DeferBroadcastEvent(EVENT* ev, int delay_ms)
{	
	unsigned int k=0;
	//for (k = 0; k < g_list_length(devices); k++) 
	foreach(device_t* d1,device_t::qlist_devices)
	{
		//device_t * d1 = (device_t *)g_list_nth_data(devices, k);
		//vega_log(INFO, "BroadcastEvent %d", d1->number);
		d1->DeferEvent(ev, delay_ms);
		k++;
	}
	return k;
}



void CRM4::dump_devices(FILE* fout)
{
	if ( NULL == fout ) return;
	//int i=0;
	//int len = g_list_length(devices);
	//fprintf(fout,"total:%d stations\n", len);
	//for (i = 0; i < len ; i++) 

	foreach(CRM4* ddd,qlist_crm4)
	{
		//device_t* dd = (device_t*)(g_list_nth_data(devices, i) );
		//CRM4* ddd = dynamic_cast<CRM4*>( dd );
		//printf("** device[%d] = %p",i, d);
		if ( ddd==NULL ) {
		}else{
			fprintf(fout,"* D%02d %12.12s C%d type%s %12.12s ",ddd->number, ddd->name, ddd->current_conference , ddd->type_string(), ddd->state_string()) ;
				CRM4* d = dynamic_cast<CRM4*>(ddd);
				if ( d){
					fprintf(fout,"[ASLT B %d S %d P %d addr %02X %02X AGA  B %d S %d P %d addr %02X %02X]",
						d->port_aslt.board_number, d->port_aslt.sbi_number , d->port_aslt.port_number,
						d->port_aslt.address.byte1, d->port_aslt.address.byte2,
						d->port_3confs.board_number, d->port_3confs.sbi_number , d->port_3confs.port_number,
						d->port_3confs.address.byte1, d->port_3confs.address.byte2) ;
					//fprintf(fout,"\n");

					fprintf(fout," %03dAGA%03d-%03d-%03d....\n",
						(d->port_aslt.mixer.sub_channel1.connected   ? d->port_aslt.mixer.sub_channel1.tslot   : 0),
						(d->port_3confs.mixer.sub_channel0.connected ? d->port_3confs.mixer.sub_channel0.tslot : 0),
						(d->port_3confs.mixer.sub_channel1.connected ? d->port_3confs.mixer.sub_channel1.tslot : 0),
						(d->port_3confs.mixer.sub_channel3.connected ? d->port_3confs.mixer.sub_channel3.tslot : 0)
					 );

				}
		}
	}
	
}

const char* CRM4::state_string()
{	
	vega_conference_t *conf = vega_conference_t::by_number(current_conference);
	if (conf){
		if ( PARTICIPANT_ACTIVE == conf->get_state_in_conference(this) ) return "ACTIVE";
		if ( PARTICIPANT_SELF_EXCLUDED == conf->get_state_in_conference(this) ) return "SELF_EXCLUDED";
		if ( PARTICIPANT_DTOR_EXCLUDED == conf->get_state_in_conference(this) ) return "DTOR_EXCLUDED";
		if ( NOT_PARTICIPANT == conf->get_state_in_conference(this) ) return "NOT_PARTICIPANT";
	}
	return "NOT_PARTICIPANT";
}

int device_t::nb_active_conf_where_device_is_active_or_dtor_excluded() // device is in an active conf and not auto excluded
{
	int N=0;
	unsigned int ic=0;
	//for (ic = 0; ic < g_list_length(conferences) ; ic++)  // parcours de toutes les conferences....
	foreach(vega_conference_t *c,vega_conference_t::qlist)
	{
		//vega_conference_t * c = (vega_conference_t*)g_list_nth_data(conferences, ic);
		if ( c && c->active ){
			//vega_log(INFO, "D%d STATE %d in active C%d",number,c->get_state_in_conference(this), c->number);
		}
		if ( c && c->active && ( 
			c->get_state_in_conference(this)==PARTICIPANT_ACTIVE ||
			c->get_state_in_conference(this)==PARTICIPANT_DTOR_EXCLUDED 
			//c->get_state_in_conference(this)==PARTICIPANT_SELF_EXCLUDED // pdl 20090911
			)
		)//is_active_participant(c,d) )
		{
			N++;
		}
	}
	//vega_log(INFO, "D%d PARTICIPANT_ACTIVE in %d active CONF",number,N);
	return N;
}

int device_t::nb_conf_where_device_is_in() // device is in an active conf and not auto excluded
{
	int N=0;
	foreach(vega_conference_t *c,vega_conference_t::qlist){
		if ( c && c->is_in(this) ) {N++;}
	}
	//vega_log(INFO, "D%d in %d active CONF",number,N);
	return N;
}

void CRM4::start_hearing_tone(int ts)
{
	if ( has_hardware_error() ) return;
	if( true==is_hearing_tone ) {
		vega_log(INFO, "WARNING: start_hearing_tone: D%d est deja en TONE !!!",number);
		//exit(-14);
		return;
	}
	is_hearing_tone = true;
	if ( port_aslt.mixer.sub_channel0.connected ){
		vega_log(INFO, "ATTENTION: D%d start_hearing_tone PORT ASLT SC 0 deja connecte !!!",number);
		mixer_disconnect_timeslot_to_sub_output(&port_aslt, sub_channel_0);
	}else{
		vega_log(INFO, "OK: D%d start_hearing_tone PORT ASLT SC 0 pas connecte",number);
	}

	mixer_connect_timeslot_to_sub_output(&port_aslt, sub_channel_0, ts, 0);
	port_aslt.mixer.sub_channel0.connected = 1;
}

void CRM4::stop_hearing_tone()
{
	if ( has_hardware_error() ) return;
	if( false==is_hearing_tone ) {
		vega_log(INFO, "WARNING: stop_hearing_tone: D%d n'est pas en TONE !!!",number);
		return;
	}
	is_hearing_tone = false;
	//mixer_disconnect_timeslot_to_sub_output(&d->port_aslt, sub_channel_0);
	if ( port_aslt.mixer.sub_channel0.connected ){
		vega_log(INFO, "OK: D%d stop_hearing_tone PORT ASLT SC 0 connecte",number);
		mixer_disconnect_timeslot_to_sub_output(&port_aslt, sub_channel_0);
		port_aslt.mixer.sub_channel0.connected = 0;
	}else{
		vega_log(INFO, "ATTENTION: D%d stop_hearing_tone PORT ASLT SC 0 non connecte !!!",number);
	}
}

bool device_t::has_conference_in_common(int devnumber)
{
	device_t* d = device_t::by_number(devnumber) ;
	if ( d) 
		foreach(vega_conference_t *c,vega_conference_t::qlist)
		{
			if ( c->is_in(d) && c->is_in(this ) ) // parcours de toutes les conferences dans laquelle le device est present et moi aussi
				return true;
		}
	return false;
}

void CRM4::init_groups_keys(char* str_keys)
{
	char temp[1024]={0};

	strncpy(temp,str_keys,sizeof(temp));
	char* ptr = temp;
	int N=strlen(temp);

	char str_touche[128]={0};
	char str_group[128]={0};


	{		// exemple touche 39 keys 1 ( LIMITE A UNE TOUCHE GROUPE pour le moment )
		// keys = 39,1;40,2
		//char *elt = strtok(temp, ";\r\n");

			int comma_found = 0;
			unsigned int k =0;
			while( N-- > 0 )
			{ 
				char carcou = *ptr++;//ptr[k];

				vega_log(INFO,"GROUP: %c [%s][%s] %d<%d",carcou,str_touche,str_group, k, strlen(ptr));

				if ( ',' == carcou ){
					comma_found = 1;
						continue;
				}

				if ( !comma_found ) 
				{
					if ( carcou >= '0' && carcou <= '9' ) str_touche[ strlen(str_touche) ] = carcou;
				}
				else
				{
					if ( carcou >= '0' && carcou <= '9' ) {
						str_group[ strlen(str_group) ] = carcou;
						vega_log(INFO,"GROUP = %c [%s][%s]",carcou,str_touche,str_group);
					}
					
					if ( carcou == ';' ||carcou == '\r' || carcou == '\n' || N==0)
					{
						int num_group = atoi(str_group);
						int num_touche = atoi(str_touche);
					
						vega_log(INFO,"GROUP ==> num_touche=%d num_group=%d \n",num_touche, num_group);

						// RAZ for next 
						comma_found = 0;
						memset(str_group,0,sizeof(str_group) );
						memset(str_touche,0,sizeof(str_touche) );

						if ( num_group && num_touche ) 
						{
							/*int nb_keys = device.crm4.keys_configuration->nb_keys;
							device.crm4.keys_configuration->device_key_configuration[nb_keys].type = ACTION_GROUP_CALL ;      
							device.crm4.keys_configuration->device_key_configuration[nb_keys].key_number = num_touche;
							device.crm4.keys_configuration->device_key_configuration[nb_keys].action.group_call.group_number = num_group;
							device.crm4.keys_configuration->nb_keys++;*/

							if ( is_valid_group_number(num_group) ) {
								//device.crm4.keys_group[num_group] = num_touche; // store per device and not common to all devices like keys_configuration

								keymap_groups[num_touche] = num_group;

								vega_log(INFO,"FOUND keymap_groups[%d]=%d on D%d...\n", num_touche ,num_group  , number);
							}
						}

						comma_found = 0;
					}
				}
			}
	  }
}


int CRM4::ExecuteStateTransition2(EVENT* ev)
{
	CRM4::LPFNStateTransition lpfn= this->linkStateTransition;
	if ( lpfn )
	{
		try{
			return (this->*lpfn)(ev);
		}
		catch(...){
			fprintf(stderr,"exception %s %d d=%p ev=%p\n",__FILE__,__LINE__,this,ev);
			vega_log(INFO,"exception %s %d d=%p ev=%p\n",__FILE__,__LINE__,this,ev);
		}
	}
	return -1;
}

int CRM4::ExecuteStateTransition(EVENT* ev)
{
	vega_log(INFO,"D%d event_t::queue ev code %d\n",number,ev->code);
	event_t::queue(this,ev,0);
	return -1;
}

int Radio::ExecuteStateTransition2(EVENT* ev)
{
	Radio::LPFNStateTransition lpfn= this->linkStateTransition;
	if ( lpfn )
	{
		try{
			return (this->*lpfn)(ev);
		}
		catch(...){
			fprintf(stderr,"exception %s %d d=%p ev=%p\n",__FILE__,__LINE__,this,ev);
			vega_log(INFO,"exception %s %d d=%p ev=%p\n",__FILE__,__LINE__,this,ev);
		}
	}
	return -1;
}

int Radio::ExecuteStateTransition(EVENT* ev)
{
	vega_log(INFO,"R%d event_t::queue ev code %d\n",number,ev->code);
	event_t::queue(this,ev,0);
	return -1;
}


int CRM4::DeferEvent(EVENT* ev, int delay_ms)
{	
	event_t::queue(this,ev,delay_ms);
	return 0;
}


int Radio::DeferEvent(EVENT* ev, int delay_ms)
{	
	event_t::queue(this,ev,delay_ms);
	return 0;
}

void event_t::queue(device_t* pdev,EVENT* ev,unsigned int timeout)
{
	event_t* PEV = new event_t(pdev,ev,timeout);
	if (PEV == NULL) {
		vega_log(CRITICAL, "cannot allocate pevent");
	}else{
		qlist_events_mutex.lock();
		qlist_events.append(PEV);
		qlist_events_mutex.unlock();
		//vega_log(INFO,"queue event:%p (%d sec) device:%p",PEV,PEV->m_timeout_sec,PEV->m_p_device);
	}
}

void event_t::init()
{
	int ret = pthread_create(&(event_thread_id), NULL, event_thread, NULL);
	if (ret < 0) {
		vega_log(INFO,"error event_thread - cause: %s", strerror(errno));
	}
}

void* event_t::event_thread(void* arg)
{
	while (1)
	{
		qlist_events_mutex.lock();
		int len = qlist_events.size();
		qlist_events_mutex.unlock();
		if ( len <= 0 )
		{
			usleep(2*MILLISECOND);
			//vega_log(INFO,"list_event EMPTY");
		}else{
			//vega_log(INFO,"ExecuteStateTransition2 event:%p (%d sec) device:%p",PEV,PEV->m_timeout_sec,PEV->m_p_device);
			qlist_events_mutex.lock();
			event_t *PEV = qlist_events.takeFirst();
			qlist_events_mutex.unlock();
			
			if (NULL!=PEV){
				if ( PEV->m_timeout_sec <= 0){
					PEV->m_p_device->ExecuteStateTransition2( &PEV->m_event );
					delete PEV;
				}else{
					time_t now=time(NULL);
					if ( now >= (PEV->m_ts_create + PEV->m_timeout_sec) ) {
						//vega_log(INFO,"DEFERED ExecuteStateTransition2:%p (timeout:%d sec difftime:%ld)",PEV,PEV->m_timeout_sec, now-PEV->m_ts_create);
						PEV->m_p_device->ExecuteStateTransition2( &PEV->m_event );
						delete PEV;
					}else{
						//vega_log(INFO,"DEFERED event :g_list_append:%p (timeout:%d sec difftime:%ld)",PEV,PEV->m_timeout_sec, now-PEV->m_ts_create);
						qlist_events_mutex.lock();
						qlist_events.append(PEV);
						qlist_events_mutex.unlock();
					}
				}
			}
		}
	}
	vega_log(CRITICAL,"EXITING event_t::event_thread !!!");
}
