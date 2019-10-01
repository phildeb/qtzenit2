// main.c
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
#include "ixpci.h"
#include "alarmes.h"
#include "debug.h"
#include "conf.h"
#include "ini.h"
#include "misc.h"
#include "rude/config.h"

// /vega/bin/vegactrl -r /vega/share/ -w /vega/etc/

extern int init_contact_secs();
extern 	void vega_test_pong_delay();
extern int init_devices();
//extern void* reload_thread(void *);

extern void* reload_groups_add(const char* fname_remote);
extern void reload_conferences_add(const char* );
extern void reload_devices_add(const char* fname_remote);
extern void reload_conferences_remove(const char* path);
extern void load_groupes_section(const char* fname);

extern int gui_devices_copy_done;
extern int gui_conferences_copy_done;
extern int gui_groups_copy_done;

extern void* listening_monitor_rx_thread(void* arg);
extern void* listening_monitor_tx_thread(void* arg);


extern void* listening_ihm_tcp_thread(void* arg);


int set_contact_P16R16(int number_of_bit /* 0 to 15*/, int value /* 1: contact on: 0: contact off*/ );
static	int b_exit_loop = 0;

#define USE_VEGA_LINK 

// commentaire qd on veut tester des fonctions non liees a VEGA

time_t demarrage_system;
char work_path[1024]="/vega/etc/"; // path where storing current config localy
char ihm_path[1024]="/vega/share/";
char log_path[1024]= _ROOT_EVENT_ALARM_DIR_;

int g_synchronize_remote_config = 0;
int g_remote_tcp_monitoring = 0;

int set_recorder_dry_contact_on_off(int conf_number , int on_off) ;
void set_alarm_dry_contact(int on_off);

char AlphaComIPaddress[32]="169.254.1.5";
unsigned short AlphaComTCPPort  = 50010;

pthread_t event_t::event_thread_id;



void init_when_link_is_up()
{
	static int init_done = 0;
	vega_log(INFO,"========= VEGA TCP link UP init_done=%d=============",init_done);
	printf("========== VEGA TCP link UP init_done=%d =============\n",init_done);
	if ( init_done ) return ;
	init_done = 1;


	vega_conference_t::init_mixer_conferences();
	device_t::init_mixer_devices(); 
	
	CRM4::init_thread_led();
	CRM4::all_devices_set_green_led_possible_conferences();
	CRM4::all_devices_display_group_key(0);
	CRM4::all_devices_display_general_call_led(0);

	Radio::start_detection_audio();
		
	/*if (g_synchronize_remote_config){
		pthread_t thread_id;
		int ret = pthread_create(&(thread_id), NULL, reload_thread, NULL);
		if (ret < 0) {
			vega_log(INFO,"error pthread_create reload_thread - cause: %s", strerror(errno));
		}else{
			vega_log(INFO,"OK pthread_create reload_thread");
		}
	}
	
	if (1){
		pthread_t tid;
		int ret = pthread_create(&(tid), NULL, listening_ihm_tcp_thread, NULL);
		if (ret < 0) {vega_log(INFO,"error pthread_create listening_ihm_tcp_thread - cause: %s", strerror(errno));}
	}
	
	if (g_remote_tcp_monitoring){
		pthread_t tid;
		int ret = pthread_create(&(tid), NULL, listening_monitor_rx_thread, NULL);
		if (ret < 0) {vega_log(INFO,"error pthread_create listening_monitor_rx_thread - cause: %s", strerror(errno));}		
		ret = pthread_create(&(tid), NULL, listening_monitor_tx_thread, NULL);
		if (ret < 0) {vega_log(INFO,"error pthread_create listening_monitor_tx_thread - cause: %s", strerror(errno));}		
	}
	*/
}


static void sigpipe(int par)
{
    printf("sigpipe(%d)\n", par);
	vega_log(INFO, "sigpipe(%d)", par);
    //fflush(stdout);
    return;
}

static void sigterm(int par)
{
	printf("sigterm(%d)\n",par);
	vega_log(INFO, "sigterm(%d)\n", par);

	if ( 2 == par ) {
		b_exit_loop = 1;
	}

}

