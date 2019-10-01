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
#include "conf.h"
#include "rude/config.h"

#define CRM4_KEYS_SECTION_NAME			"keys"

QMap<int,int>			CRM4::m_common_touche_select_device;
QMap<int,int>			CRM4::m_common_touche_select_conf;
QMap<int,int>			CRM4::m_common_keymap_number;
QMap<int,QString>		CRM4::m_common_keymap_action;
QMap<QString,int>		CRM4::m_common_key_number_by_action;

void CRM4::init_thread_led() //  static function
{
	pthread_t thread_id;
	int ret = pthread_create(&(thread_id), NULL, CRM4::led_thread, NULL);
	if (ret < 0) {
		vega_log(INFO,"error pthread_create CRM4_led_thread - cause: %s", strerror(errno));
	}
}

void* CRM4::led_thread(void *)
{
	sleep(2);
	while (1){
		foreach(CRM4* d1, qlist_crm4){
			if (d1) d1->update_all_leds(); // pdl: parcours le tableau des etats des leds voir si qq chose a change a cause de l evenement !!!
		}
		sleep(2);
	}
}

void CRM4::update_all_leds() // si un element du tableau a change depuis la derniere fois !
{
	//vega_log(INFO,"D%d:update %d leds",number,CRM4_MAX_KEYS);
	for (int i=0;i<CRM4_MAX_KEYS;i++)
	{
		if ( tab_green_led[i] != old_tab_green_led[i] )//|| INDEFINI==tab_green_led[i]) 
		{
			//vega_log(INFO,"tab_green_led[%d]:%d <= old_tab_green_led[%d]:%d", i, tab_green_led[i] , i, old_tab_green_led[i] );
			// on commence par l'eteindre !!!
			/*device_set_led(i,LED_COLOR_GREEN, LED_MODE_FIX, 0);
			device_set_led(i, LED_COLOR_GREEN, LED_MODE_SLOW_BLINK, 0) ;
			device_set_led(i, LED_COLOR_GREEN, LED_MODE_FAST_BLINK, 0) ;*/

			if ( old_tab_green_led[i]==FIXE ) device_set_led(i,LED_COLOR_GREEN, LED_MODE_FIX, 0);
			if ( old_tab_green_led[i]==CLIGNOTEMENT_RAPIDE ) device_set_led(i,LED_COLOR_GREEN, LED_MODE_FAST_BLINK, 0);
			if ( old_tab_green_led[i]==CLIGNOTEMENT_LENT ) device_set_led(i,LED_COLOR_GREEN, LED_MODE_SLOW_BLINK, 0);


			if ( tab_green_led[i]==FIXE ) device_set_led(i,LED_COLOR_GREEN, LED_MODE_FIX, 1);
			if ( tab_green_led[i]==CLIGNOTEMENT_RAPIDE ) device_set_led(i,LED_COLOR_GREEN, LED_MODE_FAST_BLINK, 1);
			if ( tab_green_led[i]==CLIGNOTEMENT_LENT ) device_set_led(i,LED_COLOR_GREEN, LED_MODE_SLOW_BLINK, 1);
			
			old_tab_green_led[i] = tab_green_led[i] ; // economise une remise a jour si la led ne change pas entre 2 update !!!
		}
		
		if ( tab_red_led[i] != old_tab_red_led[i] )//|| INDEFINI==tab_red_led[i]) 
		{
			//vega_log(INFO,"tab_red_led[%d]:%d <= old_tab_red_led[%d]:%d", i, tab_red_led[i] , i, old_tab_red_led[i] );

			/*device_set_led(i,LED_COLOR_RED, LED_MODE_FIX, 0);
			device_set_led(i, LED_COLOR_RED, LED_MODE_SLOW_BLINK, 0) ;
			device_set_led( i, LED_COLOR_RED, LED_MODE_FAST_BLINK, 0) ;*/
			if ( old_tab_red_led[i]==FIXE ) device_set_led(i,LED_COLOR_RED, LED_MODE_FIX, 0);
			if ( old_tab_red_led[i]==CLIGNOTEMENT_RAPIDE ) device_set_led(i,LED_COLOR_RED, LED_MODE_FAST_BLINK, 0);
			if ( old_tab_red_led[i]==CLIGNOTEMENT_LENT ) device_set_led(i,LED_COLOR_RED, LED_MODE_SLOW_BLINK, 0);
			

			if ( tab_red_led[i]==FIXE ) device_set_led(i,LED_COLOR_RED, LED_MODE_FIX, 1);
			if ( tab_red_led[i]==CLIGNOTEMENT_RAPIDE ) device_set_led(i,LED_COLOR_RED, LED_MODE_FAST_BLINK, 1);
			if ( tab_red_led[i]==CLIGNOTEMENT_LENT ) device_set_led(i,LED_COLOR_RED, LED_MODE_SLOW_BLINK, 1);
			
			old_tab_red_led[i] = tab_red_led[i];// economise une remise a jour si la led ne change pas entre 2 update !!!
		}
	}
}

// RED
// pdl: maintenant ,on met juste le nouvel etat de la led dans un tableau
int  CRM4::device_set_blink_fast_red_color(int button) {
	//device_set_led(button, LED_COLOR_RED, LED_MODE_FAST_BLINK, 1);
	//vega_log(INFO, "device_set_blink_fast_red_color=%d\n",button);
	if ( button< CRM4_MAX_KEYS )   tab_red_led[button]  = CLIGNOTEMENT_RAPIDE;
	return 0;
}
int  CRM4::device_set_blink_slow_red_color(int button) {
	//device_set_led(button, LED_COLOR_RED, LED_MODE_SLOW_BLINK, 1);
	//vega_log(INFO, "device_set_blink_slow_red_color=%d\n",button);
	if ( button< CRM4_MAX_KEYS )   tab_red_led[button]  = CLIGNOTEMENT_LENT;
	return 0;	
}

