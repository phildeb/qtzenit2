/* 
top -p `ps ax | grep "/bin/vegactrl" | grep "pts/1" |  cut -f2 -d" "`
project: PC control Vega Zenitel 2009
filename: statemachine.c
author: Mustafa Ozveren & Debreuil Philippe
last modif: 20090307
desciption: state machine of vega CRM4 station devices
*/
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
#include <assert.h>

#include "debug.h"
#include "conf.h"
#include "misc.h"
#include "conf.h"
#include "alarmes.h"

int set_recorder_dry_contact_on_off(int conf_number , int on_off) ;
static struct timeval  _tstart_btn_debug;

int CRM4::ConfHearingState(EVENT* ev)
{
	//vega_log(INFO,"D%d:ConfHearingState:*** EVENT %d *******\n", number , ev->code);
	switch ( ev->code ) 
	{
	default:
		vega_log(INFO,"D%d:ConfHearingState:*** UNHANDLED EVENT %d *******\n", number , ev->code);
		//vega_log(INFO,"UNHANDLED event %d",ev->code);
		break;

	case EVT_DEVICE_PRESENT: // recu au debut du scan alphacom, tout le monde est idle !
		vega_log(INFO,"D%d CRM4::EVT_DEVICE_PRESENT code:%d \n", number, ev->code);
		device_line_printf(1,"D%d:%s                        ",number,name);
		device_line_printf(2,"appuyez sur C.............");	  
		for (int i=0;i<CRM4_MAX_KEYS;i++) old_tab_green_led[i] = old_tab_red_led[i] = INDEFINI;
		set_green_led_possible_conferences();
		device_MAJ_groups_led(doing_group_call);
		display_general_call_led(device_doing_group_or_general_call );  
	case EVT_DEVICE_PLUGGED:
		{
			vega_log(INFO,"D%d CRM4::UnknownState EVT_DEVICE_PLUGGED code:%d \n", number, ev->code);
			m_is_plugged = true;
			vega_conference_t *C = vega_conference_t::by_number(current_conference);
			if ( C ) {
				foreach(CRM4* d,qlist_crm4)
					if( current_conference == current_conference )  d->device_MAJ_particpant_key(this, 1, NOT_PARTICIPANT);
			}
		}
		break;

	case EVT_DEVICE_NOT_PRESENT: // recu au debut du scan alphacom, tout le monde est idle !
		vega_log(INFO,"D%d CRM4::EVT_DEVICE_NOT_PRESENT code:%d \n", number, ev->code);
	case EVT_DEVICE_UNPLUGGED:
		{
			vega_log(INFO,"D%d CRM4::UnknownState EVT_DEVICE_NOT_PRESENT code:%d \n", number, ev->code);
			m_is_plugged = false;
			vega_conference_t *C = vega_conference_t::by_number(current_conference);
			if ( C ) {
				foreach(CRM4* d,qlist_crm4)
					if( current_conference == current_conference ) d->device_MAJ_particpant_key(this, 1, NOT_PARTICIPANT);
			}
		}
		break;

	case EVT_DEVICE_KEY_GENERAL_CALL:
	case EVT_DEVICE_KEY_GROUP_CALL:
	case EVT_CALLGENERAL_START_TONE:
	case EVT_CALLGENERAL_STOP_SPEAKING:
	case EVT_CALLGENERAL_STOP_TONE:
	case EVT_CALL_GROUP_START_TONE:
	case EVT_CALL_GROUP_STOP_TONE:
	case EVT_CALL_GROUP_STOP_SPEAKING:
		IdleState(ev);
		break;

  case EVT_DEVICE_KEY_DEBUG:
	  {
		  double tt = time_between_button(&_tstart_btn_debug); // calculer la duree depuis le dernier appui
		  gettimeofday(&_tstart_btn_debug, NULL); // memoriser le dernier appui
		  if ( tt > 1.0 )
		  {
			  vega_log(INFO,"IGNORED EVT_DEVICE_KEY_DEBUG within %f seconds\n",tt);
		  }else{
	  
			  // toggle state 
			  vega_log(INFO,"OK EVT_DEVICE_KEY_DEBUG within %f seconds\n",tt);

			  if ( display_slots == true ) {
				  display_slots = false;
				  //device_line_printf(3,"false..................................");
				  device_line_print_time_now(4);

				  printf("D%d EVT_DEVICE_KEY_DEBUG display_slots=%d\n", number,display_slots);
			  }else {
				  display_slots = true;
				  //device_line_printf(4,"true..................................");
  				  device_line_print_time_now(4);

				  printf("D%d EVT_DEVICE_KEY_DEBUG display_slots=%d\n", number,display_slots);

			  }
		  }
	  }
	  break;

  case EVT_DEVICE_KEY_CANCEL: 
	  //dump_conferences(stdout);
  case EVT_DEVICE_UPDATE_DISPLAY:
	  {
		//vega_log(INFO,"D%d EVT_DEVICE_UPDATE_DISPLAY*******\n",number );
		vega_conference_t *C = vega_conference_t::by_number(current_conference);
		if ( C ){
			device_line_printf(2,"C%d:%s(%d actives)....", current_conference, C->name, nb_active_conf_where_device_is_active_or_dtor_excluded() );	  
			if(C->participant_director.device) 
				device_line_printf(3,"Dtor:%s......",C->participant_director.device->name ) ;
			device_MAJ_participants_led(C);
		}
		device_line_printf(1,"D%d:%s                        ",number,name);
		//device_check_timselot_duplication(current_conference);
		//device_line_printf(4,"version %s %s............................."__TIME__,__DATE__);		
		if ( display_slots ) {
			device_line_printf(4,"%03d AGA%03d %03d %03d",
				(port_aslt.mixer.sub_channel1.connected   ? port_aslt.mixer.sub_channel1.tslot   : 0),
				(port_3confs.mixer.sub_channel0.connected ? port_3confs.mixer.sub_channel0.tslot : 0),
				(port_3confs.mixer.sub_channel1.connected ? port_3confs.mixer.sub_channel1.tslot : 0),
				(port_3confs.mixer.sub_channel3.connected ? port_3confs.mixer.sub_channel3.tslot : 0)
			 );
		}
		//device_line_print_time_now(4);

		break;

		foreach(vega_conference_t *C,vega_conference_t::qlist)
		{
			if ( C->is_in(this) && m_common_touche_select_conf.contains(C->number) ) 
			{
				int key = m_common_touche_select_conf[C->number];//0;//get_key_number_by_action_type( device.crm4.keys_configuration, ACTION_SELECT_CONF, C->number);

				vega_log(INFO, "FORCE LIT OFF key%d C%d LED in D%d (%d speaking)", key, C->number, number,  C->nb_people_speaking() );

				device_set_led(key, LED_COLOR_GREEN, LED_MODE_FIX, 0);
				device_set_led(key, LED_COLOR_RED, LED_MODE_FIX, 0);
				tab_green_led[key]  = tab_red_led[key]  = INDEFINI;
			}
		}

		//set_green_led_possible_conferences();
	  }
	  break;

	case EVT_ADD_CRM4_DIRECTOR:
		{	// ev->device_number is the number of the newly added device
			vega_conference_t *C = vega_conference_t::by_number(ev->conference_number);
			if ( ev->device_number == number )
			{	// it is me, i am already conferencing and GUI added my device in another conference !	
				if ( C ){
					vega_log(INFO,"EVT_ADD_CRM4 D%d in C%d (active:%d) (currently in C%d) \n",
						ev->device_number, ev->conference_number, C->active, current_conference );

					if ( C->add_in_director(this ) )
					{
						int N= nb_active_conf_where_device_is_active_or_dtor_excluded() ;
						vega_log(INFO,"******* D%d EVT_CONFERENCE_ACTIVATED C%d active_or_dtor_excluded in %d conferences",number,C->number, N);
						if ( N > MAX_SIMULTANEOUS_ACTIVE_CONF_BY_DEVICE ) { // pdl 20090930 si plus de 4, on est auto exclu !!!!
							vega_log(INFO,"D%d EVT_ADD_CRM4_DIRECTOR C%d IGNORED yet active in too much conf:%d",number, ev->conference_number,N);
							C->set_state_in_conference(this,PARTICIPANT_SELF_EXCLUDED);
							device_line_printf(1,"D%d AJOUT EXCL C%d........", number,ev->conference_number);
						}else{
							C->set_state_in_conference(this,PARTICIPANT_ACTIVE);
							if ( C->active ) {
								current_conference = C->number; // c'est notre conference selectionnee par defaut car la premiere	
								vega_log(INFO,"EVT_CONFERENCE_ACTIVATED D%d in 1st C%d ( auto SELECTED )",number,C->number);
								if ( !start_hearing_in_aga(C) ) 
									start_hearing_in_aslt(C);		
							}
						}
						vega_event_log(EV_LOG_DEVICE_ADDED, number, 0, C->number, "%s ajoute dans conference %s", name, C->name);
						set_green_led_possible_conferences();
						display_general_call_led( device_doing_group_or_general_call );
						all_devices_display_group_key( device_doing_group_or_general_call );

					}else{
						vega_log(INFO,"cannot add_in_director D%d in C%d",number,C->number);
					}

					//device_line_printf(1,"D%d ADDED in C%d........", number,ev->conference_number);
				}
			}else{// another device ( director) has been added: 
				// refresh my participants if the just added device is in my current conference 
				device_t *otherD=device_t::by_number(ev->device_number);
				if ( C && otherD && C->number==current_conference ) 
					device_MAJ_particpant_key(otherD,0,PARTICIPANT_ACTIVE);
			}
		}
		break;	

	case EVT_ADD_RADIO1:
	case EVT_ADD_RADIO2:
	case EVT_ADD_JUPITER:
		{
			vega_log(INFO,"D%d Conferencing EVT_ADD_RADIO1 in C%d\n",number,ev->conference_number);
			vega_conference_t *C = vega_conference_t::by_number(ev->conference_number);
			if ( ev->device_number == number )
			{	// it is me, i am already conferencing and GUI added my device in this conference !
			}else{// another device ( maybe a radio or jupiter !) has been added: 
				// refresh my participants if the just added device is in my current conference 
				device_t *otherD=device_t::by_number(ev->device_number);
				if ( C && otherD && C->number==current_conference ) 
					device_MAJ_particpant_key(otherD,0,C->get_state_in_conference(this));
			}
		}
		break;

	case EVT_ADD_CRM4:
		{	// ev->device_number is the number of the newly added device
			vega_conference_t *C = vega_conference_t::by_number(ev->conference_number);
			if ( ev->device_number == number )
			{	// it is me, i am already conferencing and GUI added my device in another conference !
				if ( C ){
					vega_log(INFO,"EVT_ADD_CRM4 D%d in C%d (active:%d) (currently in C%d) \n",
						ev->device_number, ev->conference_number, C->active, current_conference );
					if ( C->add_in(this) )
					{
						int N= nb_active_conf_where_device_is_active_or_dtor_excluded() ;
						vega_log(INFO,"******* D%d EVT_CONFERENCE_ACTIVATED C%d active_or_dtor_excluded in %d conferences",number,C->number, N);
						if ( N > MAX_SIMULTANEOUS_ACTIVE_CONF_BY_DEVICE ) { // pdl 20090930 si plus de 4, on est auto exclu !!!!
							vega_log(INFO,"D%d EVT_ADD_CRM4 C%d IGNORED yet active in too much conf:%d",number, ev->conference_number,N);
							C->set_state_in_conference(this,PARTICIPANT_SELF_EXCLUDED);
							device_line_printf(1,"D%d AJOUT EXCL C%d........", number,ev->conference_number);
						}else{
							C->set_state_in_conference(this,PARTICIPANT_ACTIVE);
							if ( C->active ) {
								if ( !start_hearing_in_aga(C) ) start_hearing_in_aslt(C);
							}
							vega_event_log(EV_LOG_DEVICE_ADDED, number, 0, C->number, "CRM4 %s ajoute dans %s", name, C->name);
							device_line_printf(1,"D%d AJOUTE A C%d........", number,ev->conference_number);
						}
						set_green_led_possible_conferences();
						display_general_call_led( device_doing_group_or_general_call );
						all_devices_display_group_key( device_doing_group_or_general_call );
					}else{
						vega_log(INFO,"cannot add_in D%d in C%d",number,C->number);
					}
				}
			}else{// another device ( maybe a radio or jupiter !) has been added: 
				// refresh my participants if the just added device is in my current conference 
				device_t *otherD=device_t::by_number(ev->device_number);
				if ( C && otherD && C->number==current_conference ) 
					device_MAJ_particpant_key(otherD,0,C->get_state_in_conference(this));
			}
		}
		break;

	case EVT_REMOVE_DEVICE:// todo: device can be excluded from a conference but stay active in an other conference !!!
		{
			vega_log(INFO,"ConfHearingState:*** EVT_REMOVE_DEVICE D%d from C%d *******\n",ev->device_number, ev->conference_number );
			vega_conference_t *C = vega_conference_t::by_number(ev->conference_number);	
			if ( ev->device_number == number ){ // it is me, i am already conferencing and administrator GUI removed my device  !
				if ( C->is_in(this) && NOT_PARTICIPANT!= C->get_state_in_conference(this) ){
					C->set_state_in_conference(this,NOT_PARTICIPANT);
					// remark: current_conference may be different than ev->conference_number
					stop_hearing_in_aslt(C);
					stop_hearing_in_aga(C);
					if ( C->number == current_conference ) device_MAJ_participants_led(C,1);
					// if device participant in another active conf, don t go idle..
					/*vega_conference_t* selected_C = first_active_conference_where_device_is_participant(d);
					if ( selected_C && selected_C->active ) {
					}else{
						//ChangeState(&CRM4::IdleState);
						vega_log(INFO,"D%d supprime ds C%d *******\n",ev->device_number, ev->conference_number );
					}*/
					vega_event_log(EV_LOG_DEVICE_REMOVED, number, 0, C->number, "CRM4 %s supprime dans %s", name, C->name);
					set_green_led_possible_conferences();

				}
			}else{ // another device than me has been removed, hide it if it is one of my current conference participants
				// refresh my participants if the just added device is in my current conference 
				device_t *otherD=device_t::by_number(ev->device_number);
				if ( C && otherD && C->number==current_conference ) 
					device_MAJ_particpant_key(otherD,1,C->get_state_in_conference(this));
			}
		}
		break;

	case EVT_CONFERENCE_INCLUDE_EXCLUDE: // programmation d'une exclusion ou inclusion (dans la conference en cours)
		{
			if ( activating_conference ) break;
			if ( self_excluding_including_conference ) break;
			if ( !excluding_including_conference )
			{
				excluding_including_conference = 1;
				int key_button = 4;//get_key_number_by_action_type(device.crm4.keys_configuration, ACTION_INCLUDE_EXCLUDE, -1);
				vega_log(INFO,"%d:ConfHearingState:*** EVT_CONFERENCE_INCLUDE_EXCLUDE %d *******\n", number , key_button);
				device_set_blink_slow_red_color( key_button);
				device_set_blink_slow_green_color( key_button);   
			}else{
				excluding_including_conference = 0;
				vega_log(INFO, "device %d ConfHearingState:*** STOP EX(IN)CLUDING CONFERENCE current conf %d", number, current_conference );			  
				int key_button = 4;//get_key_number_by_action_type(device.crm4.keys_configuration, ACTION_INCLUDE_EXCLUDE, -1);		  
				device_set_no_red_color( key_button); 
				device_set_no_green_color( key_button); 
			}
		}
	  break;

	case EVT_DEVICE_KEY_SELECT_DEVICE:
	  {
		  double tt = time_between_button(&_tstart_btn_conf);
		  if ( tt < 1.5 ){vega_log(INFO,"IGNORED EVT_DEVICE_KEY_SELECT_DEVICE within %f seconds\n",tt);break;}
		  gettimeofday(&_tstart_btn_conf, NULL); // memoriser 

		  int dev_number = ev->value.select.device_number; // numero de la station que le directeur veut exclure
		  if ( excluding_including_conference ) 
		  {
			  vega_log(INFO, "select D%d to be ex(in)cluded in C%d (depuis station D%d)",dev_number, current_conference, number );
			  vega_conference_t *C = vega_conference_t::by_number(current_conference);
			  if ( this==C->participant_director.device ) // c'est bien une station d'un directeur de la conference dans laquelle il parle
			  {		
				  // director CAN excluding himself !!!
					CRM4* D = dynamic_cast<CRM4*>(device_t::by_number(dev_number)); // la station a exclure 
					if ( NULL==D )
					{
						Radio* D = dynamic_cast<Radio*>(device_t::by_number(dev_number)); // la radio/jupiter a exclure 
						if ( D ) {
							vega_log(INFO, "D%d DIRECTOR of C%d can ex(in)clude D%d", number, current_conference, D->number);
							EVENT evt;
							evt.device_number = D->number;
							evt.conference_number = current_conference;
							if ( PARTICIPANT_ACTIVE == C->get_state_in_conference(D)  )
							{
								//if ( D==C->participant_director.device )  // pdl 20090922 impossible, une radi n'est jamais directeur
								{ // pour tout autre station de cette conference qui ne peut etre le directeur
									C->set_state_in_conference(D,PARTICIPANT_DTOR_EXCLUDED) ;
									device_line_printf(4,"D%d Dtorexclu C%d........",D->number, C->number);
									D->radio_stop_hearing(C); 
									D->radio_stop_speaking(C); 
									if ( C->nb_people_speaking() == 0 ){
									   C->deactivate_radio_base(D); // todo pdl20090922 ???? if still some other people speaking in conference, do not deactivate radio
									}			

									vega_event_log(EV_LOG_DEVICE_EXCLUDED, D->number,number,C->number,"%s est exclu de %s par le directeur %s", D->name, C->name, name);
									
									this->device_MAJ_particpant_key(D,0,PARTICIPANT_DTOR_EXCLUDED);

									evt.code = EVT_CONFERENCE_DTOR_EXCLUDE;		// maj de la led radio sur poste du directeur
									BroadcastEvent(&evt);
								}
							}else if ( PARTICIPANT_DTOR_EXCLUDED == C->get_state_in_conference(D)  ) { 
								// c'est bien une radio qui a ete exclu par le directeur
								// comme il est bloque dans la conference au cas ou le directeur souhaite le reinclure
								// c'est toujours possible en thorie, sauf si on l'a supprime par l'IHM
								{
									C->set_state_in_conference(D,PARTICIPANT_ACTIVE);
									// reconnecter le ts de cette conference au premier subchannel libre
									D->radio_start_hearing_conf(C);
									D->radio_start_speaking_conf(C);
									vega_log(INFO,"D%d EVT_CONFERENCE_DTOR_INCLUDE C%d",number, current_conference);
									vega_event_log(EV_LOG_DEVICE_INCLUDED, D->number,number , C->number,"%s est inclus dans %s par le directeur %s", D->name, C->name,name);
									device_line_printf(4,"D%d Dtorinclus C%d........",D->number, C->number);

									this->device_MAJ_particpant_key(D,0,PARTICIPANT_ACTIVE);
								}
								evt.code = EVT_CONFERENCE_DTOR_INCLUDE; 
								BroadcastEvent(&evt);
							}else{
							}
							
						}
					}else{  // cas d'un CRM4 
						vega_log(INFO, "D%d DIRECTOR of C%d can ex(in)clude D%d", number, current_conference, D->number);
						
						EVENT evt;
						evt.device_number = D->number;
						evt.conference_number = current_conference;
						
						if ( PARTICIPANT_ACTIVE == C->get_state_in_conference(D)  )
						{
							if ( D==C->participant_director.device ) { // pdl 20090915 c'est moi meme le directeur !
								// le directeur a appuye sur sa propre led, on ignore
								device_line_printf(4,"Dtorexcl:c'est moi !!!........");

							}else{ // pour tout autre station de cette conference qui ne peut etre le directeur
								C->set_state_in_conference(D,PARTICIPANT_DTOR_EXCLUDED) ;
								D->stop_hearing_in_aslt(C);
								D->stop_hearing_in_aga(C);		
								vega_event_log(EV_LOG_DEVICE_EXCLUDED, D->number,0,C->number,"%s est exclu de %s par le directeur", D->name, C->name);
								device_line_printf(4,"D%d DTR EXCLUS C%d........",D->number, C->number);
								//if ( C->number == current_conference ) // pdl 20090629: meme si on parle dans une autre conf, effacer la conf
								{
									D->set_green_led_of_conf(C,0);
									D->set_red_led_of_conf(C,1);
									if ( C->number == D->current_conference ) {
										D->device_MAJ_particpant_key(D,0,PARTICIPANT_DTOR_EXCLUDED);
									}
								}

								device_MAJ_particpant_key(D,0,PARTICIPANT_DTOR_EXCLUDED);

								evt.code = EVT_CONFERENCE_DTOR_EXCLUDE;	
								BroadcastEvent(&evt);
							}


						}else if ( PARTICIPANT_DTOR_EXCLUDED == C->get_state_in_conference(D)  ) { 
							// c'est bien une station qui a ete exclu par le directeur
							// comme il est bloque dans la conference au cas ou le directeur souhaite le reinclure
							// c'est toujours possible en thorie, sauf si on l'a supprime par l'IHM
							{
								C->set_state_in_conference(D,PARTICIPANT_ACTIVE);
								// reconnecter le ts de cette conference au premier subchannel libre
								if ( !D->start_hearing_in_aga(C) ) D->start_hearing_in_aslt(C);
								vega_log(INFO,"D%d EVT_CONFERENCE_DTOR_INCLUDE C%d",number, current_conference);
								vega_event_log(EV_LOG_DEVICE_INCLUDED, D->number, number , C->number,"%s est inclus dans %s par le directeur %s", D->name, C->name, name);
								device_line_printf(4,"D%d Dtorinclus C%d........",D->number, C->number);

								// pdl 20090911 bug: si on essaie d'exclure un poste deja ds 4 conferences
								//if ( C->number == current_conference ) // pdl 20090629: meme si on parle dans une autre conf, effacer la conf
								{
									D->set_green_led_of_conf(C,1);
									D->set_red_led_of_conf(C,1);
									if ( C->number == D->current_conference ) {
										D->device_MAJ_particpant_key(D,0,PARTICIPANT_ACTIVE);
									}
								}
								this->device_MAJ_particpant_key(D,0,PARTICIPANT_ACTIVE);
								
							}
							evt.code = EVT_CONFERENCE_DTOR_INCLUDE; 
							BroadcastEvent(&evt);

						}else if (PARTICIPANT_SELF_EXCLUDED == C->get_state_in_conference(D)  ) {
								device_line_printf(4,"KO:D%d AUTOEXCLU C%d.........",D->number,C->number);
						}else{
								device_line_printf(4,"KO:D%d SUPPPRIME C%d.........",D->number,C->number);
						}
					}			
			  }else{
					vega_log(INFO, "NOK, D%d NOT DIRECTOR of C%d cannot exclude %d", number, current_conference, dev_number);
					//device_line_printf(4,"NOT DTOR in C%d........", current_conference);
			  }
		  }
	  }
	  break;

	case EVT_DEVICE_KEY_ACTIVATE_CONFERENCE: // programmation/(des)activation 2eme, 3eme ou 4eme conference du systeme
		{
			//if ( activating_conference ) break;
			if ( excluding_including_conference ) break;
			if ( self_excluding_including_conference ) break;

			if ( !activating_conference )
			{
				activating_conference = 1;
				int key_button = 1;//get_key_number_by_action_type(device.crm4.keys_configuration, ACTION_ACTIVATE_CONFERENCE, -1);
				vega_log(INFO,"%d:ConfHearingState:*** EVT_DEVICE_KEY_ACTIVATE_CONFERENCE ON C%d *******\n", number , key_button);
				device_set_blink_slow_red_color( key_button);
				device_set_blink_slow_green_color( key_button);   
			}else{
				activating_conference = 0;
				vega_log(INFO, "device %d ConfHearingState:*** EVT_DEVICE_KEY_ACTIVATE_CONFERENCE OFF C%d", number, current_conference );			  
				int key_button = 1;//get_key_number_by_action_type(device.crm4.keys_configuration, ACTION_ACTIVATE_CONFERENCE, -1);		  
				device_set_no_red_color( key_button); 
				device_set_no_green_color( key_button); 
			}
		}
	  break;
	
	case EVT_DEVICE_KEY_SELECT_CONFERENCE: 
		{

		double tt = time_between_button(&_tstart_btn_conf);
		if ( tt < 1.0 ){vega_log(INFO,"IGNORED EVT_DEVICE_KEY_SELECT_CONFERENCE within %f seconds\n",tt);break;}
		gettimeofday(&_tstart_btn_conf, NULL); // memoriser 
		

		vega_log(INFO, "EVT_DEVICE_KEY_SELECT_CONFERENCE: D%d select conference C%d", number, ev->value.select.conference_number);

		int conf_excl_number = ev->value.select.conference_number;
		vega_conference_t *excl_conf = vega_conference_t::by_number(conf_excl_number);

		if ( excl_conf->has_hardware_error() ) { // pdl 20091005
			vega_log(INFO, "C%d HARDWARE ERROR !!!",excl_conf->number);
			device_line_printf(4,"C%d HARDWARE error....",excl_conf->number);
			break;
		}

		if ( self_excluding_including_conference )
		{
			  
			  vega_log(VEGA_LOG_INFO, "%d want to self exclude in conf %d", number, conf_excl_number);
			  
			  
			  if (excl_conf == NULL) {
				vega_log(ERROR, "la conference C%d n'existe pas", conf_excl_number);
			  }else{
					EVENT ev;
					ev.device_number = number; // l'initiateur de la demande de self ex(in)clusion
					ev.conference_number = conf_excl_number; // la conference de laquelle il ne veut plus de son
					vega_conference_t *C = excl_conf;
					//device_t *D=this;
					switch ( excl_conf->get_state_in_conference(this ) ) 
					{
					case PARTICIPANT_SELF_EXCLUDED : // je suis auto exclu et je veux m'auto inclure dans la conference
						if ( C->is_in(this ) )
						{
							int N= nb_active_conf_where_device_is_active_or_dtor_excluded();

							if ( MAX_SIMULTANEOUS_ACTIVE_CONF_BY_DEVICE==N ) {
								vega_log(INFO,"D%d EVT_CONFERENCE_ACTIVATED C%d IGNORED yet active in too much conf:%d",number, conf_excl_number,N);
								device_line_printf(4,"%d:trop de conf ........",N);
							}else{
								C->set_state_in_conference(this,PARTICIPANT_ACTIVE);
								// reconnecter le ts de cette conference au premier subchannel libre
								if ( !start_hearing_in_aga(C) ) start_hearing_in_aslt(C);
								
								vega_log(INFO,"D%d EVT_CONFERENCE_SELF_INCLUDE C%d number of conf:%d",number, conf_excl_number,N);

								vega_event_log(EV_LOG_DEVICE_INCLUDED, number, 0, C->number,"%s s est autoinclus dans %s", name, C->name);

								// pdl 20090911 bug: si on essaie d'exclure un poste deja ds 4 conferences
								//set_green_led_possible_conferences();
								set_green_led_of_conf(C,1);
								set_red_led_of_conf(C,1);

								device_MAJ_particpant_key(this,0,PARTICIPANT_ACTIVE);
								device_line_printf(4,"D%d:autoinclus C%d........",number,C->number);
							}
						}
						ev.code = EVT_CONFERENCE_SELF_INCLUDE; // le dire a tout le monde ( station leds )
						BroadcastEvent(&ev);
						break;

					case PARTICIPANT_ACTIVE : // je suis actif dans une conference, je veux m'auto exclure !
						if ( C->is_in(this ) ) // pdl 20090915 faire la mise a jour avant le broadcast
						{
							C->set_state_in_conference(this,PARTICIPANT_SELF_EXCLUDED) ; // jamais de refus sur l'auto exclusion

							stop_hearing_in_aslt(C);
							stop_hearing_in_aga(C);		

							vega_event_log(EV_LOG_DEVICE_EXCLUDED, number,0,C->number,"%s s est autoexclu de %s", name, C->name);
							
							//if ( C->number == current_conference ) // pdl 20090629: meme si on parle dans une autre conf, effacer la conf
							{
								set_green_led_of_conf(C,0);
								set_red_led_of_conf(C,1);
								if ( C->number == current_conference ) { // si on parle dans cette conf, mettre a jour les leds des participants 
									this->device_MAJ_particpant_key(this,0,PARTICIPANT_SELF_EXCLUDED);
								}
							}
							if ( this==C->participant_director.device ) { // pdl 20090915 lorsque le directeur s'exclue lui meme, c'est une autoexclusion
								device_line_printf(4,"DTR%d autoexclu %d........",number, C->number);
							}else
								device_line_printf(4,"D%d autoexclu %d........",number, C->number);

						}

						ev.code = EVT_CONFERENCE_SELF_EXCLUDE;	
						BroadcastEvent(&ev);
						break;

					case PARTICIPANT_DTOR_EXCLUDED:
						vega_log(ERROR, "REFUSE SELF EXCLUSION UNTIL DTOR DO NOT REINCLUDE DEVICE !!!");
						device_line_printf(4,"Demander au Dtor%d.......",excl_conf->participant_director.device->number);
						break;
					case NOT_PARTICIPANT :
						vega_log(ERROR, "REFUSE when device REMOVED !!!");
						device_line_printf(4,"Deja supprime C%d.......",excl_conf->number);
						break;
					}
			  }
		}
		else if (activating_conference)/* toggle conference state */	
		{
		  int conf_number = ev->value.select.conference_number; 
		  vega_conference_t *conf = vega_conference_t::by_number(conf_number);
		  if (conf == NULL) {
			vega_log(VEGA_LOG_ERROR, "conference %d doesn't exists", conf_number);
			break;
		  }
		  if ( !conf->is_in(this) ) {
			vega_log(VEGA_LOG_ERROR, "device %d is not participant in conference %d", number, conf->number);
			break;
		  }

		  //dump_alphacom_rack();

		  // tester si les 2 cartes AGA conf->board_number et conf->conference_mixer_board_number ne sont pas en anomalie !!!
		  /*if ( 0==alarms_board_is_in_anomaly(conf->matrix_configuration->board_number) ){
			vega_log(VEGA_LOG_ERROR, "D%d C%d board %d ANOMALY", number, conf->number, conf->matrix_configuration->board_number);
			break;
		  }
		  if ( 0==alarms_board_is_in_anomaly(conf->matrix_configuration->conference_mixer_board_number) ){
			vega_log(VEGA_LOG_ERROR, "D%d C%d board %d ANOMALY", number, conf->number, conf->matrix_configuration->conference_mixer_board_number);
			break;
		  }*/

		  if ( !conf->active ) // est elle deja active ?
		  {
				// NON, on passe SELF_EXCLUDED si deja 4 conferences en cours d'ecoute !!!!vega_conference_set_active_participant(conf,this);			
				// todo MAX_SIMULTANEOUS_ACTIVE_CONFERENCES
				int N= nb_active_conf_where_device_is_active_or_dtor_excluded() ;
				if (1)// pdl 20090911 on peut toujours activer une conference pour les autres, si on ecoute deja 4 conf, on sera auto exclu
					//!!! N < MAX_SIMULTANEOUS_ACTIVE_CONF_BY_DEVICE ) 
				{
					vega_log(INFO, "ConfHearingState D%d ACTIVATED C%d (already %d conf)", number, conf->number,N);
					

					conf->active = 1;
					conf->device_initiator = this; 

					conf->activate_radio_base(this); // 20090527
					EVENT ev;ev.code = EVT_CONFERENCE_START_TONE; 
					ev.device_number = number; // l'initiateur
					ev.conference_number = conf->number;
					BroadcastEvent(&ev);// envoi a tous les devices
					vega_event_log(EV_LOG_CONFERENCE_ACTIVATED, number, 0, conf->number, "%s initiateur de %s", name, conf->name);

					device_line_printf(4,"INITIE C%d TONE........", conf->number);

				}
		  }else{// todo: verify the device is initiator or director to accept DEACTIVATION of the conference !!!
			  if ( conf->device_initiator == this || conf->participant_director.device==this )
			  {
					vega_log(INFO, "ConfHearingState D%d DEACTIVATED C%d", number, conf->number);
					EVENT ev;
					ev.code = EVT_CONFERENCE_DEACTIVATED; // 
					ev.device_number = number; // l'initiateur de la desactivation de la conference
					ev.conference_number = conf->number;
					conf->active = 0;
					vega_log(INFO, "EVT_CONFERENCE_DEACTIVATED D%d DEACTIVATED C%d", number, conf->number);
					//device_line_printf(1,"DEACTIVE C%d........", conf->number);
					vega_event_log(EV_LOG_CONFERENCE_DEACTIVATED, number, 0, conf->number, "%s desactive %s", name, conf->name);
					BroadcastEvent(&ev); // tell all devices to exclude from this conf and update their display

					device_line_printf(4,"DESACTIV C%d........", conf->number);

			  }else{
				vega_log(VEGA_LOG_INFO, "ConfHearingState D%d cannot DEACTIVATE C%d, not initiator or director", number, conf->number);
			  }
		  }
		}
		else if ( excluding_including_conference ) // cas d'un appui sur une touche EVT_DEVICE_KEY_SELECT_CONFERENCE
		{	// demande de reinclusion au directeur depuis un poste exclu !
			  int conf_excl_number = ev->value.select.conference_number;
			  vega_log(VEGA_LOG_INFO, "D%d want  to be reincluded in conf C%d", number, conf_excl_number);
			  vega_conference_t *excl_conf = vega_conference_t::by_number(conf_excl_number);
			  if (excl_conf != NULL)
			  {// check if excluded, possible if not director of new conference !
				  if ( PARTICIPANT_DTOR_EXCLUDED == excl_conf->get_state_in_conference(this)  ) 
				  { // just update display and leds to reflect this new conference
					EVENT ev;
					ev.code = EVT_CONFERENCE_REINCLUDE_DEMAND; // 
					ev.device_number = number;
					ev.conference_number = excl_conf->number;
					excl_conf->participant_director.device->ExecuteStateTransition(&ev); // envoi au directeur une demande de reinclusion

					vega_event_log(EV_LOG_DEVICE_ASK_REINCLUSION, number, 0, excl_conf->number, "%s demande de reinclusion dans %s", name, excl_conf->name);
				  }else{
					  vega_log(VEGA_LOG_INFO, "D%d is NOT DTOR_EXCLUDED from C %d!!!",number, conf_excl_number);
				  }
			  }
		}
		else		// ALTERNATE=SWITCH between 2 (or more) conferences ( todo:can it happen while pressing M ??? )
		{
			  int new_conf_number = ev->value.select.conference_number;//action->action.select_conf.conference_number;
			  
			  //printf("SWITCH FROM C%d to C%d ?\n", current_conference, ev->value.select.conference_number);
			  vega_log(INFO, "D%d SWITCH FROM C%d to C%d ?\n", number, current_conference, ev->value.select.conference_number);

			  vega_conference_t *old_conf = vega_conference_t::by_number(current_conference);
			  vega_conference_t *new_conf = vega_conference_t::by_number(new_conf_number);

			  if ( new_conf == NULL) {
				vega_log(VEGA_LOG_ERROR, "new_conf %x doesn't exists", new_conf);
			  }else{
				  if ( !new_conf->is_in(this) ){ 
					vega_log(VEGA_LOG_ERROR, "device %d is not in conference %d", number, new_conf->number);
					break;
				  }
				  if (!new_conf->active) {
					vega_log(VEGA_LOG_ERROR, "not ACTIVE new conference %d",new_conf->number);
					break;
				  }
				  if ( current_conference == new_conf_number)
				  { // le device est deja en train de parler ds cette conference et voir les leds des participants
					  vega_log(VEGA_LOG_ERROR, "device %d ALREADY in conference %d", number, new_conf->number);
				  }
				  else
				  {
					  vega_event_log(EV_LOG_DEVICE_SWITCH_CONFERENCE, number, 0, new_conf->number, "%s passe dans %s", name, new_conf->name);
					  
					  vega_log(INFO, "D%d want to switch from C%d to C%d to speak", number, current_conference, new_conf->number);
					  
					  current_conference = new_conf->number;

					  // 20090526 : on ne switch plus les timeslot entre AGA et ASLT !!!!

					  if (old_conf ) device_MAJ(old_conf,0,1,1); // juste update participants (hide) led of old conf
					  device_MAJ_participants_led(new_conf,UNHIDE); // pdl 20090928 ( reactualiser l'affichage des participants ) 
					  
					  {
						  device_line_printf(2,"C%d:%s(%d actives)              ", current_conference, new_conf->name, nb_active_conf_where_device_is_active_or_dtor_excluded() );	  
						  if(new_conf->participant_director.device) device_line_printf(3,"Dtor:%s......",new_conf->participant_director.device->name ) ;
						  device_line_printf(4,"passe dans C%d........",new_conf->number);
					  }

				  }
			  }
		}
		}
		break;

 	case EVT_CONFERENCE_DEACTIVATED:
		{
			vega_log(INFO,"ConfHearingState:*** EVT_CONFERENCE_DEACTIVATED D%d C%d *******\n",ev->device_number, ev->conference_number );
			vega_conference_t *old_conf = vega_conference_t::by_number(ev->conference_number);
			if ( old_conf && old_conf->is_in(this) ) 
			{
				stop_hearing_in_aga(old_conf); // todo: si je suis exclu,, inutile !!!!
				stop_hearing_in_aslt(old_conf);
				
				old_conf->set_state_in_conference(this,PARTICIPANT_ACTIVE); // au cas ou on aurait ete exclu, on repartira non exclu !!!

				set_green_led_of_conf(old_conf,1);
				set_red_led_of_conf(old_conf,0);

				if ( current_conference == old_conf->number ) 
					device_MAJ_participants_led(old_conf,1); // hide all participants

				vega_conference_t *C = vega_conference_t::by_number(current_conference);
				if ( C){
					device_line_printf(2,"C%d:%s(%d activ).......", current_conference, C->name, 
						nb_active_conf_where_device_is_active_or_dtor_excluded() );	  
				}
				device_line_printf(4,"C%d DESACTIVEE........", old_conf->number);
				
				{ // send a tone to signify the end of a conference the device was participating in
					EVENT evt;
					old_conf->activate_radio_base(this); // 20090527
					evt.code = EVT_CONFERENCE_START_DEACTIVE_TONE;; // 
					evt.conference_number = ev->conference_number;
					BroadcastEvent(&evt);
				}
			}
		}
		break;

	case EVT_CONFERENCE_START_DEACTIVE_TONE:
		{
			int NC = ev->conference_number;
			vega_conference_t* C = vega_conference_t::by_number(NC);
			if ( C && C->is_in(this) ) 
			{ 
				vega_log(INFO,"D%d EVT_CONFERENCE_START_DEACTIVE_TONE IN C%d initiator D%d\n", number, NC , ev->device_number);
				start_hearing_tone(TS_RING_TONE);				
				{
					EVENT evt;
					evt.code = EVT_CONFERENCE_STOP_DEACTIVE_TONE;
					evt.device_number = ev->device_number; // transmit the initiator of group call
					evt.conference_number = ev->conference_number;
					DeferEvent(&evt,2);
				}
			}
		}
		break;//EVT_CONFERENCE_STOP_TONE

	case EVT_CONFERENCE_STOP_DEACTIVE_TONE: 
		{
			int NC = ev->conference_number;
			vega_log(INFO,"D%d EVT_CONFERENCE_STOP_DEACTIVE_TONE IN C%d initiator D%d", number, NC , ev->device_number);
			vega_conference_t* C = vega_conference_t::by_number(NC);
			if ( C && C->is_in(this) ) {
				stop_hearing_tone();
				//mixer_disconnect_timeslot_to_sub_output(&port_aslt, sub_channel_0);
			}
		}
		break;

	case EVT_CONFERENCE_STOP_TONE: 
		{
			int NC = ev->conference_number;
			vega_log(INFO,"D%d EVT_CONFERENCE_STOP_TONE IN C%d initiator D%d\n", number, NC , ev->device_number);
			vega_conference_t* C = vega_conference_t::by_number(NC);

			if ( C->is_in(this) ) {
				stop_hearing_tone();
				//mixer_disconnect_timeslot_to_sub_output(&port_aslt, sub_channel_0);
				{
					EVENT evt;
					evt.code = EVT_CONFERENCE_ACTIVATED;
					evt.device_number = ev->device_number; // transmit the initiator 
					evt.conference_number = ev->conference_number;
					ExecuteStateTransition(&evt);
				}
			}
		}
		break;//EVT_CONFERENCE_STOP_TONE
	
   case EVT_CONFERENCE_DTOR_EXCLUDE:
	  {
			int conf_number = ev->conference_number; // No of conference where include is happening ...
			int dev_number = ev->device_number; // Number of device that is supposed to be excluded !
			vega_log(INFO, "D%d: ConfHearingState: EXCLUDE D%d from C%d ( current C%d)",number, dev_number, conf_number, current_conference);
			
			vega_conference_t* C = vega_conference_t::by_number(conf_number);
			device_t *D=device_t::by_number(dev_number);
			
			if ( D && C && C->is_in(this ) ) // est ce que j appartient a la conference ou il vient d'y avoir une exclusion ?
			{
				// refresh my participants if the just added device is in my current conference 
				device_t *otherD=device_t::by_number(ev->device_number);
				if ( C && otherD && C->number==current_conference ) 
					this->device_MAJ_particpant_key(otherD,0,PARTICIPANT_SELF_EXCLUDED);
				device_line_printf(4,"D%d DtorExclu C%d........", ev->device_number, C->number);
			}
	  }
	  break;
   
   case EVT_CONFERENCE_DTOR_INCLUDE	:
	  {
			vega_log(INFO, "ConfHearingState: INCLUDE D%d in C%d",ev->device_number, ev->conference_number);

			vega_conference_t* C = vega_conference_t::by_number(ev->conference_number);
			device_t *D=device_t::by_number(ev->device_number);

			if ( C && C && C->is_in(D) ) {// cas d'un broadcast, verifier que le message nous concerne
				// refresh my participants if the just added device is in my current conference 
				device_t *otherD=device_t::by_number(ev->device_number);
				if ( otherD && C->number==current_conference ) 
					device_MAJ_particpant_key(otherD,0,PARTICIPANT_ACTIVE); // todo: use event EVT_DEVICE_REINCLUDED !!
				device_line_printf(4,"D%d DtorInclu C%d........", ev->device_number, C->number);
			}
	  }
	  break;//EVT_CONFERENCE_DTOR_INCLUDE

  case EVT_CONFERENCE_SELF_INCLUDE	:
	  {
			int conf_number = ev->conference_number; // No of conference where include is happening ...
			int dev_number = ev->device_number; // No of device that is supposed to be included !
			vega_log(INFO, "ConfHearingState: INCLUDE D%d in C%d",dev_number, conf_number);

			vega_conference_t* C = vega_conference_t::by_number(conf_number);
			device_t *D=device_t::by_number(dev_number);

			if ( C && D && C->is_in(D) ) {// cas d'un broadcast, verifier que le message nous concerne
				// refresh my participants if the just added device is in my current conference 
				device_t *otherD=device_t::by_number(ev->device_number);
				if ( C && otherD && C->number==current_conference ) 
					device_MAJ_particpant_key(otherD,0,PARTICIPANT_ACTIVE); // todo: use event EVT_DEVICE_REINCLUDED !!
				device_line_printf(4,"D%d SelfInclu C%d........", dev_number, conf_number);
			}
	  }
	  break;

  case EVT_CONFERENCE_SELF_EXCLUDE:
	  {
			int conf_number = ev->conference_number; // No of conference where include is happening ...
			int dev_number = ev->device_number; // Number of device that is supposed to be excluded !
			vega_log(INFO, "D%d: ConfHearingState: EXCLUDE D%d from C%d ( current C%d)",number, dev_number, conf_number, current_conference);
			
			vega_conference_t* C = vega_conference_t::by_number(conf_number);
			device_t *D=device_t::by_number(dev_number);
			
			if ( C && D && C->is_in(D ) ){
				// refresh my participants if the just added device is in my current conference 
				device_t *otherD=device_t::by_number(ev->device_number);
				if ( C && otherD && C->number==current_conference ) 
					this->device_MAJ_particpant_key(otherD,0,PARTICIPANT_SELF_EXCLUDED);			
				device_line_printf(4,"D%d SelfExclu C%d........", dev_number, conf_number);
			}
	  }
	  break;

	case EVT_CONFERENCE_START_TONE:
		{
			int NC = ev->conference_number;
			vega_log(INFO,"D%d START_TONE IN C%d initiator D%d\n", number, NC , ev->device_number);
			vega_conference_t* C = vega_conference_t::by_number(NC);
			if ( C && C->is_in(this) ) {
				// envoyer le tone activation CONF tous les devices  
				start_hearing_tone(TS_CONFERENCE_TONE);//mixer_connect_timeslot_to_sub_output(&port_aslt, sub_channel_0, TS_CONFERENCE_TONE /*TS_BUSY_TONE TS_GENERAL_CALL_TONE*/, 0);		
				{
					EVENT evt;
					evt.code = EVT_CONFERENCE_STOP_TONE;
					evt.device_number = ev->device_number;
					evt.conference_number = ev->conference_number;
					DeferEvent(&evt,2);
				}
				device_line_printf(4,"TONE C%d ACTIVEE........", C->number);
			}
		}
		break;

	case EVT_CONFERENCE_ACTIVATED: 
		{	// qqun a activee notre 2eme conference DONC PAS SELECTIONNEE pour parler ( sauf pour l'initiateur ? ) !!!
			int new_conf_number = ev->conference_number;
			// est ce qu'on fait partie de cette conference ?
			vega_conference_t* C = vega_conference_t::by_number(new_conf_number);

			if ( C && C->is_in(this) )
			{
				C->active=1;// pdl 20090911 impossible de refuser une activation de conference, le premier device qui recoit l'event active la conf
				// faire comme si on allait participer a cette conf, avant de tester si c'est plus de 4 !!! // pdl 20090911
				C->set_state_in_conference(this,PARTICIPANT_ACTIVE);

				int N= nb_active_conf_where_device_is_active_or_dtor_excluded() ;

				vega_log(INFO,"******* D%d EVT_CONFERENCE_ACTIVATED C%d active_or_dtor_excluded in %d conferences",number,C->number, N);

				if ( N > MAX_SIMULTANEOUS_ACTIVE_CONF_BY_DEVICE ) { // pdl 20090911 si plus de 4, on est auto exclu !!!!
					vega_log(INFO,"D%d EVT_CONFERENCE_ACTIVATED C%d IGNORED yet active in too much conf:%d",number, new_conf_number,N);
					printf("D%d EVT_CONFERENCE_ACTIVATED C%d IGNORED yet active in too much conf:%d",number, new_conf_number,N);
					C->set_state_in_conference(this,PARTICIPANT_SELF_EXCLUDED);
					set_green_led_of_conf(C,0);
					set_red_led_of_conf(C,1);
					device_line_printf(4,"C%d ACTIVEE:autoexclu...", C->number);
				}else{
					vega_log(INFO,"D%d EVT_CONFERENCE_ACTIVATED already %d conf (currently in C%d )",number,N,current_conference);

					device_line_printf(2,"C%d:%s(%d activ).......", current_conference, C->name, 
						nb_active_conf_where_device_is_active_or_dtor_excluded() );	  
					device_line_printf(4,"C%d:%dieme ACTIVEE.......", C->number,N);
					if ( N==1 ) //0==current_conference)
					{
						current_conference = C->number; // bug christian 20090604
						device_MAJ_participants_led(C,UNHIDE);
					}
					
					if ( !start_hearing_in_aga(C) ) 
						start_hearing_in_aslt(C);
					
					C->set_state_in_conference(this,PARTICIPANT_ACTIVE);
					set_green_led_of_conf(C,1);
					set_red_led_of_conf(C,1);
				}
			}
		}
		break;

	case EVT_DEVICE_KEY_MICRO_PRESSED:
	  {

		  printf("MICRO_PRESSED D%d\n",number);
		  vega_log(INFO,"MICRO_PRESSED D%d\n",number);

		  double tt = time_between_button(&_tstart_btn_micro);
		  if ( tt < 1.0 ){vega_log(INFO,"IGNORED EVT_DEVICE_KEY_MICRO_PRESSED within %f seconds\n",tt);break;}
		  gettimeofday(&_tstart_btn_micro, NULL); // memoriser 
		  
		  if ( doing_group_call && ( number == device_doing_group_or_general_call ))
		  {
			  device_open_micro();
			  device_line_printf(4,"speak in G%d",doing_group_call);
			  vega_event_log(EV_LOG_DEVICE_START_SPEAKING, number, 0, doing_group_call, "%s commence a parler au groupe %d", name,doing_group_call );
			  break;
		  }
		  else if ( doing_general_call && ( number == device_doing_group_or_general_call ))	
		  {
			  vega_log(INFO,"DTOR%d speak GAL\n",number);
			  device_open_micro();
			  device_line_printf(4,"speak general ....");	
			  vega_event_log(EV_LOG_DEVICE_START_SPEAKING, number, 0, number, "%s commence a parler sur appel general", name );
			  break;
		  }


			//device_line_printf(4,"MICRO C%d.........",current_conference);

		if ( current_conference <= 0 ) {
			vega_log(ERROR, "D%d error current_conference %d!", number, current_conference);
			device_line_printf(4,"no active conference.........");
			break;
		}

		vega_conference_t *curconf = vega_conference_t::by_number(current_conference);
		if (curconf)
		{
			if (  !curconf->active  ) {
				vega_log(INFO, "D%d current conference C%d INACTIVE !", number, curconf->number);
				device_line_printf(4,"C%d DEACTIVATED.........",current_conference);
				device_MAJ_participants_led(curconf,1);// hide_all
				break;
			}
			if (PARTICIPANT_ACTIVE != curconf->get_state_in_conference(this)  )  
			{
				vega_log(INFO, "D%d not PARTICIPANT_ACTIVE in C%d",number, curconf->number );
				stop_hearing_in_aslt(curconf); // cas ou le device est dtor_excluded pdt qu'il parle
				stop_hearing_in_aga(curconf);

				if (PARTICIPANT_DTOR_EXCLUDED == curconf->get_state_in_conference(this)  ) // pdl 20090930
					device_line_printf(4,"DTR EXCLU de C%d.........",current_conference);
				if (PARTICIPANT_SELF_EXCLUDED == curconf->get_state_in_conference(this)  ) 
					device_line_printf(4,"AUTO EXCLU de C%d.........",current_conference);

				break;
			}
			//device_check_timselot_duplication(current_conference);
			int res_to_speak_found = 0; // director must be OK, secondary CRM4 try to find a free AGA room to speak in conference
			int room=-1;

			if (this == curconf->participant_director.device) 
			{
				res_to_speak_found = 1;

				//device_line_printf(1,"DTOR%d[R%d,R%d,J%d]        ",room,curconf->radio1_device(),curconf->radio2_device(),curconf->jupiter2_device() );
				vega_log(INFO, "DTOR D%d of C%d SPEAKING", number, curconf->number);

				director_start_speaking(curconf);	// EVT_DEVICE_KEY_MICRO_PRESSED

				
				//device_line_printf(1,"DTOR%d [R%d,R%d,J%d]        ",number,curconf->radio1_device(),curconf->radio2_device(),curconf->jupiter2_device() );

			}else{

				room = curconf->alloc_speaking_ressource(this);
				vega_log(INFO, "D%d SECONDARY of C%d REQUIRE SPEAKING ROOM %d", number, curconf->number, room);

				//device_line_printf(1,"ROOM%d[R%d,R%d,J%d]        ",room,curconf->radio1_device(),curconf->radio2_device(),curconf->jupiter2_device() );
				if ( PORT_RESERVED_INVALID != room  )
				{
					res_to_speak_found = 1;

					secondary_start_stop_speaking(curconf, (port_reservation_t)room, 1 );
				}
			}			
			
			if (res_to_speak_found)
			{
				b_speaking=true;

				vega_event_log(EV_LOG_DEVICE_START_SPEAKING, number, 0, curconf->number, "%s commence a parler dans %s", name, curconf->name);

				//avoid_hear_myself_in_aga_3conf(curconf);
				device_open_micro();   

				set_recorder_dry_contact_on_off(current_conference ,1);  // to start the recording

				curconf->activate_radio_base(this);
				
				CRM4::all_devices_display_led_of_device_speaking_in_conf(curconf, this);

				//device_line_printf(2,"D%d:%s                        ",number,name);
				//device_line_printf(3,"C%d:%s                        ",curconf->number,curconf->name);	 			
				/*device_line_printf(4,"%03d AGA%03d %03d %03d",
					(port_aslt.mixer.sub_channel1.connected   ? port_aslt.mixer.sub_channel1.tslot   : 0),
					(port_3confs.mixer.sub_channel0.connected ? port_3confs.mixer.sub_channel0.tslot : 0),
					(port_3confs.mixer.sub_channel1.connected ? port_3confs.mixer.sub_channel1.tslot : 0),
					(port_3confs.mixer.sub_channel3.connected ? port_3confs.mixer.sub_channel3.tslot : 0)
				 );*/

			
				if ( display_slots ) {

					device_line_printf(4,"%03d AGA%03d %03d %03d", 
						(port_aslt.mixer.sub_channel1.connected   ? port_aslt.mixer.sub_channel1.tslot   : 0),
						(port_3confs.mixer.sub_channel0.connected ? port_3confs.mixer.sub_channel0.tslot : 0),
						(port_3confs.mixer.sub_channel1.connected ? port_3confs.mixer.sub_channel1.tslot : 0),
						(port_3confs.mixer.sub_channel3.connected ? port_3confs.mixer.sub_channel3.tslot : 0)
					 );
				}
				else
					device_line_printf(4,"parle ds C%d room %d.......", current_conference, room );


			}else{
				//device_line_printf(2,"D%d:%s                        ",number,name);
				//device_line_printf(3,"C%d:%s                        ",curconf->number,curconf->name);	 			
				device_line_printf(4,"BUSY in C%d                         ",current_conference);
				/*device_line_printf(4,"%03d AGA%03d %03d %03d",
					(port_aslt.mixer.sub_channel1.connected   ? port_aslt.mixer.sub_channel1.tslot   : 0),
					(port_3confs.mixer.sub_channel0.connected ? port_3confs.mixer.sub_channel0.tslot : 0),
					(port_3confs.mixer.sub_channel1.connected ? port_3confs.mixer.sub_channel1.tslot : 0),
					(port_3confs.mixer.sub_channel3.connected ? port_3confs.mixer.sub_channel3.tslot : 0)
				 );*/
			}
		}
		//device_check_timselot_duplication(current_conference);
	  }// case
	  break;

	case EVT_DEVICE_KEY_MICRO_RELEASED: // todo: evaluate what happens when switch to idlestate and button M still pressed !!!!
	   {
			printf("MICRO_RELEASED D%d\n",number);
			vega_log(INFO,"MICRO_RELEASED D%d\n",number);

			device_close_micro();

			//device_line_printf(4,"BUSY                          ");
			device_line_printf(4,"                          ");

			if ( doing_group_call && ( number == device_doing_group_or_general_call ))
			{
				vega_log(INFO,"D%d stop speak G%d\n",number, doing_group_call);
				device_line_printf(4,"stop speak in G%d",doing_group_call);
				vega_event_log(EV_LOG_DEVICE_STOP_SPEAKING, number, 0, doing_group_call, "%s ne parle plus dans le groupe G%d", name,doing_group_call );
				break;
			}
			else if ( doing_general_call && ( number == device_doing_group_or_general_call ))	
			{
				vega_log(INFO,"DTOR%d stop speak GAL\n",number);
				device_line_printf(4,"stop appel general ....");	
				vega_event_log(EV_LOG_DEVICE_STOP_SPEAKING, number, 0, number, "%s ne parle plus dans l'appel general", name );
				break;
			}

			vega_conference_t *conf = vega_conference_t::by_number(current_conference);
			if ( conf)		   
			{
				if (  !conf->active  ) {
					vega_log(INFO, "D%d current conference C%d INACTIVE !", number, conf->number);
					device_line_printf(4,"C%d DESACTIVEE.........",current_conference);
					device_MAJ_participants_led(conf,1);// hide_all
					break;
				}

			   vega_event_log(EV_LOG_DEVICE_STOP_SPEAKING, number, 0, conf->number, "%s ne parle plus dans %s", name, conf->name);

			   b_speaking=false;

			   if ( conf->nb_people_speaking() == 0 ){
				   conf->deactivate_radio_base(this); // if still some other people speaking in conference
				   set_recorder_dry_contact_on_off(current_conference ,0); // stop recording
			   }

			   if (this == conf->participant_director.device ) {/// il s'agit du directeur 

				   vega_log(INFO, "DIRECTOR D%d of C%d STOP SPEAKING", number, conf->number);
				   director_stop_speaking(conf); // EVT_DEVICE_KEY_MICRO_RELEASED
			   
			   }else{		   

					vega_log(INFO, "D%d SECONDARY of C%d RELEASE SPEAKING RESOURCE", number, conf->number);
					int room = conf->free_speaking_ressource(this);
					if ( PORT_RESERVED_INVALID != room  ){
						secondary_start_stop_speaking(conf, (port_reservation_t)room, 0 );
					}

			   }	  		

			   all_devices_display_led_of_device_NOT_speaking_in_conf(conf, this);  // AFFICHAGE ECRAN + LED
			   if (0)
			   {
					char str[128]="HEAR ";
					// display all conferences he can hear, eg where D is active ( not excluded ) !
					int NB=0;
					foreach(vega_conference_t *cc,vega_conference_t::qlist)
					{
						//vega_conference_t *cc = (vega_conference_t *)g_list_nth_data(conferences, k);
						if ( cc && cc->active && (PARTICIPANT_ACTIVE==cc->get_state_in_conference(this) ) ) {
							NB++;
							char s[32]={0};
							snprintf(s,sizeof(s)-1,"C%d",cc->number ) ;
							strncat(str, s, sizeof(str)-1);
						}
					}
					//device_line_printf(3, "%s (%d total).......", str ,NB);

					if ( display_slots ) {

					device_line_printf(4,"%03d AGA%03d %03d %03d",
						(port_aslt.mixer.sub_channel1.connected   ? port_aslt.mixer.sub_channel1.tslot   : 0),
						(port_3confs.mixer.sub_channel0.connected ? port_3confs.mixer.sub_channel0.tslot : 0),
						(port_3confs.mixer.sub_channel1.connected ? port_3confs.mixer.sub_channel1.tslot : 0),
						(port_3confs.mixer.sub_channel3.connected ? port_3confs.mixer.sub_channel3.tslot : 0)
					 );
					}
			   }

		   }
	   }    
	   break;

  case EVT_CONFERENCE_REINCLUDE_DEMAND: // a priori, not broadcasted (ExecuteStateTransition)
	  {
			int conf_number = ev->conference_number; // No of conference where exclure is happening ...
			int dev_number = ev->device_number; // No of device that is supposed to be excluded !
			vega_log(INFO, "ConfHearingState: EVT_CONFERENCE_REINCLUDE_DEMAND D%d in C%d director D%d",dev_number, conf_number,number);

#if 1
			// faire clignoter la touche verte de la conference dans laquelle on est directeur et il y a une demande de reinclusion
			if ( m_common_touche_select_conf.contains( ev->conference_number ) )
			{
				int no_toucheC = m_common_touche_select_conf[ev->conference_number];
				device_set_blink_slow_green_color( no_toucheC );
			}
			// faire clignoter la touche verte du device qui demande reinclusion
			if ( m_common_touche_select_device.contains( ev->device_number ) )
			{
				int no_toucheD = m_common_touche_select_device[ev->device_number];
				device_set_blink_slow_green_color( no_toucheD );
			}

			device_line_printf(4,"D%d dde reinclus C%d........",dev_number, conf_number);
#else
			// parcours m_common_keymap_action pour rechercher les keynumber qui ont la valeur "action_select_device"
			// m_common_keymap_action[20]="action_select_device"
			// m_common_keymap_number[20]=1
			QMap<int,QString>::const_iterator i = CRM4::m_common_keymap_action.constBegin();
			while (  i !=   CRM4::m_common_keymap_action.constEnd()    ) {
				if ( i.value() == "action_select_device" ) {
					int no_touche = i.key();
					if ( m_common_keymap_number[no_touche] == dev_number ) {
						device_set_blink_slow_green_color( no_touche );
					}
				}
				i++;
			}

			QMap<int,QString>::const_iterator j = CRM4::m_common_keymap_action.constBegin();
			while (  j !=   CRM4::m_common_keymap_action.constEnd()    ) {
				if ( j.value() == "action_select_conf" ) {
					int no_touche = j.key();
					if ( m_common_keymap_number[no_touche] == conf_number ) {
						device_set_blink_slow_green_color( no_touche );
					}
				}
				i++;
			}

			// faire clignoter la touche verte de la conference dans laquelle on est directeur et il y a une demande de reinclusion
			int keyD = get_key_number_by_action_type(device.crm4.keys_configuration, ACTION_SELECT_DEVICE, dev_number);
			if (keyD ) {
				//vega_log(ERROR, "cannot find any key for action ACTION_SELECT_DEVICE %d", dev_number);
				device_set_blink_slow_green_color( keyD);
			}
			int keyC = get_key_number_by_action_type(device.crm4.keys_configuration, ACTION_SELECT_CONF, conf_number);
			if (keyC  ) {
				device_set_blink_slow_green_color( keyC); // todo: arreter apres 5 secondes !
			}
#endif
			
			start_hearing_tone(TS_RING_TONE);	// brancher le timeslot 2 sur subchannel 0 du aslt du directeur pdt 1 seconde
			//mixer_connect_timeslot_to_sub_output(&port_aslt, sub_channel_0, TS_RING_TONE, 0);
			/*sleep(1);mixer_disconnect_timeslot_to_sub_output(&port_aslt, sub_channel_0);*/

			{
				EVENT evt;
				evt.device_number = ev->device_number;
				evt.conference_number = ev->conference_number;
				evt.code = EVT_CONFERENCE_REINCLUDE_STOP_BLINK;
				this->DeferEvent(&evt,5);
			}
	  }
	  break;

  case EVT_CONFERENCE_REINCLUDE_STOP_BLINK:
	  {
			int conf_number = ev->conference_number; // No of conference where exclure is happening ...
			int dev_number = ev->device_number; // No of device that is supposed to be excluded !
			vega_log(INFO, "ConfHearingState: EVT_CONFERENCE_REINCLUDE_STOP_BLINK  D%d in C%d director D%d",dev_number, conf_number,number);

			stop_hearing_tone();//mixer_disconnect_timeslot_to_sub_output(&port_aslt, sub_channel_0);


			// STOP faire clignoter la touche verte de la conference dans laquelle on est directeur et il y a une demande de reinclusion
			if ( m_common_touche_select_conf.contains( ev->conference_number ) )
			{
				int no_toucheC = m_common_touche_select_conf[ev->conference_number];
				device_set_no_green_color( no_toucheC );
			}
			// STOP faire clignoter la touche verte du device qui demande reinclusion
			if ( m_common_touche_select_device.contains( ev->device_number ) )
			{
				int no_toucheD = m_common_touche_select_device[ev->device_number];
				device_set_no_green_color( no_toucheD );
			}

			//vega_conference_t *curconf = vega_conference_t::by_number(current_conference);
			//set_led_conference( curconf ) ;

			// arreter de faire clignoter la touche verte de la conference dans laquelle on est directeur et il y a une demande de reinclusion
			/*int keyC = get_key_number_by_action_type(device.crm4.keys_configuration, ACTION_SELECT_CONF, conf_number);
				if (keyD <= 0 ) {
					vega_log(ERROR, "cannot find any key for action ACTION_SELECT_CONFERENCE %d", conf_number);
				}
			//device_set_blink_slow_green_color( keyC); // todo: arreter apres 5 secondes !
			//device_set_blink_slow_green_color( keyD); // todo: arreter apres 5 secondes !
			device_set_no_green_color( keyC); 
			device_set_color_green(keyC); */

			/*int keyD = get_key_number_by_action_type(device.crm4.keys_configuration, ACTION_SELECT_DEVICE, dev_number);
			if (keyD  ) {
				device_set_no_green_color( keyD); 
			}*/

			//maj participant no dev_number
			
			// maj led conference sur le device exclu:
			/*CRM4 *D=dynamic_cast<CRM4*> ( (device_t*)device_t::by_number(dev_number) );
			if ( D ) {
				this->device_MAJ_particpant_key(D,0,curconf->get_state_in_conference(D)); 
				D->set_led_conference(curconf);
			}*/
	  }
	  break;

	  
  case EVT_DEVICE_KEY_SELF_EXCLUSION_INCLUSION: // allow someone to self ex(in)clude from a conference
		{
			if ( activating_conference ) break;
			if ( excluding_including_conference ) break;
			if ( !self_excluding_including_conference )
			{
				self_excluding_including_conference = 1;
				int key_button = 2;//get_key_number_by_action_type(device.crm4.keys_configuration, ACTION_ON_OFF_CONF, -1);
				vega_log(INFO, "device %d ConfHearingState:*** START SELF EX(IN)CLUDING CONFERENCE current conf %d", number, current_conference );			  
				device_set_blink_slow_red_color( key_button);
				device_set_blink_slow_green_color( key_button);   
			}else{
				self_excluding_including_conference = 0;
				vega_log(INFO, "device %d ConfHearingState:*** STOP SELF EX(IN)CLUDING CONFERENCE current conf %d", number, current_conference );			  
				int key_button = 2;//get_key_number_by_action_type(device.crm4.keys_configuration, ACTION_ON_OFF_CONF, -1);		  
				device_set_no_red_color( key_button); 
				device_set_no_green_color( key_button); 
			}
		}
		break;



  }/* switch */
  return 0;
}/* ConfHearingState */






