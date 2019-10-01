/* 
project: PC control Vega Zenitel 2009
filename: radiostate.c
author: Mustafa Ozveren & Debreuil Philippe
last modif: 20090307
desciption: state machine of radio devices
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
#include <sys/ioctl.h>
#include "ixpci.h"
#include "debug.h"
#include "conf.h"
#include "misc.h"
#include "mixer.h"

//#define P8R8_RX_HEX_FILE "/tmp/rx-P8R8-vega.hex"
//#define P16R16_RX_HEX_FILE "/tmp/rx-P16R16-vega.hex"
//static int rx_P8R8_total=0;
//static int rx_P16R16_total=0;

static int fd_dev_P16R16 = 0;//open("/dev/ixpci1", O_RDWR);
static int fd_dev_P8R8 = 0;//open("/dev/ixpci2", O_RDWR);

void Radio::dump_devices(FILE* fout)
{
	if ( NULL == fout ) return;
	//int i=0;
	//int len = g_list_length(devices);
	//fprintf(fout,"total: %d stations\n", len);
	foreach(Radio* ddd,qlist_radio)//for (i = 0; i < len ; i++) 
	{
		//device_t* dd = (device_t*)(g_list_nth_data(devices, i) );
		//Radio* ddd = dynamic_cast<Radio*>( dd );
		//printf("** device[%d] = %p",i, d);
		if ( ddd==NULL ) {
		}else{
			fprintf(fout,"* D%02d %12.12s C%d type%s %12.12s ",ddd->number, ddd->name, ddd->current_conference , ddd->type_string(), ddd->state_string()) ;
			fprintf(fout,"[ radio AGA in AGA out FULLDUPLEX:%d DETECTION AUDIO:%d ]", ddd->full_duplex, ddd->audio_detection); 
			fprintf(fout,"\n");
		}
	}
	
}

int Radio::start_detection_audio()
{	
	static int running=0;
	if ( 0 == running ){
		static pthread_t thread_id;
		int ret = pthread_create(&(thread_id), NULL, radio_detection_audio_thread, NULL);
		if (ret < 0) {
			vega_log(INFO,"error radio_contact_thread create - cause: %s", strerror(errno));
		}else{
			running=1;
		}
	}
	return 0;
}
bool Radio::radio_start_speaking_conf(vega_conference_t* C)
{
	if ( this->has_hardware_error() ) return false;
	if ( C->has_hardware_error() ) return false; // pdl 20091005
	int ts_value = m_timeslot_micro;
	if (1)//d->audio_detection == 0)
	{
		if (number == C->radio1_device()) 
		{
		  // connecte time slot emission RADIO 1 vers 3 ports AGA (0,3,4) de la conferences
		  //int ts_value = FIRST_DEVICE_RADIO_INPUT_TS /*121*/ + number - FIRST_RADIO_DEVICE_NUMBER /*25*/; /* shift les postes radio commencent a  25 ! */

		  vega_log(INFO,"R%d start_speaking in C%d as radio1 TS %d",number,C->number, ts_value);// => uniquement vert, sinon rouge et vert");
		  
		  mixer_connect_timeslot_to_sub_output(&C->ports[0], sub_channel_0, ts_value, 0);
		  mixer_connect_timeslot_to_sub_output(&C->ports[3], sub_channel_1, ts_value, 0);
		  mixer_connect_timeslot_to_sub_output(&C->ports[4], sub_channel_1, ts_value, 0);

		}
		else if (number == C->radio2_device()) 
		{
		  // connecte time slot emission RADIO 2 vers 3 ports AGA (0,2,4) de la conferences
		  //int ts_value = FIRST_DEVICE_RADIO_INPUT_TS + number - FIRST_RADIO_DEVICE_NUMBER; /* shift les postes radio commencent a  25 ! */

		  vega_log(INFO,"R%d start_speaking in C%d as radio2 TS %d",number,C->number, ts_value);// => uniquement vert, sinon rouge et vert");

		  mixer_connect_timeslot_to_sub_output(&C->ports[0], sub_channel_1, ts_value, 0); // AGA mix 1,2,3
		  mixer_connect_timeslot_to_sub_output(&C->ports[2], sub_channel_1, ts_value, 0); // AGA Radio1
		  mixer_connect_timeslot_to_sub_output(&C->ports[4], sub_channel_3, ts_value, 0); // AGA Jup2
		}
		else if (number == C->jupiter2_device()) 
		{
		  // connecte time slot emission JUPITER 2 vers 3 ports AGA (0,2,3) de la conferences
		  //int ts_value = FIRST_DEVICE_RADIO_INPUT_TS + number - FIRST_RADIO_DEVICE_NUMBER; /* shift les postes radio commencent a  25 ! */

		  vega_log(INFO,"R%d start_speaking in C%d as jupiter TS %d",number,C->number, ts_value);

		  mixer_connect_timeslot_to_sub_output(&C->ports[0], sub_channel_3, ts_value, 0); // AGA mix 1,2,3
		  mixer_connect_timeslot_to_sub_output(&C->ports[2], sub_channel_3, ts_value, 0); // AGA Radio1
		  mixer_connect_timeslot_to_sub_output(&C->ports[3], sub_channel_3, ts_value, 0); // AGA Radio2
		}else{
			vega_log(INFO,"ERREUR GRAVE radio_start_speaking_conf C%d D%d",C->number, number);
			printf("ERREUR GRAVE radio_start_speaking_conf C%d D%d\n",C->number, number);
			//exit(-1);
		}
	}
	return true;
}

