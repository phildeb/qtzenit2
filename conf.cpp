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
#include "misc.h"
#include "alarmes.h"
#include "rude/config.h"

conference_configuration_t vega_conference_t::configuration_conferences[10/*MAX_CONFERENCES*/] = {
  {1, 19, 0, 31,  24, 0, 6}, 			/* premiere conference sur la carte 19 , SBI 0, commence a� ts 31 */
  {2, 19, 1, 40,  24, 0, 7},			/* 2ieme conference sur la carte 19 , SBI 1 */
  {3, 18, 0, 49,  24, 1, 0},
  {4, 18, 1, 58,  24, 1, 1}, 
  {5, 17, 0, 67,  24, 1, 2},
  {6, 17, 1, 76,  24, 1, 3},
  {7, 16, 0, 85,  24, 1, 4},
  {8, 16, 1, 94,  24, 1, 5}, 
  {9, 15, 0, 103, 24, 1, 6},
  {10,15, 1, 112, 24, 1, 7}
};

bool vega_conference_t::has_hardware_error()
{
	if ( false == map_boards[matrix_configuration->board_number] ) { // pdl 20091005
		vega_log(INFO, "C%d carte %d absente ou en defaut !!!",number, matrix_configuration->board_number);
		return true;
	}
	if ( false == map_boards[matrix_configuration->conference_mixer_board_number] ) {
		vega_log(INFO, "C%d MIXER CONF BOARD %d absente ou en defaut !!!",number, matrix_configuration->conference_mixer_board_number);
		return true;
	}
	return false;
}

/* TODO:  faire la gestion de la presence des radio ou jupiter */
vega_conference_t *vega_conference_t::create(int number, const char* name)
{
	/* recupere le numro de carte + numéro de port à placer en dure */
	if (number < 1 || number > MAX_CONFERENCES) {
		vega_log(CRITICAL, "invalid conference number %d ( 1 -> %d )",MAX_CONFERENCES);
		return NULL;
	}

	vega_conference_t* c = new vega_conference_t;//&tab_confs[number];
	
	memset(c,0,sizeof(vega_conference_t));
	c->number = number;
	if ( name && name[0] ) strncpy(c->name, name, sizeof(c->name) ) ;
	else snprintf(c->name, sizeof(c->name) -1, "CONF%02d", c->number);
	c->nb_particpants = 0;
	c->device_initiator=NULL;
	c->active=0;

	c->matrix_configuration = &vega_conference_t::configuration_conferences[number-1];//config;

	vega_log(INFO, "     create CONF C%d @ B%d - SBI%d:%p      ",  number, 
		c->matrix_configuration->board_number, 
		c->matrix_configuration->sbi_number, 
		c->matrix_configuration);

	c->participant_radio1.device = NULL;
	c->participant_radio2.device = NULL;
	c->participant_jupiter.device = NULL;
	c->participant_radio1.device = NULL;

	c->participant_radio2.state = NOT_PARTICIPANT;
	c->participant_jupiter.state = NOT_PARTICIPANT;
	c->participant_director.state = NOT_PARTICIPANT;
	c->participant_director.state = NOT_PARTICIPANT;

	return c;
}

void vega_conference_t::activate_radio_base(device_t* d /* device that is pressing M to talk */)
{	// la base radio passe en emission vers le terminal radio des qu'un intervenant (autre que lui) parle
	vega_log(INFO, "activate_radio_base (D%d speaking in C%d R%d R%d)", d->number, this->number, this->radio1_device(),this->radio2_device());
	if (d->number == this->radio1_device()) 
	{
		device_t *D2=device_t::by_number(this->radio2_device());
		if ( NULL!=D2){set_radio_dry_contact_on_off(D2->number, 1);}
	}
	else if (d->number == this->radio2_device()) 
	{
		device_t *D1=device_t::by_number(radio1_device());
		if ( NULL!=D1){set_radio_dry_contact_on_off(D1->number, 1);}
	}else{
		// it is a CRM4 or jupiter talking
		device_t *D1=device_t::by_number(this->radio2_device());
		if ( NULL!=D1){set_radio_dry_contact_on_off(D1->number, 1);}
		
		device_t* D2=device_t::by_number(radio1_device());
		if ( NULL!=D2){set_radio_dry_contact_on_off(D2->number, 1);}
	}
}

void vega_conference_t::deactivate_radio_base(device_t* d /* device that is releasing M when stop to talk */)
{	// todo: seulement si personne ne parle dans la conference !!!!
	// we need to desactivate a dry contact for the radio 1 and 2 be able to stop hearing conference when nobody is talking ...
	device_t *D=device_t::by_number(this->radio1_device());
	if ( NULL!=D ){
		set_radio_dry_contact_on_off(D->number, 0);
	}
	D=device_t::by_number(this->radio2_device());
	if ( NULL!=D){
		set_radio_dry_contact_on_off(D->number, 0);
	}
}