int fetch_args(int argc, char *argv[]) 
{
	printf("fetch_args %s %d\n", argv[0], argc);
	int z = 1;
	int more = 1;	
	if (argc <= 1) {
		//strncpy( ihm_path, "/srv/www/htdocs/reservation", );
		//strncpy( ihm_path, "."  ,);
		return 0;
	}
	while (more) 
	{
		printf("argv[z]=%s\n",argv[z]);
		if (   0== strncmp(argv[z], "-r", 2)  || (0== strncmp(argv[z], "-d", 2))   ) 
		{

			

			if (++z == argc) 
			{
				printf("no ihm_path given, using default %s\n",ihm_path);
				syslog(LOG_INFO,"no ihm_path given, using default %s\n",ihm_path);
			}else{
				strncpy(ihm_path,argv[z],sizeof(ihm_path) );
				printf("ihm_path given: %s\n",ihm_path);
				syslog(LOG_INFO,"ihm_path given: %s\n",ihm_path);
			}			
			z++;
		}
		else if (   0== strncmp(argv[z], "-w", 2)    ) {

			if (++z == argc) 
			{
				printf("no work_path given, using default %s\n",work_path);
				syslog(LOG_INFO,"no work_path given, using default %s\n",work_path);
			}else{
				strncpy(work_path,argv[z],sizeof(work_path));
				printf("work_path given: %s\n",work_path);
				syslog(LOG_INFO,"work_path given: %s\n",work_path);
			}			
			z++;
		}
		else if (!strcasecmp(argv[z], "-u")) {
			g_synchronize_remote_config = 1;
			printf("update with ihm share directory\n");
			syslog(LOG_INFO,"update with ihm share directory");
			z++;
		}
		else if (!strcasecmp(argv[z], "-m")) {
			g_remote_tcp_monitoring = 1;
			printf("remote monitoring enabled\n");
			syslog(LOG_INFO,"remote monitoring enabled");
			z++;
		}/*
		else if (0==strcasecmp(argv[z], "-notcp")) {
			is_tcp = false;
			printf("no tcp connections\n");
			syslog(LOG_INFO,"no tcp connections");
			z++;
		}
		else if (0==strcasecmp(argv[z], "-debug")) {
			is_debug = true;
			printf("debug version\n");
			syslog(LOG_INFO,"debug version");
			z++;
		}*/		
		else if (!strncmp(argv[z], "-h", 2)) {
		}
		else if (!strncmp(argv[z], "/h", 2)) {
		}
		else if (!strncmp(argv[z], "/?", 2)) {
		}
		else 
		{
			printf("unhandled option number %d:%s\n", z, argv[z]);
			more = 0;
		}
		
		if (z == argc) {
			more = 0;
		}
	}
	/*int first;
	int nbfiles;
	first = fetch_args(argc, argv);
	nbfiles = argc - first;
	for (z = 0; z < nbfiles; z++) {
	}
	*/
	return z;
}