bool Radio::radio_start_hearing_conf(vega_conference_t* C)
{
	if ( has_hardware_error() ) return false;
	if ( C->has_hardware_error() ) return false; // pdl 20091005
	////////// CONNECT Cf+T
	if (full_duplex == 1) 
	{
		vega_log(INFO,"radio_start_hearing_conf FULLDUPLEX RADIO D%d in C%d TS %d",number,C->number, m_timeslot_micro);// => uniquement vert, sinon rouge et vert");
		/* connecte le Cf+T du sous channel 1 vers la sortie mix conf*/
		/* test pour envoyer un tone busy sur les radios */
		/*     mixer_connect_timeslot_to_sub_output(&port_aga_reception, sub_channel_0,6/\* conf->matrix_configuration->ts_start + 8 *\/, 0);  */
		/*     mixer_connect_timeslot_to_sub_output(&port_aga_reception, sub_channel_1,37/\* conf->matrix_configuration->ts_start + 8 *\/, 0); */
		mixer_connect_timeslot_to_sub_output(&port_aga_reception, 
					sub_channel_1,
					C->matrix_configuration->ts_start + 8 ,
					0);
	}else {
		if (number == C->radio1_device()) 
		{
			vega_log(INFO,"radio_start_hearing_conf HALFDUPLEX RADIO D%d in C%d TS %d",number,C->number, m_timeslot_micro);// => uniquement vert, sinon rouge et vert");
			mixer_connect_timeslot_to_sub_output(&port_aga_reception, 
						sub_channel_1,
						C->matrix_configuration->ts_start + 2 ,
						0);
		}
		else if (number == C->radio2_device()) 
		{
			vega_log(INFO,"radio_start_hearing_conf RADIO HALFDUPLEX D%d in C%d TS %d",number,C->number, m_timeslot_micro);// => uniquement vert, sinon rouge et vert");
			mixer_connect_timeslot_to_sub_output(&port_aga_reception, 
						sub_channel_1,
						C->matrix_configuration->ts_start + 3 ,
						0);
		}
		else if (number == C->jupiter2_device()) 
		{
			vega_log(VEGA_LOG_ERROR,"ERREUR radio_start_hearing_conf JUPITER HALFDUPLEX !!!! D%d in C%d TS %d",number,C->number, m_timeslot_micro);// => uniquement vert, sinon rouge et vert");
		}else{
			vega_log(VEGA_LOG_ERROR,"radio_start_hearing_conf BAD NUMBER RADIO HALFDUPLEX D%d in 1st C%d",number,C->number);// => uniquement vert, sinon rouge et vert");
		}
	}
	vega_log(INFO,"radio_start_hearing_conf R%d C%d TS micro:%d",number,C->number,m_timeslot_micro);// => uniquement vert, sinon rouge et vert");
	return true;
}

void Radio::radio_stop_hearing(vega_conference_t* c)
{
	if ( has_hardware_error() ) return;
	if ( c->has_hardware_error() ) return; // pdl 20091005
	vega_log(INFO, "radio_stop_hearing port_aga_reception sub_channel_1");
	mixer_disconnect_timeslot_to_sub_output(&port_aga_reception, sub_channel_1 );
	return ;
}


void Radio::radio_stop_speaking(vega_conference_t *c)
{
	if ( has_hardware_error() ) return;
	if ( c->has_hardware_error() ) return; // pdl 20091005
	//int ts_value = m_timeslot_micro;//FIRST_DEVICE_RADIO_INPUT_TS /*121*/ + number - FIRST_RADIO_DEVICE_NUMBER /*25*/; /* shift les postes radio commencent a  25 ! */
	
	if ( this==c->participant_radio1.device )
	{	
		// deconnecte time slot emission RADIO 1 vers 3 ports AGA (0,3,4) de la conferences
		vega_log(INFO,"radio_stop_speaking as radio 1 D%d (with AD) in C%d TS %d",number,c->number, m_timeslot_micro);// => uniquement vert, sinon rouge et vert");
		mixer_disconnect_timeslot_to_sub_output(&c->ports[0], sub_channel_0);
		mixer_disconnect_timeslot_to_sub_output(&c->ports[3], sub_channel_1);
		mixer_disconnect_timeslot_to_sub_output(&c->ports[4], sub_channel_1);
	}
	else if (this==c->participant_radio2.device) 
	{
		// deconnecte time slot emission RADIO 2 vers 3 ports AGA (0,2,4) de la conferences
		vega_log(INFO,"radio_stop_speaking as radio 2 D%d (with AD) in C%d TS %d",number,c->number, m_timeslot_micro);// => uniquement vert, sinon rouge et vert");
		mixer_disconnect_timeslot_to_sub_output(&c->ports[0], sub_channel_1); // AGA mix 1,2,3
		mixer_disconnect_timeslot_to_sub_output(&c->ports[2], sub_channel_1); // AGA Radio1
		mixer_disconnect_timeslot_to_sub_output(&c->ports[4], sub_channel_3); // AGA Jup2
	}
	else if (this==c->participant_jupiter.device) 
	{
		// deconnecte time slot emission jupiter vers 3 ports AGA (0,2,3) de la conferences
		vega_log(INFO,"radio_stop_speaking as jupiter D%d (with AD) in C%d TS %d",number,c->number, m_timeslot_micro);// => uniquement vert, sinon rouge et vert");
		mixer_disconnect_timeslot_to_sub_output(&c->ports[0], sub_channel_3); // AGA mix 1,2,3
		mixer_disconnect_timeslot_to_sub_output(&c->ports[2], sub_channel_3); // AGA Radio1
		mixer_disconnect_timeslot_to_sub_output(&c->ports[3], sub_channel_3); // AGA Radio2
	}else{
		printf("Radio::radio_stop_speaking ERROR\n");
		//exit(-1);
	}
}


/*cat /proc/ixpci/ixpci
maj: 253
mod: ixpcip16x16 ixpcip8r8
dev: ixpci1 0 0xf8402000 0xd300 0xd400 0x0 0x0 0x0 0x12341616c1a20823 PCI-P16C16/P16R16/P16POR16
dev: ixpci2 0 0xf8401000 0xd100 0xd200 0x0 0x0 0x0 0x12340808c1a20823 PCI-P8R8*/

int set_contact_P16R16(int number_of_bit /* 0 to 15*/, int value /* 1: contact on: 0: contact off*/ )
{
	static unsigned short dout = 0;

	{
		//printf("set_contact_P16R16: number_of_bit:%d\n",number_of_bit);
		vega_log(INFO,	"set_contact_P16R16: number_of_bit:%d\n",number_of_bit);

		// this is the case of the P16R16
		//printf("-->dout: %04X\n",dout);
		//vega_log(INFO,"-->dout: %04X\n",dout);
		if ( value == 0 ){
			unsigned short mask = (1 << number_of_bit) ;
			//printf("-->mask: %04X\n",mask);
			dout = dout & ~mask  ;

			//dout = 0x00000; // todo: remove this big bug 
		}else{
			dout = dout | (1 << number_of_bit);
			
			//dout = 0x0FFFF; // todo: remove this big bug 
		}
		//printf("<--dout: %04X\n",dout);
		//vega_log(INFO,"<--dout: %04X\n",dout);


		//int fd_dev_P16R16 = open("/dev/ixpci1", O_RDWR);
		if (fd_dev_P16R16 > 0 ) {
			printf("write fd_dev_P16R16: %04X\n",dout);
			ioctl(fd_dev_P16R16, IXPCI_IOCTL_DO, &dout);
			vega_log(INFO, "write /dev/ixpci1 %04X\n",dout);			
			//sleep(3);
			//printf("RAZ\n");
			//dout=0;
			//ioctl(fd_dev_P16R16, IXPCI_IOCTL_DO, &dout);
		}
	}
	return 0;
}