vega_conference_t* vega_conference_t::by_director(const device_t *d)
{	// new: peut etre directeur de plusieurs conferences !!!!
	vega_conference_t * c=NULL;
	int ic=0;
	foreach(vega_conference_t *c,vega_conference_t::qlist)
	//for (ic = 0; ic < g_list_length(conferences) ; ic++)  // parcours de toutes les conferences....
	{
		//c = (vega_conference_t*)g_list_nth_data(conferences, ic);
		int k=0;
		for ( k=0;k<MAX_PARTICIPANTS; k++){
			if ( d==c->participant_director.device )	return c;
		}
	}
	return NULL;
}

vega_conference_t* vega_conference_t::first_active_conference_where_device_is_participant(const device_t *d)
{
	vega_conference_t * c=NULL;
	int ic=0;
	//for (ic = 0; ic < g_list_length(conferences) ; ic++)  // parcours de toutes les conferences....
	foreach(vega_conference_t *c,vega_conference_t::qlist)
	{
		//c = (vega_conference_t*)g_list_nth_data(conferences, ic);
		if ( c->active ) {
			int k=0;for ( k=0;k<MAX_PARTICIPANTS; k++){
				if ( d==c->participants[k].device  && c->participants[k].state != NOT_PARTICIPANT )	
					return c;
			}
		}
	}
	return NULL;
}

vega_conference_t *vega_conference_t::by_number(int number)
{
  foreach(vega_conference_t *conf,vega_conference_t::qlist)
  {
    if (conf->number == number)
      return conf;
  }
  return NULL;
}

void vega_conference_t::dump(FILE* fout)
{

	char str_participants[128]={0};
	int k=0;
	for (k=0; k< nb_particpants  ; k++){
		char str[128]={0};
		if ( participants[k].device){
			snprintf(str,sizeof(str)-1,"%d(%d) ",participants[k].device->number,participants[k].state);
			strncat(str_participants,str,sizeof(str)-1);
			//printf("%s\n",str_participants);
		}
	}

	fprintf(fout,"C%d %10.10s ACTIV:%d INIT:%02d DTOR%02d R1:%02d(%d) R2:%02d(%d) J:%02d(%d) TS[%d-%d] speaking[%d] particip[%s]\n",
		number,
		name,
		active,
		device_initiator?device_initiator->number:0,
		participant_director.device?participant_director.device->number:0,
		radio1_device(),
		participant_radio1.state,
		radio2_device(),
		participant_radio2.state,
		jupiter2_device(),
		participant_jupiter.state,
		matrix_configuration->ts_start,
		matrix_configuration->ts_start+8,
		nb_people_speaking(),
		str_participants	
		);

	vega_log(INFO,"C%d %8.8s ACTIV:%d INIT:%02d DTOR%02d R1:%02d(%d) R2:%02d(%d) J:%02d(%d) TS[%d-%d] speaking[%d] particip[%s]\n",
		number,
		name,
		active,
		device_initiator?device_initiator->number:0,
		participant_director.device?participant_director.device->number:0,
		radio1_device(),
		participant_radio1.state,
		radio2_device(),
		participant_radio2.state,
		jupiter2_device(),
		participant_jupiter.state,
		matrix_configuration->ts_start,
		matrix_configuration->ts_start+8,
		nb_people_speaking(),
		str_participants	
		);

}

void dump_conferences(FILE* fout)
{
	if ( NULL==fout) return;
	
	int i;
	//for (i = 0; i < g_list_length(conferences) ; i++) 
	foreach(vega_conference_t *c,vega_conference_t::qlist)
	{
		//vega_conference_t* c = (vega_conference_t*)g_list_nth_data(conferences, i);
		if ( c ){
			c->dump(fout);
		}
		fprintf(fout,"\n");
	}
}