int  CRM4::device_set_color_red(int button) {
	//device_set_led(button, LED_COLOR_RED, LED_MODE_FIX, 1);
	//vega_log(INFO, "device_set_color_red=%d\n",button);
	if ( button< CRM4_MAX_KEYS )  tab_red_led[button]  = FIXE;
	return 0;
}

int CRM4::device_set_no_red_color(int button)
{
	//vega_log(INFO, "device_set_no_red_color=%d\n",button);
	if ( button< CRM4_MAX_KEYS )    tab_red_led[button]  = ETEINTE;
	// todo : memoriser le dernier mode afin de n'envoyer qu'une seule commande !!!
	//device_set_led(button, LED_COLOR_RED, LED_MODE_SLOW_BLINK, 0) ;
	//device_set_led( button, LED_COLOR_RED, LED_MODE_FAST_BLINK, 0) ;
	//device_set_led(button, LED_COLOR_RED, LED_MODE_FIX, 0);
	return 0;
}

// GREEN
int  CRM4::device_set_blink_fast_green_color(int button) {
	//device_set_led(button, LED_COLOR_GREEN, LED_MODE_FAST_BLINK, 1);
	//vega_log(INFO, "device_set_blink_fast_green_color=%d\n",button);
	if ( button< CRM4_MAX_KEYS )   tab_green_led[button]  = CLIGNOTEMENT_RAPIDE;
	return 0;
}

int  CRM4::device_set_blink_slow_green_color(int button) {
	//device_set_led(button, LED_COLOR_GREEN, LED_MODE_SLOW_BLINK, 1);
	//vega_log(INFO, "device_set_blink_slow_green_color=%d\n",button);
	if ( button< CRM4_MAX_KEYS )   tab_green_led[button]  = CLIGNOTEMENT_LENT;
	return 0;
}

int  CRM4::device_set_color_green(int button) {
	//device_set_led( button, LED_COLOR_GREEN, LED_MODE_FIX, 1);
	//vega_log(INFO, "device_set_color_green=%d\n",button);
	if ( button< CRM4_MAX_KEYS )    tab_green_led[button]  = FIXE;
	return 0;
}

int CRM4::device_set_no_green_color(int button)
{
	//vega_log(INFO, "device_set_no_green_color=%d\n",button);
	if ( button< CRM4_MAX_KEYS )  tab_green_led[button]  = ETEINTE;
	//device_set_led(button, LED_COLOR_GREEN, LED_MODE_SLOW_BLINK, 0);
	//device_set_led( button, LED_COLOR_GREEN, LED_MODE_FAST_BLINK, 0);
	//device_set_led(button, LED_COLOR_GREEN, LED_MODE_FIX, 0);
	return 0;
}

/*[D11]
number = 11
type = crm4
gain = 0
name = D11
[D25]
number = 25
type = radio
full_duplex = 0
contact_sec = 0
name = D25*/
void CRM4::load_crm4_device_section(const char* fname)
{	
	rude::Config config;
	if (config.load(fname) == false) {
		printf("rude cannot load %s file", fname);
		return ;
	}

	int dnumber=0;
	for ( dnumber=1;dnumber<=46;dnumber++)
	{
		QString strdevsection = QString("D%1").arg(dnumber);
		if ( true == config.setSection(qPrintable(strdevsection),false) ) 
		{
			//int key_number = i;char *elt2 = strdup(config.getStringValue("gain")); // ce qu'il y a apres le "="

			int gain = atoi(config.getStringValue("gain"));
			int full_duplex = atoi(config.getStringValue("full_duplex"));
			int audio_detection = atoi(config.getStringValue("contact_sec"));

			if ( strlen( config.getStringValue("name")  ) > 0 ) {
				strdevsection = config.getStringValue("name") ;
			}

			CRM4 *d = NULL;//(CRM4*)device_t::by_number(dnumber);
			Radio *r = NULL;
			if ( dnumber >=1 && dnumber <= 24 ) {
				d = CRM4::create(dnumber,qPrintable(strdevsection),gain);
			}else{
				r = Radio::create(dnumber,qPrintable(strdevsection),gain);
			}

			printf("D%d: config.getStringValue(keys)=%s\n",dnumber, config.getStringValue("keys"));
			if ( d ) {
				if ( strlen( config.getStringValue("keys")  ) > 0 ) 
				{
					char temp[1024];
					strncpy(temp,config.getStringValue("keys"),sizeof(temp));
					d->init_groups_keys( temp );
					// GESTION DES touches de GROUPES propre a chaque CRM4
				}
			}
			switch(dnumber)
			{
				// CRM4 stations					
				case 1:
				if (d) d->init(  1, 0, 0, 20, 0, 0);
				break;
				case 2:
				if (d) d->init(  1, 0, 1, 20, 0, 1);
				break;
				case 3:
				if (d) d->init(  1, 0, 2, 20, 0, 2);
				break;
				case 4:
				if (d) d->init(  1, 0, 3, 20, 0, 3);
				break;
				case 5:
				if (d) d->init(  1, 0, 4, 20, 0, 4);
				break;
				case 6:
				if (d) d->init(  1, 0, 5, 20, 0, 5);
				break;
				case 7:
				if (d) d->init(  2, 0, 0, 20, 0, 6);
				break;
				case 8:
				if (d) d->init(  2, 0, 1, 20, 0, 7);
				break;
				case 9:
				if (d) d->init(  2, 0, 2, 20, 1, 0);
				break;
				case 10:
				if (d) d->init(  2, 0, 3, 20, 1, 1);
				break;
				case 11:
				if (d) d->init(  2, 0, 4, 20, 1, 2);
				break;

				// RADIOS
				case 25:
				if (r) r->init_radio( full_duplex, 22, 0, 0, audio_detection);
				break;

				case 26:
				if (r) r->init_radio( full_duplex, 22, 0, 2,audio_detection);
				break;

				case 27:
				if (r) r->init_radio( full_duplex, 22, 0, 4,audio_detection);
				break;
				case 28:
				if (r) r->init_radio( full_duplex, 22, 0, 6,audio_detection);
				break;

				case 29:
				if (r) r->init_radio( full_duplex, 22, 1, 0,audio_detection);
				break;
				case 30:
				if (r) r->init_radio( full_duplex, 22, 1, 2,audio_detection);
				break;
				case 31:
				if (r) r->init_radio( full_duplex, 22, 1, 4,audio_detection);
				break;
				case 32:
				if (r) r->init_radio( full_duplex, 22, 1, 6,audio_detection);
				break;
				case 33:
				if (r) r->init_radio( full_duplex, 23, 0, 0,audio_detection);
				break;
				case 34:
				if (r) r->init_radio( full_duplex, 23, 0, 2,audio_detection);
				break;
				case 35:
				if (r) r->init_radio( full_duplex, 23, 0, 4,audio_detection);
				break;


				// JUPITERS
				case 36:
				if (r) r->init_jupiter( 23, 0, 6, audio_detection);
				break;

				case 41:
				if (r) r->init_jupiter( 23, 1, 0, audio_detection);
				break;			
				case 42:
				if (r) r->init_jupiter( 23, 1, 2, audio_detection);
				break;			
				case 43:
				if (r) r->init_jupiter( 23, 1, 4, audio_detection);
				break;			
				case 44:
				if (r) r->init_jupiter( 23, 1, 6, audio_detection);
				break;			
				case 45:
				if (r) r->init_jupiter( 24, 0, 0, audio_detection);
				break;	
				case 46:
				if (r) r->init_jupiter( 24, 0, 2, audio_detection);
				break;	
			}
		}
	}
}