int set_contact_P8R8(int number_of_bit /* 0 to 5 */, int value /* 1: contact on: 0: contact off*/ )
{
	static unsigned short dout = 0;
	{

		vega_log(INFO,"set_contact_P8R8 number_of_bit:%d\n",number_of_bit);

		// this is the case of the P16R16
		//vega_log(INFO,"-->dout: %04X\n",dout);
		if ( value == 0 ){
			unsigned short mask = (1 << number_of_bit) ;
			//printf("-->mask: %04X\n",mask);
			dout = dout & ~mask  ;
		}else{
			dout = dout | (1 << number_of_bit);
		}
		//vega_log(INFO,"<--dout: %04X\n",dout);


		//int fd_dev_P8R8= open("/dev/ixpci2", O_RDWR);
		if (fd_dev_P8R8 > 0 ) {
			//printf("write: %04X\n",dout);
			vega_log(INFO, "write /dev/ixpci2 %04X\n",dout);			
			ioctl(fd_dev_P8R8, IXPCI_IOCTL_DO, &dout);
		}
	
	}
	return 0;
}

int set_radio_dry_contact_on_off(int device_number , int on_off) 
{	// passage de la base radio en emission
	switch (device_number)
	{
	case 25: set_contact_P16R16(0, on_off); break;// VHF1 = D25 test avec dio: write 0x0001 on /dev/ixpci1
	case 26: set_contact_P16R16(1, on_off); break;// VHF2 = D26
	case 27: set_contact_P16R16(2, on_off); break;// VHF3 = D27
	case 28: set_contact_P16R16(3, on_off); break;// VHF4 = D28 test avec dio: write 0x0008 on /dev/ixpci1 (bit 3)
	
	case 29: set_contact_P16R16(8, on_off); break;// VHF5 = D29  test avec dio: write 0x0100 on /dev/ixpci1 ( bit9 )
	case 30: set_contact_P16R16(9, on_off); break;// VHF6 = D30
	case 31: set_contact_P16R16(10, on_off); break;// VHF7 = D31
	case 32: set_contact_P16R16(11, on_off); break;// VHF8 = D32  test avec dio: write 0x0800 on /dev/ixpci1
	
	case 33: set_contact_P8R8(0, on_off); break;// VHF9 = D33 test avec dio: write 0x01 on /dev/ixpci2
	case 34: set_contact_P8R8(1, on_off); break;// VHF10 = D34
	case 35: set_contact_P8R8(2, on_off); break;// VHF11 = D35 test avec dio: write 0x04 on /dev/ixpci2
	default: 
		vega_log(INFO, "set_radio_dry_contact_on_off: unhandled device_number D%d",device_number);
		break;
	}
	return 0;
}


int set_recorder_dry_contact_on_off(int conf_number , int on_off) 
{	// passage de la base radio en emission
	switch (conf_number)
	{
	case 1: set_contact_P16R16(4, on_off); break;// C1 test avec dio: write 0x0010 on /dev/ixpci1
	case 2: set_contact_P16R16(5, on_off); break;// 
	case 3: set_contact_P16R16(6, on_off); break;// 
	case 4: set_contact_P16R16(7, on_off); break;// C4 test avec dio: write 0x0080 on /dev/ixpci1 (bit 3)
	
	case 5: set_contact_P16R16(12, on_off); break;// C5 test avec dio: write 0x1000 on /dev/ixpci1
	case 6: set_contact_P16R16(13, on_off); break;// C6 test avec dio: write 0x2000 on /dev/ixpci1
	case 7: set_contact_P16R16(14, on_off); break;//C7 test avec dio: write 0x4000 on /dev/ixpci1
	case 8: set_contact_P16R16(15, on_off); break;// C8 test avec dio: write 0x8000 on /dev/ixpci1
	
	case 9: set_contact_P8R8(3, on_off); break;// 
	case 10: set_contact_P8R8(4, on_off); break;// 
	default: 
		vega_log(INFO, "set_recorder_dry_contact_on_off: unhandled conf_number C%d",conf_number);
		break;
	}
	return 0;
}

void set_alarm_dry_contact(int on_off)
{	// utilisation du contact sec utilise before pour radio 12
	set_contact_P8R8(3, on_off); // VHF12 = D35 test avec dio: write 0x08 on /dev/ixpci2 (bit 3)
	vega_log(INFO, "set_alarm_dry_contact: on_off %d",on_off);
}

