/* 
project: PC control Vega Zenitel 2009
filename: statemachine.c
author: Mustafa Ozveren & Debreuil Philippe
last modif: 20091105
desciption: state machine of vega CRM4 station devices*/

#include "const.h"
#include "conf.h"
#include "debug.h"
#include "misc.h"

int CRM4::device_doing_group_or_general_call = 0;
int CRM4::doing_group_call=0; // protect against simultaneous group calls and general calls
int CRM4::doing_general_call=0; // protect against simultaneous group calls and general calls

#if 0
int CRM4::UnknownState(EVENT* ev) // plugged or not present ???
{
	vega_log(INFO,"CRM4 UnknownState D%d =============\n",number );
	switch ( ev->code ) 
	{
	default:
		vega_log(INFO,"D%d CRM4::UnknownState UNHANDLED EVENT code:%d \n", number, ev->code);
		break;

	case EVT_ADD_CRM4_DIRECTOR:
		IdleState(ev);
		break;
	case EVT_ADD_CRM4:
		IdleState(ev);
		break;
	case EVT_REMOVE_DEVICE:
		IdleState(ev);
		break;

	case EVT_DEVICE_NOT_PRESENT: // recu au debut du scan alphacom, tout le monde est idle !
		vega_log(INFO,"D%d CRM4::UnknownState EVT_DEVICE_NOT_PRESENT code:%d \n", number, ev->code);
		m_is_plugged = false;
		foreach(vega_conference_t *c,vega_conference_t::qlist){
			if ( c) c->set_state_in_conference(this,NOT_PARTICIPANT);
		}
		/*CRM4::all_devices_set_green_led_possible_conferences();
		CRM4::all_devices_display_group_key(0);
		CRM4::all_devices_display_general_call_led(0);*/
		break;

	case EVT_DEVICE_PLUGGED:
		vega_log(INFO,"D%d CRM4::UnknownState EVT_DEVICE_PLUGGED code:%d \n", number, ev->code);
		m_is_plugged = true;
		foreach(vega_conference_t *c,vega_conference_t::qlist){
			//if ( c && c->is_in(this) )
			{
				c->set_state_in_conference(this,PARTICIPANT_ACTIVE);
			}
		}
		int N= nb_active_conf_where_device_is_active_or_dtor_excluded() ;

		ChangeState(&CRM4::IdleState);

			set_green_led_possible_conferences();
			device_MAJ_groups_led(doing_group_call);
			display_general_call_led(device_doing_group_or_general_call );  

			device_line_printf(1,"D%d:%s                        ",number,name);
			device_line_printf(2,"%d conf actives.............",N);	  
			device_line_printf(3,"%d conf actives.............",N);	  
			device_line_printf(4,".............................");

		foreach(vega_conference_t *c,vega_conference_t::qlist)
		{
			//if ( c && c->is_in(this) ) 
			{
				vega_log(INFO,"D%d is in C%d \n", number, c->number);
				if ( c->active ) 
				{
					device_line_printf(4,"C%d ACTIVEE....................",c->number);
					vega_log(INFO,"simulate D%d EVT_CONFERENCE_ACTIVATED C%d \n", number, c->number);
					
					EVENT evt;
					evt.device_number = number;
					evt.conference_number = c->number;
					evt.code = EVT_CONFERENCE_ACTIVATED;
					ExecuteStateTransition(&evt);
				}
			}
		}

		/*{
			EVENT evt;
			evt.device_number = number;
			evt.code = EVT_DEVICE_UPDATE_DISPLAY;
			DeferEvent(&evt,1);
		}*/
		break;
	}
}
#endif