void CRM4::check_modif_crm4_device_section(const char* fname)
{
	printf("check_modif_crm4_device_section %s \n",fname);

	rude::Config config;
	if (config.load(fname) == false) {
		printf("rude cannot load %s file", fname);
		return ;
	}

	int dnumber=0;
	for ( dnumber=1;dnumber<=46;dnumber++)
	{
		CRM4 *d = dynamic_cast<CRM4*>(device_t::by_number(dnumber));
		QString strdevsection = QString("D%1").arg(dnumber);
		if ( true == config.setSection(qPrintable(strdevsection),false) ) 
		{
			//int key_number = i;char *elt2 = strdup(config.getStringValue("gain")); // ce qu'il y a apres le "="

			int gain = atoi(config.getStringValue("gain"));
			int full_duplex = atoi(config.getStringValue("full_duplex"));
			int audio_detection = atoi(config.getStringValue("contact_sec"));

			if ( strlen( config.getStringValue("name")  ) > 0 ) {
				printf("device changed name to %s\n",config.getStringValue("name"));
				//vega_log(INFO, "device changed name %s -> %s\n",d->name, config.getStringValue("name"));
				if ( dnumber >=1 && dnumber <= 24 ) 
				{
					if ( d ) {
						strncpy(d->name, config.getStringValue("name"),sizeof(d->name) );
						//d->device_line_printf(1,"D%d:%s********",d->number,d->name);
						EVENT evt;
						evt.device_number = d->number;
						evt.code = EVT_DEVICE_UPDATE_DISPLAY; 
						BroadcastEvent(&evt);
					}
				}
			}

			if ( dnumber >=1 && dnumber <= 24 && gain >=-12 && gain <=14) {
				if ( d ) {
					printf("device changed gain %d -> %d\n",d->gain, gain);
					vega_log(INFO, "D%d changed gain %d -> %d\n",dnumber, d->gain, gain);
					d->gain = gain; // todo : check gain entre -12 et 14
					d->device_change_gain(d->gain);
					printf("D%d: config.getStringValue(gain)=%s\n",dnumber, config.getStringValue("gain"));
				}
			}
		}
	}
}