int vega_conference_t::alloc_speaking_ressource( device_t* d)
{
	vega_conference_t *conf=this;
	int i=0;

	int k;for (k = 0 ; k < PORT_MAX_SPEAKER_RESSOURCES; k ++) 
	{
		vega_log(INFO, "BEFORE speaking_ressources[%d]=%d (D%d) R%d R%d J%d", k, 
			conf->speaking_ressources[k].busy,
			conf->speaking_ressources[k].device_number,
			conf->radio1_device(),
			conf->radio2_device(),
			conf->jupiter2_device());
	}

	for (i = 0 ; i < PORT_MAX_SPEAKER_RESSOURCES; i ++) 
	{

		if ( PORT_RESERVED_JUPITER2 == conf->speaking_ressources[i].reservation_type && 0!=conf->jupiter2_device() ) {
			vega_log(INFO, "CANNOT ALLOC PORT_RESERVED_JUPITER2 speaking_ressources[%d]=%d for D%d taken by J%d", 
				i, conf->speaking_ressources[i].busy,d->number,conf->jupiter2_device());
			continue;
		}
		if ( PORT_RESERVED_RADIO1 == conf->speaking_ressources[i].reservation_type && 0!=conf->radio1_device() ) {
			vega_log(INFO, "CANNOT ALLOC PORT_RESERVED_RADIO1 speaking_ressources[%d]=%d for D%d taken by R%d",
				i, conf->speaking_ressources[i].busy,d->number, conf->radio1_device());
			continue;
		}
		if ( PORT_RESERVED_RADIO2 == conf->speaking_ressources[i].reservation_type && 0!=conf->radio2_device() ) {
			vega_log(INFO, "CANNOT ALLOC PORT_RESERVED_RADIO2 speaking_ressources[%d]=%d for D%d taken by R%d", 
				i, conf->speaking_ressources[i].busy,d->number, conf->radio2_device());
			continue;
		}

		if ( 0==conf->speaking_ressources[i].busy )
		{
			vega_log(INFO, "ALLOC speaking_ressources[%d]=%d for D%d TYPE %d", 
				i, conf->speaking_ressources[i].busy,d->number,conf->speaking_ressources[i].reservation_type);
			
			conf->speaking_ressources[i].busy = 1;
			conf->speaking_ressources[i].device_number = d->number;

			int k;for (k = 0 ; k < PORT_MAX_SPEAKER_RESSOURCES; k ++) 
			{
				vega_log(INFO, "AFTER speaking_ressources[%d]=%d (D%d)", k, 
					conf->speaking_ressources[k].busy,
					conf->speaking_ressources[k].device_number);
			}

			return i;//conf->speaking_ressources[i].reservation_type;
		}else{
			vega_log(INFO, "CANNOT ALLOC PORT_RESERVED_SECONDARY speaking_ressources[%d]=%d for D%d taken by D%d", 
				i, conf->speaking_ressources[i].busy,d->number, conf->speaking_ressources[i].device_number );
			continue;
		}
	}

	{
		int k;for (k = 0 ; k < PORT_MAX_SPEAKER_RESSOURCES; k ++) 
		{
			vega_log(INFO, "AFTER speaking_ressources[%d]=%d (D%d)", k, 
				conf->speaking_ressources[k].busy,
				conf->speaking_ressources[k].device_number);
		}
	}
	
	vega_log(INFO, "$$$$$$$$$$$$$$$$ CANNOT ALLOC ANY speaking_ressources for D%d $$$$$$$$$$$$$$$$",d->number);
	return PORT_RESERVED_INVALID;
}


int vega_conference_t::free_speaking_ressource(device_t* d)
{
	vega_conference_t *conf = this;
	int k;for (k = 0 ; k < PORT_MAX_SPEAKER_RESSOURCES; k ++) 
	{
		vega_log(INFO, "AFTER speaking_ressources[%d]=%d (D%d)", k, 
			conf->speaking_ressources[k].busy,
			conf->speaking_ressources[k].device_number);
	}

	int i=0;
	for (i = 0 ; i < PORT_MAX_SPEAKER_RESSOURCES; i ++) 
	{
		if ( conf->speaking_ressources[i].busy && d->number == conf->speaking_ressources[i].device_number)
		{
			vega_log(INFO, "FREE speaking_ressources[%d]=%d used by D%d", i, conf->speaking_ressources[i].busy,d->number);

			conf->speaking_ressources[i].busy = 0;
			conf->speaking_ressources[i].device_number = 0;
			
			int k;for (k = 0 ; k < PORT_MAX_SPEAKER_RESSOURCES; k ++) 
			{
				vega_log(INFO, "AFTER speaking_ressources[%d]=%d (D%d)", k, 
					conf->speaking_ressources[k].busy,
					conf->speaking_ressources[k].device_number);
			}
			return i;// conf->speaking_ressources[i].reservation_type;
		}
	}

	{
		int k;for (k = 0 ; k < PORT_MAX_SPEAKER_RESSOURCES; k ++) 
		{
			vega_log(INFO, "AFTER speaking_ressources[%d]=%d (D%d)", k, 
				conf->speaking_ressources[k].busy,
				conf->speaking_ressources[k].device_number);
		}
	}
	return PORT_RESERVED_INVALID;
}

