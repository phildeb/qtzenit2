#ifndef __CONF_H__
#define __CONF_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <syslog.h>
#include <sys/ioctl.h>

#include <pthread.h>
#include <QLinkedList>
#include <QMutex>
#include <QMap>
#include <QString>

#include "const.h"
#include "mixer.h"

extern int verbose; // debug verbose ?
extern char ihm_path[1024];//=".";
extern char work_path[1024];//="."; // path where storing current config
extern char log_path[];
void* reload_config_thread(void *arg);
void init_when_link_is_up();

typedef enum {
  TYPE_CRM4 = 0,
  TYPE_RADIO,
  TYPE_JUP2
} device_type_t;

typedef enum
{
	EVT_DEVICE_KEY_MICRO_RELEASED=0,
	EVT_DEVICE_KEY_MICRO_PRESSED,
	EVT_DEVICE_KEY_CANCEL,
	EVT_RADIO_MICRO_RELEASED=0,
	EVT_RADIO_MICRO_PRESSED,

	EVT_DEVICE_KEY_ACTIVATE_CONFERENCE=10,
	EVT_DEVICE_KEY_SELECT_CONFERENCE,
	EVT_DEVICE_KEY_SELF_EXCLUSION_INCLUSION,

	EVT_CONFERENCE_ACTIVATED=20,
	EVT_CONFERENCE_DEACTIVATED,

	EVT_CALL_GROUP_START_TONE,
	//EVT_CALL_GROUP_START_SPEAKING,
	EVT_CALL_GROUP_STOP_TONE,
	EVT_CALL_GROUP_STOP_SPEAKING,
	
	//EVT_CALLGENERAL_START_SPEAKING,
	EVT_CALLGENERAL_STOP_SPEAKING,
	EVT_CALLGENERAL_START_TONE,
	EVT_CALLGENERAL_STOP_TONE,

	EVT_CONFERENCE_INCLUDE_EXCLUDE=30,
	EVT_CONFERENCE_SELF_EXCLUDE,
	EVT_CONFERENCE_SELF_INCLUDE,
	EVT_CONFERENCE_REINCLUDE_DEMAND,
	EVT_CONFERENCE_REINCLUDE_STOP_BLINK,

	EVT_CONFERENCE_START_TONE,
	EVT_CONFERENCE_STOP_TONE,

	EVT_CONFERENCE_DTOR_INCLUDE,
	EVT_CONFERENCE_DTOR_EXCLUDE,


	EVT_DEVICE_KEY_GENERAL_CALL=40,
	EVT_DEVICE_KEY_GROUP_CALL,

	EVT_DEVICE_KEY_SELECT_DEVICE=50,
	EVT_ADD_CRM4,
	EVT_DEVICE_ADDED,
	EVT_REMOVE_DEVICE,
	EVT_ADD_CRM4_DIRECTOR,
	EVT_ADD_RADIO1,
	EVT_ADD_RADIO2,
	EVT_ADD_JUPITER,

	EVT_DEVICE_UPDATE_DISPLAY=60,

	EVT_CONFERENCE_START_DEACTIVE_TONE,
	EVT_CONFERENCE_STOP_DEACTIVE_TONE,

	EVT_DEVICE_KEY_DEBUG, // pour afficher les timeslot de aslt et aga
	EVT_DEVICE_NOT_PRESENT, // pas present, message envoye au debut
	EVT_DEVICE_UNPLUGGED,		// on debranche le poste
	EVT_DEVICE_PLUGGED,		// on rebranche le poste
	EVT_DEVICE_PRESENT // present, message envoye au debut

}T_CODE_EVENT;

typedef struct 
{
	int device_number; // device originating the EVENT
	int conference_number; // conference concerned by the EVENT
	int group_number; // group concerned by the EVENT
	T_CODE_EVENT code;
	int delay_ms; // delay before processing this event
	union
	{	
		struct{
			int number;
		}key;
		struct{
			int type; // ex ACTION_GENERAL_CALL
		}action;
		struct{
			int conference_number;
			int device_number;
		}select;
	}value;	

	char unused[1024];
}EVENT;