/*[keys]
1=action_activate_conf
2=action_on_off_conf
4=action_exclude_include_conf
5=action_select_conf,1
6=action_select_conf,2
7=action_select_conf,3
8=action_select_conf,4
9=action_select_conf,5
10=action_select_conf,6
11=action_select_conf,7
12=action_select_conf,8
13=action_select_conf,9
14=action_select_conf,10
15=action_select_device,25
16=action_select_device,26
17=action_select_device,27
18=action_select_device,28
19=action_select_device,29
20=action_select_device,1
21=action_select_device,2
22=action_select_device,3
23=action_select_device,4
24=action_select_device,5
25=action_select_device,6
26=action_select_device,7
27=action_select_device,8
28=action_select_device,9
29=action_select_device,10
30=action_select_device,11
32=action_select_device,41
33=action_select_device,42
34=action_select_device,43
35=action_select_device,44
36=action_select_device,45
38=action_general_call
*/	
void CRM4::load_crm4_keys_section(const char* fname)
{	
	rude::Config config;
	if (config.load(fname) == false) {
		printf("rude cannot load %s file", fname);
		return ;
	}

	int nb_keys=0;
	for (int i = 1; i <= 38; i++)
	{
		//config.setSection(qPrintable(config.getSectionNameAt(i));
		if ( true == config.setSection(CRM4_KEYS_SECTION_NAME,false) ) {

			QString str = QString("%1").arg(i);

			int key_number = i;
			char *elt2 = strdup(config.getStringValue(qPrintable(str))); // ce qu'il y a apres le "="

			nb_keys++;
			vega_log(INFO, "nb_keys:%d key_number=%d elt2=%s\n",nb_keys, key_number, elt2);
			printf("nb_keys:%d key_number=%d elt2=%s\n",nb_keys, key_number, elt2);
			
			m_common_key_number_by_action[elt2] = key_number; // m_common_key_number_by_action["action_activate_conf"]=1

			if (strstr(elt2, "action_activate_conf")) 
			{
				m_common_keymap_action[key_number]=elt2;
				m_common_keymap_number[key_number]=key_number;

			  //device_keys_configuration->device_key_configuration[nb_keys].type = ACTION_ACTIVATE_CONFERENCE ;
			  //device_keys_configuration->device_key_configuration[nb_keys].key_number = key_number;
      
			  
			  nb_keys ++;
			}
			else if (strstr(elt2, "action_on_off_conf")) {

				m_common_keymap_action[key_number]=elt2;
				m_common_keymap_number[key_number]=key_number;

			  //device_keys_configuration->device_key_configuration[nb_keys].type = ACTION_ON_OFF_CONF;
			  //device_keys_configuration->device_key_configuration[nb_keys].key_number = key_number;


			  nb_keys ++;
			}
			else if (strstr(elt2, "action_exclude_include_conf")) 
			{
				m_common_keymap_action[key_number]=elt2;
				m_common_keymap_number[key_number]=key_number;
				//device_keys_configuration->device_key_configuration[nb_keys].type = ACTION_INCLUDE_EXCLUDE;
				//device_keys_configuration->device_key_configuration[nb_keys].key_number = key_number;
				nb_keys ++;
			}
			else if (strstr(elt2, "action_select_conf")) 
			{/*
				5=action_select_conf,1
				6=action_select_conf,2
				7=action_select_conf,3
				8=action_select_conf,4
				9=action_select_conf,5
				10=action_select_conf,6
				11=action_select_conf,7
				12=action_select_conf,8
				13=action_select_conf,9
				14=action_select_conf,10*/

				vega_log(INFO, "action_select_conf %d %d", nb_keys, key_number );
				//device_keys_configuration->device_key_configuration[nb_keys].type = ACTION_SELECT_CONF;
				//device_keys_configuration->device_key_configuration[nb_keys].key_number = key_number;
				char *e1;
				char *e2;
				e1 = strtok(elt2, ",");
				e2 = strtok(NULL, ",");
				//device_keys_configuration->device_key_configuration[nb_keys].action.select_conf.conference_number = atoi(e2);

				m_common_keymap_action[key_number]=elt2;
				m_common_keymap_number[key_number]=atoi(e2);;

				m_common_touche_select_conf[atoi(e2)] = key_number; 

				nb_keys ++;
			}
			else if(strstr(elt2, "action_select_device")) 
			{
				/*15=action_select_device,25
					16=action_select_device,26
					17=action_select_device,27
					18=action_select_device,28
					19=action_select_device,29
					20=action_select_device,1
					21=action_select_device,2
					22=action_select_device,3
					23=action_select_device,4
					24=action_select_device,5
					25=action_select_device,6
					26=action_select_device,7
					27=action_select_device,8
					28=action_select_device,9
					29=action_select_device,10
					30=action_select_device,11
					32=action_select_device,41
					33=action_select_device,42
					34=action_select_device,43
					35=action_select_device,44
					36=action_select_device,45			*/
      
				//device_keys_configuration->device_key_configuration[nb_keys].type = ACTION_SELECT_DEVICE;
				//device_keys_configuration->device_key_configuration[nb_keys].key_number = key_number;
				char *e1;
				char *e2;
				e1 = strtok(elt2, ",");
				e2 = strtok(NULL, ",");
				//device_keys_configuration->device_key_configuration[nb_keys].action.select_device.device_number = atoi(e2);


				m_common_keymap_action[key_number]=elt2; // m_common_keymap_action[20]="action_select_device"
				m_common_keymap_number[key_number]=atoi(e2); // m_common_keymap_number[20]=1

				m_common_touche_select_device[atoi(e2)] = key_number;

				nb_keys ++;
			}
			// pdl 20090927 les appels de groupe sont dynamiques et charges depuis le fichier devices.conf
			/*else if(strstr(elt2, "action_group_call")) { 
			  device_keys_configuration->device_key_configuration[nb_keys].type = ACTION_GROUP_CALL;
			  device_keys_configuration->device_key_configuration[nb_keys].key_number = key_number;
			  char *e1;
			  char *e2;
			  e1 = strtok(elt2, ",");
			  e2 = strtok(NULL, ",");
			  device_keys_configuration->device_key_configuration[nb_keys].action.group_call.group_number = atoi(e2);
			  nb_keys ++;
			}*/
			else if(strstr(elt2, "action_general_call")) 
			{
				//device_keys_configuration->device_key_configuration[nb_keys].type = ACTION_GENERAL_CALL;
				//device_keys_configuration->device_key_configuration[nb_keys].key_number = key_number;
				m_common_keymap_action[key_number]=elt2;
				m_common_keymap_number[key_number]=key_number;


				nb_keys ++;
			}    

		}
	}
	//exit(-1);
}