unsigned int vega_conference_t::nb_people_speaking()
{// compute how many devices speaks in this active conference
	unsigned int k=0,N=0;

	if ( nb_particpants >= MAX_PARTICIPANTS ) nb_particpants= MAX_PARTICIPANTS-1; 

	if ( active ) {
		for (k=0; k< nb_particpants  ; k++){
			if ( participants[k].state == PARTICIPANT_ACTIVE ) { // parle t'il, si oui, est ce ds cette conference ???
				if ( participants[k].device && participants[k].device->is_speaking() && participants[k].device->current_conference == this->number) {
					N++;
				}
			}
		}
		if ( participant_director.device && participant_director.device->is_speaking() && participant_director.state == PARTICIPANT_ACTIVE && participant_director.device->current_conference == this->number ) 
			N++;
		if ( participant_radio1.device && participant_radio1.device->is_speaking() && participant_radio1.state == PARTICIPANT_ACTIVE && participant_radio1.device->current_conference == this->number ) 
			N++;
		if ( participant_radio2.device && participant_radio2.device->is_speaking() && participant_radio2.state == PARTICIPANT_ACTIVE && participant_radio2.device->current_conference == this->number ) 
			N++;
		if ( participant_jupiter.device && participant_jupiter.device->is_speaking() && participant_jupiter.state == PARTICIPANT_ACTIVE && participant_jupiter.device->current_conference == this->number ) 
			N++;
	}
	//vega_log(INFO,"C%d: nb_still_speaking = %d",number, N);
	return N;
}

int vega_conference_t::radio1_device()
{
	if ( participant_radio1.device != NULL && participant_radio1.state!=NOT_PARTICIPANT ) 
		return participant_radio1.device->number;
	else 
		return 0;
}
int vega_conference_t::jupiter2_device()
{
	if ( participant_jupiter.device != NULL && participant_jupiter.state!=NOT_PARTICIPANT ) 
		return participant_jupiter.device->number;
	else 
		return 0;
}

int vega_conference_t::director_device()
{
	if ( participant_director.device != NULL && participant_director.state!=NOT_PARTICIPANT ) 
		return participant_director.device->number;
	else 
		return 0;
}

int vega_conference_t::radio2_device()
{
	if ( participant_radio2.device != NULL && participant_radio2.state!=NOT_PARTICIPANT ) 
		return participant_radio2.device->number;
	else 
		return 0;
}

int vega_conference_t::is_in(const device_t *d)
{
	return ( get_state_in_conference(d)!=NOT_PARTICIPANT );
}

bool vega_conference_t::add_in_radio1(Radio* d)
{
	if ( participant_radio1.state==NOT_PARTICIPANT ) {
		participant_radio1.device = d;
		vega_log(INFO, "OK RADIO1 ADDED R%d in C%d (total:%d)", d->number, number, nb_particpants);
		return true;
	}
	return false;
}

bool vega_conference_t::add_in_radio2(Radio* d)
{
	if ( participant_radio2.state==NOT_PARTICIPANT ) {
		participant_radio2.device = d;
		vega_log(INFO, "OK RADIO2 ADDED R%d in C%d (total:%d)", d->number, number, nb_particpants);
		return true;
	}
	return false;
}

bool vega_conference_t::add_in_jupiter(Radio* d)
{
	if ( participant_jupiter.state==NOT_PARTICIPANT ) {
		participant_jupiter.device = d;
		vega_log(INFO, "OK JUP ADDED R%d in C%d (total:%d)", d->number, number, nb_particpants);
		return true;
	}
	return false;
}

bool vega_conference_t::add_in_director(CRM4* d)
{
	if ( participant_director.state==NOT_PARTICIPANT ) {
		participant_director.device = d;
		vega_log(INFO, "OK DTOR ADDED D%d in C%d (total:%d)", d->number, number, nb_particpants);
		return true;
	}
	return false;
}

bool vega_conference_t::add_in(CRM4* d)
{
	if ( nb_particpants < MAX_PARTICIPANTS )
	{
		int IDX = nb_particpants;
		unsigned int j;for (j = 0; j < nb_particpants; j ++) {if ( d== participants[j].device) IDX=j;}
		participants[IDX].device = d;
		participants[IDX].state = PARTICIPANT_ACTIVE; 
		nb_particpants++;
		vega_log(INFO, "OK ADDED participant[%d] D%d to C%d (total:%d)", IDX, d->number, number,nb_particpants);
		return true;
	}
	vega_log(INFO,"NOK ADDED participant D%d to C%d (total:%d MAX_PARTICIPANTS)", d->number, number,nb_particpants);
	return false;
}

bool vega_conference_t::set_state_in_conference(device_t *d, participant_st ST)
{
	dump(stdout);
	if (d == participant_radio1.device){
		participant_radio1.state=ST;
		return true;
	}else if (d == participant_radio2.device){
		participant_radio2.state=ST;
		return true;
	}else if (d == participant_jupiter.device){
		participant_jupiter.state=ST;
		return true;
	}else if (d == participant_director.device){ // le directeur peut s'auto exclure !!!!
		participant_director.state=ST;
		return true;
	}

	unsigned int j=0;for (j = 0; j < nb_particpants; j ++) {
		if (d == participants[j].device){
			participants[j].state =ST ; 
			vega_log(INFO, "participants[%d] set state %d in C%d", j, ST, number);
			return true;
		}
	}
	return false;
}