typedef enum {
  PORT_RESERVED_SECONDARY1, // order is very important 
  PORT_RESERVED_SECONDARY2,
  PORT_RESERVED_JUPITER2,
  PORT_RESERVED_RADIO1,
  PORT_RESERVED_RADIO2,
} port_reservation_t;

class vega_conference_t;

class device_t
{
public:
	device_t(){name[0]=0;number=0;current_conference=0;m_timeslot_micro=0;
		init_mixer_done = is_hearing_tone = b_speaking=false;}
	virtual ~device_t(){}

	int board_number;
	int sbi_number;
	int port_number;
	int number;
	char name[256];
	int current_conference;
	bool b_speaking;
	int m_timeslot_micro;

	static QLinkedList<device_t *> qlist_devices;

	// VIRTUAL
	virtual int is_speaking(){return 0;}
	virtual const char* state_string(){return "?device_t?";}
	virtual const char* type_string(){return "?device_t?";}
	virtual int ExecuteStateTransition(EVENT* ){return 0;}
	virtual int ExecuteStateTransition2(EVENT* ){return 0;}
	virtual void dump_mixers(){}
	virtual int DeferEvent(EVENT* , int ){return 0;}
	virtual bool has_hardware_error(){return false;}

	//int nb_active_conf_where_device_is_active_and_not_dtor_excluded(); // device is in an active conf and not dtor/auto excluded
	int nb_active_conf_where_device_is_active_or_dtor_excluded(); // device is in an active conf and not auto excluded
	int nb_conf_where_device_is_in(); // device has this conf led green
	
	virtual void fprint_mixers( FILE* ){}
	bool init_mixer_done;
	virtual void init_mixers(){}

	bool is_hearing_tone;
	virtual void start_hearing_tone(int ){}
	virtual void stop_hearing_tone(){}

	bool has_conference_in_common(int ); // for general call 

	static void init_mixer_devices();

	static int is_valid_device_number(int N){
		if  ( N >=1 && N<= MAX_DEVICE_NUMBER ) return 1;
		return 0;
	}
	static device_t *by_number(int device_number){
	  foreach(device_t* elt,device_t::qlist_devices){	if (elt->number == device_number)return elt;}
	  return NULL;  
	}
	char unused[1024];
};/* device_t*/

typedef	enum {
	NOT_PARTICIPANT= 0,
	PARTICIPANT_ACTIVE =1,
	PARTICIPANT_DTOR_EXCLUDED,
	PARTICIPANT_SELF_EXCLUDED,
} participant_st;


class CRM4: public device_t
{
public:
	typedef enum{ETEINTE,FIXE,CLIGNOTEMENT_RAPIDE,CLIGNOTEMENT_LENT,INDEFINI}ETAT_LED;
	ETAT_LED tab_green_led[CRM4_MAX_KEYS];
	ETAT_LED tab_red_led[CRM4_MAX_KEYS];
	ETAT_LED old_tab_green_led[CRM4_MAX_KEYS];
	ETAT_LED old_tab_red_led[CRM4_MAX_KEYS];
	void update_all_leds(); // si un element du tableau a change depuis la derniere fois !
	void init_all_leds()
	{
		for (int i=0;i<CRM4_MAX_KEYS;i++){
			tab_green_led[i] = old_tab_green_led[i] = ETEINTE;
			tab_red_led[i] = old_tab_red_led[i] = ETEINTE;
		}
		device_reset_all_leds();
	}
	//int device_set_led_red_and_green(int key, T_LED_MODE mode_red, char status_red, T_LED_MODE mode_green, char status_green);
	int device_set_led(int key, unsigned char color, char select, char status);
	int device_reset_all_leds();

	int  device_set_blink_fast_red_color(int button) ;
	int  device_set_blink_slow_red_color(int button) ;
	int  device_set_color_red(int button) ;
	int  device_set_blink_fast_green_color(int button) ;
	int  device_set_blink_slow_green_color(int button) ;
	int  device_set_color_green(int button) ;
	int  device_set_no_red_color(int button);
	int  device_set_no_green_color(int button);

	enum{
		VEGA_PORT_ASLT_SECONDARY,
		VEGA_PORT_MIXER_THREE_CONFS,
	};