void* Radio::radio_detection_audio_thread(void *)
{
	static unsigned long int din16_memory=0;	
	static unsigned long int din8_memory=0;	
	
	while (1)
	{
		/*if ( fd_dev_P16R16 > 0 ) {
			close(fd_dev_P16R16);
			fd_dev_P16R16=0;
		}
		if ( fd_dev_P8R8 > 0 ) { // cas ou on a une erreur au demarrage ( driver non chargé )
			close(fd_dev_P8R8); 
			fd_dev_P8R8=0;
		}*/
		//if ( fd_dev_P16R16 < 0 ) 
		fd_dev_P16R16 = open("/dev/ixpci1", O_RDWR);

		//if ( fd_dev_P8R8 < 0 ) 
		fd_dev_P8R8 = open("/dev/ixpci2", O_RDWR);

		fprintf(stderr,"IXPCI : fd_dev_P16R16=%d fd_dev_P8R8=%d\n",fd_dev_P16R16,fd_dev_P8R8);
		vega_log(INFO,"IXPCI : fd_dev_P16R16=%d fd_dev_P8R8=%d\n",fd_dev_P16R16,fd_dev_P8R8);

		if ( fd_dev_P16R16 < 0 ) {
			vega_alarm_log(EV_ALARM_BOARD_ANOMALY,2,0,"P16R16 Down ");
			//exit(-31);
		}
		if ( fd_dev_P8R8 < 0 ) {
			vega_alarm_log(EV_ALARM_BOARD_ANOMALY,2,0,"P8R8 Down ");
			//exit(-31);
		}

		sleep(1); // ds le cas d'un pb de chargement du driver du contact secs
		while (1)
		{
			usleep( 500*MILLISECOND);

			unsigned long din16 = 0;
			
			/*if (fd_dev_P16R16 < 0) 
			{
				vega_log(INFO,"Cannot open device file \"%s.\"\n", "/dev/ixpci1");
				close(fd_dev_P16R16);
				sleep(5);
	
				char cmd[]="/vega/bin/insmod-ixpci.sh";
				int ret = system(cmd);
				vega_log(INFO,"ret %d:%s",cmd);
				
				sleep(5);

				fd_dev_P16R16 = open("/dev/ixpci1", O_RDWR);
				break;
			}
			else*/
			{
				if (ioctl(fd_dev_P16R16, IXPCI_IOCTL_DI, &din16)) 
				{
					fprintf(stderr, "Failure of ioctl command IXPCI_IOCTL_DI on fd_dev_P16R16\n");
					vega_log(INFO, "Failure of ioctl command IXPCI_IOCTL_DI on fd_dev_P16R16\n");
					//exit(-31);

					//close(fd_dev_P16R16);
					//fd_dev_P16R16 = open("/dev/ixpci1", O_RDWR);
					//break;

					sleep(2);

				}else{
					//printf("P16R16 din16 = %X\n", din16);
					if( din16 != din16_memory )
					{
						printf("CHANGE: din16 %08x -> %08x\n", (unsigned int)din16 , (unsigned int)din16_memory);
						vega_log(INFO, "CHANGE: din16 %08x -> %08x\n", (unsigned int)din16 , (unsigned int)din16_memory);

						unsigned int i=0;
						long int index = 0x00000001; 

						do
						{
							long int result_old = din16_memory & index;
							long int result_current = din16 & index;

//CHANGE: din16 00000000 -> 00001000
//bit 12 change state 4096->0 ( D37 = (nil) ) ERROR : c'est D41 le jupiter 1
							if(result_old != result_current)
							{
								int dev_number = 0;//FIRST_RADIO_DEVICE_NUMBER = 25

								switch (i)
								{
								default:dev_number = FIRST_RADIO_DEVICE_NUMBER + i ; break;
								/*case 11:dev_number = 36; break;// D36 Jupiter 7  instead of radio 12*/

								case 12:dev_number = 41;break;
								case 13:dev_number = 42;break;
								case 14:dev_number = 43;break;
								case 15:dev_number = 44;break;
								case 16:dev_number = 45;break;
								}

								device_t* D = device_t::by_number(dev_number);
								printf("bit %d change state %ld->%ld ( D%d = %p ) \n", i, result_old , result_current, dev_number , D);
								vega_log(INFO,"bit %d change state %ld->%ld ( D%d = %p ) \n", i, result_old , result_current, dev_number , D);

								if ( D ) {
									EVENT ev;
									ev.device_number = D->number;
									ev.conference_number = D->current_conference;
									if(result_old == 0)
										ev.code = EVT_RADIO_MICRO_PRESSED;
									else
										ev.code = EVT_RADIO_MICRO_RELEASED;

									D->ExecuteStateTransition(&ev);
									//usleep( 200*MILLISECOND );
								}else{
									vega_log(INFO,"ERROR DEVICE D%d audio detection", dev_number);
								}
							}
							
							index = index << 1;
							i++;
							
							

						}while( i < ( sizeof(din16) * 8 ) );   			// we always have 24 bits of data

						din16_memory = din16; // christian 20090520 bug !!!
					}
				}


			}


			unsigned short din8 = 0; // 32 bits
			if (fd_dev_P8R8 <= 0) 
			{
				fprintf(stderr,"BAD fd_dev_P8R8 %d device file %s\n",  fd_dev_P8R8, "/dev/ixpci2");
				//close(fd_dev_P8R8);
				//fd_dev_P8R8 = open("/dev/ixpci2", O_RDWR);vega_log(INFO,"IXPCI : open fd_dev_P8R8=%d\n",fd_dev_P8R8);

			}
			else
			{
				if (ioctl(fd_dev_P8R8, IXPCI_IOCTL_DI, &din8)) 
				{ 
					fprintf(stderr,"Failure of ioctl command IXPCI_IOCTL_DI on fd_dev_P8R8=%d\n",fd_dev_P8R8);
					//close(fd_dev_P8R8);fd_dev_P8R8 = open("/dev/ixpci2", O_RDWR);vega_log(INFO,"IXPCI : open fd_dev_P8R8=%d\n",fd_dev_P8R8);
				}else{
					//printf("P8R8 din8 = %X\n", din8);
					if( din8 != din8_memory)
					{
						printf("CHANGE: din8 %08x -> %08x\n", (unsigned char)din8 , (unsigned char)din8_memory);


						unsigned int i=0;
						long int index = 0x00000001; 
						do
						{
								long int result_old = din8_memory & index;
								long int result_current = din8 & index;

								if(result_old != result_current)
								{
									printf("P8R8 bit %d change state %ld->%ld\n", i, result_old , result_current);
									vega_log(INFO,"P8R8 bit %d change state %d->%d\n", i, result_old , result_current);

									// pdl 20090825
									if ( 7 == i ) { 
										if ( result_current != 0 ) { // pdl 20090901
											vega_alarm_log(EV_ALARM_POWER_DOWN,2,0,"alimentation 2 DOWN ");// 0x80 == din8
											//set_alarm_dry_contact(1);

										}else{
											vega_alarm_log(EV_ALARM_POWER_UP,2,0,"alimentation 2 UP ");
											//set_alarm_dry_contact(0);
										}
									}
									else if ( 6==i){
										if ( result_current != 0 ) { // pdl 20090901
											vega_alarm_log(EV_ALARM_POWER_DOWN,1,0,"alimentation 1 DOWN "); //0x40 == din8
											//set_alarm_dry_contact(1);
										}else{
											vega_alarm_log(EV_ALARM_POWER_UP,1,0,"alimentation 1 UP ");
											//set_alarm_dry_contact(0);
										}
									}
									else if ( 0==i){ // read bit 0 change state on P8R8:  jupiter 5 = D45 
										device_t* D = device_t::by_number(45);
										EVENT ev;
										if ( result_current != 0 ) { //detection audio
											vega_log(INFO," Il faut allumer la led de la radio qui parle");
											ev.code = EVT_RADIO_MICRO_PRESSED;
										}else{
											vega_log(INFO," Il faut eteindre la led de la radio qui arrete de parler");
											ev.code = EVT_RADIO_MICRO_RELEASED;
										}
										if ( D ) {
											ev.device_number = D->number;
											ev.conference_number = D->current_conference;
											D->ExecuteStateTransition(&ev);// send the news to the device state machine
										}
									}									
									else if ( 1==i){ // read bit 1 change state on P8R8:  jupiter 6 = D46 
										device_t* D = device_t::by_number(46);
										EVENT ev;
										if ( result_current != 0 ) { //detection audio
											vega_log(INFO," Il faut allumer la led de la radio qui parle");
											ev.code = EVT_RADIO_MICRO_PRESSED;
										}else{
											vega_log(INFO," Il faut eteindre la led de la radio qui arrete de parler");
											ev.code = EVT_RADIO_MICRO_RELEASED;
										}
										if ( D ) {
											ev.device_number = D->number;
											ev.conference_number = D->current_conference;
											D->ExecuteStateTransition(&ev);// send the news to the device state machine
										}
									}
								}
								i++;
								index = index << 1;
						}
						while( i < ( sizeof(din8) * 8 ) ); 
						din8_memory = din8; // pdl 20090522
					}
				}
			}

		}// while(1)
	}
}