/* no conference activated yet ! general call or group call possible ! */
int CRM4::IdleState(EVENT* ev)
{
	vega_log(INFO,"CRM4 IdleState D%d =============\n",number );
	switch ( ev->code ) 
	{
	default:
		vega_log(INFO,"D%d CRM4::IdleState UNHANDLED EVENT code:%d \n", number, ev->code);
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
					if( current_conference == d->current_conference ) d->device_MAJ_participants_led(C);
			}
		}
		break;

	case EVT_DEVICE_NOT_PRESENT: // recu au debut du scan alphacom, tout le monde est idle !
		vega_log(INFO,"D%d CRM4::EVT_DEVICE_NOT_PRESENT code:%d \n", number, ev->code);
	case EVT_DEVICE_UNPLUGGED:
		{
			vega_log(INFO,"D%d CRM4::IdleState EVT_DEVICE_UNPLUGGED code:%d \n", number, ev->code);
			m_is_plugged = false;
			vega_conference_t *C = vega_conference_t::by_number(current_conference);
			if ( C ) {
				foreach(CRM4* d,qlist_crm4)
					if( current_conference == d->current_conference ) d->device_MAJ_participants_led(C);
			}
		}
		break;

	/*case EVT_DEVICE_NOT_PRESENT: // recu au debut du scan alphacom, tout le monde est idle !
		vega_log(INFO,"D%d CRM4::IdleState EVT_DEVICE_NOT_PRESENT code:%d \n", number, ev->code);
		m_is_plugged = false;
		foreach(vega_conference_t *c,vega_conference_t::qlist){
			if ( c) c->set_state_in_conference(this,NOT_PARTICIPANT);
		}
		CRM4::all_devices_set_green_led_possible_conferences();
		CRM4::all_devices_display_group_key(0);
		CRM4::all_devices_display_general_call_led(0);
		break;

	case EVT_DEVICE_PLUGGED:
		vega_log(INFO,"D%d CRM4::IdleState EVT_DEVICE_PLUGGED code:%d \n", number, ev->code);
		m_is_plugged = true;
		foreach(vega_conference_t *c,vega_conference_t::qlist){
			if ( c) c->set_state_in_conference(this,PARTICIPANT_ACTIVE);
		}
		CRM4::all_devices_set_green_led_possible_conferences();
		CRM4::all_devices_display_group_key(0);
		CRM4::all_devices_display_general_call_led(0);
		break;*/

	case EVT_DEVICE_KEY_CANCEL:
	case EVT_DEVICE_UPDATE_DISPLAY:
		{
			set_green_led_possible_conferences();
			device_MAJ_groups_led(doing_group_call);
			display_general_call_led(device_doing_group_or_general_call );  

			device_line_printf(1,"D%d:%s                        ",number,name);
			device_line_printf(2,"no conference.............");	  
			device_line_printf(3,__DATE__" "__TIME__);//"no conference.............");	  
			//device_line_printf(4,".............................");

		device_line_printf(4,"%03dAGA%03d-%03d-%03d....",
			(port_aslt.mixer.sub_channel1.connected   ? port_aslt.mixer.sub_channel1.tslot   : 0),
			(port_3confs.mixer.sub_channel0.connected ? port_3confs.mixer.sub_channel0.tslot : 0),
			(port_3confs.mixer.sub_channel1.connected ? port_3confs.mixer.sub_channel1.tslot : 0),
			(port_3confs.mixer.sub_channel3.connected ? port_3confs.mixer.sub_channel3.tslot : 0)
		 );

		}
		break;
	
	case EVT_DEVICE_KEY_GENERAL_CALL:		
		{
			printf("EVT_DEVICE_KEY_GENERAL_CALL D%d\n",number);
			/*double tt = time_between_button(&_tstart_btn_general);
			if ( tt < 3 ){vega_log(INFO,"IGNORED EVT_DEVICE_KEY_GENERAL_CALL within %f seconds\n",tt);break;}
			gettimeofday(&_tstart_btn_general, NULL); // memoriser */


			if ( doing_group_call ) {
				device_line_printf(4,"No D%d calling G%d !!!!!!!!!!", device_doing_group_or_general_call,doing_group_call);
				break;
			}

			vega_conference_t* conf_of_dd = vega_conference_t::by_director(this); 
			if ( conf_of_dd ) { // check if we are director in a conference !!!
				if ( !doing_general_call )
				{	// nobody is currently doing a general call 
					doing_general_call = 1;
					device_doing_group_or_general_call = number; // memorise the initiator of general call

					all_devices_display_general_call_led(1);
					device_line_printf(4,"OK D%d appel gal",device_doing_group_or_general_call);
					vega_event_log(EV_LOG_DEVICE_START_GENERAL_CALL, number, 0, conf_of_dd->number, "%s initie un appel general", name);

					{
						/*EVENT evt;
						evt.code = EVT_CALLGENERAL_START_TONE;
						BroadcastEvent(&evt);*/

						//for (int i = 0; i < g_list_length(devices) ; i++) 
						foreach(device_t * D,device_t::qlist_devices)
						{
							//device_t * D = (device_t *)g_list_nth_data(devices, i);
							if ( D->has_conference_in_common( device_doing_group_or_general_call) ) 
							{
								D->start_hearing_tone(0);
								vega_log(INFO,"D%d EVT_CALLGENERAL_START_TONE initiated by D%d (has conf in common)\n", D->number, device_doing_group_or_general_call );
								{
									EVENT evt;
									evt.device_number = D->number;
									evt.code = EVT_CALLGENERAL_STOP_TONE;
									D->DeferEvent(&evt,2);
								}
							}
						}
					}
				}
				else if (doing_general_call) 
				{
					if ( this->number == device_doing_group_or_general_call){

						all_devices_display_general_call_led(0);
						device_line_printf(4,"OK stop gal D%d",device_doing_group_or_general_call);
						{
							foreach(device_t * D,device_t::qlist_devices)//for (int i = 0; i < g_list_length(devices) ; i++) 
							{
								//device_t * D = (device_t *)g_list_nth_data(devices, i);
								if ( D->has_conference_in_common( device_doing_group_or_general_call) )
									if ( D->number != device_doing_group_or_general_call ) 
									{
										vega_log(INFO,"D%d stop_hearing ts APPEL GAL D%d\n", D->number,device_doing_group_or_general_call  );
										D->stop_hearing_tone();
									}

							}

							/*EVENT evt;
							evt.code = EVT_CALLGENERAL_STOP_SPEAKING;
							BroadcastEvent(&evt);*/
						}
						doing_general_call = device_doing_group_or_general_call = doing_group_call = 0;

						vega_event_log(EV_LOG_DEVICE_STOP_GENERAL_CALL, number, 0, conf_of_dd->number, "%s termine un appel general", name);
					}else{

						device_line_printf(4,"NO! stop gal in D%d !!!!",device_doing_group_or_general_call);
					}

				}
			}else{
				device_line_printf(4,"appel general impossible");
				//device_line_printf(4,"not director");
			}
		}
		break;


	case EVT_DEVICE_KEY_GROUP_CALL:
		{
			printf("EVT_DEVICE_KEY_GROUP_CALL D%d doing_group_call:%d device_doing_group_or_general_call:%d\n",number,doing_group_call,device_doing_group_or_general_call);
			vega_log(INFO,"EVT_DEVICE_KEY_GROUP_CALL D%d doing_group_call:%d device_doing_group_or_general_call:%d\n",number,doing_group_call,device_doing_group_or_general_call);
			
			//double tt = time_between_button(&_tstart_btn_general);
			//if ( tt < 3.5 ){vega_log(INFO,"IGNORED EVT_DEVICE_KEY_GROUP_CALL within %f seconds\n",tt);break;}
			//gettimeofday(&_tstart_btn_general, NULL); // memoriser 

			if ( doing_general_call ) {
				device_line_printf(4,"NO D%d calling Gal !!!!!!!!!!", device_doing_group_or_general_call);
				break;
			}

			if ( !is_valid_group_number(ev->group_number) ) {
				vega_log(INFO,"BAD GRP NUMBER G%d !!",ev->group_number);
				device_line_printf(4," BAD GRP G%d !!!!!!!!!!", ev->group_number);
				break;
			}

			//if ( false==b_protect_group_call ) 
			{
				//b_protect_group_call = true;

				if ( !doing_group_call )
				{
					device_doing_group_or_general_call = number; // memorise the initiator of general call
					doing_group_call = ev->group_number;


					all_devices_display_general_call_led(1);
					all_devices_display_group_key(1);
					
					device_line_printf(4,"appel group G%d (%d)........", ev->group_number,doing_group_call);

					vega_log(INFO,"D%d G%d EVT_DEVICE_KEY_GROUP_CALL doing_group_call:%d *******\n",number, ev->group_number,doing_group_call);
					vega_event_log(EV_LOG_DEVICE_START_GROUP_CALL, number, 0, ev->group_number, "%s initie un appel de groupe %d", name, ev->group_number);

					foreach(device_t * D,device_t::qlist_devices)//for (int i = 0; i < g_list_length(devices) ; i++) 
					{
						//device_t * D = (device_t *)g_list_nth_data(devices, i);
						// parcourir tous les devices qui ont cette touche de groupe doivent l'afficher!
						
						/*EVENT evt;
						evt.code = EVT_CALL_GROUP_START_TONE;
						evt.group_number = doing_group_call;
						BroadcastEvent(&evt);*/

						if ( is_device_number_in_group(D->number,doing_group_call ) )///|| device_doing_group_or_general_call==this->number )  ) 
						{	// le device a l'origine de l'appel de groupe n'est pas forcement dans le groupe, il a juste la touche de groupe
							D->start_hearing_tone(1); 
							//device_MAJ_groups_led(doing_group_call);
							EVENT evt;
							evt.code = EVT_CALL_GROUP_STOP_TONE;
							evt.group_number = doing_group_call;
							evt.device_number = device_doing_group_or_general_call; // transmit the initiator of group call
							D->DeferEvent(&evt,2);
						}else{

						}
					}
				}
				else if (doing_group_call  ) 
				{
					if ( number == device_doing_group_or_general_call )
					{					
						if ( ev->group_number == doing_group_call)
						{
							//for (int i = 0; i < g_list_length(devices) ; i++) 
							foreach(device_t * D,device_t::qlist_devices)
							{
								//device_t * D = (device_t *)g_list_nth_data(devices, i);
								/*EVENT evt;
								evt.code = EVT_CALL_GROUP_STOP_SPEAKING;
								evt.group_number = doing_group_call;
								BroadcastEvent(&evt); // tout le monde va faire un stop_hearing_group*/

								if ( is_device_number_in_group(D->number,doing_group_call) ) 
								{
									if ( device_doing_group_or_general_call == D->number ) {
										vega_log(INFO," j'ai initie l'appel de groupe, mais je ne l'ecoute pas");
									}else {
										vega_log(INFO,"D%d STOP group G%d\n", D->number, doing_group_call );
										D->stop_hearing_tone();
										//D->device_MAJ_groups_led(0);
										CRM4* crm4 = dynamic_cast<CRM4*>(D);
										if ( crm4 ) crm4->device_MAJ_groups_led(0);
									}
								}
							}


							vega_event_log(EV_LOG_DEVICE_STOP_GROUP_CALL, number, 0, ev->group_number, "%s termine un appel de groupe G%d", name, ev->group_number);
							
							all_devices_display_general_call_led(0);
							all_devices_display_group_key(0);

							device_line_printf(4,"OK stop G%d in D%d.............",doing_group_call,device_doing_group_or_general_call);
							
							doing_general_call = device_doing_group_or_general_call = doing_group_call = 0;
						}else{
							device_line_printf(4,"NO! btn G%d for G%d !!!!",ev->group_number,doing_group_call);
						}

					}else{

						device_line_printf(4,"NO! arreter G%d (D%d) !!!!",doing_group_call,device_doing_group_or_general_call);

					}
				}

				//b_protect_group_call = false;
			}
		}
		break;


	case EVT_REMOVE_DEVICE:// todo: device can be excluded from a conference but stay active in an other conference !!!
		{
			vega_log(INFO,"IdleState:*** EVT_REMOVE_DEVICE D%d from C%d *******\n",ev->device_number, ev->conference_number );
			vega_conference_t *C = vega_conference_t::by_number(ev->conference_number);	
			if ( ev->device_number == number ){ // it is me, i am already conferencing and administrator GUI removed my device  !
				if (C) {	
					if ( NOT_PARTICIPANT != C->get_state_in_conference(this) ) {
						C->set_state_in_conference(this,NOT_PARTICIPANT);
						// remark: current_conference may be different than ev->conference_number
						vega_event_log(EV_LOG_DEVICE_REMOVED, number, 0, C->number, "%s supprime de la conference %s", name, C->name);
						if ( C->number == current_conference ) device_MAJ_participants_led(C,1);
						set_green_led_possible_conferences();
						//C->remove_from(this);
					}
				}
			}else{ // todo: another device than me has been removed, hide it if it is one of my current conference participants
			}
		}
		break;

	case EVT_ADD_CRM4_DIRECTOR:
		{	// ev->device_number is the number of the newly added device
			if ( ev->device_number == number )
			{	// it is me, i am already conferencing and GUI added my device in another conference !
				vega_conference_t *C = vega_conference_t::by_number(ev->conference_number);
				if ( C ){
					vega_log(INFO,"EVT_ADD_CRM4_DIRECTOR D%d in C%d (active:%d) (currently in C%d) \n",
						ev->device_number, ev->conference_number, C->active, current_conference );

					if ( C->add_in_director(this ) )
					{
						C->participant_director.device = this;//add_in(C,true);
						C->set_state_in_conference(this,PARTICIPANT_ACTIVE);
						if ( C->active ) {
							current_conference = C->number; // c'est notre conference selectionnee par defaut car la premiere	
							vega_log(INFO,"EVT_ADD_CRM4_DIRECTOR D%d in 1st C%d ( auto SELECTED )",number,C->number);
							ChangeState(&CRM4::ConfHearingState);
							start_hearing_in_aslt(C);					
						}
						vega_event_log(EV_LOG_DEVICE_ADDED, number, 0, C->number, "%s ajoute dans la conference %s", name, C->name);
					}else{
						vega_log(INFO,"cannot add_in_director D%d in C%d",number,C->number);
					}

					set_green_led_possible_conferences();
					display_general_call_led( device_doing_group_or_general_call );

					all_devices_display_group_key( device_doing_group_or_general_call );

					device_line_printf(4,"D%d ADDED in C%d........", number,ev->conference_number);
				}
			}
		}
		break;

	case EVT_ADD_CRM4:
		{	// ev->device_number is the number of the newly added device
			if ( ev->device_number == number )
			{	// it is me, i am already conferencing and GUI added my device in another conference !
				vega_conference_t *C = vega_conference_t::by_number(ev->conference_number);
				if ( C ){
					vega_log(INFO,"EVT_ADD_CRM4 D%d in C%d (active:%d) (currently in C%d) \n",
						ev->device_number, ev->conference_number, C->active, current_conference );
					if ( C->add_in(this ) ){
						C->set_state_in_conference(this,PARTICIPANT_ACTIVE);
						if ( C->active ) {
							current_conference = C->number; // c'est notre conference selectionnee par defaut car la premiere	
							vega_log(INFO,"EVT_ADD_CRM4 D%d in 1st C%d ( auto SELECTED )",number,C->number);
							ChangeState(&CRM4::ConfHearingState);
							start_hearing_in_aslt(C);					
						}
						set_green_led_possible_conferences();
						display_general_call_led( device_doing_group_or_general_call );
						all_devices_display_group_key( device_doing_group_or_general_call );

						device_line_printf(4,"D%d AJOUTE A C%d........", number,ev->conference_number);
						//maj des participants car je parle automatiquement dans cette conference SI ACTIVE !!!!
						//device_MAJ(C,1,1,0); // lit conferences and participants led of new conf
						device_MAJ_participants_led(C,0);
						vega_event_log(EV_LOG_DEVICE_ADDED, number, 0, C->number, "%s ajoute a la conference %s", name, C->name);

					}else{
						vega_log(INFO,"cannot add_in D%d in C%d",number,C->number);
					}
				}
			}
		}
		break;

	case EVT_DEVICE_KEY_MICRO_PRESSED:
	  {
		  printf("EVT_DEVICE_KEY_MICRO_PRESSED D%d\n",number);
		  double tt = time_between_button(&_tstart_btn_conf);
		  if ( tt < 1.0 ){vega_log(INFO,"IGNORED MICRO_PRESSED within %f seconds\n",tt);break;}
		  gettimeofday(&_tstart_btn_conf, NULL); // memoriser 
		  
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
			  vega_event_log(EV_LOG_DEVICE_START_SPEAKING, number, 0, number, "%s commence un appel general", name );
			  break;

		  }else{
				device_line_printf(4,"no active conference.........");
		  }
	  }
	  break;

  case EVT_DEVICE_KEY_MICRO_RELEASED:
	   {
			printf("DEVICE_KEY_MICRO_RELEASED D%d\n",number);
			device_close_micro();
	   }
	   break;

	case EVT_DEVICE_KEY_ACTIVATE_CONFERENCE: // je veux activer la premiere conference du systeme
		{
			if ( !activating_conference ){
				int key_button = 1;// pdl 20090916 force bouton 1 !!!! get_key_number_by_action_type(device.crm4.keys_configuration, ACTION_ACTIVATE_CONFERENCE, -1);
				vega_log(INFO,"%d:IdleState:*** EVT_DEVICE_KEY_ACTIVATE_CONFERENCE %d *******\n", number , key_button);

				activating_conference = 1;

				QMap<QString, int>::const_iterator i = m_common_key_number_by_action.find("action_activate_conf");
				if (i != m_common_key_number_by_action.end() ) {
					int key = m_common_key_number_by_action["action_activate_conf"];
					device_set_blink_slow_red_color( key_button);
					device_set_blink_slow_green_color( key_button);   
				}
				else // if (key_button == -1) 
				{
					vega_log(VEGA_LOG_ERROR, "cannot find any key for action ACTION_ACTIVATE_CONFERENCE");
				}


			}else{
				activating_conference = 0;
				vega_log(INFO, "device %d IdleState:*** STOP ACTIVATING CONFERENCES current conf %d", number, current_conference );			  
				int key_button = 1;// get_key_number_by_action_type(device.crm4.keys_configuration, ACTION_ACTIVATE_CONFERENCE, -1);		  
				device_set_no_red_color( key_button); 
				device_set_no_green_color( key_button); 
			}
		}
	  break;

	case EVT_DEVICE_KEY_SELECT_CONFERENCE:
		{
			double tt = time_between_button(&_tstart_btn_conf);
			if ( tt < 1.0 ){vega_log(INFO,"IGNORED EVT_DEVICE_KEY_SELECT_CONFERENCE within %f seconds\n",tt);break;}
			gettimeofday(&_tstart_btn_conf, NULL); // memoriser la date a laquelle on prend en compte l'evt*/
			  
			if (activating_conference)
			{
			  int conf_number = ev->value.select.conference_number; 
			  vega_log(INFO, "D%d EVT_DEVICE_KEY_SELECT_CONFERENCE  C%d", number, conf_number);
			  vega_conference_t *conf = vega_conference_t::by_number(conf_number);
			  if (conf == NULL) {
				vega_log(INFO, "C%d doesn't exists, cannot activate", conf_number);
				break;
			  }
			
			  vega_log(INFO, "C%d exists: %s", conf->number, conf->name);
			  if ( !conf->is_in(this) ) {
				vega_log(INFO, "D%d is not in C%d, cannot activate", number, conf->number);
				break;
			  }// note: on ne peut pas activer une conference dont on ne fait pas partie !!!


			  if ( conf->has_hardware_error() ) { // pdl 20091005
					vega_log(INFO, "C%d HARDWARE ERROR !!!",conf->number);
					device_line_printf(4,"C%d HARDWARE ERROR",conf->number);
					conf->active = 0;
					break;	
			  }

			  vega_log(INFO, "D%d is in C%d (active:%d)", number, conf->number,conf->active);
			  if ( !conf->active ) // est elle deja active ?
			  {
					vega_log(INFO, "Idle CRM4 D%d INITIATOR of C%d", number, conf->number);
					conf->active = 1;
					conf->device_initiator = this;
					current_conference = conf->number; // his first conference is its current
					conf->set_state_in_conference(this,PARTICIPANT_ACTIVE);
					
					//device_display_members_of_conference(conf); // le refleter sur les led des touches
					//device_MAJ(new_conf,1,1,0);

					EVENT ev;
					conf->activate_radio_base(this); // 20090527
					ev.code = EVT_CONFERENCE_START_TONE;; // 
					ev.device_number = number; // l'initiator of this conference
					ev.conference_number = conf->number;
					BroadcastEvent(&ev);
					vega_event_log(EV_LOG_CONFERENCE_ACTIVATED, number, 0, conf->number, "%s initie la conference %s", name, conf->name);
			  }else{
				  vega_log(INFO, "deactivate a conference impossible in idle state, because you are conferencing....");
			  }
			}
		}
		break;


	case EVT_CONFERENCE_START_TONE:
		{
			int NC = ev->conference_number;
			vega_conference_t* C = vega_conference_t::by_number(NC);
			if ( !C->is_in(this) ) { 
				vega_log(ERROR,"D%d EVT_CONFERENCE_START_TONE NOT IN C%d\n", number, NC );
			}else{
				vega_log(INFO,"D%d START_TONE IN C%d initiator D%d\n", number, NC , ev->device_number);
				this->start_hearing_tone(TS_CONFERENCE_TONE);
				EVENT evt;
				evt.code = EVT_CONFERENCE_STOP_TONE;
				evt.device_number = ev->device_number; // transmit the initiator of group call
				evt.conference_number = ev->conference_number;
				DeferEvent(&evt,2);
			}
		}
		break;//EVT_CONFERENCE_STOP_TONE


	case EVT_CONFERENCE_STOP_TONE: 
		{
			int NC = ev->conference_number;
			vega_conference_t* C = vega_conference_t::by_number(NC);
			if ( !C->is_in(this) ){
				vega_log(ERROR,"D%d EVT_CONFERENCE_START_TONE NOT IN C%d\n", number, NC );
			}else{
				vega_log(INFO,"D%d EVT_CONFERENCE_STOP_TONE IN C%d initiator D%d\n", number, NC , ev->device_number);
				stop_hearing_tone();
				{
					EVENT evt;
					evt.code = EVT_CONFERENCE_ACTIVATED; 
					evt.device_number = ev->device_number; // transmit the initiator of conference
					evt.conference_number = ev->conference_number;
					ExecuteStateTransition(&evt);
				}
			}
		}
		break;//EVT_CONFERENCE_STOP_TONE

	case EVT_CALLGENERAL_START_TONE: {}
		break;

	case EVT_CALLGENERAL_STOP_SPEAKING:	{}
		break;

	case EVT_CALLGENERAL_STOP_TONE:
		{
			{
				stop_hearing_tone();

				if ( this->number != device_doing_group_or_general_call ) 
				{ // everybody listen except initiator !
					int ts_value = FIRST_DEVICE_CRM4_INPUT_TS;
					ts_value += device_doing_group_or_general_call -1; 
					// attention : poste aslt 1 correspond a  144  ; poste aslt 7 correspond a  144 + 6
					vega_log(INFO,"D%d EVT_CALLGENERAL_STOP_TONE D%d on TS%d\n", number, device_doing_group_or_general_call, ts_value );
					start_hearing_tone(ts_value);
				}
			}
		}
		break;

	case EVT_CALL_GROUP_START_TONE:
		{
			vega_log(INFO,"------------------ D%d START_TONE IN G%d initiator D%d\n", number, doing_group_call , device_doing_group_or_general_call);
		}
		break;

	case EVT_CALL_GROUP_STOP_TONE: 
		{
			vega_log(INFO,"------------------ D%d STOP_TONE IN group G%d initiator D%d\n", number, doing_group_call, device_doing_group_or_general_call );
			//if ( ( is_device_number_in_group(number,doing_group_call) || device_doing_group_or_general_call==number ) ) 
			{	// le device a l'origine de l'appel de groupe n'est pas forcement dans le groupe, il a juste la touche de groupe
				stop_hearing_tone();
			
				if ( device_doing_group_or_general_call != this->number )
				{ // everybody listen except initiator qui ne s ecoute pas lui meme!
					int ts_value = FIRST_DEVICE_CRM4_INPUT_TS;
					ts_value += device_doing_group_or_general_call -1; 
					// attention : poste aslt 1 correspond a  144  ; poste aslt 7 correspond a  144 + 6
					vega_log(INFO,"D%d HEAR G%d on TS%d\n", number, doing_group_call, ts_value );
					
					start_hearing_tone(ts_value);
				}
			}
		}
		break;

	case EVT_CALL_GROUP_STOP_SPEAKING:
		{
			int NG = ev->group_number;
		}
		break;


	case EVT_CONFERENCE_ACTIVATED: // qqun a activee une conference, SELECTIONNEE par defaut si on en fait partie!!!
		{
			vega_log(INFO,"D%d:*** EVT_CONFERENCE_ACTIVATED C%d by D%d *******\n", number , ev->conference_number, ev->device_number);
			int first_conf_number = ev->conference_number;
			vega_conference_t* C = vega_conference_t::by_number(first_conf_number);

			if ( C ) 
			{
				C->active=1; // pdl 20090911 impossible de refuser une activation de conference, le premier device qui recoit l'event active la conf
				if ( C->is_in(this) )
				{ // if (device_is_in(C, d) == -1) 
					vega_log(VEGA_LOG_ERROR, "Idle: D%d is in C%d", number, C->number);
					current_conference = C->number; // c'est notre conference selectionnee par defaut car la premiere	
					C->set_state_in_conference(this,PARTICIPANT_ACTIVE);
					vega_log(INFO,"EVT_CONFERENCE_ACTIVATED D%d in 1st C%d ( auto SELECTED )",number,C->number);

					device_line_printf(2,"C%d:%s(%d active)              ", current_conference, C->name, nb_active_conf_where_device_is_active_or_dtor_excluded() );	  
					if(C->participant_director.device) device_line_printf(3,"Dtor:%s......",C->participant_director.device->name ) ;

					device_line_printf(4,"C%d ACTIVEE (1ere).......", C->number);
					
					set_green_led_of_conf(C,1);
					set_red_led_of_conf(C,1);
					
					device_MAJ_participants_led(C,UNHIDE);
					
					start_hearing_in_aslt(C);
					ChangeState(&CRM4::ConfHearingState);
				}		
			}
		}
		break;
	}/*switch*/
}