	vega_port_t port_aslt;
	vega_port_t port_3confs;

	bool display_slots;

    virtual ~CRM4(){}

	CRM4(int n){ 
		b_speaking=display_slots=false;
		number = n;
		linkStateTransition = &CRM4::IdleState;
		m_is_plugged  = false; // pdl 20091002
	}
	
	static int doing_group_call;//=0; // protect against simultaneous group calls and general calls
	static int doing_general_call;//=0; // protect against simultaneous group calls and general calls
	static int device_doing_group_or_general_call;// = 0;
	static int all_devices_display_group_key(int working);

	// device doing a special action
	bool m_is_plugged;
	int gain;
	int activating_conference;
	int excluding_including_conference;
	int self_excluding_including_conference;
	struct timeval _tstart_btn_device;
	struct timeval _tstart_btn_conf;
	//struct timeval _tstart_btn_general;
	struct timeval _tstart_btn_micro;

	// VIRTUAL 
	virtual bool has_hardware_error();
	virtual const char* type_string(){return "CRM4";}

	typedef int (CRM4::*LPFNStateTransition)(EVENT* pEv);
	LPFNStateTransition linkStateTransition;
	
	virtual int ChangeState(LPFNStateTransition LPFN){if ( LPFN ) linkStateTransition = LPFN;return 0;}
	virtual int ExecuteStateTransition(EVENT* ev);
	virtual int ExecuteStateTransition2(EVENT* ev);
	//int UnknownState(EVENT* ev); // specific to CRM4 (plugged, unplugged, not present at all)
	virtual int IdleState(EVENT* ev);
	virtual const char* state_string();
	virtual int DeferEvent(EVENT* ev, int delay_ms);
	virtual void init_mixers();

	// NON VIRTUAL
	int ConfHearingState(EVENT* ev);
	int is_speaking(){ return b_speaking;}
	//virtual bool add_in(vega_conference_t* C);

	//int device_handle_event( vega_event_t *evt);
	int process_input_key(int input_number );

	// DISPLAY
	QMap<int /*crm4key*/,int /*numero_group*/> keymap_groups; // dynamic keys different for each CRM4
	bool has_group_key(int grpnum); 

	void init_groups_keys(char* str_keys);

	static void all_devices_set_green_led_possible_conferences();
	void set_green_led_possible_conferences();
		
	static int all_devices_display_general_call_led(int busy/* if a director is doing a general call*/);
	int display_general_call_led(int busy /* if a director is doing a general call*/);
	
	int device_MAJ(vega_conference_t *C, int update_conferences_led,int update_participants_led, int hide_participants);
	void all_devices_update_display();

	int device_MAJ_participants_led( vega_conference_t *C, int hide_all=0);//,int update_participants_led, int hide_participants);
	int device_MAJ_groups_led(int grpnum);
	int device_MAJ_particpant_key(device_t* d1, int hide_participant, participant_st etat);

	int set_green_led_of_conf(vega_conference_t *conf,int on);
	int set_red_led_of_conf(vega_conference_t *conf,int on);
	//int blink_led_of_device_speaking_in_conf(vega_conference_t *conf,device_t * d);	
	static int all_devices_display_led_of_device_speaking_in_conf(vega_conference_t *conf, device_t * d);
	static int all_devices_display_led_of_device_NOT_speaking_in_conf(vega_conference_t *conf, device_t * d);
	
	int device_display_conferences_state();
	void set_led_conference(vega_conference_t* C);


	int all_devices_display_group_call_led(int GN, int in_use, int speaking);
	int  device_display_msg( int line_number, char *message);
	int device_line_printf(int line, const char* fmt, ...);
	int device_line_print_time_now(int line);


	int device_check_timselot_duplication(int confnum);

	void fprint_mixers( FILE* fout);
	void dump_mixers();

	int device_side_tone_off();
	int device_reset_display();
	int device_is_board_used( int board_number);
	int device_change_gain( int value);
	int device_enable_events();

	int device_open_micro();
	int device_close_micro();
	int device_set_option_display_present();
	int device_set_backlight_on();