void CRM4::load_crm4_keys_configuration(const char* fname)
{	
	//device_keys_configuration_t* device_keys_configuration = &the_unic_static_device_keys_configuration;
	char line[128]={0};
	int i;
	int nb_keys = 0;


	FILE *fp = fopen(fname, "r");
	if (fp == NULL) {
		vega_log(ERROR, "cannot open file %s - cause : %s", fname, strerror(errno));
		return;
	}

	/* read a line */
	int f = 0;
	while (1) 
	{

		bzero(line, sizeof(line));
		for (i = 0; ; i++) {
		if (fread(line + i, 1, 1, fp) != 1) 
		{
			f = 1;
			break;
		}
		if (line[i] == '\n')
			break;
		}

		if (line[i] == '\n')
			line[i] = '\0';

		if (!strcmp(line, ""))
			break;

		if (line[0] == ';')
			continue;

		char *elt1;
		char *elt2;

		if ((elt1 = strtok(line, "=")) == NULL) {
		  vega_log(ERROR, "cannot process line %s", line);
		}

		//vega_log(ERROR, "processing line %s", line);
		int key_number = atoi(elt1);

		if ((elt2 = strtok(NULL, "=")) == NULL) {
		  vega_log(ERROR, "cannont process line %s", line);
		  continue;
		}

		vega_log(INFO, "nb_keys:%d key_number=%d elt2=%s\n",nb_keys, key_number, elt2);
		m_common_key_number_by_action[elt2] = key_number; // m_common_key_number_by_action["action_activate_conf"]=1

		//38=action_general_call       m_common_key_number_by_action["action_general_call"]=38

	

		if (strstr(elt2, "action_activate_conf")) 
		{
			m_common_keymap_action[key_number]=elt2;
			m_common_keymap_number[key_number]=key_number;

		  //device_keys_configuration->device_key_configuration[nb_keys].type = ACTION_ACTIVATE_CONFERENCE ;
		  //device_keys_configuration->device_key_configuration[nb_keys].key_number = key_number;
      
		  
		  nb_keys ++;
		}
		else if (strstr(elt2, "action_on_off_conf")) {

			m_common_keymap_action[key_number]=elt2;
			m_common_keymap_number[key_number]=key_number;

		  //device_keys_configuration->device_key_configuration[nb_keys].type = ACTION_ON_OFF_CONF;
		  //device_keys_configuration->device_key_configuration[nb_keys].key_number = key_number;


		  nb_keys ++;
		}
		else if (strstr(elt2, "action_exclude_include_conf")) 
		{
			m_common_keymap_action[key_number]=elt2;
			m_common_keymap_number[key_number]=key_number;
			//device_keys_configuration->device_key_configuration[nb_keys].type = ACTION_INCLUDE_EXCLUDE;
			//device_keys_configuration->device_key_configuration[nb_keys].key_number = key_number;
			nb_keys ++;
		}
		else if (strstr(elt2, "action_select_conf")) 
		{/*
			5=action_select_conf,1
			6=action_select_conf,2
			7=action_select_conf,3
			8=action_select_conf,4
			9=action_select_conf,5
			10=action_select_conf,6
			11=action_select_conf,7
			12=action_select_conf,8
			13=action_select_conf,9
			14=action_select_conf,10*/

			vega_log(INFO, "action_select_conf %d %d", nb_keys, key_number );
			//device_keys_configuration->device_key_configuration[nb_keys].type = ACTION_SELECT_CONF;
			//device_keys_configuration->device_key_configuration[nb_keys].key_number = key_number;
			char *e1;
			char *e2;
			e1 = strtok(elt2, ",");
			e2 = strtok(NULL, ",");
			//device_keys_configuration->device_key_configuration[nb_keys].action.select_conf.conference_number = atoi(e2);

			m_common_keymap_action[key_number]=elt2;
			m_common_keymap_number[key_number]=atoi(e2);;

			m_common_touche_select_conf[atoi(e2)] = key_number; 

			nb_keys ++;
		}
		else if(strstr(elt2, "action_select_device")) 
		{
			/*15=action_select_device,25
				16=action_select_device,26
				17=action_select_device,27
				18=action_select_device,28
				19=action_select_device,29
				20=action_select_device,1
				21=action_select_device,2
				22=action_select_device,3
				23=action_select_device,4
				24=action_select_device,5
				25=action_select_device,6
				26=action_select_device,7
				27=action_select_device,8
				28=action_select_device,9
				29=action_select_device,10
				30=action_select_device,11
				32=action_select_device,41
				33=action_select_device,42
				34=action_select_device,43
				35=action_select_device,44
				36=action_select_device,45			*/
      
			//device_keys_configuration->device_key_configuration[nb_keys].type = ACTION_SELECT_DEVICE;
			//device_keys_configuration->device_key_configuration[nb_keys].key_number = key_number;
			char *e1;
			char *e2;
			e1 = strtok(elt2, ",");
			e2 = strtok(NULL, ",");
			//device_keys_configuration->device_key_configuration[nb_keys].action.select_device.device_number = atoi(e2);


			m_common_keymap_action[key_number]=elt2; // m_common_keymap_action[20]="action_select_device"
			m_common_keymap_number[key_number]=atoi(e2); // m_common_keymap_number[20]=1

			m_common_touche_select_device[atoi(e2)] = key_number;

			nb_keys ++;
		}
		// pdl 20090927 les appels de groupe sont dynamiques et charges depuis le fichier devices.conf
		/*else if(strstr(elt2, "action_group_call")) { 
		  device_keys_configuration->device_key_configuration[nb_keys].type = ACTION_GROUP_CALL;
		  device_keys_configuration->device_key_configuration[nb_keys].key_number = key_number;
		  char *e1;
		  char *e2;
		  e1 = strtok(elt2, ",");
		  e2 = strtok(NULL, ",");
		  device_keys_configuration->device_key_configuration[nb_keys].action.group_call.group_number = atoi(e2);
		  nb_keys ++;
		}*/
		else if(strstr(elt2, "action_general_call")) 
		{
			//device_keys_configuration->device_key_configuration[nb_keys].type = ACTION_GENERAL_CALL;
			//device_keys_configuration->device_key_configuration[nb_keys].key_number = key_number;
			m_common_keymap_action[key_number]=elt2;
			m_common_keymap_number[key_number]=key_number;


			nb_keys ++;
		}    
		if (f)
		  break;

	}/*while*/
	vega_log(INFO,"load_crm4_keys_configuration: nb_keys=%d\n",nb_keys);
}

int CRM4::all_devices_display_general_call_led(int busy /* if a director is doing a general call*/)
{
	foreach(CRM4* d,qlist_crm4)	{
		if ( d) d->display_general_call_led(busy);  
	}
	return 0;
} 