participant_st vega_conference_t::get_state_in_conference(const device_t *d)
{
	if (d == participant_radio1.device){
		return participant_radio1.state;
	}else if (d == participant_radio2.device){
		return participant_radio2.state;
	}else if (d == participant_jupiter.device){
		return participant_jupiter.state;	
	}else if (d == participant_director.device){
		return participant_director.state;	
	}
	unsigned int j=0; for (j = 0; j < nb_particpants; j ++) {
		if (d == participants[j].device){
			//vega_log(INFO, "participants[%d] D%d STATE:%d in C%d", j, d->number, participants[j].state, number);
			return participants[j].state;
			break;
		}
	}
	return NOT_PARTICIPANT;
}


/*[C1]
devices = 3,2,4,5,6,7
name = KOUROUC1
director = 1
radio1 = 0
radio2 = 0
jupiter2 = 0
number = 1

[G1]
number = 1
members = 3,2,1,4,5*/
void vega_conference_t::reload_devices(const char* fname)
{	
	printf("reload_devices %s\n",fname);
	
	rude::Config config;
	if (config.load(fname) == false) {
		printf("rude cannot load %s file", fname);
		return ;
	}
	for ( int cnumber=1;cnumber<=10;cnumber++)
	{
		QString strconfsection = QString("C%1").arg(cnumber);
		if ( true == config.setSection(qPrintable(strconfsection),false) ) 
		{
			int new_director_number = config.getIntValue("director");
			int new_radio_number1 = config.getIntValue("radio1");
			int new_radio_number2 = config.getIntValue("radio2");
			int new_jupiter2_number = config.getIntValue("jupiter");
			QString strmembers = config.getStringValue("devices");
			QString strname= config.getStringValue("name");

			vega_conference_t* C=vega_conference_t::by_number(cnumber);

			if ( NULL==C) continue;

			if ( new_director_number ) 
			{
				vega_log(INFO,"add dtor %d to C%d\n", new_director_number,C->number);
				EVENT ev;								
				ev.code = EVT_ADD_CRM4_DIRECTOR;								
				ev.device_number = new_director_number;								
				ev.conference_number = C->number;	
				device_t *d = device_t::by_number(new_director_number);
				if ( NULL!=d ) d->ExecuteStateTransition(&ev);// send the news to the device state machine EVT_ADD_DEVICE
				//BroadcastEvent(&ev);
			}
			// RADIO 1 ADDED ?
			if ( new_radio_number1 ) 
			{
				vega_log(INFO,"!!!! add radio1 D%d to C%d\n", new_radio_number1,C->number);
				EVENT ev;								
				ev.code = EVT_ADD_RADIO1;								
				ev.device_number = new_radio_number1;								
				ev.conference_number = C->number;	
				device_t *d = device_t::by_number(new_radio_number1);
				//if ( NULL!=d ) d->ExecuteStateTransition(&ev);
				BroadcastEvent(&ev, d);
			}
			// RADIO 2 ADDED ?
			if ( new_radio_number2 ) 
			{
				vega_log(INFO,"add radio2 D%d to C%d\n", new_radio_number2,C->number);
				EVENT ev;								
				ev.code = EVT_ADD_RADIO2;								
				ev.device_number = new_radio_number2;								
				ev.conference_number = C->number;	
				device_t *d = device_t::by_number(new_radio_number2);
				//if ( NULL!=d ) d->ExecuteStateTransition(&ev);
				BroadcastEvent(&ev, d);	
			}
			// JUPITER ADDED ?
			{
				if ( new_jupiter2_number )
				{
					vega_log(INFO,"add jupiter D%d to C%d\n", new_jupiter2_number,C->number);
					EVENT ev;								
					ev.code = EVT_ADD_JUPITER;								
					ev.device_number = new_jupiter2_number;								
					ev.conference_number = C->number;	
					device_t *d = device_t::by_number(new_jupiter2_number);
					//if ( NULL!=d ) d->ExecuteStateTransition(&ev);
					BroadcastEvent(&ev, d);	
				}
			}

			{
				// CRM4 ADDED ? //printf("      1-check if the file contain new participants in the conference\n");
				int participants[MAX_PARTICIPANTS]={0};  /* liste des device number des participants ds la conference */
				int participants_idx=0;
				char str_particpants[256]={0};
				
				strncpy(str_particpants, qPrintable(strmembers ) ,sizeof(str_particpants) -1);// = atoi(temp);

				vega_log(INFO,"str_particpants = %s\n", str_particpants);
				printf("str_particpants = %s\n", str_particpants);
				char *elt = strtok(str_particpants, ",");
				while (elt != NULL) 
				{
					int device_number = atoi(elt);
					participants[participants_idx++] = device_number;
					//printf("entry devices contains elt: %d!\n", device_number);
					//printf("check if participant D%02d is in C%d ?\n", device_number, C->number);						 
					device_t *d = device_t::by_number(device_number);						  
					if (d == NULL) {					  
						vega_log(ERROR, "wrong device %s in this conference !", elt);				  
						printf("wrong device %s in this conference !", elt);				  
					}else { // this device exist in devices.conf, does it exist in the conference ?
						if ( !C->is_in(d) )
						{
							vega_log(INFO, "=====> D%d is a NEW CRM4 participant in C%d\n", d->number, C->number);		
							//printf( "===> D%d is a NEW CRM4 participant in C%d\n", d->number, C->number);		

							EVENT ev;								
							ev.code = EVT_ADD_CRM4;								
							ev.device_number = d->number;								
							ev.conference_number = C->number;								
							//d->ExecuteStateTransition(&ev);// send the news to the device state machine EVT_ADD_DEVICE
							BroadcastEvent(&ev, d);
						}
					}
					elt = strtok(NULL, ",;* |:");
				}		
			}
		}
	}
}