	// SPEAKING JOB
	int director_start_speaking(vega_conference_t *c );
	int director_stop_speaking(vega_conference_t *c);
	int director_start_stop_speaking(vega_conference_t *c,  int enable);
	int secondary_start_stop_speaking(vega_conference_t *c, port_reservation_t res_type,  int enable);
	// HEARING JOB
	//void avoid_hear_myself_in_aga_3conf( vega_conference_t *c);
	bool stop_hearing_in_aslt(vega_conference_t* c);
	bool start_hearing_in_aslt_no_echo(vega_conference_t* C,int ts_offset); // when secondary speaks, avoid hear himself
	bool start_hearing_in_aslt(vega_conference_t* C);
	bool stop_hearing_in_aga(vega_conference_t* c);
	bool start_hearing_in_aga(vega_conference_t* c);
	bool start_hearing_in_aga_no_echo(vega_conference_t* c,int ts_offset);

	void start_hearing_tone(int ts);
	void stop_hearing_tone();

	static void* led_thread(void *);
	static void init_thread_led();
	static void dump_devices(FILE* fout);

	//static device_keys_configuration_t* 
	static void load_crm4_keys_configuration(const char* fname);
	static void load_crm4_keys_section(const char* fname);
	//static device_keys_configuration_t the_unic_static_device_keys_configuration; // commune a tous les CRM4
	static void load_crm4_device_section(const char* fname);
	static void check_modif_crm4_device_section(const char* fname);


	static QMap<int,int>		m_common_touche_select_conf; // m_common_touche_select_conf[NoConf] = touche
	static QMap<int,int>		m_common_touche_select_device; // m_common_touche_select_conf[NoDevice] = touche
	static QMap<int,int>		m_common_keymap_number; // todo: delete
	static QMap<int,QString>	m_common_keymap_action; // m_common_keymap_action[1]="action_activate_conf"
	static QMap<QString,int>	m_common_key_number_by_action; // m_common_key_number_by_action[action_exclude_include_conf] = 4

	static CRM4 *create(int device_number, const char* name,int gain);
	static QLinkedList<CRM4 *> qlist_crm4;

	int  init(int board_number, int sbi_number, int port_number, int conf3_board_number,int conf3_sbi_number, int conf3_port_number);


};/*CRM4*/

class Radio: public device_t
{
public:
    //union  {struct {
		  int state;
		  int full_duplex;		
		  int audio_detection;		
		  vega_port_t port_aga_emission;
		  vega_port_t port_aga_reception;
	//	} radio;	} device;

	enum{/* radios */
	  VEGA_PORT_RADIO_EMISSION,
	  VEGA_PORT_RADIO_RECEPTION,
	  /* jupiter 2 */
	  VEGA_PORT_JUP2_EMISSION,
	  VEGA_PORT_JUP2_RECEPTION,
	};
	
	Radio(int n){ 
		b_speaking=false;
		number = n;
		printf("CTOR Radio D%d\n",number);
		linkStateTransition = &Radio::IdleState;
	}
	virtual ~Radio(){}

	virtual bool has_hardware_error();
	virtual const char* type_string(){return "Radio";}
	virtual void init_mixers();
	
	typedef int (Radio::*LPFNStateTransition)(EVENT* pEv);
	LPFNStateTransition linkStateTransition;
	virtual int ChangeState(LPFNStateTransition LPFN){if ( LPFN ) linkStateTransition = LPFN;return 0;}
	virtual int ExecuteStateTransition(EVENT* ev);
	virtual int ExecuteStateTransition2(EVENT* ev);
	virtual int DeferEvent(EVENT* ev, int delay_ms);

	virtual const char* state_string(){ if ( &Radio::RadioConferencingState==linkStateTransition) return "Conferencing"; else return "IdleState"; }
	virtual void dump_mixers(){}
	virtual int IdleState(EVENT* ev);	

	void start_hearing_tone(int ts);
	void stop_hearing_tone();

	// NON VIRTUAL
	int is_speaking(){ return b_speaking;}
	int RadioConferencingState(EVENT* ev);