int CRM4::display_general_call_led(int busy /* if a director is doing a general call*/)
{
	//int key = 0;//get_key_number_by_action_type(device.crm4.keys_configuration, ACTION_GENERAL_CALL, 0 );
	if ( m_common_key_number_by_action.contains( "action_general_call" ) )
	{
		int key=m_common_key_number_by_action["action_general_call"];
		device_set_color_green( key);		
		if ( busy )
			device_set_color_red( key);
		else
			device_set_no_red_color( key);
	}
	/*
			QMap<int,QString>::const_iterator i = CRM4::m_common_keymap_action.constBegin();
			while (  i !=   CRM4::m_common_keymap_action.constEnd()    ) {
				if ( i.value() == "action_general_call" ) {
					int no_touche = i.key();

					device_set_color_green( key);		
					if ( busy )
						device_set_color_red( key);
					else
						device_set_no_red_color( key);

				}
				i++;
			}*/
	//vega_log(INFO,"set led ACTION_GENERAL_CALL key %d on D%d...\n", key , d->number);
	return 0;
} 

int CRM4::all_devices_display_group_key(int allume) // static 
{	// STATIC 
	vega_log(INFO,"all_devices_display_group_key G%d <= %d..\n", doing_group_call);
	//unsigned int i=0;for (i = 0; i < g_list_length(devices) ; i++) 
	foreach(CRM4* d,qlist_crm4)
	{
		 QMap<int, int>::const_iterator i = d->keymap_groups.constBegin();
		 while (i != d->keymap_groups.constEnd()) 
		 {
			 //printf("D%d has_group_key G%d in KEY %d ( doing_group_call=G%d )\n", d->number, i.value() , i.key(),doing_group_call);
			 d->device_set_color_green( i.key());	
			 if ( doing_group_call == i.value() ) 
			 {
				if (allume)
					d->device_set_blink_fast_red_color(i.key());
				else
					d->device_set_no_red_color( i.key());
			 }
			 ++i;
		 }
	}
	return 0;			
}

int CRM4::device_MAJ_groups_led(int doing_group_cal)
{
	CRM4* d = this;
	vega_log(INFO, "----------------------------------->DISPLAY G%d LED on D%d",doing_group_cal, number);

	QMap<int, int>::const_iterator i = keymap_groups.constBegin();
	 while (i != keymap_groups.constEnd()) 
	 {
		 //cout << i.key() << ": " << i.value() << endl;
		 {
			 printf("D%d has_group_key G%d in KEY %d\n", d->number, i.value() , i.key());
			 d->device_set_color_green( i.key());	

			 if ( i.value() == doing_group_cal ) 
			//if ( working ) 
				//d->device_set_color_red( key);
				d->device_set_blink_fast_red_color(i.key());
			else
				d->device_set_no_red_color( i.key());
		 }
		 ++i;
	 }
	
	//QMap<int /*crm4key*/,int /*numero_group*/> keymap_groups; // dynamic keys different for each CRM4
	/*foreach( int key, this->keymap_groups ) {
			//int key = get_key_number_by_action_type(d->device.crm4.keys_configuration, ACTION_GROUP_CALL, NG );			
			//vega_log(INFO,"set led ACTION_GROUP_CALL keys_group[%d]=%d on D%d...\n", NG, key , number);
			if ( key > 0 ) {
				device_set_color_green( key);		
				if ( grpnum == grpnum ) 
					device_set_color_red( key);
				else
					device_set_no_red_color( key);
			}

	}*/
	/*
	
	unsigned int NG;
	for ( NG=0; NG< MAX_GROUPS; NG++)
	{
		//int key = device.crm4.keys_group[NG];

		if ( keymap_groups.contains(NG) )
		{ // Returns true if the map contains an item with key key; otherwise returns false.

			int key = keymap_groups[NG] ;//= num_touche;

			//int key = get_key_number_by_action_type(d->device.crm4.keys_configuration, ACTION_GROUP_CALL, NG );			
			//vega_log(INFO,"set led ACTION_GROUP_CALL keys_group[%d]=%d on D%d...\n", NG, key , number);
			if ( key > 0 ) {
				device_set_color_green( key);		
				if ( grpnum == NG ) 
					device_set_color_red( key);
				else
					device_set_no_red_color( key);
			}
		}
	}*/
	return 0;
} 

void CRM4::all_devices_set_green_led_possible_conferences()
{
	foreach(CRM4* d,qlist_crm4)
	{
		if ( d ) d->set_green_led_possible_conferences();
	}

}

void CRM4::set_green_led_possible_conferences()
{
	//vega_log(INFO, "UPDATE CONFERENCE LEDS in D%d (active in %d conferences)", number, nb_active_conf_where_device_is_active()  );
	//printf("UPDATE CONFERENCE LEDS in D%d (active in %d conferences)", number, nb_active_conf_where_device_is_active()  );
	unsigned int k;
	foreach(vega_conference_t *C,vega_conference_t::qlist){
		if ( C ) set_led_conference(C); // pdl 20090911
	}
}