void vega_conference_t::load_conference_section(const char* fname)
{	
	rude::Config config;
	if (config.load(fname) == false) {
		printf("rude cannot load %s file", fname);
		return ;
	}

	for ( int cnumber=1;cnumber<=10;cnumber++)
	{
		QString strconfsection = QString("C%1").arg(cnumber);
		if ( true == config.setSection(qPrintable(strconfsection),false) ) 
		{
			int new_director_number = config.getIntValue("director");
			int new_radio_number1 = config.getIntValue("radio1");
			int new_radio_number2 = config.getIntValue("radio2");
			int new_jupiter2_number = config.getIntValue("jupiter");
			QString strmembers = config.getStringValue("devices");
			QString strname= config.getStringValue("name");

			vega_conference_t *C=vega_conference_t::create(cnumber,qPrintable(strname));
			if ( C ) 
				vega_conference_t::qlist.append(C);
			else {
				vega_log(CRITICAL, "error vega_conference_create C%d %s\n",cnumber,qPrintable(strname));
				continue;
			}

			printf("C%d: strname:%s\n",cnumber, qPrintable(strname) );
			printf("new_director_number:%d\n",new_director_number);
			printf("new_radio_number1:%d\n",new_radio_number1);
			printf("new_radio_number2:%d\n",new_radio_number2);
			printf("new_jupiter2_number:%d\n",new_jupiter2_number);
			printf("strmembers:%s\n",qPrintable(strmembers) );

			if ( new_director_number ) 
			{
				vega_log(INFO,"add dtor %d to C%d\n", new_director_number,C->number);
				EVENT ev;								
				ev.code = EVT_ADD_CRM4_DIRECTOR;								
				ev.device_number = new_director_number;								
				ev.conference_number = C->number;	
				device_t *d = device_t::by_number(new_director_number);
				if ( NULL!=d ) d->ExecuteStateTransition(&ev);// send the news to the device state machine EVT_ADD_DEVICE
				//BroadcastEvent(&ev);
			}
			// RADIO 1 ADDED ?
			if ( new_radio_number1 ) 
			{
				vega_log(INFO,"add radio1 D%d to C%d\n", new_radio_number1,C->number);
				EVENT ev;								
				ev.code = EVT_ADD_RADIO1;								
				ev.device_number = new_radio_number1;								
				ev.conference_number = C->number;	
				device_t *d = device_t::by_number(new_radio_number1);
				//if ( NULL!=d ) d->ExecuteStateTransition(&ev);
				BroadcastEvent(&ev, d);
			}
			// RADIO 2 ADDED ?
			if ( new_radio_number2 ) 
			{
				vega_log(INFO,"add radio2 D%d to C%d\n", new_radio_number2,C->number);
				EVENT ev;								
				ev.code = EVT_ADD_RADIO2;								
				ev.device_number = new_radio_number2;								
				ev.conference_number = C->number;	
				device_t *d = device_t::by_number(new_radio_number2);
				//if ( NULL!=d ) d->ExecuteStateTransition(&ev);
				BroadcastEvent(&ev, d);	
			}
			// JUPITER ADDED ?
			{
				if ( new_jupiter2_number )
				{
					vega_log(INFO,"add jupiter D%d to C%d\n", new_jupiter2_number,C->number);
					EVENT ev;								
					ev.code = EVT_ADD_JUPITER;								
					ev.device_number = new_jupiter2_number;								
					ev.conference_number = C->number;	
					device_t *d = device_t::by_number(new_jupiter2_number);
					//if ( NULL!=d ) d->ExecuteStateTransition(&ev);
					BroadcastEvent(&ev, d);	
				}
			}

			{
				// CRM4 ADDED ? //printf("      1-check if the file contain new participants in the conference\n");
				int participants[MAX_PARTICIPANTS]={0};  /* liste des device number des participants ds la conference */
				int participants_idx=0;
				char str_particpants[256]={0};
				
				strncpy(str_particpants, qPrintable(strmembers ) ,sizeof(str_particpants) -1);// = atoi(temp);

				vega_log(INFO,"str_particpants = %s\n", str_particpants);
				printf("str_particpants = %s\n", str_particpants);
				char *elt = strtok(str_particpants, ",");
				while (elt != NULL) 
				{
					int device_number = atoi(elt);
					participants[participants_idx++] = device_number;
					//printf("entry devices contains elt: %d!\n", device_number);
					//printf("check if participant D%02d is in C%d ?\n", device_number, C->number);						 
					device_t *d = device_t::by_number(device_number);						  
					if (d == NULL) {					  
						vega_log(ERROR, "wrong device %s in this conference !", elt);				  
						printf("wrong device %s in this conference !", elt);				  
					}else { // this device exist in devices.conf, does it exist in the conference ?
						if ( !C->is_in(d) )
						{
							vega_log(INFO, "=====> D%d is a NEW CRM4 participant in C%d\n", d->number, C->number);		
							//printf( "===> D%d is a NEW CRM4 participant in C%d\n", d->number, C->number);		

							EVENT ev;								
							ev.code = EVT_ADD_CRM4;								
							ev.device_number = d->number;								
							ev.conference_number = C->number;								
							//d->ExecuteStateTransition(&ev);// send the news to the device state machine EVT_ADD_DEVICE
							BroadcastEvent(&ev, d);
						}
					}
					elt = strtok(NULL, ",;* |:");
				}		
			}
		}//if ( true == config.setSection(strconfsection,false) ) 
	}//	for ( int cnumber=1;cnumber<=46;cnumber++)
}