int Radio::RadioConferencingState(EVENT* ev)
{
	//vega_log(INFO,"R%d: ConferencingState:*** EVENT %d *******\n", number , ev->code);
	switch ( ev->code ) 
	{
		default:
			//vega_log(INFO,"UNHANDLED event %d", ev->code);
			vega_log(INFO,"R%d: ConferencingState:*** UNHANDLED EVENT %d *******\n", number , ev->code);
			break;
	case EVT_CONFERENCE_STOP_TONE:
			vega_log(INFO,"R%d EVT_CONFERENCE_STOP_TONE\n", number);
			stop_hearing_tone();
		break;

	case EVT_CALLGENERAL_STOP_TONE:
	case EVT_CALL_GROUP_STOP_TONE:
		{
			//int NC = ev->conference_number;
			vega_log(INFO,"R%d EVT_XXXX_STOP_TONE device_doing_group_or_general_call=%d\n", number,CRM4::device_doing_group_or_general_call);
			stop_hearing_tone();
			//vega_conference_t* C = vega_conference_t::by_number(current_conference);
			//if (C) C->deactivate_radio_base(this);
			//if ( CRM4::device_doing_group_or_general_call != this->number )
			{ // everybody listen except initiator qui ne s ecoute pas lui meme!
				CRM4* d = (CRM4*)device_t::by_number(CRM4::device_doing_group_or_general_call);
				/*int ts_value = FIRST_DEVICE_CRM4_INPUT_TS;
				ts_value += device_doing_group_or_general_call -1; 
				// attention : poste aslt 1 correspond a  144  ; poste aslt 7 correspond a  144 + 6
				vega_log(INFO,"D%d HEAR G%d on TS%d\n", number, doing_group_call, ts_value );
				start_hearing_tone(ts_value);*/
				if (d) start_hearing_tone(d->m_timeslot_micro);

			}
		}
		break;

		case EVT_RADIO_MICRO_PRESSED:
		{
		   printf("R%d EVT_RADIO_MICRO_PRESSED\n",number);
		   vega_log(INFO,"R%d EVT_RADIO_MICRO_PRESSED audio detection:%d\n",number,audio_detection);
		   if ( current_conference <= 0 ) {
			   vega_log(INFO, "R%d is not in a conference !", number);
		   }else{
				vega_conference_t *conf = vega_conference_t::by_number(current_conference);
				if ( conf ) {
					if (  !conf->active  ) {
						vega_log(INFO, "R%d current C%d INACTIVE !", number, conf->number);
					}else{
						if (conf->is_in(this) == -1) {
							vega_log(INFO, "R%d is not in C%d", number, conf->number);
						}else{
							/* on verifie si le membre n'est pas exclu d'abord */
							if ( PARTICIPANT_ACTIVE == conf->get_state_in_conference (this) )  {
								b_speaking=true;
								vega_log(INFO, "R%d START SPEAKING in C%d", number, conf->number);
								radio_start_speaking_conf(conf); // christian test jupiter qui ne parle pas 20090529
								conf->activate_radio_base(this);
								CRM4::all_devices_display_led_of_device_speaking_in_conf(conf, this);

								set_recorder_dry_contact_on_off(current_conference ,1);  // to start the recording

								vega_event_log(EV_LOG_DEVICE_START_SPEAKING, number, 0, conf->number, "%s commence a parler dans %s", name, conf->name);

							}else{
								vega_log(INFO, "R%d not PARTICIPANT_ACTIVE in C%d", number, conf->number);
							}
						}
					}
				}else{
					vega_log(INFO,"R%d current_conference=%d\n",number,current_conference );
				}
		   }
	  }// case
	  break;

	  case EVT_RADIO_MICRO_RELEASED:
	  {
		   printf("R%d EVT_RADIO_MICRO_RELEASED\n",number);
		   vega_log(INFO,"R%d EVT_RADIO_MICRO_RELEASED audio detection:%d\n",number,audio_detection);
		   b_speaking=false;
		   vega_conference_t *conf = vega_conference_t::by_number(current_conference);	
		   if ( conf ) { // ne pas tester conf->active car elle a pu etre desactivee pdt qu'on parlait ???
			   b_speaking = false;
			   if ( audio_detection == 1 ){
				   vega_log(INFO, "R%d RADIO STOP SPEAKING in C%d", number, conf->number);
				   radio_stop_speaking(conf);
			   }
			   CRM4::all_devices_display_led_of_device_NOT_speaking_in_conf(conf, this);
			   if ( conf->nb_people_speaking() == 0 ){
				   conf->deactivate_radio_base(this); // if still some other people speaking in conference, do not deactivate radio
				   set_recorder_dry_contact_on_off(current_conference ,0); // stop recording
			   }

				vega_event_log(EV_LOG_DEVICE_STOP_SPEAKING, number, 0, conf->number, "%s arrete de parler dans %s", name, conf->name);

		   }else{
				vega_log(INFO,"R%d no current_conference %d\n",number,current_conference);
		   }
	  }    
	  break;

 	case EVT_CONFERENCE_DEACTIVATED:
		{
			int deactivated_conf_number = ev->conference_number;
			vega_log(INFO, "R%d EVT_CONFERENCE_DEACTIVATED C%d ( currently in C%d)",number,deactivated_conf_number,current_conference);

			if ( deactivated_conf_number == current_conference ) 
			{
				vega_conference_t *conf = vega_conference_t::by_number(current_conference);
				if ( conf ) 
				{
					conf->set_state_in_conference(this,PARTICIPANT_ACTIVE);
					vega_log(INFO, "R%d EVT_CONFERENCE_DEACTIVATED C%d",number, conf->number);
					radio_stop_hearing(conf);
				   
					//if ( audio_detection == 1 )
					{
					   radio_stop_speaking(conf);
					   CRM4::all_devices_display_led_of_device_NOT_speaking_in_conf(conf, this);
				   
					}

					if ( conf->nb_people_speaking() == 0 ){
					   conf->deactivate_radio_base(this); // if still some other people speaking in conference, do not deactivate radio
					}			
				}

				b_speaking=false;
				//current_conference = 0; pdl20091221
				//ChangeState(&Radio::IdleState);

				{				
					start_hearing_tone(TS_RING_TONE);
					conf->activate_radio_base(this); // si radio, on active le contact sec pour mettre la radio en ecoute
					EVENT evt;
					evt.code = EVT_CONFERENCE_STOP_DEACTIVE_TONE;
					evt.device_number = number; // transmit my number
					evt.conference_number = current_conference;
					DeferEvent(&evt,3);
				}

			}
		}
		break;
#if 1
	case EVT_CONFERENCE_STOP_DEACTIVE_TONE:
		{
			stop_hearing_tone();
			vega_log(INFO,"R%d EVT_CONFERENCE_STOP_DEACTIVE_TONE IN C%d current_conference C%d\n", number, ev->conference_number , current_conference);
			vega_conference_t* C = vega_conference_t::by_number(ev->conference_number);
			if (C) {
				C->deactivate_radio_base(this);
				CRM4::all_devices_display_led_of_device_NOT_speaking_in_conf(C, this);
			}
			current_conference = 0; //pdl20091221
			ChangeState(&Radio::IdleState);

		}
		break;
#endif


	case EVT_CONFERENCE_DTOR_EXCLUDE: // radio cannot do self exclusion !!!
	  {
			int conf_number = ev->conference_number; // Number of conference where exclure is happening ...
			int dev_number = ev->device_number; // Number of device that is supposed to be excluded !
			vega_log(INFO, "RadioConferencingState: EVT_CONFERENCE_DTOR_EXCLUDE D%d from C%d",dev_number, conf_number);
			/*vega_conference_t* C = vega_conference_t::by_number(conf_number); pdl 20090922 : debranchements mixers deja faits !!!
			device_t *D=device_t::by_number(dev_number);

			if ( C && D && (this == D) ) { // is that me that has been excluded ???
				C->set_state_in_conference(this,PARTICIPANT_DTOR_EXCLUDED);
				vega_event_log(EV_LOG_DEVICE_EXCLUDED, number,0,C->number,"%s exclu de %s", name, C->name);
				vega_log(INFO, "it is me D%d excluded from conference C%d by director", number, conf_number );
				radio_stop_hearing(C);// disconnect the timeslot where RADIO hear the conference ////////// DISCONNECT Cf+T
				radio_stop_speaking(C);
				CRM4::all_devices_display_led_of_device_NOT_speaking_in_conf(C, this);
				if ( C->nb_people_speaking() == 0 ){
					C->deactivate_radio_base(this); // if still some other people speaking in conference, do not deactivate radio
				}
			}*/
	  }
	  break;
/*
18:40:08.555 RadioConferencingState: EVT_CONFERENCE_DTOR_INCLUDE D43 in C4
18:40:08.555 C4 LANCEUR4 ACTIV:1 INIT:04 DTOR05 R1:30(1) R2:00(0) J:43(2) TS[58-66] speaking[0] particip[2(1) 1(1) 3(1) 4(1) 6(1) 7(1) ]
18:40:08.555 radio_start_hearing_conf FULLDUPLEX RADIO D43 in C4 TS 139
18:40:08.555 CONN TS66 -> suboutput SC1 P5 SBI1 B23
18:40:08.555 radio_start_hearing_conf R43 C4 TS micro:139
18:40:08.555 D44 Radio::IdleState =============	  */
  case EVT_CONFERENCE_DTOR_INCLUDE	:
	  {
			int conf_number = ev->conference_number; // No of conference where exclure is happening ...
			int dev_number = ev->device_number; // No of device that is supposed to be re-included !
			vega_log(INFO, "RadioConferencingState: EVT_CONFERENCE_DTOR_INCLUDE D%d in C%d",dev_number, conf_number);
			/*vega_conference_t* C = vega_conference_t::by_number(conf_number);pdl 20090922 : branchements mixers deja faits !!!
			device_t *D=device_t::by_number(dev_number);

			if ( C && D && (this == D) ) {	
				C->set_state_in_conference(this,PARTICIPANT_ACTIVE);
				vega_event_log(EV_LOG_DEVICE_INCLUDED, number,0,C->number,"%s inclus dans %s", name, C->name);
				radio_start_hearing_conf(C);////////// CONNECT Cf+T				
				radio_start_speaking_conf(C);////////// CONNECT Cf+T				
			}*/
	  }
	  break;

	case EVT_REMOVE_DEVICE:
		{
			vega_log(INFO,"RadioState:*** EVT_REMOVE_DEVICE D%d from C%d *******\n",ev->device_number, ev->conference_number );
			
			if ( ev->device_number == number )
			{ // it is me, i am already conferencing and administrator removed my device in conf !
				vega_conference_t *C = vega_conference_t::by_number(current_conference);
				if ( C){
					if ( NOT_PARTICIPANT != C->get_state_in_conference(this) ) {
						radio_stop_speaking(C); 
						radio_stop_hearing(C); // disconnect mixer before deleting device
						b_speaking = false;
						C->set_state_in_conference(this,NOT_PARTICIPANT);
						if ( C->nb_people_speaking() == 0 ){
							C->deactivate_radio_base(this); // if still some other people speaking in conference, do not deactivate radio
						}
						vega_log(INFO, "RADIO R%d now NOT_PARTICIPANT in C%d", number, C->number );
					}
					vega_event_log(EV_LOG_DEVICE_REMOVED, number, 0, C->number, "%s supprime dans %s", name, C->name);
					current_conference = 0;
					ChangeState(&Radio::IdleState); // todo : simulate a .EVT_DEVICE_KEY_MICRO_RELEASED before if we are talking
					CRM4::all_devices_display_led_of_device_NOT_speaking_in_conf(C, this);
				}
			}
		}
		break;

	}
	return 0;
}