int main(int argc, char **argv)
{
	time(&demarrage_system);	
	//signal (SIGPIPE, SIG_IGN);	// ignore le signal indiquant qu'une fifo a ete coupee
	signal(SIGPIPE, sigpipe);
	signal(SIGKILL, sigterm);
	signal(SIGINT , sigterm);
	signal(SIGQUIT, sigterm);
	signal(SIGTERM, sigterm);

	printf("============================================================\n");
	printf("starting %s with %d args %s %s\n", argv[0], argc, __DATE__, __TIME__ );
	printf("============================================================\n");
	vega_log(INFO, "starting %s with %d args %s %s designed by Debreuil Systems www.debreuil.fr\n", argv[0], argc, __DATE__, __TIME__ );
	openlog ("vega", LOG_PID, LOG_LOCAL0);
	syslog(LOG_INFO,"starting %s with %d args %s %s designed by Debreuil Systems www.debreuil.fr\n", argv[0], argc, __DATE__, __TIME__ );

	fetch_args(argc, argv);

	system("mkdir -p /vega/etc/");
	system("mkdir -p /vega/log/");
	//system("mkdir -p /vega/fifo/");

	//vega_event_log(EV_LOG_CONFIGURATION_RELOADED, 0 ,0,0, "starting %s version:%s %s (pid %d,work:%s;ihm:%s)", 
	//	argv[0], __DATE__, __TIME__ , getpid(),work_path,ihm_path);

	char var_run_pid_fname[128]={0};
	sprintf(var_run_pid_fname,"/var/run/vega.pid");
	FILE* pid_fd = fopen(var_run_pid_fname,"w");
	if ( NULL!=pid_fd ) 
	{
		char str[128]={0};
		snprintf(str,sizeof(str),"%d\n",getpid());
		fprintf(pid_fd, "%d\n",getpid());
		fclose(pid_fd);
		syslog(LOG_INFO,"pid %d written, kill process with [kill -SIGTERM `cat %s`]\n",getpid(),var_run_pid_fname);				
		printf("pid %d written, kill process with [kill -SIGTERM `cat %s`]\n",getpid(),var_run_pid_fname);				
	}else{
		fprintf(stderr,"cannot write %s\n", var_run_pid_fname);
	}

	//vega_event_log(EV_LOG_VEGA_CONTROL_STARTED, getpid() ,0,0, work_path);
	//vega_event_log(EV_LOG_VEGA_CONTROL_STARTED, getpid() ,0,0, ihm_path);

	printf("ihm_path=%s\n",ihm_path);
	printf("work_path=%s\n",work_path);
	if ( chdir(work_path)<0 ) {
		syslog(LOG_INFO,"cannot chdir %s",work_path);
		chdir("/tmp");
	}else{
		syslog(LOG_INFO,"chdir %s",work_path);
		printf("chdir %s\n",work_path);
	}

	event_t::init();


	//init_switch();

	// tentative de copie des fichiers .conf sur le repertoire distant en 2 secondes
	/*if (g_synchronize_remote_config)
	{
		static pthread_t thread_id;
		int ret = pthread_create(&(thread_id), NULL, copy_gui_config_files_thread, NULL);
		if (ret < 0) {
			vega_log(INFO,"error try_load_GUI_devices_thread - cause: %s", strerror(errno));
			// load local config files !!!
		}else{
			sleep(2); // todo : handle this timeout in the thread
			pthread_cancel(thread_id);
		}
	}*/

	// chargement des 4 fichiers de conf dans /vega/etc
	/*{
		char fname_local[1024]={0};
		snprintf(fname_local, sizeof(fname_local)  - 1,"%s/%s", work_path, DEVICE_KEYS_CONF) ;
		CRM4::load_crm4_keys_configuration(fname_local);

		snprintf(fname_local, sizeof(fname_local)  - 1,"%s/%s", work_path, DEVICES_CONF) ;
		reload_devices_add(fname_local);

		snprintf(fname_local, sizeof(fname_local)  - 1,"%s/%s", work_path, CONFERENCES_CONF) ;
		reload_conferences_add(fname_local);

		snprintf(fname_local, sizeof(fname_local)  - 1,"%s/%s", work_path, GROUPS_CONF) ;
		reload_groups_add(fname_local);

	}*/


	///////////////// chargement des alarmes en cours
	/*alarms = vega_control_alarms_create();
	alarms_load_current_alarms(alarms);
	alarms_dump(stdout, alarms->alarms);*/

#define TEST
#ifdef TEST
	if (1){
		pthread_t tid;
		int ret = pthread_create(&(tid), NULL, listening_ihm_tcp_thread, NULL);
		if (ret < 0) {vega_log(INFO,"error pthread_create listening_ihm_tcp_thread - cause: %s", strerror(errno));}
	}
#else
#endif

#ifdef USE_VEGA_LINK

	if ( NULL==pAlphaComLink ) {
		//enable_radio_dry_contact();
		printf("start link TCP with VEGA...\n");
		//vega_alarm_log(EV_ALARM_CNX_ALPHACOM_CONTROL_UP, 0,0,"connexion pc control<->ALPHACOM version "__DATE__" "__TIME__" alphacom en cours...");
		pAlphaComLink = vega_control_create();
		vega_control_init(pAlphaComLink, AlphaComIPaddress, AlphaComTCPPort ) ;//"169.254.1.5", 50010); /*  */
		// todo: faire comme si toutes les cartes etaient en alarme au demarrage
		/*int number = 1;
		vega_alarm_log(EV_ALARM_DEVICE_ANOMALY,  0 , number, "debut alarme station %d", number); 
		int board_number = 1;
		vega_alarm_log(EV_ALARM_BOARD_ANOMALY, board_number, 0, "Anomalie sur la carte %d", board_number); */
	}
#endif

	// lecture de VEGACTRL.CONF
	{
		char fname_local[1024]={0};
		snprintf(fname_local, sizeof(fname_local)  - 1,"%s/%s", work_path, VEGACTRL_CONF) ;
		// todo: recuperer le nom de la configuration dans la section general
		{
			rude::Config config;
			if (config.load(fname_local) == false) {
				printf("rude cannot load %s file", fname_local);
			}else{
				QString strconfigname;
				if ( true == config.setSection("general",false) ) 
				{
					strconfigname = config.getStringValue("name");

				}
				if ( strconfigname.length() > 1 ) {
					vega_event_log(EV_LOG_CONFIGURATION_RELOADED, 0,0,0, "chargement %s", qPrintable(strconfigname) );
					//vega_alarm_log(EV_ALARM_CONFIGURATION_RELOADED, 0,0,"chargement %s", qPrintable(strconfigname) );
				}else{
					vega_event_log(EV_LOG_CONFIGURATION_RELOADED, 0,0,0, "chargement %s", fname_local);
					//vega_alarm_log(EV_ALARM_CONFIGURATION_RELOADED, 0,0,"chargement %s", fname_local);
				}
			}
		}
		// lecture section [keys] pour les touches des crm4
		CRM4::load_crm4_keys_section(fname_local);
		// lecture sections [D1] a D46 (gain , name, detection audio....)
		CRM4::load_crm4_device_section(fname_local);
		// lecture section [C1] a C8 puis ajout des directeur, radio1, devices...
		vega_conference_t::load_conference_section(fname_local);
		load_groupes_section(fname_local);
	}
	// ECOUTE DES DEMANDES TCP DE IHM
	if (1){
		pthread_t tid;
		int ret = pthread_create(&(tid), NULL, listening_ihm_tcp_thread, NULL);
		if (ret < 0) {vega_log(INFO,"error pthread_create listening_ihm_tcp_thread - cause: %s", strerror(errno));}
	}
	
	if (g_remote_tcp_monitoring){
		pthread_t tid;
		int ret = pthread_create(&(tid), NULL, listening_monitor_rx_thread, NULL);
		if (ret < 0) {vega_log(INFO,"error pthread_create listening_monitor_rx_thread - cause: %s", strerror(errno));}		
		ret = pthread_create(&(tid), NULL, listening_monitor_tx_thread, NULL);
		if (ret < 0) {vega_log(INFO,"error pthread_create listening_monitor_tx_thread - cause: %s", strerror(errno));}		
	}

	install_options();

	// polling du repertoire ihm avant meme que le link soit monte !
	//

	while (!b_exit_loop)
    {
		WORD c;
		//fprintf(stderr, "Enter your selection:\n");
		//while (!kbhit (1));
		//printf("press 'l' to start TCP link (partie materielle)...\n");
		//printf("press 'L' to load conferences and devices from ini file...\n");
		//printf("press a key...('Q' to quit)\n");//press 'L' to load conferences and devices from ini file...\n");
		
		while (!dos_kbhit ());

		c = getc (stdin);
		printf ("c=%02X\n", c);
		if (c >= 0x20 && c < 0x7f)	{		}
		switch (c)
		{
		case '\n':
			{
				//printf("\n");
			}
			break;

		case '!':
			critical_dump();
			break;

		/*case 'r':
			{
				printf("reload_conferences_remove...\n");
				char fname_remote[1024]={0};
				if( find_config_filename_in_directory(ihm_path , CONFERENCES_CONF, fname_remote, sizeof(fname_remote)  - 1) )
				{
					reload_conferences_remove(fname_remote);
				}
			}
			break;*/
		case 'y':
			{
				printf("set dry contact bit number (0..15)");
				
				char str_no_bit[16]={0};
				while (1)
				{//!dos_kbhit ());
					c = getc (stdin);
					printf("%c",c);
					//printf ("accumule c=%02X\n", c);
					str_no_bit[strlen(str_no_bit)] = c;
					
					if ( 0x1B == c ) { 
						printf("\n"); break; 
					}
					else if ( 0x0D == c || 0x0A == c )	
					{
						int number_of_bit = atoi(str_no_bit);
						printf("number_of_bit:%d\n",number_of_bit);
/*[mon@vega local]$ cat /proc/ixpci/ixpci
maj: 253
mod: ixpcip16x16 ixpcip8r8
dev: ixpci1 0 0xf8402000 0xd300 0xd400 0x0 0x0 0x0 0x12341616c1a20823 PCI-P16C16/P16R16/P16POR16
dev: ixpci2 0 0xf8401000 0xd100 0xd200 0x0 0x0 0x0 0x12340808c1a20823 PCI-P8R8
*/
						// this is the case of the P16R16
						unsigned short dout = 0;
						dout = 1 << number_of_bit; 
						//dout = 0x0FFFFFFFF;
						
						//char dev_relay[]="/dev/ixpci2";

						/*
						int fd_dev_P16R16 = open( dev_relay, O_RDWR);

						if (fd_dev_P16R16 < 0) {
							fprintf(stderr,"Cannot open device file \"%s.\"\n", "/dev/ixpci1");
						}

						printf("write: %x in device %s\n",dout,dev_relay);
						ioctl(fd_dev_P16R16, IXPCI_IOCTL_DO, &dout);

						sleep(2);

						printf("RAZ\n");
						dout=0;
						ioctl(fd_dev_P16R16, IXPCI_IOCTL_DO, &dout);

						close(fd_dev_P16R16);

						*/
						break;
					}
				}

			}// case 'r'
			break;
		/*case 'a':
			{
				char str_no_device[128]={0};
				while (1){//!dos_kbhit ());
					c = getc (stdin);
					printf("%c",c);
					//printf ("accumule c=%02X\n", c);
					str_no_device[strlen(str_no_device)] = c;
					if ( 0x1B == c ) { printf("\n"); break; }
					else if ( 0x0D == c || 0x0A == c )	
					{
						int i=0;
						printf("str_no_device=%s\n",str_no_device);
						int len = g_list_length(devices);
						for (i = 0; i < len ; i++) 
						{
							device_t* d = (device_t *)g_list_nth_data(devices, i);
							printf("\n** device number %d current_conference %d type %d **\n",
								d->number, d->type, d->current_conference ) ;
							if ( atoi(str_no_device) == d->number ) 
							{
								//device_display_reset_all_devices_led(d);
								//device_display_conferences_state(d);
								break;

								for (i = 11; i  < 48 ; i++)  // saute les boutons des 10 conferences
								{
									int key = i;//get_key_number_by_action_type(d1->device.crm4.keys_configuration, ACTION_SELECT_DEVICE, i);

									//vega_log(INFO, "get_key_number_by_action ACTION_SELECT_DEVICE = %d", key);
									//if (key < 0)      continue;
									device_set_no_red_color(d, key);
									device_set_no_green_color(d, key);    
								}
							}
						}
						break;
					}
				}
			}
			break;

		case 'G':
			{
				all_devices_display_general_call_led(0);
				printf("START APPEL GENERAL\nentrez le numero du directeur d'une des conferences: ");
				{
					char str_no_device[128]={0};
					while (1){//!dos_kbhit ());
						c = getc (stdin);
						printf("%c",c);
						//printf ("accumule c=%02X\n", c);
						str_no_device[strlen(str_no_device)] = c;
						if ( 0x1B == c ) { printf("\n"); break; }
						else if ( 0x0D == c || 0x0A == c )	
						{
							int director = atoi(str_no_device);
							printf("look for device director number %d...\n",director );
							device_t *dd = devices_t::by_number( director);
							if ( dd ) {
								vega_conference_t* conf_of_dd = vega_conference_t::by_director(dd);
								printf("START APPEL GENERAL depuis %d ( director of %d)\n",dd->number, conf_of_dd->number);
								vega_appel_general_start( dd , conf_of_dd );
							}
							break;
						}
					}
				}
				break;
			}
			break;
		case 'g':
			{
				all_devices_display_general_call_led(1);
				printf("STOP APPEL GENERAL\nentrez le numero du directeur: ");
				{
					char str_no_device[128]={0};
					while (1){//!dos_kbhit ());
						c = getc (stdin);
						printf("%c",c);
						//printf ("accumule c=%02X\n", c);
						str_no_device[strlen(str_no_device)] = c;
						if ( 0x1B == c ) { printf("\n"); break; }
						else if ( 0x0D == c || 0x0A == c )	
						{
							int director = atoi(str_no_device);
							printf("look for device director number %d...\n",director );
							device_t *dd = devices_t::by_number( director);
							if ( dd ) {
								vega_conference_t* conf_of_dd = vega_conference_t::by_director(dd);
								printf("STOP APPEL GENERAL depuis %d ( director of %d)\n",dd->number, conf_of_dd->number);
								vega_appel_general_stop( dd , conf_of_dd );
							}
							break;
						}
					}
				}
				break;
			}
			break;*/

		case 'g':
			{
				dump_group(stdout);
			}
			break;

		case 's':
			{
				if ( NULL==pAlphaComLink ) {
					printf("starting VEGA TCP link...\n");
					//enable_radio_dry_contact();
					printf("start link TCP...\n");
					pAlphaComLink = vega_control_create();
					vega_control_init(pAlphaComLink, "169.254.1.5", 50010); /*  */
				}else{
					printf("already started VEGA TCP link...\n");
				}
			}
			break;

		case 'c':
			{
				//int len = g_list_length(conferences);
				//printf("dump %d conferences:\n", len);
				dump_conferences(stdout);
				//init_conferences_leds();				
			}
			break;

		case 'l':
			{
				printf("loading devices...\n");
			}
			break;

		case 'w':
			{
				/*printf("copy_remote_config_file...\n");
				copy_remote_config_file( work_path, ihm_path, DEVICES_CONFIG_FNAME ) ;
				copy_remote_config_file( work_path, ihm_path, CONFERENCES_CONFIG_FNAME ) ;
				copy_remote_config_file( work_path, ihm_path, GROUPS_CONFIG_FNAME ) ;
				copy_remote_config_file( work_path, ihm_path, BUTTON_CONFIG_FILENAME ) ;*/
			}
			break;


		case 'L':
			{
				//printf("loading conferences...\n");
				//load_devices();
				//char c = getc (stdin);

				//load_conferences();
				
				//load_groups();

				//init_conferences_leds();
				
				if (0){
					printf("Initialize CONFERENCE (cabling 9 AGA) number: ");
					char str[128]={0};
					while (1){//!dos_kbhit ());
						c = getc (stdin);
						printf("%c",c);
						//printf ("accumule c=%02X\n", c);
						str[strlen(str)] = c;
						if ( 0x1B == c ) { printf("\n"); break; }
						else if ( 0x0D == c || 0x0A == c )	
						{
							int N = atoi(str);
							vega_log(INFO,"cabling conference %d...\n",N );

							//vega_conference_t *conf = vega_conference_t::by_number(N);

							//if ( conf ) vega_conference_init_mixers(conf);

							break;
						}
					}
				}
			}
			break;

		case 'C':
			{
				printf("ACTIVATION CONFERENCE entrez le numero : ");
				{
					char str_no_device[128]={0};
					while (1){//!dos_kbhit ());
						c = getc (stdin);
						printf("%c",c);
						//printf ("accumule c=%02X\n", c);
						str_no_device[strlen(str_no_device)] = c;
						if ( 0x1B == c ) { printf("\n"); break; }
						else if ( 0x0D == c || 0x0A == c )	
						{
							int N = atoi(str_no_device);
							printf("activating conference %d...\n",N );

							vega_conference_t *conf = vega_conference_t::by_number(N);
		  
							if (conf != NULL && !conf->active ) // est elle deja active ?
							{	// non
							
								conf->active = 1;
								
								//conf->device_initiator = conf->participant_director.device;
								
								printf( "CONSOLE ACTIVATED conference %d ( INITIATOR)", conf->number);

								// je traite l'event ci dessous : EVT_CONFERENCE_ACTIVATED
								EVENT ev;
								ev.code = EVT_CONFERENCE_ACTIVATED; // 
								ev.device_number = 0;//conf->device_director.number; // l'initiateur
								ev.conference_number = conf->number;
								BroadcastEvent(&ev);// envoi EVT_CONFERENCE_ACTIVATED a tous les devices
							}
							break;
						}
					}
				}
			}
			break;

		case 'v':
			printf("toggle verbose %d...\n",verbose);
			if ( 1==verbose ) verbose=0;	else verbose =1;
			printf("verbose=%d\n",verbose);
			break;

		case 'd':
			CRM4::dump_devices(stdout);
			Radio::dump_devices(stdout);
			//Jupiter::dump_devices(stdout);

			break;
		case 'm':
			{
				//printf("dump %d stations mixers:\n",g_list_length(devices));
				//int i;for (i = 0; i <  g_list_length(devices) ; i++) 
				foreach(device_t* d, device_t::qlist_devices)
				{
					//device_t* d = (device_t*)g_list_nth_data(devices, i);
					d->fprint_mixers(stdout);
				}
			}
			break;
		case 't':
			printf("dump_timeslots...\n");
			dump_timeslots(stdout,1);
			break;

		case 'x':
			{
				printf("DTOR EXCLUSION dans la CONFERENCE numero : ");
				{
					int numero_conference=0;
					int numero_device=0;

					{
						char c = getc (stdin);
						printf("%c\n",c);
						numero_conference = c -'0';//atoi(c);


						char str_no_device[128]={0};
						while (1){//!dos_kbhit ());
							c = getc (stdin);
							printf("%c",c);
							printf ("accumule c=%02X\n", c);
							str_no_device[strlen(str_no_device)] = c;
							if ( 0x1B == c ) { printf("\n"); break; }
							else if ( 0x0D == c || 0x0A == c )	
							{
								numero_device = atoi(str_no_device);
								printf("selected device No %d...\n",numero_device );
								break;
							}
						}


						vega_conference_t *conf = vega_conference_t::by_number(numero_conference);
							//device_t* D = devices_t::by_number(numero_device);
		  
							//if (conf != NULL && conf->active ) // est elle deja active ?
							{	// oui
								vega_log(VEGA_LOG_INFO, "CONSOLE EXCLUDING device %d in conference %d", numero_device, conf->number);


								// je traite l'event ci dessous : EVT_CONFERENCE_ACTIVATED
								EVENT ev;
								ev.code = EVT_CONFERENCE_DTOR_EXCLUDE; // 
								ev.device_number = numero_device;
								ev.conference_number = conf->number;
								BroadcastEvent(&ev);// envoi EVT_CONFERENCE_ACTIVATED a tous les devices
							}
							break;
						}
				}
				
			}
			break;

		case 'Z':
			{
				printf("RAZ\n");
				{
					char cmd[128]={0};
					sprintf(cmd,"rm *.log");
					int res = system(cmd);
					printf("result: %d cmd:%s\n", res, cmd);
				}
			}
			break;

		case 'X':
			{
				printf("DTOR INCLUSION dans la CONFERENCE numero : ");
				{
					int numero_conference=0,numero_device=0;
					{
						char c = getc (stdin);
						printf("%c\n",c);
						numero_conference = c -'0';//atoi(c);

						printf("No du device : ");
						c = getc (stdin);
						numero_device = c-'0';//atoi(c);

						printf("%d\n",numero_device);


							vega_conference_t *conf = vega_conference_t::by_number(numero_conference);
							//device_t* D = devices_t::by_number(numero_device);
		  
							//if (conf != NULL && conf->active ) // est elle deja active ?
							{	// oui
								vega_log(VEGA_LOG_INFO, "CONSOLE EXCLUDING device %d in conference %d", numero_device, conf->number);

								// je traite l'event ci dessous : EVT_CONFERENCE_ACTIVATED
								EVENT ev;
								ev.code = EVT_CONFERENCE_DTOR_INCLUDE; // 
								ev.device_number = numero_device; // l'initiateur
								ev.conference_number = conf->number;
								BroadcastEvent(&ev);// envoi EVT_CONFERENCE_ACTIVATED a tous les devices
							}
							break;
						}
				}
				
			}
			break;

		case '=':
			{

				printf("set_contact_P16R16 bit No : ");
				{
					int bit_number_of_radio=0;
					
						char c ;

						char str_no_device[128]={0};
						while (1){//!dos_kbhit ());
							c = getc (stdin);
							printf("%c",c);
							printf ("accumule c=%02X\n", c);
							str_no_device[strlen(str_no_device)] = c;
							if ( 0x1B == c ) { 
								printf("\n"); 
								//unsigned int dout = 0xFFFF;
								/*if (fd_dev_P16R16 > 0 ) {
									printf("write fd_dev_P16R16: %04X\n",dout);
									ioctl(fd_dev_P16R16, IXPCI_IOCTL_DO, &dout);
									vega_log(INFO, "write /dev/ixpci1 %04X\n",dout);			
									break;
								}*/

								break; 
							}
							else if ( 0x0D == c || 0x0A == c )	{
								bit_number_of_radio = atoi(str_no_device);
								break;
							}
						}

						printf("bit_number_of_radio=%d\n",bit_number_of_radio);
						int on_off=1;
						set_contact_P16R16(bit_number_of_radio, on_off);

						if ( bit_number_of_radio ==99 ) 
						set_contact_P16R16(bit_number_of_radio, on_off);
							//	
				}
			}
			break;

		case '+':
			{
				printf("ADD DEVICE IN CONFERENCE number ( must exist in conferences.conf) : ");
				{
					int numero_conference=0,numero_device=0;
					
						char c = getc (stdin);
						printf("%c\n",c);
						numero_conference = c -'0';//atoi(c);
						
						vega_conference_t *C = vega_conference_t::by_number(numero_conference);
						if ( C) C->dump(stdout);// display participants:

						printf("WHICH participant: ");
						char str_no_device[128]={0};
						while (1){//!dos_kbhit ());
							c = getc (stdin);
							printf("%c",c);
							//printf ("accumule c=%02X\n", c);
							str_no_device[strlen(str_no_device)] = c;
							if ( 0x1B == c ) { printf("\n"); break; }
							else if ( 0x0D == c || 0x0A == c )	{
								numero_device = atoi(str_no_device);
								break;
							}
						}

						printf("%d\n",numero_device);

						
						device_t* D = device_t::by_number(numero_device);
	  
						printf("CONSOLE ADDING device %02d in conference %d %p %p\n", numero_device, numero_conference, D, C);

						if (NULL == D){
							printf( "NO DEVICE %d", numero_device);
						}
						else if (NULL == C){
							printf( "NO CONFERENCE %d", numero_conference);
						}
						else
						{
								EVENT ev;
								ev.code = EVT_ADD_CRM4;
								ev.device_number = D->number;
								ev.conference_number = C->number;
								//D->ExecuteStateTransition(&ev);// send the news to the device state machine EVT_ADD_DEVICE
								BroadcastEvent(&ev);

						}
				}
				
			}
			break;
		case '-':
			{
				printf("DEL DEVICE IN CONFERENCE number ( must exist in conferences.conf) : ");
				{

					int numero_conference=0,numero_device=0;
					
						char c = getc (stdin);
						printf("%c\n",c);
						numero_conference = c -'0';//atoi(c);

						vega_conference_t *C = vega_conference_t::by_number(numero_conference);
						if ( C) C->dump(stdout);// display participants:
						
						printf("WHICH participant: ");
						char str_no_device[128]={0};
						while (1){//!dos_kbhit ());
							c = getc (stdin);
							printf("%c",c);
							//printf ("accumule c=%02X\n", c);
							str_no_device[strlen(str_no_device)] = c;
							if ( 0x1B == c ) { printf("\n"); break; }
							else if ( 0x0D == c || 0x0A == c )	{
								numero_device = atoi(str_no_device);
								break;
							}
						}

						//c = getc (stdin);
						//numero_device = c-'0';//atoi(c);

						printf("%d\n",numero_device);

						device_t* D = device_t::by_number(numero_device);
	  

						if (NULL == D){
							printf( "NO DEVICE %d", numero_device);
						}
						else if (NULL == C){
							printf( "NO CONFERENCE %d", numero_conference);
						}
						else
						{

							printf("CONSOLE ADDING device %02d in conference %d %p %p\n", numero_device, numero_conference, D, C);

						
							EVENT ev;
							ev.code = EVT_REMOVE_DEVICE;
							ev.device_number = D->number;
							ev.conference_number = C->number;
							D->ExecuteStateTransition(&ev);// send the news to the device state machine EVT_ADD_DEVICE

							
						}
				}
				
			}
			break;

		/*case 27: // ESCAPE ignored
			printf("user want to exit application (%02x)\n", c);
			break;*/
		case 'Q':
		//case 'q':
		  b_exit_loop = 1;
		  break;
		}
	}
  
	restaure_options();
	vega_log(INFO,"%s ended normally\n",argv[0]);
  //while (1) sleep (1);
	return 0;
}
/*Address                  HWtype  HWaddress           Flags Mask            Iface
10.5.104.100             ether   00:05:1a:cb:ec:80   C                     eth0
10.5.104.202             ether   00:18:7d:02:ba:58   C                     eth0
10.5.104.5               ether   00:13:72:52:0a:a4   C                     eth0
169.254.1.5              ether   00:13:cb:00:34:c4   C                     eth0
169.254.1.2              ether   00:18:7d:02:ba:58   C                     eth0
10.5.104.203             ether   00:14:22:a7:d0:cc   C                     eth0
V*/