	bool radio_start_speaking_conf(vega_conference_t* C);
	bool radio_start_hearing_conf(vega_conference_t* C);
	void radio_stop_hearing( vega_conference_t* C);
	void radio_stop_speaking( vega_conference_t *c);

	static void dump_devices(FILE* fout);
	virtual void fprint_mixers( FILE* fout);

	static Radio *create(int device_number, const char* name,int gain);
	static QLinkedList<Radio *> qlist_radio;

	int init_jupiter(int board_number, int sbi_number, int port_number, int audio_detection);
	int init_radio(int full_duplex, int board_number, int sbi_number, int port_number, int audio_detection);

	static int start_detection_audio();
	static void* radio_detection_audio_thread(void *arg);

	//int radio_port_set_initial_mixer_config(vega_port_t *p);

};/*Radio*/

class event_t
{
public:
	EVENT m_event; // copy of the event passed to int CRM4::ExecuteStateTransition(EVENT* ev)
	device_t* m_p_device;				// device concerned by the event
	time_t m_ts_create;		// time when event was created
	unsigned int m_timeout_sec; // time before executing the event

	event_t(device_t* pdev,EVENT* pev, unsigned int timeout){
		memcpy(&m_event,pev,sizeof(EVENT));
		m_p_device		=pdev;
		m_timeout_sec	=timeout;
		m_ts_create		=time(NULL);
	}
	static void* event_thread(void* arg);
	static pthread_t event_thread_id;
	static void init();
	static void queue(device_t* pdev,EVENT* ev,unsigned int timeout);
	char unsued[1024];


};/*event_t*/
int BroadcastEvent(EVENT* ev, device_t* first_device=NULL);
int DeferBroadcastEvent(EVENT* ev, int delay_ms);


//typedef struct vega_conference_s {
class vega_conference_t
{
public:
	int number; /* de 1 a 10 */
	int active; 		/* conference is active */
	char name[512];
	
	vega_port_t ports[CONF_NB_AGA_PORTS]; /* 9 port AGA sont associés à une configuration */  
	
	conference_configuration_t *matrix_configuration; /* configuration matricielle de la conference */

	enum{
		VEGA_PORT_AGA_MIX_JUP2_RADIO1_RADIO2 = 0,
		VEGA_PORT_AGA_MIX_DIRECTOR_SECONDARY1_SECONDARY2,
		VEGA_PORT_AGA_RADIO1,
		VEGA_PORT_AGA_RADIO2,
		VEGA_PORT_AGA_JUPITER2,
		VEGA_PORT_AGA_SECONDARY1,
		VEGA_PORT_AGA_SECONDARY2,
		//VEGA_PORT_AGA_SECONDARY3,
		//VEGA_PORT_AGA_SECONDARY4,
		//VEGA_PORT_AGA_SECONDARY5,
		VEGA_PORT_AGA_DIRECTOR,
		VEGA_PORT_AGA_MIX_CONF,
	};
	
	struct  {
	  port_reservation_t reservation_type;
	  int busy;
	  int device_number;
	}  speaking_ressources[PORT_MAX_SPEAKER_RESSOURCES];  	
	
	CRM4 *device_initiator; 	/* initiateur de la conference */

	struct conference_participant_s {
		device_t *device;
		participant_st  state; // PARTICIPANT_ACTIVE or PARTICIPANT_DTOR_EXCLUDED or PARTICIPANT_SELF_EXCLUDED
	}participants[MAX_PARTICIPANTS];   /* liste des participants configures dans la conference ( member ds conference.conf ) */
	int nb_particpants; 		/* nombre de participants */

	struct conference_participant_s participant_radio1;
	struct conference_participant_s participant_radio2;
	struct conference_participant_s participant_jupiter;
	struct conference_participant_s participant_director;



	// functions
	int radio1_device();
	int radio2_device();
	int jupiter2_device();
	int director_device();