void Radio::start_hearing_tone(int ts)
{
	if ( has_hardware_error() ) return;
	if( true==is_hearing_tone ) {
		vega_log(INFO, "WARNING: start_hearing_tone: D%d est deja en TONE !!!",number);
		//exit(-14);
		return;
	}
	is_hearing_tone = true;
	if ( port_aga_reception.mixer.sub_channel0.connected ){
		vega_log(INFO, "ATTENTION: D%d start_hearing_tone PORT AGA Reception SC 0 deja connecte !!!",number);
		mixer_disconnect_timeslot_to_sub_output(&port_aga_reception,  sub_channel_0);
		//mixer_disconnect_timeslot_to_sub_output(&device.crm4.port_aslt, sub_channel_0);
	}else{
		vega_log(INFO, "OK: D%d start_hearing_tone PORT AGA Reception SC 0 pas connecte",number);
	}

	mixer_connect_timeslot_to_sub_output(&port_aga_reception,  sub_channel_0, ts , 0);
	port_aga_reception.mixer.sub_channel0.connected = 1;

	set_radio_dry_contact_on_off(number, 1); // pdl 20091005 la base passe en emission pour que la radio entende
}


void Radio::stop_hearing_tone()
{
	if ( has_hardware_error() ) return;
	if( false==is_hearing_tone ) {
		vega_log(INFO, "WARNING: stop_hearing_tone: Radio D%d n'est pas en TONE !!!",number);
		return;
	}
	is_hearing_tone = false;
	//mixer_disconnect_timeslot_to_sub_output(&d->device.crm4.port_aslt, sub_channel_0);
	if ( port_aga_reception.mixer.sub_channel0.connected ){
		vega_log(INFO, "OK: D%d stop_hearing_tone PORT AGA Reception SC 0 connecte",number);
		mixer_disconnect_timeslot_to_sub_output(&port_aga_reception, sub_channel_0);
		port_aga_reception.mixer.sub_channel0.connected = 0;
	}else{
		vega_log(INFO, "ATTENTION: D%d stop_hearing_tone PORT AGA Reception SC 0 non connecte !!!",number);
	}

	set_radio_dry_contact_on_off(number, 0);

}