void vega_conference_t::update_remove_devices_from_conference(const char* fname)
{	
	vega_log(INFO,"update_remove_devices_from_conference %s",fname);
	printf("update_remove_devices_from_conference %s\n",fname);
	rude::Config config;
	if (config.load(fname) == false) {
		printf("rude cannot load %s file", fname);
		return ;
	}

	for ( int cnumber=1;cnumber<=10;cnumber++)
	{
		QString strconfsection = QString("C%1").arg(cnumber);
		if ( true == config.setSection(qPrintable(strconfsection),false) ) 
		{
			int new_director_number = config.getIntValue("director");
			int new_radio_number1 = config.getIntValue("radio1");
			int new_radio_number2 = config.getIntValue("radio2");
			int new_jupiter2_number = config.getIntValue("jupiter");
			QString strmembers = config.getStringValue("devices");
			QString strname= config.getStringValue("name");

			vega_conference_t *C=vega_conference_t::by_number(cnumber);
			if ( NULL == C ) {
				vega_log(INFO, "error vega_conference_create C%d %s\n",cnumber,qPrintable(strname));
				continue;
			}

			printf("C%d: strname:%s\n",cnumber, qPrintable(strname) );
			printf("new_director_number:%d\n",new_director_number);
			printf("new_radio_number1:%d\n",new_radio_number1);
			printf("new_radio_number2:%d\n",new_radio_number2);
			printf("new_jupiter2_number:%d\n",new_jupiter2_number);
			printf("strmembers:%s\n",qPrintable(strmembers) );



				// RADIO 1 REMOVED ?
				{
					int r1 = new_radio_number1;		
					if ( NOT_PARTICIPANT != C->participant_radio1.state )
					if ( NULL!=C->participant_radio1.device  && (r1!=C->participant_radio1.device->number ) )
					{
						vega_log(INFO,"!!!! remove radio1 D%d with D%d in C%d\n",C->participant_radio1.device->number, r1 , C->number);
						EVENT ev;								
						ev.code = EVT_REMOVE_DEVICE;								
						ev.device_number = C->participant_radio1.device->number;								
						ev.conference_number = C->number;	
						device_t *d = device_t::by_number(C->participant_radio1.device->number);
						//if ( NULL!=d ) d->ExecuteStateTransition(&ev);// send the news to the device state machine EVT_REMOVE_DEVICE
						BroadcastEvent(&ev,d);
					}
				}

				// RADIO 2 REMOVED ?
				{
					int r2 = new_radio_number2;
					if ( NOT_PARTICIPANT != C->participant_radio2.state )
					if ( NULL!=C->participant_radio2.device  && (r2!=C->participant_radio2.device->number ) )
					{
						vega_log(INFO,"!!!! remove radio2 D%d with D%d in C%d\n",C->participant_radio2.device->number, r2 , C->number);
						EVENT ev;								
						ev.code = EVT_REMOVE_DEVICE;								
						ev.device_number = C->participant_radio2.device->number ;								
						ev.conference_number = C->number;	
						device_t *d = device_t::by_number(C->participant_radio2.device->number );
						//if ( NULL!=d ) d->ExecuteStateTransition(&ev);// send the news to the device state machine EVT_REMOVE_DEVICE
						BroadcastEvent(&ev,d);
					}
				}

				// JUPITER REMOVED ?
				{
					int j2 = new_jupiter2_number;
					if ( NOT_PARTICIPANT != C->participant_jupiter.state )
					if ( C->participant_jupiter.device && (j2!=C->participant_jupiter.device->number ) )
					{
						vega_log(INFO,"!!!! remove jupiter D%d with D%d in C%d\n",C->participant_jupiter.device->number, j2 , C->number);
						EVENT ev;								
						ev.code = EVT_REMOVE_DEVICE;								
						ev.device_number = C->participant_jupiter.device->number ;								
						ev.conference_number = C->number;	
						device_t *d = device_t::by_number(C->participant_jupiter.device->number );
						//if ( NULL!=d ) d->ExecuteStateTransition(&ev);// send the news to the device state machine EVT_REMOVE_DEVICE
						BroadcastEvent(&ev,d);
					}
				}

				// CRM4 REMOVED ?
				//printf("      1-check if the file contain new participants in the conference\n");
				//printf("      ==============================================================\n");
				int participants[MAX_PARTICIPANTS]={0};  /* liste des device number des participants ds la conference */
				int participants_idx=0;
				int removed_participants[MAX_PARTICIPANTS]={0};  /* liste des device number des participants ds la conference */
				int removed_participants_idx=0;


				char str_particpants[256]={0};
				strncpy(str_particpants ,qPrintable(strmembers),sizeof(str_particpants));
				printf("str_particpants = %s\n", str_particpants);

				char *elt = strtok(str_particpants, ",");
				while (elt != NULL) 
				{
					int device_number = atoi(elt);
					participants[participants_idx++] = device_number;
					printf("entry devices contains elt: %d!\n", device_number);
					elt = strtok(NULL, ",;* |:");
				}

				if ( C )
				{ // check if this conference has changed !
					// iterate current participants of the conference...
					int j;for (j = 0; j < C->nb_particpants; j++) 
					{
						device_t* partip_dev = C->participants[j].device;
						if (NULL!=partip_dev)
						{
							printf("find current participant D%d in the tab ?\n", partip_dev->number);
							
							int found=0; // can we find this participants in the 'devices' section of ini file ?
							if ( C->participant_radio1.device && C->participant_radio1.device->number == partip_dev->number ) continue;
							if ( C->participant_radio2.device && C->participant_radio2.device->number == partip_dev->number ) continue;
							if ( C->participant_jupiter.device && C->participant_jupiter.device->number == partip_dev->number ) continue;
							if ( C->participant_director.device && C->participant_director.device->number == partip_dev->number ) continue; // pdl20090519 bug C->device_director NULL ?

							
							int k=0;
							// look in participants[] if this device has not been removed !
							for (k=0; k < participants_idx; k++)
							{
								//printf("participants[%d] %d == partip_dev->number %d ?\n", k, participants[k] , partip_dev->number);
								if ( participants[k] == partip_dev->number ) {
									found = 1;
									break;
								}
							}
							if ( !found ){			
										
								vega_log(INFO,"=================================> D%d remove from participant in C%d\n", partip_dev->number, C->number);
								printf("====> D%d remove from participant in C%d\n", partip_dev->number, C->number);
								removed_participants[removed_participants_idx++] = partip_dev->number;
							}
						}
					}

					vega_log(INFO,"%d participants to be removed",removed_participants_idx);
					printf("%d participants to be removed\n",removed_participants_idx);

					unsigned int n; for (n=0;n<removed_participants_idx ; n++)
					{
							EVENT ev;
							printf("D%d EVT_REMOVE_DEVICE in C%d\n",removed_participants[n],C->number );
							ev.code = EVT_REMOVE_DEVICE;
							ev.device_number = removed_participants[n];
							ev.conference_number = C->number;
							device_t *d = device_t::by_number(removed_participants[n]);		
							//d->ExecuteStateTransition(&ev);
							//BroadcastEvent(&ev);
							BroadcastEvent(&ev,d);
					}			
				}// if ( C )
			
		}// if ( true == config.setSection(strconfsection,false) ) 
		
	}//for ( int cnumber=1;cnumber<=46;cnumber++)
}