	bool set_state_in_conference(device_t *d, participant_st a_state);
	participant_st get_state_in_conference(const device_t *d);
	int is_in(const device_t *d);
	int free_speaking_ressource(device_t* d);
	int alloc_speaking_ressource( device_t* d);
	int timeslot_belongs_t_conference(int ts){
		if ( (ts >= matrix_configuration->ts_start+0)  && (ts <= matrix_configuration->ts_start + 8) )return 1 ;
		return 0;
	}
	void activate_radio_base(device_t* d); // la base radio passe en emission vers le terminal radio des qu'un intervenant (autre que lui) parle
	void deactivate_radio_base(device_t* d); // la base radio passe en emission vers le terminal radio des qu'un intervenant (autre que lui) parle
	unsigned int nb_people_speaking();// compute how many devices speaks in this active conference
	void dump(FILE* fout);
	int init_mixers();

	//bool remove_from(device_t *d);
	bool add_in(CRM4* d);
	bool add_in_director(CRM4* d);
	bool add_in_radio1(Radio* d);
	bool add_in_radio2(Radio* d);
	bool add_in_jupiter(Radio* d);

	static void init_mixer_conferences();
	bool has_hardware_error();

	static conference_configuration_t configuration_conferences[10];

	static vega_conference_t* by_director(const device_t *d); // get unic conference where d is director
	static vega_conference_t* first_active_conference_where_device_is_participant(const device_t *d);
	static vega_conference_t *by_number(int number);
	static vega_conference_t *create(int number, const char* name=0);

	static QLinkedList<vega_conference_t *> qlist;//vega_conference_t::qlist


	static int conf_vega_port_set_initial_mixer_config(vega_port_t *p);

	static void load_conference_section(const char* fname);
	static void reload_devices(const char* fname);
	static void update_remove_devices_from_conference(const char* fname);

};// vega_conference_t;
void dump_conferences(FILE* fout);


///  GROUPS
typedef struct{
	int tab_devices[MAX_DEVICE_IN_GROUP_CALL];
	int nb_devices;
	QLinkedList<device_t *>	qlist_devices;
}GROUP_T;

/*class Group{
	int number;
	static QLinkedList<device_t *> members;
};*/

int dump_group(FILE*);
int is_valid_group_number(int NG);
int is_device_number_in_group(int devnum,int NG);

// BOARD + ALPHACOM
//int board_display_busy(int board_number);

/* les etats possibles du lien avec l'alpha com */
enum {
  ST_disconnected = 0,		/* deconnecté : etat initial du lien */
  ST_definitely_disconnected,	/* definitivement deconnecté : timer de tentative de reconnection est à 0 */
  ST_disconnected_pending_slave, /* deconnecté, en attente de reconnection en mode slave */
  ST_connected_master,		/* connecté, en mode master */
  ST_connected_unknown,		/* connecté, dans un mode inconnu */
  ST_connected_slave,		/* connecté, dans un mode esclave (mode OK) */
  ST_testing_boards,		/* on scanne les cartes */
  ST_init_boards,		/* initiliaze les cartes */
};

typedef struct vega_control_s {
  struct sockaddr_in peer_addr;
  int sock;
  int tx_seq_num;
  int nb_msgs_sent;
  int nb_msgs_received;
  int cnx_duration;
  int state;
  pthread_t rx_thread;  
  pthread_t tx_thread;  
  int timer; 			/* timer de reconnection  */  
  int order_stop;

  BYTE current_frame[MAX_MSG_SIZE];
  int current_frame_size;

  pthread_mutex_t mutex_recv;
  int rx_next_waited_seqnum;
  int first_ping_sent ;

  /* indique a la tache de lecture des messages qu'il faut continuer
   la lecture de la frame courante. Utile dans une lecture non bloquante
  si le timeout a claque avant la fin de lecture totale du message. (ne devrait
  pas arriver vu la taille des messages)*/
  int continue_recv_current_frame ;
  int wait_duration;
  int clock_100_milliseconds;
  int tick_ping_pong;
} vega_control_t;

vega_control_t *vega_control_create();
int vega_control_init(vega_control_t *plink, char *ip_address, int port) ;
int vega_control_send_alphacom_msg1(vega_control_t *plink, const unsigned char* data, const unsigned int datalen);
int vega_control_send_alphacom_msg(bool,vega_control_t *plink, const unsigned char* data, const unsigned int datalen);
extern vega_control_t *pAlphaComLink;

extern QMap<int /*board number*/,bool /*is_good*/> map_boards;

#endif