/* no conference activated yet ! general call or group call are possible ! */
int Radio::IdleState(EVENT* ev)
{
	//vega_log(INFO,"D%d Radio::IdleState =============\n",number );
	switch ( ev->code ) 
	{
	default:
		//vega_log(INFO,"D%d Radio::IdleState UNHANDLED EVENT code:%d \n", number, ev->code);
		break;

	case EVT_CONFERENCE_START_TONE: // broadcasted !!!
		{
			int NC = ev->conference_number;
			vega_log(INFO,"D%d START_TONE IN C%d initiator D%d\n", number, NC , ev->device_number);
			vega_conference_t* C = vega_conference_t::by_number(NC);
			if ( C ) 
			{
				if ( C->is_in(this) ) 
				{
					vega_log(INFO,"R%d EVT_CONFERENCE_START_TONE in C%d", number, NC );
					//mixer_connect_timeslot_to_sub_output(&port_aga_reception,  sub_channel_0, TS_CONFERENCE_TONE , 0); 	
					//conf->activate_radio_base(this);
					//set_radio_dry_contact_on_off(number, 1);
					start_hearing_tone(1);
					{
						EVENT evt;
						evt.code = EVT_CONFERENCE_STOP_TONE;
						evt.device_number = ev->device_number; // transmit the initiator of conference call
						evt.conference_number = ev->conference_number;
						DeferEvent(&evt,3);
					}
				}
			}
		}
		break;

#if 0
	case EVT_CONFERENCE_STOP_DEACTIVE_TONE:
		{
			stop_hearing_tone();
			vega_log(INFO,"R%d EVT_CONFERENCE_STOP_DEACTIVE_TONE IN C%d current_conference C%d\n", number, ev->conference_number , current_conference);
			vega_conference_t* C = vega_conference_t::by_number(ev->conference_number);
			if (C) {
				C->deactivate_radio_base(this);
				CRM4::all_devices_display_led_of_device_NOT_speaking_in_conf(C, this);
			}
			current_conference = 0; //pdl20091221
			ChangeState(&Radio::IdleState);

		}
		break;
#endif

	case EVT_CONFERENCE_STOP_TONE:
		{
			//int NC = ev->conference_number;
			vega_log(INFO,"D%d EVT_CONFERENCE_STOP_TONE IN C%d initiator D%d\n", number, ev->conference_number , ev->device_number);
			stop_hearing_tone();
			vega_conference_t* C = vega_conference_t::by_number(ev->conference_number);
			if (C) 
				C->deactivate_radio_base(this);
			{
				EVENT evt;
				evt.code = EVT_CONFERENCE_ACTIVATED; 
				evt.device_number = ev->device_number; // transmit the initiator of conference
				evt.conference_number = ev->conference_number;
				ExecuteStateTransition(&evt);			
			}
		}
		break;

	case EVT_CALLGENERAL_STOP_TONE:
	case EVT_CALL_GROUP_STOP_TONE:
		{
			//int NC = ev->conference_number;
			vega_log(INFO,"R%d EVT_XXXX_STOP_TONE device_doing_group_or_general_call=%d\n", number,CRM4::device_doing_group_or_general_call);
			stop_hearing_tone();
			//vega_conference_t* C = vega_conference_t::by_number(current_conference);
			//if (C) C->deactivate_radio_base(this);
			//if ( CRM4::device_doing_group_or_general_call != this->number )
			{ // everybody listen except initiator qui ne s ecoute pas lui meme!
				CRM4* d = (CRM4*)device_t::by_number(CRM4::device_doing_group_or_general_call);
				/*int ts_value = FIRST_DEVICE_CRM4_INPUT_TS;
				ts_value += device_doing_group_or_general_call -1; 
				// attention : poste aslt 1 correspond a  144  ; poste aslt 7 correspond a  144 + 6
				vega_log(INFO,"D%d HEAR G%d on TS%d\n", number, doing_group_call, ts_value );
				start_hearing_tone(ts_value);*/
				if (d) start_hearing_tone(d->m_timeslot_micro);

			}
		}
		break;


	case EVT_ADD_RADIO1:
		{	// ev->device_number is the number of the newly added device
			vega_log(INFO,"R%d IdleState EVT_ADD_RADIO1 in C%d\n",number,ev->conference_number);
			if ( ev->device_number == number )
			{	// it is me, i am NOT conferencing and GUI added my device in this conference !
				vega_conference_t *C = vega_conference_t::by_number(ev->conference_number);
				if ( C ){
					vega_log(INFO,"====================> EVT_ADD_RADIO1 D%d in C%d (active:%d) (currently in C%d) \n",
						ev->device_number, ev->conference_number, C->active, current_conference );
					
					if ( C->add_in_radio1(this) ){
						C->set_state_in_conference(this,PARTICIPANT_ACTIVE);
						if ( C->active ) {
							current_conference = C->number; // c'est notre conference selectionnee par defaut car la premiere	
							vega_log(INFO,"R%d EVT_ADD_RADIO1 in C%d",number,C->number);
							ChangeState(&Radio::RadioConferencingState);
							radio_start_hearing_conf(C);// do not consider audio detection first time
							radio_start_speaking_conf(C);
						}
						vega_event_log(EV_LOG_DEVICE_ADDED, number, 0, C->number, "ajout de %s dans %s", name, C->name);

					}else{
						vega_log(INFO,"already RADIO1 in C%d",C->number);
					}
				}
			}
		}
		break;

	case EVT_ADD_RADIO2:
		{	// ev->device_number is the number of the newly added device
			vega_log(INFO,"R%d IdleState EVT_ADD_RADIO2 in C%d\n",number,ev->conference_number);
			if ( ev->device_number == number )
			{	// it is me, i am NOT conferencing and GUI added my device in this conference !
				vega_conference_t *C = vega_conference_t::by_number(ev->conference_number);
				if ( C ){
					vega_log(INFO,"====================> EVT_ADD_RADIO2 D%d in C%d (active:%d) (currently in C%d) \n",
						ev->device_number, ev->conference_number, C->active, current_conference );
					
					if ( C->add_in_radio2(this) ){
						C->set_state_in_conference(this,PARTICIPANT_ACTIVE);
						if ( C->active ) {
							current_conference = C->number; // c'est notre conference selectionnee par defaut car la premiere	
							vega_log(INFO,"R%d EVT_ADD_RADIO2 in C%d",number,C->number);
							ChangeState(&Radio::RadioConferencingState);
							radio_start_hearing_conf(C);// do not consider audio detection first time
							radio_start_speaking_conf(C);
						}
						vega_event_log(EV_LOG_DEVICE_ADDED, number, 0, C->number, "ajout de %s dans %s", name, C->name);

					}else{
						vega_log(INFO,"already RADIO2 in C%d",C->number);
					}
				}
			}
		}
		break;
	case EVT_ADD_JUPITER:
		{	// ev->device_number is the number of the newly added device
			vega_log(INFO,"R%d IdleState EVT_ADD_JUPITER in C%d\n",number,ev->conference_number);
			if ( ev->device_number == number )
			{	// it is me, i am NOT conferencing and GUI added my device in this conference !
				vega_conference_t *C = vega_conference_t::by_number(ev->conference_number);
				if ( C ){
					vega_log(INFO,"====================> EVT_ADD_JUPITER D%d in C%d (active:%d) (currently in C%d) \n",
						ev->device_number, ev->conference_number, C->active, current_conference );
					
					if ( C->add_in_jupiter(this) ){
						C->set_state_in_conference(this,PARTICIPANT_ACTIVE);
						if ( C->active ) {
							current_conference = C->number; // c'est notre conference selectionnee par defaut car la premiere	
							vega_log(INFO,"J%d EVT_ADD_JUPITER in C%d",number,C->number);
							ChangeState(&Radio::RadioConferencingState);
							radio_start_hearing_conf(C);// do not consider audio detection first time
							radio_start_speaking_conf(C);
						}
						vega_event_log(EV_LOG_DEVICE_ADDED, number, 0, C->number, "ajout de %s dans %s", name, C->name);
					}
				}else{
					vega_log(INFO,"already JUPITER in C%d",C->number);
				}
			}
		}
		break;


	case EVT_CONFERENCE_ACTIVATED: // qqun a activee une conference, SELECTIONNEE par defaut si on en fait partie!!!
		{	
			vega_log(INFO,"D%d:*** EVT_CONFERENCE_ACTIVATED C%d by D%d *******\n", number , ev->conference_number, ev->device_number);
			vega_conference_t* C = vega_conference_t::by_number(ev->conference_number);
			if (C)
			{
				C->active=1; // pdl 20090911 impossible de refuser une activation de conference, le premier device qui recoit l'event active la conf
				if ( C->is_in(this) )
				{
					vega_log(INFO, "OK IdleState: R%d is PARTICIPANT_ACTIVE in C%d", number, C->number);
					current_conference = C->number; // c'est notre conference selectionnee par defaut car la premiere	
					C->set_state_in_conference(this,PARTICIPANT_ACTIVE);
					vega_log(INFO,"R%d EVT_CONFERENCE_ACTIVATED C%d",number,C->number);
					ChangeState(&Radio::RadioConferencingState);
					radio_start_hearing_conf(C); // do not consider audio detection first time
					radio_start_speaking_conf(C);
				}
			}
		}

	case EVT_REMOVE_DEVICE:
		{
			vega_log(INFO,"IdleState:*** EVT_REMOVE_DEVICE D%d from C%d current C%d\n",ev->device_number, ev->conference_number ,current_conference);
			vega_conference_t *C = vega_conference_t::by_number(ev->conference_number);
			if ( ev->device_number == number )
			{ // it is me, i am already conferencing and ihm administrator removed my device in conf !
				if ( C){
					if ( NOT_PARTICIPANT != C->get_state_in_conference(this) ) {
						C->set_state_in_conference(this,NOT_PARTICIPANT);
						vega_log(INFO, "RADIO R%d now NOT_PARTICIPANT in C%d", number, C->number );
						vega_event_log(EV_LOG_DEVICE_REMOVED, number, 0, C->number, "suppression de %s dans %s", name, C->name);
					}
				}
				current_conference = 0;
				ChangeState(&Radio::IdleState);
			}
		}
		break;

	}
	return 0;
}/*Radio::IdleState*/