int CRM4::device_MAJ_particpant_key(device_t* d1, int hide_participant, participant_st etat)
{
	if ( d1 ){
		int keyD = 0;//get_key_number_by_action_type(device.crm4.keys_configuration, ACTION_SELECT_DEVICE, d1->number);
		/*QMap<int,QString>::const_iterator i = CRM4::m_common_keymap_action.constBegin();
		while (  i !=   CRM4::m_common_keymap_action.constEnd()    ) {
			if ( i.value() == "action_select_device" ) {
				keyD = i.key();
			}
			i++;
		}*/
		//m_common_touche_select_conf[NoDevice] = touche
		if ( m_common_touche_select_device.contains(d1->number ) )
		{
			keyD = m_common_touche_select_device[d1->number] ;
			//vega_log(INFO, "D%d: particpant_key of D%d (key %d)", number, d1->number, keyD);
		}else{
			//vega_log(INFO, "D%d: NO particpant_key for D%d (key %d)", number, d1->number, keyD);
		}

		if (keyD > 0)
		{
			// STOP blinking red led even if not speaking in conf
			//device_set_no_red_color(keyD);

			if ( m_is_plugged ) 
			{
				if ( hide_participant ){ // hide particpants of this conference ( used when swicthing from on conf to another )
					vega_log(INFO, "HIDE participants D%d in D%d",d1->number, number );
					device_set_no_red_color( keyD);
					device_set_no_green_color( keyD);
				}
				else
				{
				  if ( NOT_PARTICIPANT==etat)//C->get_state_in_conference(d1) )
				  {
					device_set_no_green_color( keyD);
					device_set_no_red_color( keyD);
				  }
				  else  if ( PARTICIPANT_ACTIVE==etat)//C->get_state_in_conference(d1) )
				  {
					  if ( d1->b_speaking ) {
						device_set_color_green( keyD);
						//device_set_( keyD);
						device_set_blink_fast_red_color(keyD);
					  }else{
						device_set_color_green( keyD);
						  device_set_no_red_color( keyD);
					  }
				  }else{ // EXCLUDED participant
					  device_set_no_green_color( keyD);
					  device_set_color_red(keyD);
				  }
				}
			}else{
				device_set_no_red_color(keyD);
				device_set_no_green_color(keyD);
			}
		}
	}
}

/* update conferences led ( and show/hide participants led ) on mydevice CRM4 */
int CRM4::device_MAJ(vega_conference_t *C, int update_conferences_led,int update_participants_led, int hide_participants)
{
	if ( update_participants_led && C ){
		device_MAJ_participants_led(C,hide_participants);
	}		
	if ( update_conferences_led ) {
		set_green_led_possible_conferences();
	}
}

int CRM4::device_MAJ_participants_led( vega_conference_t *C, int hide_participant)
{
	unsigned int j=0;
	if (!C) return -1;
	/*unsigned int devnum =0;
	for ( devnum=15; devnum<=37; devnum++)
	{
		int keyD = get_key_number_by_action_type(device.crm4.keys_configuration, ACTION_SELECT_DEVICE, devnum);
		//vega_log(INFO, "participants[%d] is device %d (key %d)", j, d1->number, keyD);
		if (keyD > 0){
			{ // hide particpants of this conference ( used when swicthing from on conf to another )
				device_set_no_red_color( keyD);
				device_set_no_green_color( keyD);
			}
		}
	}*/	
	if ( C->active || hide_participant)
	{ // mise a jour des participants : soit conference active, soit effacement complet !
		if ( C->participant_director.device ) 
			device_MAJ_particpant_key(C->participant_director.device,hide_participant,C->get_state_in_conference(C->participant_director.device));

		if ( C->participant_radio1.device ) 
			device_MAJ_particpant_key(C->participant_radio1.device,hide_participant,C->get_state_in_conference(C->participant_radio1.device));

		if ( C->participant_radio2.device ) 
			device_MAJ_particpant_key(C->participant_radio2.device,hide_participant,C->get_state_in_conference(C->participant_radio2.device));
		
		if ( C->participant_jupiter.device ) 
			device_MAJ_particpant_key(C->participant_jupiter.device,hide_participant,C->get_state_in_conference(C->participant_jupiter.device));
		
		for (j = 0; j < C->nb_particpants; j++) {
			if ( C->participants[j].device ) 
				this->device_MAJ_particpant_key(C->participants[j].device,hide_participant,C->get_state_in_conference( C->participants[j].device ));
		}
	}else{
		vega_log(INFO, "D%d: no need to device_MAJ_participants_led when C%d not active!!!",number,C->number);
	}

	return 0;
}

#define MAX_LEN_CRM4_LINE 18
int CRM4::device_line_printf(int line, const char* fmt, ...)
{
	//if ( line == 1 ) return 0;
	//if ( line == 4 ) return 0;
	if ( line >= 5 ) return 0;
	if ( line <= 0 ) return 0;

	if (  fmt==NULL || line <= 0 || line > 4 ) return -1;
	char temp[1024]={0};
	//char str[128]={0};

	va_list args;
	va_start(args, fmt);
	int nb = vsnprintf(temp, sizeof(temp) , fmt, args);
	//int nb = vsprintf(temp, fmt, args);
	va_end (args);
		
	temp[MAX_LEN_CRM4_LINE]=0;	  // force end of line
	device_display_msg( line, temp);

	return 0;
}

int CRM4::device_line_print_time_now(int line)
{
	struct timeval tv;
	  struct tm* ptm;
	  char str[32];
	  gettimeofday (&tv, NULL);
	  ptm = localtime (&tv.tv_sec);
	  strftime (str, sizeof(str)-1, "%d/%m %H:%M:%S ......", ptm);
	  device_line_printf(line,str);
}

int CRM4::device_display_conferences_state()
{
	CRM4* d = this;
	d->device_MAJ(NULL,1,0,0);
	return 0;
}

int CRM4::set_green_led_of_conf(vega_conference_t *conf,int on)
{
	//if ( conf->is_in(this) ) 
	if ( m_common_touche_select_conf.contains( conf->number ) )
	{
		vega_log(INFO, "set in D%d the GREEN led of C%d: %d", number, conf->number,on);

		int keyC = m_common_touche_select_conf[conf->number];//get_key_number_by_action_type(this->device.crm4.keys_configuration, ACTION_SELECT_CONF, conf->number);

		if ( on ) this->device_set_color_green(keyC); else device_set_no_green_color(keyC);

		/*QMap<int,QString>::const_iterator i = CRM4::m_common_keymap_action.constBegin();
		while (  i !=   CRM4::m_common_keymap_action.constEnd()    ) {
			if ( i.value() == "action_select_conf" ) {
				int no_touche = i.key();
				if ( m_common_keymap_number[no_touche] == conf->number ) {
					if ( on ) this->device_set_color_green(keyC);
					else device_set_no_green_color(keyC);
				}
			}
			i++;
		}*/
	}
	return 0;
}

int CRM4::set_red_led_of_conf(vega_conference_t *conf,int on)
{
	if ( m_common_touche_select_conf.contains( conf->number ) )
	{
		vega_log(INFO, "set in D%d the RED led of C%d: %d", number, conf->number,on);

		int keyC = m_common_touche_select_conf[conf->number];//get_key_number_by_action_type(this->device.crm4.keys_configuration, ACTION_SELECT_CONF, conf->number);

		if ( on ) this->device_set_color_red(keyC); else device_set_no_red_color(keyC);


		/*QMap<int,QString>::const_iterator i = CRM4::m_common_keymap_action.constBegin();
		while (  i !=   CRM4::m_common_keymap_action.constEnd()    ) {
			if ( i.value() == "action_select_conf" ) {
				int no_touche = i.key();
				if ( m_common_keymap_number[no_touche] == conf->number ) {
					if ( on ) this->device_set_color_red(keyC);
					else device_set_no_red_color(keyC);
				}
			}
			i++;
		}*/
	}
	return 0;
}

int CRM4::all_devices_display_led_of_device_speaking_in_conf(vega_conference_t *conf, device_t * d) // STATIC
{
	if ( NULL==d || NULL==conf) return -1;
	vega_log(INFO, "display EVERYWHERE D%d speaking in C%d(current==%d)", d->number, conf->number, d->current_conference);
	/* diffuse ,pour chaque device dans la conference qui est en train de parler, l'etat de tous les postes crm4 de la conference qui sont dedans */
	foreach(CRM4* d1,qlist_crm4)
	{    
		d1->set_led_conference(conf);
	    if ( d1 && d1->current_conference==conf->number ) {//d1->set_led_conference(conf); // pdl20090624
			d1->device_MAJ_particpant_key(d,0,conf->get_state_in_conference(d)); // probleme when MICRO released just before being excluded by director!
		}		
	}
	return 0;
}

int CRM4::all_devices_display_led_of_device_NOT_speaking_in_conf(vega_conference_t *conf, device_t * d)
{
	if ( NULL==d || NULL==conf) return -1;
	vega_log(INFO, "display EVERYWHERE D%d NOT speaking in C%d", d->number, conf->number);
	foreach(CRM4* d1,qlist_crm4)
	{	
		d1->set_led_conference(conf);
		if (d1->current_conference==conf->number ) 
		{
			vega_log(INFO, "display on D%d led of D%d NOT speaking in C%d", d1->number, d->number, conf->number);
			d1->device_MAJ_particpant_key(d,0,conf->get_state_in_conference(d)); // probleme when MICRO released just before being excluded by director!
		}
	}
	return 0;
}

void CRM4::set_led_conference(vega_conference_t* C)
{
	unsigned int k;
	if(C)
	{
		if ( m_common_touche_select_conf.contains(C->number) ) 
		{
			int key = m_common_touche_select_conf[C->number];//0;//get_key_number_by_action_type( device.crm4.keys_configuration, ACTION_SELECT_CONF, C->number);

			vega_log(INFO, "OK:UPDATE key%d C%d LED in D%d (%d speaking)", key, C->number, number,  C->nb_people_speaking() );

			// RAZ in case of slow blink for director reinclusion !
			//device_set_no_green_color( key);
			//device_set_no_red_color( key);

			switch ( C->get_state_in_conference(this) )
			{

			case NOT_PARTICIPANT:
				//vega_log(INFO, "UPDATE CONFERENCE LEDS D%d NOT_PARTICIPANT in C%d",number,C->number);
 				//device_set_no_green_color( key);
				//device_set_no_red_color( key);
				device_set_no_green_color( key);
				device_set_no_red_color( key);
				break;

			case PARTICIPANT_ACTIVE:
				vega_log(INFO, "UPDATE CONFERENCE LEDS D%d PARTICIPANT_ACTIVE in C%d",number,C->number);
				if ( C->active ) {
					if ( C->nb_people_speaking() > 0 ) {
						//device_set_no_red_color(key);
						device_set_blink_fast_red_color( key); 
					}else {
						//device_set_no_red_color(key); // trick to stop the blink!!!
						device_set_color_red( key); 
					}
				}else{ 
					device_set_no_red_color( key);
				}
				//device_set_no_green_color( key);
				device_set_color_green( key);
				break;

			case PARTICIPANT_SELF_EXCLUDED:
				vega_log(INFO, "UPDATE CONFERENCE LEDS D%d PARTICIPANT_SELF_EXCLUDED in C%d",number,C->number);
				if ( C->active ) {
					if ( C->nb_people_speaking() > 0 ){ 
						//device_set_no_red_color(key);
						device_set_blink_fast_red_color( key); 
					}else {
						//device_set_no_red_color(key);// trick to stop the blink!!!
						device_set_color_red( key);
					}
					device_set_color_green( key);
				}else {
					device_set_no_red_color( key);
				}
				device_set_no_green_color( key);
				break;
			

			case PARTICIPANT_DTOR_EXCLUDED:
				vega_log(INFO, "UPDATE CONFERENCE LEDS D%d PARTICIPANT_DTOR_EXCLUDED in C%d",number,C->number);
				if ( C->active ) {
					if ( C->nb_people_speaking() > 0 ){ 
						//device_set_no_red_color(key);
						device_set_blink_fast_red_color( key); 
					}else {
						//device_set_no_red_color(key);// trick to stop the blink!!!
						device_set_color_red( key);
					}
					device_set_color_green( key);
				}else {
					device_set_no_red_color( key);
				}
				device_set_no_green_color( key);
				break;
			}
		}else{
			vega_log(INFO, "NOK:UPDATE C%d LED in D%d (%d speaking)", C->number, number,  C->nb_people_speaking() );
		}
	}
}
