// vega_control.c
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
#include <sys/time.h>
#include <unistd.h>
#include <syslog.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <QList>
#include <QLinkedList>
#include <QMutex>


#include "const.h"
#include "misc.h"
#include "debug.h"
#include "conf.h"
#include "alarmes.h"
#include "switch.h"


typedef struct frame_s{
	int data_len;
	char data[MAX_TCP_FRAME_SIZE];
}frame_t;

//#define LOG_TCP_2_FILE_ON
#define TCP_TX_FILE "/tmp/tx-tcp-vega.bin"
#define TCP_RX_FILE "/tmp/rx-tcp-vega.bin"
#define TCP_RX_HEX_FILE "/tmp/rx-tcp-vega.hex"
#define TCP_TX_HEX_FILE "/tmp/tx-tcp-vega.hex"

extern time_t demarrage_system;

//#define USE_SO_RCVBUF
#define USE_TCP_NODELAY
//#define USE_SO_LINGER

int tx_total = 0;
int rx_total = 0;

static QLinkedList<frame_t *> qlist_frames;
static QLinkedList<frame_t *> qlist_frames_not_urgent;
static QMutex qlist_frames_mutex;

static QLinkedList<frame_t *> qlist_rx_frames;
static QMutex qlist_rx_frames_mutex;

//static QLinkedList<frame_t *> qlist_monitor_tx_frames;
//static QMutex qlist_monitor_tx_frames_mutex;

static QLinkedList<BYTE> qlist_monitor_rx_bytes;
static QMutex qlist_monitor_rx_bytes_mutex;

static QLinkedList<BYTE> qlist_monitor_tx_bytes;
static QMutex qlist_monitor_tx_bytes_mutex;

extern QMap<int /*board number*/,bool /*is_good*/> map_boards;

extern void set_alarm_dry_contact(int on_off);

/*
SO_RCVBUF  

    Sets receive buffer size information. This option takes an int value in the optval argument.

SO_KEEPALIVE  

    Keeps connections active by enabling periodic transmission of messages, if this is supported by the protocol.

    If the connected socket fails to respond to these messages, the connection is broken and processes writing to that socket are notified with an ENETRESET errno. This option takes an int value in the optval argument. This is a BOOL option.

Sous linux man 7 socket mentionne:
SO_RCVTIMEO/SO_SNDTIMEO (timeout),
SO_RCVLOWAT/SO_SNDLOWAT (buffer mémoire),
SO_SNDBUF/SO_RCVBUF (buffer socket)
 
Vaguement relatif:
SO_LINGER (timeout fermeture)
SO_PRIORITY (priorité IP)
 
Ioctls:
SIOCGSTAMP (timestamp du dernier octet reçu) 
*/
#define USE_SO_KEEPALIVE

vega_control_t *pAlphaComLink = NULL;
static struct timeval _tstart_pong;

static vega_control_t g_link;

//switch_t backpanel;

// defini dans le main.c : les alarmes sont deja chargées

int vega_control_get_next_msg_in_current_frame(vega_control_t *plink, unsigned char* data, int len);
int vega_control_recv_message(vega_control_t *plink);
void *vega_control_handle_link_thread(void *arg);

void vega_test_pong_delay()
{

	  double tt = time_between_button(&_tstart_pong);

	  vega_log(INFO,"PONG within %f seconds",tt);
	  if ( tt > 21.0 ){
		  // ALARME VEGA DOWN
		  //vega_event_log(EV_LOG_VEGA_CONTROL_NETWORK_DOWN, 0,0,0,"!!! Control DEFINITIVELY DISCONNECTED with alphacom ");
		  vega_alarm_log(EV_ALARM_CNX_ALPHACOM_CONTROL_DOWN, 0,0,"Deconnexion Alphacom <-> PC Controle");
		  vega_alarm_log(EV_ALARM_CNX_IHM_CONTROL_DOWN, 0,0,"Deconnexion IHM <-> PC Controle");

			printf("!!! DEFINITIVELY DISCONNECTED with alphacom (%s:%d) !!!\n",  inet_ntoa(g_link.peer_addr.sin_addr), g_link.peer_addr.sin_port);
			critical_dump();
			exit(-9);
	  }
}

void *vega_control_handle_send_thread(void *arg)
{
	
	while(1)
	{
		int nbUrgent=0;
		int nbNotUrgent=0;


		qlist_frames_mutex.lock();
		{
			nbUrgent = qlist_frames.size();
			nbNotUrgent = qlist_frames_not_urgent.size();
		}
		qlist_frames_mutex.unlock();


		/*pthread_mutex_lock(&mutex_send);
		if ( list_frames_urgent ) nbUrgent = g_list_length(list_frames_urgent);
		if ( list_frames_not_urgent ) nbNotUrgent = g_list_length(list_frames_not_urgent) ;
		pthread_mutex_unlock(&mutex_send);*/

			//vega_log(INFO,"tcp %d urgent/%d not urgent",nbUrgent,nbNotUrgent );

		if ( nbUrgent == 0 ){
			usleep(1*MILLISECOND);
		}
		else
		{
	

			//if ( NULL != list_frames_urgent )
			{
				/*pthread_mutex_lock(&mutex_send);
				frame_t *f = (frame_t *)g_list_nth_data(list_frames_urgent, 0);
				list_frames_urgent = g_list_remove(list_frames_urgent, f); // pdl BIG BUG 20090923
				pthread_mutex_unlock(&mutex_send);*/


				qlist_frames_mutex.lock();
				if ( qlist_frames.size() > 0 ) 
				{
					//QLinkedList<frame_t *>::const_iterator i;
					//for (i = qlist_frames.begin(); i != qlist_frames.end(); ++i)
					frame_t* f = qlist_frames.takeFirst();
					if ( f ) 
					{
						//vega_log(INFO,"%d bytes: tcp %d urgent/%d not urgent",f->data_len , nbUrgent,nbNotUrgent );
						if (f && pAlphaComLink && pAlphaComLink->sock>=3 && f->data_len>0)
						{  
							struct timeval tm_before;gettimeofday(&tm_before, NULL);		
							double t1 =  (double)tm_before.tv_sec;t1 += (double)tm_before.tv_usec/(1000*1000);

							int ret = send(pAlphaComLink->sock, (const void*) &f->data[0], f->data_len, 0);// MSG_DONTWAIT);
							if ( ret>0) {
								/*qlist_monitor_tx_bytes_mutex.lock();
								for ( int k=0;k<ret; k++){qlist_monitor_tx_bytes.append(f->data[k]);}
								qlist_monitor_tx_bytes_mutex.unlock();*/

								FILE* ftxhex = fopen(TCP_TX_HEX_FILE, "a+b");
								if ( ftxhex ) {
									int k;for (k=0;k< ret; k++){
										tx_total++;
										if (0==(tx_total%16)) fprintf(ftxhex,"\n");
										fprintf(ftxhex,"%2.2X ", f->data[k] & 0xff);
									}
									fprintf(ftxhex,"\n");
									fclose(ftxhex);
								}


							}
							usleep( (unsigned long ) (f->data_len*MILLISECOND));



							struct timeval tm_after;gettimeofday(&tm_after, NULL);
							double t2 =  (double)tm_after.tv_sec;t2 += (double)tm_after.tv_usec/(1000*1000);

							//vega_log(INFO,"______ nb_sent %d/%d within %f sec ( %.1f bytes/sec)\n", f->data_len , f->data_len, t2-t1, f->data_len/(t2-t1));
							//fprintf(stdout, "       %d %.1f bytes/sec (URGENT)\n", ret, f->data_len/(t2-t1));
							free(f);//printf("free %d = %p\n", sizeof(frame_t), f);
						}
					}
				}
				qlist_frames_mutex.unlock();

			}
			continue; // continuer a traiter les trames urgentes en priorite
		}

		// on arrive ici seulement quand il n'y a plus de trames urgentes a emettre
		if (  nbNotUrgent==0 ){
			usleep(1*MILLISECOND);
		}
		else
		{
			qlist_frames_mutex.lock();
			if ( qlist_frames_not_urgent.size() > 0 ) 
			{
				frame_t* f = qlist_frames_not_urgent.takeFirst();

				if (f && pAlphaComLink && pAlphaComLink->sock>=3 && f->data_len>0)
				{  
					struct timeval tm_before;gettimeofday(&tm_before, NULL);		
					double t1 =  (double)tm_before.tv_sec;t1 += (double)tm_before.tv_usec/(1000*1000);

					int ret = send(pAlphaComLink->sock, (const void*) &f->data[0], f->data_len, 0);// MSG_DONTWAIT);
					if ( ret>0) {
						/*qlist_monitor_tx_bytes_mutex.lock();
						for ( int k=0;k<ret; k++){qlist_monitor_tx_bytes.append(f->data[k]);}
						qlist_monitor_tx_bytes_mutex.unlock();*/

								FILE* ftxhex = fopen(TCP_TX_HEX_FILE, "a+b");
								if ( ftxhex ) {
									int k;for (k=0;k< ret; k++){
										tx_total++;
										if (0==(tx_total%16)) fprintf(ftxhex,"\n");
										fprintf(ftxhex,"%2.2X ", f->data[k] & 0xff);
									}
									fprintf(ftxhex,"\n");
									fclose(ftxhex);
								}

					}
					usleep( (unsigned long ) (f->data_len*MILLISECOND));

					struct timeval tm_after;gettimeofday(&tm_after, NULL);
					double t2 =  (double)tm_after.tv_sec;t2 += (double)tm_after.tv_usec/(1000*1000);

					//vega_log(INFO,"______ nb_sent %d/%d within %f sec ( %.1f bytes/sec)\n", f->data_len , f->data_len, t2-t1, f->data_len/(t2-t1));
					//fprintf(stdout, "      %d %.1f bytes/sec (NOT_URGENT)\n", ret,f->data_len/(t2-t1));
					free(f);
				}
			}
			qlist_frames_mutex.unlock();
		}

	}/*while (1)*/
	return NULL;
}

int vega_send(bool urgent, const void* data, size_t datalen)
{
	//vega_log(INFO, "vega_send %d bytes", datalen);
	if ( datalen>0 && pAlphaComLink && pAlphaComLink->state == ST_connected_slave ) 
	{
		frame_t *f = (frame_t *)calloc(sizeof(frame_t), 1);
		if (f == NULL) {
			vega_log(CRITICAL, "cannot allocate frame");
			return -1;	
		}else{
			if ( datalen >= MAX_TCP_FRAME_SIZE ) datalen = MAX_TCP_FRAME_SIZE-1;
			f->data_len = datalen;
			memcpy( f->data, data, datalen );  
			//vega_log(INFO,"vega_send: g_list_append %p datalen=%d", f, datalen);
			qlist_frames_mutex.lock();
			{
				if ( urgent ) 
					qlist_frames.append(f);
				else
					qlist_frames_not_urgent.append(f);
			}
			qlist_frames_mutex.unlock();
		}
	}else{
		vega_log(CRITICAL, "cannot vega_send when link down");
		fprintf(stderr,"cannot vega_send when link down\n");
	}
	return datalen; // todo: check if it is correct even when link down
}


int vega_control_send_alphacom_msg(bool urgent,vega_control_t *plink, const unsigned char* data, const unsigned int datalen)
{  
  int ret = -1;

  if ( data==NULL || datalen==0 ) 
    return -1;

  if (plink == NULL)
    return -1;

  tcp_msg_t msg;
  msg.size = datalen;
  memcpy(msg.data, data, datalen);
/*   const char *mm = dump_message(&msg); */
/*   if (mm) */
/*     free (mm); */
/*   printf("%s", mm); */

 /*  if (plink->state != ST_connected_slave && */
/*       plink->state != ST_connected_unknown && */
/*       plink->state != ST_connected_master) { */
/*     vega_log(ERROR, "send message (%d bytes) error : not in connected / slave mode", datalen); */
/*     return -1; */
/*   } */
  


  BYTE trame[1024]={0};
  unsigned int idx=0;
  
  BYTE SlaveByte = 64 + plink->tx_seq_num * 4;
  
  BYTE 	CheckSum = SlaveByte;
  
  if ( 241==SlaveByte ) {// Then
    trame[idx++] = 240 ;
    trame[idx++] = 0 ;
    //SlaveStr = Chr(240) + Chr(0)
  }else if ( 240 == CheckSum ) {//     ElseIf CheckSum = 240 Then
    //SlaveStr = Chr(240) + Chr(1)
    trame[idx++] = 240 ;
    trame[idx++] = 1 ;
  }else { //Else
    //     SlaveStr = Chr(SlaveByte)
    trame[idx++] = SlaveByte ;
  }//End If
  unsigned int i;
  for (  i=0; i < datalen; i++ ) {
    
    BYTE HelpByte = data[i];
    CheckSum = CheckSum ^ HelpByte; // xor
    if ( HelpByte ==  241 ) {
      // SlaveStr = SlaveStr + Chr(240) + Chr(0)
      trame[idx++] = 240 ;
      trame[idx++] = 0 ;
    }else{
      trame[idx++] = HelpByte ;
    }
    
  }
  

  if ( 241 == CheckSum ) // 0xF1
    {
      trame[idx++] = 240 ;
      trame[idx++] = 0 ;
      trame[idx++] = 241 ;

    }
  else if ( 240 == CheckSum ) // 0xF0
    {
      trame[idx++] = 240 ;
      trame[idx++] = 1;
      trame[idx++] = 241 ;
    }
  else{

    trame[idx++] = CheckSum;
    trame[idx++] = 241 ;
  }
  

  /*{
	  int k;
  
	  for (  k =0 ; k < idx; k++){ 
     
		  printf("%d 0x%02X \n", (unsigned char)trame[k], trame[k]); 

	  }
  }*/
  
  // emission
  //ret = send(plink->sock, trame,  idx, 0);
  ret = vega_send(urgent, trame,  idx);

  if ( ret>0){
								FILE* ftxhex = fopen(TCP_TX_HEX_FILE, "a+b");
								if ( ftxhex ) {
									int k;for (k=0;k< ret; k++){
										tx_total++;
										if (0==(tx_total%16)) fprintf(ftxhex,"\n");
										fprintf(ftxhex,"%2.2X ", trame[k] & 0xff);
									}
									fprintf(ftxhex,"\n");
									fclose(ftxhex);
								}
  }
  
  if ( ret != idx ) {
	  vega_log(INFO,"ERROR tcp sent only %d/%d bytes", ret, idx);
  }
  
  if (ret == idx) {
   /*  msg_log(vega_message_direction_out, trame, idx); */
  }
  
  plink->tx_seq_num ++;
  if (plink->tx_seq_num > 7)
    plink->tx_seq_num = 1;
  
  /*const char *m = dump_message_serialized(trame, idx);
  if (m) {
    printf("%s\n", m);
    free ((char*)m);
  }*/
  
  
  if (ret == idx) {
    return 0;
  }


  return -1;
}


int vega_control_send_alphacom_msg1(vega_control_t *plink, const unsigned char* data, const unsigned int datalen)
{  
  int ret = -1;

  
  if ( plink==NULL || data==NULL || datalen==0 )//|| (plink->state==ST_disconnected) ) //pAlphaComLink->sock<=3 ) 
    return -1;

  if (plink == NULL)
    return -1;

  tcp_msg_t msg;
  msg.size = datalen;
  memcpy(msg.data, data, datalen);

  BYTE trame[1024]={0};
  unsigned int idx=0;
  
  BYTE SlaveByte = 64 + plink->tx_seq_num * 4;
  
  BYTE 	CheckSum = SlaveByte;
  
  if ( 241==SlaveByte ) {// Then
    trame[idx++] = 240 ;
    trame[idx++] = 0 ;
    //SlaveStr = Chr(240) + Chr(0)
  }else if ( 240 == CheckSum ) {//     ElseIf CheckSum = 240 Then
    //SlaveStr = Chr(240) + Chr(1)
    trame[idx++] = 240 ;
    trame[idx++] = 1 ;
  }else { //Else
    //     SlaveStr = Chr(SlaveByte)
    trame[idx++] = SlaveByte ;
  }//End If
  unsigned int i;
  for (  i=0; i < datalen; i++ ) {
    
    BYTE HelpByte = data[i];
    CheckSum = CheckSum ^ HelpByte; // xor
    if ( HelpByte ==  241 ) {
      // SlaveStr = SlaveStr + Chr(240) + Chr(0)
      trame[idx++] = 240 ;
      trame[idx++] = 0 ;
    }else{
      trame[idx++] = HelpByte ;
    }
    
  }
  

  if ( 241 == CheckSum ) // 0xF1
    {
      trame[idx++] = 240 ;
      trame[idx++] = 0 ;
      trame[idx++] = 241 ;

    }
  else if ( 240 == CheckSum ) // 0xF0
    {
      trame[idx++] = 240 ;
      trame[idx++] = 1;
      trame[idx++] = 241 ;
    }
  else{

    trame[idx++] = CheckSum;
    trame[idx++] = 241 ;
  }
  
  
  // emission

  ret = send(plink->sock, trame,  idx, 0);

  if ( ret>0){
								FILE* ftxhex = fopen(TCP_TX_HEX_FILE, "a+b");
								if ( ftxhex ) {
									int k;for (k=0;k< ret; k++){
										tx_total++;
										if (0==(tx_total%16)) fprintf(ftxhex,"\n");
										fprintf(ftxhex,"%2.2X ", trame[k] & 0xff);
									}
									fprintf(ftxhex,"\n");
									fclose(ftxhex);
								}

  }
  //usleep( (unsigned long ) (idx*MILLISECOND));

	//vega_log(INFO,"vega_control_send_alphacom_msg1 returned %d (vega_control_send_alphacom_msg1)\n",ret);

  if ( ret != idx ) {
	  vega_log(INFO,"ERROR tcp sent only %d/%d bytes", ret, idx);
  }else{
	  vega_log(INFO,"OK tcp sent %d/%d bytes", ret, idx);

	  int k;  
	  for (  k =0 ; k < idx; k++){ 
		  printf("tx:%d[0x%02X]\n", (unsigned char)trame[k], trame[k]); 
	  }

  }
  
  plink->tx_seq_num ++;
  if (plink->tx_seq_num > 7)
    plink->tx_seq_num = 1;
  
  /*const char *m = dump_message_serialized(trame, idx);
  if (m) {
    printf("%s\n", m); 
    free ((char*)m);
  }*/
  
  
  if (ret == idx) {
    return 0;
  }


  return -1;
};

vega_control_t *vega_control_create()
{
	  //printf("sizeof(g_link)=%d\n",sizeof(g_link));
	  return &g_link;
}


/* UN FICHIER POUR LES MESSAGES RECUS et UN FICHIER POUR LES MESSAGES ENVOYES */
int vega_control_reinit_link(vega_control_t *plink)
{
	  if (plink == NULL)
		return -1;
  
	  close (plink->sock);
	  int ret = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	  if (ret < 0) {
		vega_log(ERROR,"error link init socket - cause: %s", strerror(errno));
		return -1;
	  }

#ifdef USE_SO_LINGER
			const linger ling = { 1, 3 };
			int rc = setsockopt( ret, SOL_SOCKET, SO_LINGER,		(char *) &ling, sizeof(ling );
			if ( rc != 0 ) {			
				vega_log(ERROR,"error setsockopt(SO_LINGER) - cause: %s", strerror(errno));
			}else{
				vega_log(INFO,"OK setsockopt(SO_LINGER)");
			}
#endif
				

#ifdef USE_TCP_NODELAY
	{
		int dummy = 1;

		vega_log(INFO,"IPPROTO_TCP=%d", IPPROTO_TCP );
		vega_log(INFO,"TCP_NODELAY=%d", TCP_NODELAY );

		int rc = setsockopt( ret, IPPROTO_TCP, TCP_NODELAY,	(char*) &dummy, sizeof(dummy) );
		if ( rc != 0 )
		{
			vega_log(ERROR,"error setsockopt(TCP_NODELAY) - cause: %s", strerror(errno));
		}else{
			vega_log(ERROR,"OK setsockopt(TCP_NODELAY) - %d", rc );
		}
	}
#endif

#ifdef USE_SO_KEEPALIVE
	  {
			/* enable keep alives */
			int tmp = 1;
			int rc = setsockopt( ret, SOL_SOCKET, SO_KEEPALIVE,		(char *) &tmp, sizeof(tmp) );
			if ( rc != 0 ) {			
				vega_log(ERROR,"error setsockopt(SO_KEEPALIVE) - cause: %s", strerror(errno));
			}else{
				vega_log(INFO,"OK setsockopt(SO_KEEPALIVE)");
			}
	  }
#endif

	  
#ifdef USE_SO_RCVBUF
	  {
			/*  */
			int tmp = 1;
			int rc = setsockopt( ret, SOL_SOCKET, SO_RCVBUF,		(char *) &tmp, sizeof(tmp) );
			if ( rc != 0 ) {			
				vega_log(ERROR,"error setsockopt(SO_RCVBUF) - cause: %s", strerror(errno));
			}else{
				vega_log(INFO,"OK setsockopt(SO_RCVBUF)");
			}
	  }
#endif


	  /* set the new socket value */
	  plink->sock = ret;

	  vega_log(INFO,"TCP socket=%d",plink->sock);
	  /* reinit some data */
	  plink->tx_seq_num = 1;
	  plink->rx_next_waited_seqnum = 1;
	  plink->first_ping_sent = 0;
	  plink->tick_ping_pong = 10000000; /* toutes les 10 secondes */
	  plink->wait_duration = KEEP_ALIVE_INTERVAL / 1000;
	  return 0;
}

void vega_control_destroy_connection(vega_control_t *plink)
{
	  if (plink == NULL)
		return;

	  /* arreter la thread de gestion du lien */
	  plink->order_stop = 1;
	  void *ret;

	  pthread_join(plink->rx_thread, &ret);

	  if (plink->state == ST_connected_slave ||
		  plink->state == ST_connected_master ||
		  plink->state == ST_connected_unknown) {
    
		vega_control_reinit_link(plink);
	  }
  
	  /*destroy a given queue*/
	  //vega_queue_destroy(plink->rx_queue);


	  free (plink);
}


int vega_control_init(vega_control_t *plink, char *ip_address, int port) 
{

	  if (!ip_address || !port)
		return -1;
  
	  int ret = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	  if (ret < 0) {
		vega_log(ERROR,"error link init - cause: %s", strerror(errno));
		return -1;
	  }
  
	  plink->sock = ret;
 
	  plink->peer_addr.sin_family = AF_INET;
	  plink->peer_addr.sin_addr.s_addr = inet_addr(ip_address);
	  plink->peer_addr.sin_port = htons(port);
  
	  /* mode initial : on est deconnectÃ© */
	  plink->state = ST_disconnected;
	  /* seqnum commence Ã  1 */
	  plink->tx_seq_num = 1;
	  /* timer de tentative de reconnectionn Ã  1 min */
	  plink->timer = 60;
	  plink->first_ping_sent = 0;
	  plink->continue_recv_current_frame = 0;

	  /* initiliaze le numero de sequence attendu en reception Ã  1 */
	  plink->rx_next_waited_seqnum = 1;

	  /*init the mutex*/
	  pthread_mutex_init(&plink->mutex_recv, NULL);

	  plink->tick_ping_pong = 10000000; /* toutes les 10 secondes */
	  plink->wait_duration = KEEP_ALIVE_INTERVAL / 1000; 

	  ret = pthread_create(&plink->rx_thread, NULL, vega_control_handle_link_thread, plink);
	  if (ret < 0) {
		vega_log(CRITICAL,"error link thread create - cause: %s", strerror(errno));
		return -1;
	  }

	  ret = pthread_create(&plink->tx_thread, NULL, vega_control_handle_send_thread, plink);
	  if (ret < 0) {
		vega_log(CRITICAL,"error link thread create - cause: %s", strerror(errno));
		return -1;
	  }

  
	  return 0;
}


int control_call_back(vega_event_t* evt)
{
	static int crm_number=1; // D1

  switch (evt->type) {
  case ALPHACOM_LINK_UP:{
		init_when_link_is_up();
    }
    break;

  case ALPHACOM_CRM4_INPUT:
    /* evenement appui de touche sur un poste CRM4 */
    /* 1.trouver le device */
    {

		
		int device_number  = 0;
		BYTE k = evt->event.input_event.board_number;

		int board_number = (k >> 3) & 0xff;
		int port_number = k & 0x07;
		if (board_number > 1)
			device_number = ( board_number - 1 ) * 5 + 1;
      
		device_number += port_number  + 1;
		
		device_t *d = device_t::by_number(device_number);

		CRM4* crm = dynamic_cast<CRM4*>(d);
		if (crm == NULL) {
			vega_log(VEGA_LOG_ERROR, "no device for input_board_number D%d", device_number);
		}else {
			crm->process_input_key(evt->event.input_event.input_number);
		}
    }
    break;

  case ALPHACOM_TIMER:{
		  printf("ALPHACOM_TIMER\n");
		  vega_test_pong_delay();
	}
	break;

  case ALPHACOM_PING_SENT:{
		  printf("ALPHACOM_PING_SENT\n");
	  }
	  break;
  }
  return 0;
}

int vega_control_set_non_blocking_mode(vega_control_t *plink)
{
  
	  int flags;
	  /* Set socket to non-blocking mode */
  
	  if((flags = fcntl(plink->sock, F_GETFL, 0)) < 0) {
		vega_log(CRITICAL, "error getting the socket flags - cause : %s", strerror(errno));
		return -1;
	  } 
  
	  if((fcntl(plink->sock, F_SETFL, flags | O_NONBLOCK)) < 0) { 
		vega_log(CRITICAL, "error setting non blocking mode - cause : %s", strerror(errno));
		return -1;
	  }
	  return 0;
} 
	
//-------------------------------------------------
int vega_control_set_blocking_mode(vega_control_t *plink)
{
	  int flags;
	  /* Set socket to blocking mode */ 
  
	  if((flags = fcntl(plink->sock, F_GETFL, 0)) < 0) {
		vega_log(CRITICAL, "error getting the socket flags - cause : %s", strerror(errno));
		return -1;
	  } 
  
	  if (fcntl(plink->sock, F_SETFL, flags & (~O_NONBLOCK)) < 0) {
		vega_log(CRITICAL, "error setting blocking mode - cause : %s", strerror(errno));
		return -1;
	  }
  
	  return 0;
}

/* 
   essai de lire un message en mode non bloquant pendant secs secondes 
   retourne -1 si erreur , 0 si perte de connexion et > 0 si caracteres lus
*/
int vega_control_recv_message_nonbloquant(vega_control_t *plink, int milli)
{
	  fd_set rfds;
	  struct timeval tv;
	  int retval;
  

	  FD_ZERO(&rfds);


	  /* Pendant 5 secondes maxi */
	  tv.tv_sec = 0;
	  tv.tv_usec = milli * 1000 ;
  

	  FD_SET(plink->sock, &rfds);
	  retval = select(plink->sock + 1, &rfds, NULL, NULL, &tv);
  
	  if (retval == -1) {
		perror("select()");
		return -1;
	  }
	  else if (retval) {
    
		if (FD_ISSET(plink->sock, &rfds))
		  printf("FD_ISSET plink->sock\n");
    
		return vega_control_recv_message(plink);
	  }
	  else 
		return -2;


}

/* 
fd_set SocketSet;
FD_ZERO (&SocketSet);      // clears SocketSet
FD_SET (m_Socket, &SocketSet); // adds a stream
int nError;
 
nError = select (m_Socket + 1, &SocketSet, NULL, NULL, NULL);
if(nError == -1)
	return 0; //connection Lost
 
if(0 >= recv(m_Socket, (char*) &dwThreadID, sizeof(DWORD), 0))
	return 0; // If Connection Lost

je rajouterais 2 ou 3 choses :

    * si il y a des read ou write, une erreur peut se traduire par un crash
    * un socket peut-être "gelé", c'est à dire qu'il n'est "ni mort, ni vivant" du point de vue du client et/ou serveur
    * l'un des 2 peut être mort par un kill, un ctrl C, etc..
    * faire un shutdown du socket est propre

Si sur unixoide, tester par un poll avant de tenter une lecture/écriture/rec/send. Et ajouter les détections de signal (SIGTERM, SIGQUIT, SIGKILL, SIGHUP, SIGINT)

Avoir un code qui gère tous les cas est assez pénible, mais une fois que ça y est, c'est pour tout le monde

L'avantage est aussi une détection et re-démarrage quasi instantanée.. 


lit sur la socket jusque recevoir un message   En cas d'echec (perte de lien), on retourne -1, sinon 0 (un message a ete recu) */
int vega_control_recv_message(vega_control_t *plink)
{
	  BYTE carcou;
	  int ret;

	  pthread_mutex_lock(&plink->mutex_recv);
  
	  /* initialize le message courant a une taille de 0 on va remplir le buffer element par element */
	  if (plink->continue_recv_current_frame == 1) {
		plink->continue_recv_current_frame = 0;
	  }
	  else {
		plink->current_frame_size  = 0;
	  }

	  while (1)
	  {
		/* receive socket */
		ret = recv(plink->sock, &carcou, 1, 0);

		if (1==ret){
			/*qlist_monitor_rx_bytes_mutex.lock();
			qlist_monitor_rx_bytes.append(carcou);
			qlist_monitor_rx_bytes_mutex.unlock();*/


								FILE* frxhex = fopen(TCP_RX_HEX_FILE, "a+b");
								if ( frxhex ) 
								{
									rx_total++;
									fprintf(frxhex,"%2.2X ",carcou & 0xff);
									if (carcou==0xF1)//(0==(rx_total%32)) 
										fprintf(frxhex,"\n");
									fclose(frxhex);	
								}

		}
    
		//printf("recv returned %d bytes %02X\n",ret, carcou);
		//vega_log(INFO,"recv returned %d bytes %02X\n",ret, carcou);

		if (ret <= 0) {
		  perror("recv()\n");
		  vega_log(ERROR, "recv() current_frame_size=%d",plink->current_frame_size);
		  plink->current_frame_size = 0;
		  pthread_mutex_unlock(&plink->mutex_recv);

			fprintf(stderr,"recv error (acceptable when going slave mode)\n");

		  return -1;
		}

   
		if (carcou == EOMSG) 
		{

		  if (plink->current_frame_size == 0) { /* fin de message mais pas de donnÃ©e */

			  //vega_log(INFO, "receive EOMSG without any payload");

			  continue;
		  }
      
		  // respect current_frame limit
		  if ( plink->current_frame_size < sizeof(plink->current_frame) -1 ){ 
			plink->current_frame[plink->current_frame_size++] = carcou;      
		  }else{
			  vega_log(ERROR, "current_frame_size exceeded %d (max:%d)", plink->current_frame_size, sizeof(plink->current_frame));
		  }

#ifdef LOG_TCP_2_FILE_ON
			FILE* frx = fopen(TCP_RX_FILE, "a+b");
			if ( frx ) {
				fwrite( &carcou, 1,1, frx);
				fclose(frx);
			}
#endif

		  /* nouvel evenement : un message de taille current_frame_size */
		  //msg_log(vega_message_direction_in, plink->current_frame, plink->current_frame_size);

		  break;
		}

		  // respect current_frame limit
		  if ( plink->current_frame_size < sizeof(plink->current_frame) -1 ){ 
			plink->current_frame[plink->current_frame_size++] = carcou;
			//vega_log(INFO, "current_frame_size[%d]=%02X",plink->current_frame_size-1,carcou);
		  }else{
			  vega_log(ERROR, "current_frame_size exceeded %d (max:%d)", plink->current_frame_size, sizeof(plink->current_frame));
		  }
		//vega_log(ERROR,"carcou = 0x%02x (current frame size = %d)" , carcou & 0xff, plink->current_frame_size);
	  }




	  pthread_mutex_unlock(&plink->mutex_recv);
	  return 0; 
}



/*
09:12:57.695 => NEW STATE : ST_connected_slave
09:12:57.750 scanning boards to check presence ...
09:12:57.750 scanning board 1 ...
09:12:57.750 OK tcp sent 5/5 bytes
09:12:57.769 received_data 11/1024
09:12:57.769 board number 1 present
09:12:57.769 backpanel.boards[0].configuration.present=1 
09:12:57.769 scanning board 2 ...
09:12:57.769 OK tcp sent 5/5 bytes
09:12:57.779 received_data 11/1024
09:12:57.780 board number 2 present
09:12:57.780 backpanel.boards[1].configuration.present=1 
09:12:57.780 scanning board 17 ...
09:12:57.780 OK tcp sent 5/5 bytes
09:12:57.781 received_data 11/1024
09:12:57.781 board number 17 present
09:12:57.781 backpanel.boards[16].configuration.present=0 
09:12:57.781 scanning board 18 ...
09:12:57.781 OK tcp sent 5/5 bytes
09:12:57.783 received_data 11/1024
09:12:57.783 board number 18 present
09:12:57.783 backpanel.boards[17].configuration.present=0 
09:12:57.783 scanning board 19 ...
09:12:57.783 OK tcp sent 5/5 bytes
09:12:57.784 received_data 11/1024
09:12:57.784 board number 19 present
09:12:57.784 backpanel.boards[18].configuration.present=0 
09:12:57.785 scanning board 20 ...
09:12:57.785 OK tcp sent 5/5 bytes
09:12:57.786 received_data 11/1024
09:12:57.786 board number 20 present
09:12:57.786 backpanel.boards[19].configuration.present=0 
09:12:57.786 scanning board 22 ...
09:12:57.786 OK tcp sent 5/5 bytes
09:12:57.787 received_data 11/1024
09:12:57.787 board number 22 present
09:12:57.788 backpanel.boards[21].configuration.present=0 
09:12:57.788 scanning board 23 ...
09:12:57.788 OK tcp sent 5/5 bytes
09:12:57.789 received_data 11/1024
09:12:57.790 board number 23 present
09:12:57.790 backpanel.boards[22].configuration.present=0 
09:12:57.790 scanning board 24 ...
09:12:57.790 OK tcp sent 5/5 bytes
09:12:57.791 received_data 11/1024
09:12:57.791 board number 24 present
09:12:57.791 backpanel.boards[23].configuration.present=0 
09:12:57.791 scanning board 25 ...
09:12:57.791 OK tcp sent 5/5 bytes
09:12:57.793 received_data 11/1024
09:12:57.793 board number 25 present
09:12:57.793 backpanel.boards[24].configuration.present=0 
09:12:57.846 initialize ALST and AGA boards...


09:23:13.276 => NEW STATE : ST_connected_slave
09:23:13.326 scanning boards to check presence ...
09:23:13.326 scanning board 1 ...
09:23:13.326 OK tcp sent 5/5 bytes
09:23:13.334 received_data 11/1024
09:23:13.334 board number 1 present
09:23:13.334 backpanel.boards[0].configuration.present=1 
09:23:13.334 scanning board 2 ...
09:23:13.334 OK tcp sent 5/5 bytes
09:23:13.344 received_data 11/1024
09:23:13.344 board number 2 present
09:23:13.344 backpanel.boards[1].configuration.present=1 
09:23:13.344 scanning board 3 ...
09:23:13.344 OK tcp sent 5/5 bytes
09:23:15.342 timeout sur la lecture board 3
09:23:15.342 board number 3 not present
09:23:15.342 Anomaly on board 3
09:23:15.343 scanning board 4 ...
09:23:15.343 OK tcp sent 5/5 bytes
09:23:17.342 timeout sur la lecture board 4
09:23:17.342 board number 4 not present
09:23:17.342 Anomaly on board 4
09:23:17.342 scanning board 17 ...
09:23:17.343 OK tcp sent 5/5 bytes
09:23:17.344 received_data 11/1024
09:23:17.344 board number 17 present
09:23:17.344 backpanel.boards[16].configuration.present=0 
09:23:17.344 scanning board 18 ...
09:23:17.344 OK tcp sent 5/5 bytes
09:23:17.346 received_data 11/1024
09:23:17.346 board number 18 present
09:23:17.346 backpanel.boards[17].configuration.present=0 
09:23:17.346 scanning board 19 ...
09:23:17.347 OK tcp sent 5/5 bytes
09:23:17.348 received_data 11/1024
09:23:17.348 board number 19 present
09:23:17.348 backpanel.boards[18].configuration.present=0 
09:23:17.348 scanning board 20 ...
09:23:17.348 OK tcp sent 5/5 bytes
09:23:17.349 received_data 11/1024
09:23:17.349 board number 20 present
09:23:17.349 backpanel.boards[19].configuration.present=0 
09:23:17.349 scanning board 22 ...
09:23:17.349 OK tcp sent 5/5 bytes
09:23:17.351 received_data 11/1024
09:23:17.351 board number 22 present
09:23:17.351 backpanel.boards[21].configuration.present=0 
09:23:17.352 scanning board 23 ...
09:23:17.352 OK tcp sent 5/5 bytes
09:23:17.353 received_data 11/1024
09:23:17.353 board number 23 present
09:23:17.353 backpanel.boards[22].configuration.present=0 
09:23:17.353 scanning board 24 ...
09:23:17.353 OK tcp sent 5/5 bytes
09:23:17.354 received_data 11/1024
09:23:17.354 board number 24 present
09:23:17.355 backpanel.boards[23].configuration.present=0 
09:23:17.355 scanning board 25 ...
09:23:17.355 OK tcp sent 5/5 bytes
09:23:17.357 received_data 11/1024
09:23:17.357 board number 25 present
09:23:17.357 backpanel.boards[24].configuration.present=0 
09:23:17.406 initialize ALST and AGA boards...
09:23:17.406 clear error on board 1
09:23:17.406 OK tcp sent 5/5 bytes
09:23:17.406 clear busy on board 1
09:23:17.406 OK tcp sent 5/5 bytes
09:23:17.406 clear error on board 2
09:23:17.406 OK tcp sent 5/5 bytes
09:23:17.407 clear busy on board 2
09:23:17.407 OK tcp sent 5/5 bytes
09:23:17.407 clear error on board 17
09:23:17.407 OK tcp sent 5/5 bytes
09:23:17.407 clear busy on board 17
09:23:17.407 OK tcp sent 5/5 bytes
09:23:17.407 clear error on board 18
09:23:17.407 OK tcp sent 5/5 bytes
09:23:17.407 clear busy on board 18
09:23:17.407 OK tcp sent 5/5 bytes
09:23:17.407 clear error on board 19
09:23:17.407 OK tcp sent 5/5 bytes
09:23:17.407 clear busy on board 19
09:23:17.407 OK tcp sent 5/5 bytes
09:23:17.407 clear error on board 20
09:23:17.407 OK tcp sent 5/5 bytes
09:23:17.407 clear busy on board 20
09:23:17.407 OK tcp sent 5/5 bytes
09:23:17.407 clear error on board 22
09:23:17.408 OK tcp sent 5/5 bytes
09:23:17.408 clear busy on board 22
09:23:17.408 OK tcp sent 5/5 bytes
09:23:17.408 clear error on board 23
09:23:17.408 OK tcp sent 5/5 bytes
09:23:17.408 clear busy on board 23
09:23:17.408 OK tcp sent 5/5 bytes
09:23:17.408 clear error on board 24
09:23:17.408 OK tcp sent 5/5 bytes
09:23:17.408 clear busy on board 24
09:23:17.408 OK tcp sent 5/5 bytes
09:23:17.408 clear error on board 25
09:23:17.408 OK tcp sent 5/5 bytes

*/

void *vega_control_handle_link_thread(void *arg)
{
	static int AG_Led_initialised = 0;
  vega_control_t *plink = (vega_control_t*) arg;
  //BYTE byte;
  BYTE msg[128];
  int ret;
  int tick = 50000;
  int sleep_value = 0;
  int clock_ping_pong = 0;
  fd_set rfds;
  struct timeval tv;
  int retval;
  
  int _tstart_pong_init_done = 0;

  /*if ( 0==_tstart_pong_init_done ) {
	  _tstart_pong_init_done=1;
	  gettimeofday(&_tstart_pong, NULL);
  }*/

  FD_ZERO(&rfds);

  while (1) {

    FD_ZERO(&rfds);


    //printf(" Pendant 5 secondes maxi \n");
    tv.tv_sec = 0;
    tv.tv_usec = tick;
    if (plink->state != ST_disconnected) {
      FD_SET(plink->sock, &rfds);
      retval = select(plink->sock + 1, &rfds, NULL, NULL, &tv);
    }
    else {
      retval = select(0, NULL, NULL, NULL, &tv);
    }

    if (retval == -1) {
      perror("select()");
      return NULL;
    }
    else if (retval) {
      if (FD_ISSET(plink->sock, &rfds)) {
		  //printf("%d rx on socket %d\n", retval, plink->sock); 
      }
    }
    else  
	{

		  if (plink->state == ST_connected_slave) {
		


		
			plink->clock_100_milliseconds += tick;
			clock_ping_pong += tick;
			
			if (clock_ping_pong >= plink->tick_ping_pong) 
			{
			  
			  BYTE a_byte = ICCMT_CC_PING;
			  //printf("send ping\n");
			  if (vega_control_send_alphacom_msg(VEGA_URGENT,plink, &a_byte, 1) < 0) 
			  {
				vega_log(ERROR, "Ping sent failed - cause : %s", strerror(errno));
				/* ici on ne touche pas Ã  l'etat de la connection, l'erreur sera
				   detectÃ©e dans vega_control_recv_message */
			  }
			  vega_log(VEGA_LOG_INFO, "vega_control_handle_link_thread: Ping sent");
			  clock_ping_pong = 0;

				vega_event_t event;
				event.type = ALPHACOM_PING_SENT;
				control_call_back( &event);

			}
		
			//if (plink->clock_100_milliseconds >= 100000) {
			int x = 2;
			if (plink->clock_100_milliseconds >= (x*1000000) ) {
				// pdl 20090519: every x seconds, refresh CRM D%d			  
			  if (plink->state == ST_connected_slave) {

				vega_event_t event;
				event.type = ALPHACOM_TIMER;
				control_call_back( &event);
			  }
			  plink->clock_100_milliseconds = 0;

			}
			continue;
		  }


      
    }
    
    if (plink->order_stop == 1) {
		  return NULL;
    }
    
   
        

    if (plink->state == ST_testing_boards) 
	{
	
		printf("on scanne les cartes...\n");
      int i;
      int board_number;
      
      vega_log(INFO, "scanning boards to check presence ...");
      
      //for (i = 0; i < backpanel.nb_boards; i++) 
      //for (i = 1; i <= MAX_BOARD; i++) // 1 a 24
      for (i = 1; i <= 26; i++) // 1 a 26 : remarque de Hans et Axel 20091223
	  {
			

		  board_number=i;
			vega_log(INFO, "scanning board %d ...", board_number);
			printf( "scanning board %d ...\n", board_number);

			msg[0] = ICCMT_BOARD_SIMPLE_SCAN;
			msg[1] = board_number;
	
			int bytes_sent = 0;
			if (vega_control_send_alphacom_msg1(plink, msg, 2) < 0)
			{
			  plink->state = ST_disconnected;
			  vega_log(ERROR, "=> NEW STATE : ST_disconnected");	
			  vega_control_reinit_link(plink); 
			  break;
			}else{
			}

			printf("en attente pendant 1 secondes...\n");
			ret = vega_control_recv_message_nonbloquant(plink, 500);
	
			if  (ret == -1) {
			  vega_log(ERROR,"cnx loosed (cause : %s) during scan board %d", strerror(errno), board_number);
			  plink->state = ST_disconnected;
			  vega_log(ERROR, "=> NEW STATE : ST_disconnected");	
			  vega_control_reinit_link(plink);
			}
			else if (ret == -2) 
			{
			  printf("timeout sur la lecture board %d\n",board_number );
			  vega_log(CRITICAL, "board number %d not present", board_number);	
	  
			  map_boards[board_number] = false;

			}
			else 
			{
	
			printf("On a recu un message\n");

			unsigned char data[1024]={0};
			int data_len = vega_control_get_next_msg_in_current_frame(plink, data, sizeof(data) );
			if ( data_len)
			  {
				if (data[0] == ICCMT_BOARD_AVAILABLE) 
				{
				  vega_log(INFO, "board number %d present", board_number);

				  map_boards[board_number] = true;

				  vega_alarm_log(EV_ALARM_END_BOARD_ANOMALY, board_number, board_number, 
					  "carte %d presente au demarrage", board_number); // pdl: carte verte sur ihm 

				}
			}
			else 
			{
			vega_log(ERROR, "received invalid message during scan board %d", board_number);
			i --; 		/* permet de refaire le scan */
		  }  
		}
      }
	  
	  fprintf(stderr,"<-vega_control_handle_link_thread: fin detection des cartes VEGA\n");

      /* TODO : que faire en cas d'erreur ??? */
      plink->state = ST_init_boards;
    }
    else if (plink->state == ST_init_boards) 
	{
      int i;
      int board_number;
      //board_type_t board_type;
      
      vega_log(VEGA_LOG_INFO, "initialize ALST and AGA boards...");


      for (i = 1; i <= MAX_BOARD; i++) // 1 a MAX_BOARD=24
	  {
		  board_number=i;
      //for (i = 0; i < backpanel.nb_boards; i++) 
	
		//if (backpanel.boards[i].configuration.present == 0)  {continue;}
		
		//board_number = backpanel.boards[i].configuration.place;
		//board_type = backpanel.boards[i].configuration.type;
		int k;
		int msg_size;

		for (k = 0; k < 2 ; k ++) {
	  
		  if (k == 0) {
			vega_log(INFO, "clear error on board %d", board_number);
			msg[0] = ICCMT_BOARD_CLEAR_ERROR;
			msg[1] = board_number;
			msg_size = 2;
		  }
		  else {
			if (0)//board_number>=25)//board_type == board_alphacom) 
			{
			  
			  vega_log(VEGA_LOG_INFO, "set alphacom (board number %d) as busy", board_number);	      
			  msg[0] = ICCMT_BOARD_SET_BUSY;
			  msg[1] = board_number;
			  msg_size = 2;
			}
			else //if (board_type == board_aga || board_type == board_aslt) 
			{
			  vega_log(VEGA_LOG_INFO, "clear busy on board %d", board_number);	      
			  msg[0] = ICCMT_BOARD_CLEAR_BUSY;
			  msg[1] = board_number;
			  msg_size = 2;
			}
		  }
		  
		  if (msg_size>0 && vega_control_send_alphacom_msg1(plink, msg, msg_size) < 0)
		  {
			plink->state = ST_disconnected;
			vega_log(ERROR, "=> NEW STATE : ST_disconnected");	
			vega_control_reinit_link(plink);
			break;
		  }

		  
		}

      }// for 1 a 24

		/*On the AMC (25)

		Set Error - red flashing
		Clear Error - Steady green
		Set Busy - green flashing
		Clear Busy - Steady green
 
		On the APC(26):
		Set Error - red flashing
		Clear Error - steady green	  
	  
Here is our sequence of board settings:

We are send Clar_Error and Clar_Busy to all boards, except  board 25 (AMC)

To board 25 we send Clar_Error and Set_Busy.

 

For example:  we have AGA on boards 19..22,  AMC and APC

 

12:54:54.328 SEND = 4C 50 13 0F F1   cmd= BoardClearBusy  B=19

12:54:54.328 SEND = 50 52 13 11 F1    cmd= BoardClearError B=19

12:54:54.328 SEND = 54 50 14 10 F1    cmd= BoardClearBusy  B=20

12:54:54.328 SEND = 58 52 14 1E F1    cmd= BoardClearError B=20

12:54:54.328 SEND = 5C 50 15 19 F1    cmd= BoardClearBusy  B=21

12:54:54.328 SEND = 44 52 15 03 F1    cmd= BoardClearError B=21

12:54:54.328 SEND = 48 50 16 0E F1    cmd= BoardClearBusy  B=22

12:54:54.328 SEND = 4C 52 16 08 F1    cmd= BoardClearError B=22

12:54:54.328 SEND = 50 51 19 18 F1    cmd= BoardSetBusy    B=25

12:54:54.328 SEND = 54 52 19 1F F1    cmd= BoardClearError B=25

12:54:54.328 SEND = 58 50 1A 12 F1    cmd= BoardClearBusy  B=26

12:54:54.328 SEND = 5C 52 1A 14 F1   cmd= BoardClearError B=26	  
	  */
	
	  // led verte sur carte mere 
	  {
		  vega_log(VEGA_LOG_INFO, "INIT CARTE 25");	      
		  msg[0] = 0x51;//ICCMT_BOARD_CLEAR_ERROR;
		  msg[1] = 0x19;
		  vega_control_send_alphacom_msg1(plink, msg, 2)	;
		  msg[0] = 0x52;//ICCMT_BOARD_CLEAR_ERROR;
		  msg[1] = 0x19;
		  vega_control_send_alphacom_msg1(plink, msg, 2)	;
	  }

	  {
		  vega_log(VEGA_LOG_INFO, "INIT CARTE 26");	      
		  msg[0] = 0x50;//ICCMT_BOARD_CLEAR_ERROR;
		  msg[1] = 0x1A;
		  vega_control_send_alphacom_msg1(plink, msg, 2)	;
		  msg[0] = 0x52;//ICCMT_BOARD_CLEAR_ERROR;
		  msg[1] = 0x1A;
		  vega_control_send_alphacom_msg1(plink, msg, 2)	;
	  }
      
      plink->state = ST_connected_slave;

	  vega_alarm_log(EV_ALARM_CNX_ALPHACOM_CONTROL_UP, 0,0,"connexion pc control<->ALPHACOM mode SLAVE   version "__DATE__" "__TIME__);

	  gettimeofday(&_tstart_pong, NULL);

		//vega_event_log(EV_LOG_VEGA_CONTROL_NETWORK_UP, 0,0,0, __FILE__" ST_init_boards -> ST_connected_slave" );
		//vega_alarm_log(EV_ALARM_CONTROL_NETWORK_UP, 0,0,"attente connexion avec alphacom en mode esclave..." );

    }
    else if (plink->state == ST_connected_slave) 
	{

		//printf("ST_connected_slave\n");
  
		if (plink->first_ping_sent == 0) 
		{
			/* on envoie le premier		ping */
			BYTE a_byte = ICCMT_CC_PING;
			if (vega_control_send_alphacom_msg(VEGA_URGENT,plink, &a_byte, 1) < 0) 
			{
				vega_log(ERROR, "Ping sent failed - cause : %s", strerror(errno));
				/* ici on ne touche pas a l'etat de la connection, l'erreur sera
				detectee dans vega_control_recv_message */	
			}
			vega_log(INFO, "vega_control_handle_link_thread: Ping sent");
			plink->first_ping_sent = 1;
			//if (plink->callback) 
			{
			  vega_event_t evt;
			  evt.type = ALPHACOM_LINK_UP;
			  control_call_back(&evt);
			}

			//vega_alarm_log(EV_ALARM_CNX_ALPHACOM_CONTROL_UP, 0,0,"connexion ethernet CONTROL <-> alphacom etablie %s:%d...\n",inet_ntoa(plink->peer_addr.sin_addr),ntohs(plink->peer_addr.sin_port));

		}

		ret = vega_control_recv_message(plink);
      
		if  (ret == -1) 
		{
			vega_log(ERROR,"TCP cnx lost (cause : %s)", strerror(errno));
			plink->state = ST_disconnected;
			vega_log(ERROR, "=> NEW STATE : ST_disconnected");	
			vega_control_reinit_link(plink);

			fprintf(stderr,"TCP cnx lost (cause : %s)\n", strerror(errno));
			syslog(LOG_CRIT|LOG_PID,"TCP cnx lost (cause : %s)\n", strerror(errno));

		  vega_alarm_log(EV_ALARM_CNX_ALPHACOM_CONTROL_DOWN, 0,0,"Deconnexion Alphacom <-> PC Controle");
		  vega_alarm_log(EV_ALARM_CNX_IHM_CONTROL_DOWN, 0,0,"Deconnexion IHM <-> PC Controle");

			critical_dump();
			
			exit(-8);
		  
		}
		else
		{

			vega_event_t event;
			
			unsigned char data[1024]={0};
			int data_len = vega_control_get_next_msg_in_current_frame(plink, data, sizeof(data) );
			if ( data_len)
			{	/* On a recu un message  */

				int device_number  = 0;
				BYTE k = data[1] & 0xff;

				int board_number = (k >> 3) & 0xff;
				int port_number = k & 0x07;
				if (board_number > 1)
					device_number = ( board_number - 1 ) * 5 + 1;

				device_number += port_number  + 1;

				if (data[0] != ICCMT_CC_PONG) {
					vega_log(INFO, "ICCMT_BP_MSG CRM4 D%d B%d input code data[0]=%02X data[1]=%02X\n", device_number, board_number, data[0] , data[1] );
				}

				if (data[0] == ICCMT_CC_PONG) 
				{
					vega_log(DEBUG, "received pong data[0]=%02X", (unsigned char)data[0]);	 	
					// positionner la date du dernier pong recu !
					gettimeofday(&_tstart_pong, NULL); // memoriser pour la prochaine comparaison

				}
				else if (data[0] == ICCMT_BP_MSG_ICCO_UNIT)  
				{ // 0x8E

					  event.type = ALPHACOM_CRM4_INPUT;
					  event.event.input_event.board_number = data[1] & 0xff;
					  /* get the input code */
					  {
						  if (data[3] == 0x40 ) { // 64 decimal
							  /* key input */
							  event.event.input_event.input_number = data[5] & 0xff;
							  control_call_back( &event);
						  }
						  /*else if (data[3] == 0x41) { // 65 decimal
						  event.event.input_event.input_number = data[5] & 0xff;
						  control_call_back( &event);
						}*/
						else if (data[3] == 68) {
						  /* Micro input */
						  event.event.input_event.input_number = DEVICE_KEY_MICRO_PRESSED;
						  control_call_back( &event);
						}
						else if (data[3] == 69) {
						  /* Micro input */
						  event.event.input_event.input_number = DEVICE_KEY_MICRO_RELEASED;
						  control_call_back( &event);
						}
						else if (data[3] == 66) {
						  /* Cancel input */
						  event.event.input_event.input_number = DEVICE_KEY_CANCEL;
						  control_call_back( &event);

						}else if (data[3] == 0x4A) {


							/*
							int crm4_number = data[1] - 7; // D7->0x10 D6 -> 0x0D D4->0x0B D3->0x0A D2->0x09 D1->0x08
							printf("ALARM PLUGGED CRM4 D%d (%02X %02X)\n", crm4_number, data[1] , data[3] );
							vega_log(INFO,"ALARM PLUGGED CRM4 D%d (%02X %02X)\n", crm4_number, data[1] , data[3] );
							vega_alarm_log(EV_ALARM_END_DEVICE_ANOMALY, crm4_number, -1, "fin anomalie CRM4 %d", crm4_number); 

  
							*/

							event.event.input_event.input_number = DEVICE_PLUGGED;
							control_call_back( &event);

							

						}
						else if (data[3] == 0x4B) 
						{
							event.event.input_event.input_number = DEVICE_UNPLUGGED;
							control_call_back( &event);

						}
						else if ( data[3] == ICCMT_STATION_UNAVAILABLE )
						{
							vega_log(INFO,"NOT PRESENT CRM4 input code data[1]==%02X data[2]==%02X data[3]==%02X", (unsigned char)data[1],(unsigned char)data[2], (unsigned char)data[3]);
							
							event.event.input_event.input_number = DEVICE_NOT_PRESENT;
							control_call_back( &event);
						}
						else if ( data[3] == 0x47 ) // pdl 20091107
						{
							vega_log(INFO,"AGAIN PRESENT CRM4 input code data[1]==%02X data[2]==%02X data[3]==%02X", (unsigned char)data[1],(unsigned char)data[2], (unsigned char)data[3]);
							
							event.event.input_event.input_number = DEVICE_PRESENT;
							control_call_back( &event);
						}
						else
						{
							vega_log(INFO,"unhandled CRM4 input code data[1]==%02X data[2]==%02X data[3]==%02X", (unsigned char)data[1],(unsigned char)data[2], (unsigned char)data[3]);
						}	
					  
					  }// get input code
					
			
				}
				else if (data[0] == ICCMT_BOARD_NOT_AVAILABLE) // 0x4E
				{

					int board_number = data[1] & 0xff;
					map_boards[board_number ] = false;
				
					vega_alarm_log(EV_ALARM_BOARD_ANOMALY, board_number, board_number, 
						"Debut anomalie sur la carte %d", board_number); // pdl: carte rouge sur ihm  
					fprintf(stderr,"board number %d !!!!!!!!!!!!!\n", board_number);
				}
				
				else if (data[0] == ICCMT_BOARD_AVAILABLE) // 0x4C
				
				{
					int board_number = data[1] & 0xff;
					map_boards[board_number] = true;
				  
					vega_alarm_log(EV_ALARM_END_BOARD_ANOMALY, board_number, board_number, 
						"Fin anomalie sur la carte %d", board_number); // pdl: carte verte sur ihm 
					fprintf(stderr,"board number %d !!!!!!!!!!!!!\n", board_number);

					{
						unsigned char msg[2];
						vega_log(INFO, "clear error on board %d", board_number);
						msg[0] = ICCMT_BOARD_CLEAR_ERROR;
						msg[1] = board_number;
						vega_control_send_alphacom_msg1(plink, msg, 2); // todo: verifier si c'est la bonne fonction

						vega_log(INFO, "clear busy on board %d", board_number);	      
						msg[0] = ICCMT_BOARD_CLEAR_BUSY;
						msg[1] = board_number;
						vega_control_send_alphacom_msg1(plink, msg, 2); // todo: verifier si c'est la bonne fonction

					}
					// pdl 20091005: si des conferences utilisent cette carte, faire l'init mixers
					foreach(vega_conference_t *c,vega_conference_t::qlist ) {
						if ( board_number == c->matrix_configuration->board_number ) {
							vega_log(INFO, "Carte %d OK: Remise en fonctionnement de C%d",board_number, c->number);
							c->init_mixers(); 
						}
					}

				}
				else 
				{
					fprintf(stderr,"unhandled ICCMT_BP_MSG input code data[0]=%02X data[1]=%02X\n", data[0] , data[1] );
					vega_log(INFO, "unhandled ICCMT_BP_MSG input code data[0]=%02X data[1]=%02X\n", data[0] , data[1] );
				}
			}// if datalen
		}// else
    }
    else if (plink->state == ST_disconnected) 
	{	/* !!! ETAT initial du LIEN !!! */
      int counter = 0;
      while (counter  < plink->timer) 
	  {
	
		if (counter)
		  vega_log(VEGA_LOG_DEBUG, "try again connection with alphacom - address : %s:%d...", 
			   inet_ntoa(plink->peer_addr.sin_addr),
			   ntohs(plink->peer_addr.sin_port));
		else {
		  vega_log(VEGA_LOG_DEBUG, "try to connect with alphacom - address : %s:%d...", 
			   inet_ntoa(plink->peer_addr.sin_addr),
			   ntohs(plink->peer_addr.sin_port));
	
		}
	

		printf("try connect alphacom:%s:%d...\n",inet_ntoa(plink->peer_addr.sin_addr),ntohs(plink->peer_addr.sin_port));

		if (connect(plink->sock, (struct sockaddr *) &plink->peer_addr, sizeof(plink->peer_addr)) < 0) {
		  vega_log(ERROR,"connect error - cause: %s", strerror(errno));
		  printf("connect error - cause: %s\n", strerror(errno));
		}
		else {

			//vega_alarm_log(EV_ALARM_CNX_ALPHACOM_CONTROL_UP, 0,0,"connexion ethernet CONTROL <-> alphacom etablie %s:%d...\n",inet_ntoa(plink->peer_addr.sin_addr),ntohs(plink->peer_addr.sin_port));
			//vega_event_log(EV_LOG_VEGA_CONTROL_NETWORK_UP, 0,0,0,"-");
		  
			vega_log(INFO,"connexion ethernet CONTROL <-> alphacom etablie %s:%d...\n",inet_ntoa(plink->peer_addr.sin_addr),ntohs(plink->peer_addr.sin_port));
		  plink->state = ST_connected_unknown;
		  vega_log(VEGA_LOG_DEBUG, "==> NEW STATE : ST_connected_unknown");
		  /* on attend 5 seconds (que le soft soit pret ???) */
		  sleep (5); 
		  break;
		}
		
		if (plink->order_stop == 1) {
		  return NULL;
		}

		sleep(3);		/* attente avant nouvelle connexion */
		counter ++;
      }/* while */
      
      if (counter == plink->timer && plink->state == ST_disconnected) 
	  {
		vega_log(CRITICAL, "!!! DEFINITIVELY disconnected with the alphacom !!!");
		plink->state = ST_definitely_disconnected;
		/* TODO: on fait quoi dans ce cas ? */
		vega_alarm_log(EV_ALARM_CNX_ALPHACOM_CONTROL_DOWN, 0,0,"deconnexion ethernet avec alphacom definitive" );
      }
      

    }
    else if (plink->state == ST_disconnected_pending_slave) 
	{
      /* cet etat n'est pas indispensable : mais prefere le garder pour des questions de debug.
       On pourrait utiliser l'etat ST_disconnected*/
      int counter = 0;
      
      while (counter  < plink->timer) 
	  {
	
		if (counter)
		  vega_log(VEGA_LOG_DEBUG, "WAITING REBOOT SLAVE - try again connection with alphacom - address : %s:%d...", 
			   inet_ntoa(plink->peer_addr.sin_addr),
			   ntohs(plink->peer_addr.sin_port));
		else {
		  vega_log(VEGA_LOG_DEBUG, "WAITING REBOOT SLAVE - try to connect with alphacom - address : %s:%d...", 
			   inet_ntoa(plink->peer_addr.sin_addr),
			   ntohs(plink->peer_addr.sin_port));
		}
	

		if (connect(plink->sock, (struct sockaddr *) &plink->peer_addr, sizeof(plink->peer_addr)) < 0) {
		  vega_log(ERROR,"connect error - cause: %s", strerror(errno));
		}
		else {

			//vega_event_log(EV_LOG_VEGA_CONTROL_NETWORK_UP, 0,0,0,"-");
			//vega_alarm_log(EV_ALARM_CONTROL_NETWORK_UP, 0,0,"connexion ethernet avec alphacom" );

		  vega_log(VEGA_LOG_DEBUG,"we are connected!!!");
		  plink->state = ST_connected_unknown;
		  vega_log(VEGA_LOG_DEBUG, "==> NEW STATE : ST_connected_unknown");
		  /* on attend 5 seconds (que le soft soit pret ???) */
		  sleep (5);
		  break;
		}

		if (plink->order_stop == 1) {
		  return NULL;
		}

		sleep(1);
		counter ++;
      }/*while*/
      
      if (counter == plink->timer && plink->state == ST_disconnected_pending_slave) 
	  {
		vega_log(CRITICAL, "!!! DEFINITIVELY disconnected with the alphacom after a slave request  !!!");
		plink->state = ST_definitely_disconnected;
		/* TODO: on fait quoi dans ce cas ? */
      }
      /* else on bascule dans le mode ST_connected_unknown : dans cet etat on devrait alors basculer
       dans le mode ST_connected_slave*/
    
	}
	else if (plink->state == ST_connected_master) 
	{
      
      BYTE set_slave_mode = ICCMT_CC_SET_MODE;

      /* on envoie une demande de requete pour passer en mode esclave */
      vega_log(VEGA_LOG_DEBUG, "try going to slave mode");
      if (vega_control_send_alphacom_msg1(plink, &set_slave_mode, 1) < 0) {
		vega_log(ERROR, "send slave mode request failed  - cause : %s", strerror(errno));
		vega_control_reinit_link(plink);
		plink->state = ST_disconnected;
		  
	  } 
	  else 
	  {
		vega_log(VEGA_LOG_DEBUG, "slave mode request sent");
		vega_log(VEGA_LOG_DEBUG, "going to ST_disconnected_pending_slave ...wait for reconnection");
		/* bloquant, on va donc avoir une deconnection */
		vega_control_recv_message(plink);
		vega_control_reinit_link(plink);
		plink->state = ST_disconnected_pending_slave;
		vega_log(VEGA_LOG_DEBUG, "==> NEW STATE : ST_disconnected_pending_slave");
      }

    }
    else if (plink->state == ST_connected_unknown) 
	{


		/* dans cet etat, on lance une commande slave mode */
		BYTE get_software_mode = ICCMT_CC_GET_SW_VSN;

		if (vega_control_send_alphacom_msg1(plink, &get_software_mode, 1) < 0)
		{
		vega_log(ERROR, "disconnected from alphaCOM - cause : %s", strerror(errno));
		plink->state = ST_disconnected;
		vega_control_reinit_link(plink);
		} 
		else 
		{
			/* on attend la reponse en mode pendant 5 secondes */
			printf("attente d'une reponse !!\n");
			
			int ret =  vega_control_recv_message_nonbloquant(plink, 5000);

			if (ret < 0) 
			{
			  if (ret == -2) {
				/* on a recu aucun message ! donc l'alphacom est en mode maitre !!  */
			  going_slave_mode:
				vega_log(VEGA_LOG_DEBUG, "On est en mode maitre !!!\n");
				vega_log(VEGA_LOG_DEBUG, "==> NEW STATE : ST_connected_master");
				plink->state = ST_connected_master;
			  }
			  else {
				vega_log(ERROR, "link disconnected during waiting for get software mode response - cause : %s",strerror(errno));
				plink->state = ST_disconnected;
				vega_control_reinit_link(plink);
				vega_log(VEGA_LOG_DEBUG, "==> NEW STATE : ST_disconnected");
			  }
			}
			else 
			{
			  /* on a recu une reponse, on check bien si il s'agit d'un message de reponse ICCMT_CC_GET_SW_VSN */
			  printf("reponse recue !!\n");

				unsigned char data[1024]={0};
				int data_len = vega_control_get_next_msg_in_current_frame(plink, data, sizeof(data) );
				if ( data_len <= 0)
				{
				/* erreur message : on retourne en mode  */
				vega_log(ERROR, "response to our ICCMT_CC_GET_SW_VSN: no an valid message");
				goto going_slave_mode;
			  
				}
			  
				else 
				{
					if (data[0] != ICCMT_CC_SW_VSN) {
					  vega_log(ERROR, "error in received message (response to our ICCMT_CC_GET_SW_VSN)");
					  goto going_slave_mode;
					}else {
					  vega_log(VEGA_LOG_DEBUG, "received ICCMT_CC_GET_SW_VSN response !");
					  vega_log(VEGA_LOG_DEBUG, "=> NEW STATE : ST_connected_slave");
					  /* plink->state = ST_connected_slave; */
					  plink->state = ST_testing_boards;
					  
					}			  
				}
			}
		}
	}
    else if (plink->state == ST_definitely_disconnected) 
	{


		  vega_alarm_log(EV_ALARM_CNX_ALPHACOM_CONTROL_DOWN, 0,0,"Deconnexion Alphacom <-> PC Controle");
		  vega_alarm_log(EV_ALARM_CNX_IHM_CONTROL_DOWN, 0,0,"Deconnexion IHM <-> PC Controle");

		critical_dump();
		exit(-10);
    }
  }
}

/* retourne un  message si  valide*/
int vega_control_get_next_msg_in_current_frame(vega_control_t *plink, unsigned char* data, int max_data_len)
{  
  
  if (plink->current_frame_size == 0) {
    return 0;
  }
  
  int current_idx;
  BYTE SlaveByte = 64 + plink->rx_next_waited_seqnum * 4;


  /* partie uniquement control ICI sur le premier octect ! */

  /* echapement du flag de fin de message  */
  if ( 241==SlaveByte ) {
    if (plink->current_frame[0] != 240 || plink->current_frame[1] != 0) {
      vega_log(ERROR, "SlaveByte = 241 but received = %d , %d ", plink->current_frame[0], plink->current_frame[1]);
      return NULL;
    }
    current_idx = 2;
  }
  else if ( 240 == SlaveByte ) {
    if (plink->current_frame[0] != 240 || plink->current_frame[1] != 1) {
      vega_log(ERROR, "SlaveByte = 240 but received = %d , %d ", plink->current_frame[0], plink->current_frame[1]);
      return 0;
    }
    current_idx = 2;
  }
  else { 
    if (SlaveByte != plink->current_frame[0]) {
      vega_log(ERROR, "SlaveByte value = 0x%2.2X %d # receveid value 0x%2.2X %d", SlaveByte, SlaveByte, plink->current_frame[0],
	       plink->current_frame[0]);
      return 0;
    }
    current_idx = 1;
  }
  



  BYTE 	CheckSum = SlaveByte;
  int nb_rx_bytes = 0;
  BYTE receivedCheckSum = 0;
  BYTE received_data[1024] = {0};




  for (  ;current_idx < plink->current_frame_size ; current_idx ++ ) 
  {
    
    BYTE HelpByte = plink->current_frame[current_idx]; // ca casse tout !!! plink->current_frame[current_idx]=0; // pdl 20091002 RAZ 

    if (current_idx == plink->current_frame_size - 2 )
	{
      /* on a mainteannt le checksum recu */
      receivedCheckSum = plink->current_frame[current_idx];
      break;
    }
    else if (plink->current_frame[current_idx] == 240 && plink->current_frame[current_idx+1] == 0) 
	{
      if (current_idx == plink->current_frame_size - 3) {
		/* on est dans le cas ou la valeur du checksum vaut EOMSG */
		receivedCheckSum = 241;
		break;
      }
      else 
	  {

			received_data[nb_rx_bytes ++] = 241;
			CheckSum = CheckSum ^ 241;
			current_idx ++;		/* + 1 parce qu'on a traitÃ© 2 bytes */
      }
    }
    else if (plink->current_frame[current_idx] == 240 && plink->current_frame[current_idx+1] == 1) 
	{
      /* echappement de la valeur 240  */
      if (current_idx == plink->current_frame_size - 3) {
			receivedCheckSum = 240;
			break;
      }
      else 
	  {
			received_data[nb_rx_bytes ++] = 240;
			CheckSum = CheckSum ^ 240;
			current_idx ++;
      }
    }
    else{
		  received_data[nb_rx_bytes ++] = HelpByte;
		  CheckSum = CheckSum ^ HelpByte; // xor
    }
  }

  int msg_len = current_idx - 1;
  plink->current_frame_size = 0;
  
  if (CheckSum != receivedCheckSum)
  {
		vega_log(ERROR, "receivedCheckSum %d (%2.2X) != computed CheckSum %d (%2.2X)", 
			 receivedCheckSum, receivedCheckSum & 0xff, CheckSum, CheckSum & 0xff);
		fprintf(stderr,"error receivedCheckSum %d (%2.2X) != computed CheckSum %d (%2.2X)", 
			 receivedCheckSum, receivedCheckSum & 0xff, CheckSum, CheckSum & 0xff);
		return 0;
  }
  else {

	  vega_log(VEGA_LOG_DEBUG, "OK:receivedCheckSum (%d)", receivedCheckSum);
/*     vega_log(VEGA_LOG_DEBUG, "msg size = %d", msg_len); */
  }



  /*{
	  char *m  = dump_message_buffer(received_data, msg_len); 
	  printf("%s", m);
	  free (m);
  }*/

  //printf(" sizeof(tcp_msg_t)=%d\n",sizeof(tcp_msg_t));

  //vega_log(INFO,"calloc %d\n", sizeof(tcp_msg_t));

  //tcp_msg_t* pmsg = (tcp_msg_t*) calloc(1, sizeof(tcp_msg_t));
  //pmsg->rx_seq_num = plink->rx_next_waited_seqnum;
  //pmsg->size = msg_len;
  //memcpy(pmsg->data, received_data, pmsg->size);

  vega_log(INFO,"received_data %d/%d", msg_len, max_data_len);
  memcpy( data, received_data, min(msg_len, max_data_len) ) ;

  plink->rx_next_waited_seqnum ++;
  if (plink->rx_next_waited_seqnum > 7)
    plink->rx_next_waited_seqnum = 1;

  return min(msg_len, max_data_len) ;
};

/*
From: Hans van Dop
Sent: vendredi 21 novembre 2008 09:20
To: François Weppe
Subject: Hot plug in
Hi François,
I visited Datis yesterday and we solved the hot plug-in problem, where audio could not be used from the AGA board inputs. I think you will need this information for Vega as well:
When a board reports that it is unavailable, all its inputs and outputs must be disconnected from the timeslots, before the board is scanned again:
Output disconnect:

·               icc_msg(1) = 0x09 (ICCMT_TS_2_SC0_DISC, Sub channel 0 disconnect)

·               icc_msg(1) = 0x0a (ICCMT_TS_2_SC1_DISC, Sub channel 1 disconnect)

·               icc_msg(1) = 0x0c (ICCMT_TS_2_SC3_DISC, Sub channel 3 disconnect)

·               if (output on SBI0)
icc_msg(2)=bbbbbccc
else
icc_msg(2)=00000sss
icc_msg(3)=bbbbbccc
endif

Input disconnect:

·          icc_msg(1) = 0x02 (ICCMT_UNIT_2_TS_DISC)

·           if (input on SBI0)
icc_msg(2)=bbbbbccc
else
icc_msg(2)=00000sss
icc_msg(3)=bbbbbccc
endif

When an input is disconnected ALL outputs which are tapping that timeslot must also be disconnected, even the outputs on other cards.
When this is done, the card can be scanned again, and once it reports availability its inputs and outputs can be used again.
Regards,
Hans	  */

#define DELAI_ANTI_REBOND_SECONDES (2.5)

int CRM4::process_input_key( int input_number )
{
	CRM4* d=this;
	static struct timeval button_timeval[1024]={0};

	//static int nb_ticks=0;
  /* gerer un evenement (appui touche..) => action  */
  /* evenements possible : appui micro, appui selection confernce, appui demande, appui exclusion (directeur), audio on/off sur conference */
  //switch (evt->type) 
  {

  //case ALPHACOM_CRM4_INPUT:
    {
      /* trouver l'action qui correspond au code de la touche */
/* Depending on the key which is pressed, the sequences are: */
 
/* 1) Digit/DAK-key */
/* a) Press: Header 142 PortNr 0 64 0 KeyCode Checksum 241 */
/* b) Release: Header 142 PortNr 0 65 255 255 Checksum 241 */
 
/* 2) M-key: */
/* a) Press: Header 142 PortNr 0 68 Checksum 241 */
/* b) Release: Header 142 PortNr 0 69 Checksum 241 */
 
/* 3) C-key: */
/* a) Press: Header 142 PortNr 0 66 Checksum 241 */
/* b) Release: Header 142 PortNr 0 67 Checksum 241 */
 
 
/* 4) Handset: */
/* a) Lift (Off-hook): Header 142 PortNr 0 70 Checksum 241 */
/* b) Replace (On-hook): Header 142 PortNr 0 71 Checksum 241 */
 
/* As you can see, the digit release code is always the same for each key, so you don't know which key has been released. This is not a problem as you are only interested in which key has been pressed. */
/* On many intercom stations the digit release code is sent when the key is physically released. On an CRM IV station (the intercom units used for VEGA, the digit release is done by the unit's processor, fairly quickly after the key has been pressed, and is totally unrelated to how long you keep the button pressed. */
/* This is probably why you see the 255 code. To be able to map all keys you should make the 'pressed' code visible for a longer time. */

 	  EVENT ev;
	  memset(&ev,0,sizeof(EVENT));
	  ev.device_number = d->number;

	  BYTE button_code[128] = {	
		  0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  , // ( 0 a 9 ) 10
		  11 ,12 ,13 ,14 ,15 ,16 ,17 ,18 ,19 ,20 , // ( 10 a 19 ) 20
		  0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  , // 30
		  0  ,0  ,1  ,2  ,3  ,4  ,5  ,6  ,7  ,8  , // 40 
		  9  ,10 ,21 ,22 ,23 ,24 ,25 ,26 ,27 ,28 , // 50
		  29 ,30 ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  , // 60
		  0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,127,128, // 70
		  0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  , // 80
		  0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  , // 90 
		  0  ,0  ,0  ,0  ,0  ,0  ,31 ,32 ,33 ,34 , // 100
		  35 ,36 ,37 ,38 ,39 ,40 ,41 ,42 ,43 ,44 , // (100 a 109 ) 110
		  45 ,46 ,47 ,48 ,49 ,50 ,0  ,0  ,0  ,0  , // (110 a 119 ) 120
		  0  ,0  ,0  ,0  ,0  ,0  ,0  ,51    // (120 a 127 ) 128
	  };

      int code = 0;//input_number;
      if (input_number == DEVICE_KEY_MICRO_PRESSED || input_number == DEVICE_KEY_MICRO_RELEASED ||
		  input_number == DEVICE_PLUGGED || input_number == DEVICE_UNPLUGGED ||		  
		  input_number == DEVICE_NOT_PRESENT || input_number == DEVICE_PRESENT ||	input_number == DEVICE_KEY_CANCEL) 
	  {
			code = input_number;
      }
	  else 
	  {
		  if ( input_number <sizeof(button_code) ) 
			code = button_code[input_number];
		  else
			  return 0;


		  if ( 3==code){
			  ev.code = EVT_DEVICE_KEY_DEBUG;
			  d->ExecuteStateTransition(&ev);
			  return 0;
		  }


		  // traitement de l'antiretour pour tous les boutons sauf DEVICE_KEY_MICRO_PRESSED et DEVICE_KEY_MICRO_RELEASED !!!! pdl 20090923
		  {
				double tt = time_between_button( &button_timeval[code] );//_tstart_btn_general);
				if ( tt < DELAI_ANTI_REBOND_SECONDES){
					vega_log(INFO,"IGNORED BUTTON PRESSED within %f seconds\n",tt);
					printf("IGNORED BUTTON PRESSED within %f seconds\n",tt);
					return 0;
				}
				gettimeofday(&button_timeval[code] , NULL); // memoriser le dernier appui accepte
				vega_log(INFO,"D%d: ACCEPTED BUTTON code: %d PRESSED within %f seconds", d->number, code,tt);      
				printf("D%d: BUTTON code: %d\n", d->number, code);      
		  }
      }

	

	  // >>>>>>>CALLING STATE MACHINE

	  vega_log(INFO,"device_handle_event D%d input:%d code:%d\n", d->number, input_number , code );

	  bool code_found_in_group = false;
	  
	  // touches de GRPOUPE dynamique propres a chaque device:
	  {
		QMap<int, int>::const_iterator i = d->keymap_groups.constBegin();
		while (i != d->keymap_groups.constEnd()) 
		{
		 //cout << i.key() << ": " << i.value() << endl;
			 //printf("D%d has_group_key G%d in KEY %d\n", d->number, i.value() , i.key());
			 //vega_log(INFO,"D%d has_group_key G%d in KEY %d\n", d->number, i.value() , i.key());
			 if ( i.key()== code ){
				 code_found_in_group = true;
				 vega_log(INFO,"FOUND");

			 }
			 ++i;
		}

		if ( code_found_in_group )
		{

			  int NG = d->keymap_groups[code];

			  vega_log(INFO,"PUSH G%d = key %d on D%d...\n", NG, code , d->number);
			  printf("PUSH G%d = key %d on D%d...\n", NG, code , d->number);
			  ev.code = EVT_DEVICE_KEY_GROUP_CALL;
			  ev.group_number = NG;
			  ev.device_number = d->number;
			  d->ExecuteStateTransition(&ev);//{return ((*d->linkStateTransition))(d,pev);}
			  return 0;

		}else{
			  //vega_log(INFO, "PUSH NO GROUP key %d on D%d...\n", code , d->number);
			  //printf("PUSH NO GROUP key %d on D%d...\n", code , d->number);
		}
	  }
	
	  vega_log(INFO, "DEVICE_KEY code %d on D%d...\n", code , d->number);
	  if ( DEVICE_KEY_CANCEL==code ) 
	  {
		  ev.code = EVT_DEVICE_KEY_CANCEL;
		  d->ExecuteStateTransition(&ev);
	  }
	  else if ( DEVICE_KEY_MICRO_RELEASED==code )
	  { 
		  ev.code = EVT_DEVICE_KEY_MICRO_RELEASED;
		  d->ExecuteStateTransition(&ev);
	  }
	  else if ( DEVICE_KEY_MICRO_PRESSED==code ) 
	  {
		  ev.code = EVT_DEVICE_KEY_MICRO_PRESSED;		  
		  d->ExecuteStateTransition(&ev);
	  }
	  else if ( DEVICE_UNPLUGGED==code ) 
	  {
		  vega_alarm_log(EV_ALARM_DEVICE_ANOMALY,  d->number , d->number, "debut alarme station %s cable abonne debranche %d sec", 
			  d->name, time(NULL) - demarrage_system); 
		  ///vega_event_log(EV_LOG_DEVICE_UNPLUGGED,d->number, d->number, d->current_conference, "debut alarme station %s debranchee", d->name); 
		  set_alarm_dry_contact(1);
		  ev.code = EVT_DEVICE_UNPLUGGED;
		  d->ExecuteStateTransition(&ev);
	  }
	  else if ( DEVICE_PLUGGED==code ) 
	  { 
		  if ( ( time(NULL) - demarrage_system ) > 30 ) // cas d'un CRM4 qui serait debranche plus de 55 sec apres le demarrage
		  {
			vega_alarm_log(EV_ALARM_END_DEVICE_ANOMALY,  d->number , d->number, "fin alarme station %s cable abonne rebranche %d sec",
				d->name, time(NULL) - demarrage_system); 
			//vega_event_log(EV_LOG_DEVICE_PLUGGED,d->number, d->number, d->current_conference, "fin alarme station %s rebranchee", d->name); 
			set_alarm_dry_contact(0);
			ev.code = EVT_DEVICE_PLUGGED;
			ev.value.key.number = code;
			d->ExecuteStateTransition(&ev);
		  }
	  }
	  else if (  DEVICE_NOT_PRESENT==code ) 
	  { // pdl 20091107 nombre de secondes écoulées depuis le 1er Janvier 1970 
		  if ( ( time(NULL) - demarrage_system ) > 60 ) // cas d'un CRM4 qui serait debranche plus de 55 sec apres le demarrage
		  {
			vega_alarm_log(EV_ALARM_DEVICE_ANOMALY,  d->number , d->number, "alarme station %s alimentation debranchee %d sec", 
				d->name, time(NULL) - demarrage_system); 
			//vega_event_log(EV_LOG_DEVICE_UNPLUGGED,d->number, d->number, d->current_conference, "alarme station %s debranchee", d->name); 
			set_alarm_dry_contact(1);
			ev.code = EVT_DEVICE_NOT_PRESENT;
			ev.value.key.number = code;
			d->ExecuteStateTransition(&ev);
		  }
	  }
	  else if (DEVICE_PRESENT==code ) 
	  {
		  if ( ( time(NULL) - demarrage_system ) > 60 ) // cas d'un CRM4 qui serait rebranche plus de 55 sec apres le demarrage
		  {
			vega_alarm_log(EV_ALARM_END_DEVICE_ANOMALY,  d->number , d->number, "alarme station %s alimentation rebranchee %d sec", 
				d->name,time(NULL) - demarrage_system);
			//vega_event_log(EV_LOG_DEVICE_PLUGGED,d->number, d->number, d->current_conference, "alarme station %s rebranchee", d->name); 
			set_alarm_dry_contact(0);
			ev.code = EVT_DEVICE_PRESENT;
			ev.value.key.number = code;
			d->ExecuteStateTransition(&ev);
		  }
	  }else{

		  vega_log(INFO,"systeme demarre depuis %d secondes\n", time(NULL) - demarrage_system );
		  vega_log(INFO,"m_common_keymap_action=%s\n", qPrintable( CRM4::m_common_keymap_action[code] ) );
			vega_log(INFO,"m_common_keymap_number=%d\n", CRM4::m_common_keymap_number[code]);

			if ( "action_activate_conf" == CRM4::m_common_keymap_action[code]){
				
				ev.code = EVT_DEVICE_KEY_ACTIVATE_CONFERENCE;

			}else if ( "action_on_off_conf" == CRM4::m_common_keymap_action[code]){

				ev.code = EVT_DEVICE_KEY_SELF_EXCLUSION_INCLUSION;				

			}else if ( "action_exclude_include_conf" == CRM4::m_common_keymap_action[code]){

				ev.code = EVT_CONFERENCE_INCLUDE_EXCLUDE;

			}else if ( "action_select_conf" == CRM4::m_common_keymap_action[code]){

				ev.code = EVT_DEVICE_KEY_SELECT_CONFERENCE;

			}else if ( "action_select_device" == CRM4::m_common_keymap_action[code]){

				ev.code = EVT_DEVICE_KEY_SELECT_DEVICE;

			}else if ( "action_general_call" == CRM4::m_common_keymap_action[code]){

				ev.code = EVT_DEVICE_KEY_GENERAL_CALL;			
			}


			//ev.value.action.type = action->type; // ex ACTION_GENERAL_CALL
		  
			ev.value.select.conference_number = CRM4::m_common_keymap_number[code];//action->action.select_conf.conference_number;
		  
			ev.value.select.device_number = CRM4::m_common_keymap_number[code];//action->action.select_device.device_number;
		 
			d->ExecuteStateTransition(&ev);
	  }
    }
    //break;
  }  
  return 0;
}

static void* tcp_monitor_rx_thread(void* data)
{
	SOCKET socket = (SOCKET)data;
	vega_log(INFO,"->tcp_monitor_rx_thread\n");
	while (1) 
	{  
		qlist_monitor_rx_bytes_mutex.lock();
		int size=qlist_monitor_rx_bytes.size() ;
		qlist_monitor_rx_bytes_mutex.unlock();
		
		if ( size > 0){
			qlist_monitor_rx_bytes_mutex.lock();
			BYTE carcou= qlist_monitor_rx_bytes.takeFirst();
			qlist_monitor_rx_bytes_mutex.unlock();
			
			vega_log(INFO,"monitor rx: try send %02X",carcou);

			if ( INVALID_SOCKET!=socket){
				int ret = send(socket, (const void*) &carcou, 1, MSG_NOSIGNAL);// MSG_DONTWAIT);
				if ( ret<0 ) {
					vega_log(INFO,"NOK monitor rx: ret %d",ret);
					return NULL;
				}else{
					//vega_log(INFO,"OK monitor rx: send %02X",carcou);
				}
			}else {
				vega_log(INFO,"NOK monitor rx: INVALID_SOCKET");
				return NULL;
			}
		}
		else
			usleep(1*MILLISECOND);
	}
}


static void* tcp_monitor_tx_thread(void* data)
{
	SOCKET socket = (SOCKET)data;
	vega_log(INFO,"->tcp_monitor_tx_thread\n");
	while (1) 
	{  
		qlist_monitor_tx_bytes_mutex.lock();
		int size=qlist_monitor_tx_bytes.size() ;
		qlist_monitor_tx_bytes_mutex.unlock();
		
		if ( size > 0){
			qlist_monitor_tx_bytes_mutex.lock();
			BYTE carcou= qlist_monitor_tx_bytes.takeFirst();
			qlist_monitor_tx_bytes_mutex.unlock();
			
			vega_log(INFO,"monitor tx: try send %02X",carcou);

			if ( INVALID_SOCKET!=socket){
				int ret = send(socket, (const void*) &carcou, 1, MSG_NOSIGNAL);// MSG_DONTWAIT);
				if ( ret<0 ) {
					vega_log(INFO,"NOK monitor tx: ret %d",ret);
					return NULL;
				}else{
					//vega_log(INFO,"OK monitor tx: send %02X",carcou);
				}
			}else {
				vega_log(INFO,"NOK monitor tx: INVALID_SOCKET");
				return NULL;
			}
		}
		else
			usleep(1*MILLISECOND);
	}
}

void* listening_monitor_rx_thread(void* arg)
{
	while (1)
	{
		vega_log(INFO,"create listening socket port %d in listening_monitor_rx_thread",IHM_LISTENING_MONITOR_RX_PORT);
		SOCKET listeningSocket = socket(PF_INET, SOCK_STREAM, 0);
		if (listeningSocket == INVALID_SOCKET) {
			vega_log(INFO,"Could not create listening socket");
			sleep(5); continue;
		}
		
		int one = 1;
		setsockopt(listeningSocket,SOL_SOCKET,SO_REUSEADDR,&one,sizeof(one));
		setsockopt(listeningSocket,SOL_SOCKET,SO_LINGER,&one,sizeof(one));		// attend la fin de l'emission lors du close()
 		
		struct sockaddr_in sinInterface;
		sinInterface.sin_family = AF_INET;
		sinInterface.sin_addr.s_addr = INADDR_ANY;
		sinInterface.sin_port = htons(IHM_LISTENING_MONITOR_RX_PORT);
		int nRet = bind(listeningSocket, (struct sockaddr*)&sinInterface, sizeof(sinInterface));	
		if (nRet == SOCKET_ERROR) {
			vega_log(INFO,"bind error:");
			close(listeningSocket);
			sleep(5); continue;
		}
		
		//LISTEN
		nRet = listen(listeningSocket, 0);
		if (nRet == SOCKET_ERROR) {
			vega_log(INFO,"listen error:");
			close(listeningSocket);
			sleep(5); continue;
		}

		while(INVALID_SOCKET!=listeningSocket)
		{		
			struct sockaddr_in sinRemote;
			int nAddrSize = sizeof(sinRemote);
			SOCKET client_socket = accept(listeningSocket, (struct sockaddr *)  &sinRemote, 		(unsigned int*)		&nAddrSize);	
			
			if (client_socket == INVALID_SOCKET) {
				vega_log(INFO,"Could not accept TCP connection client_socket:%d\n",client_socket);
				close(listeningSocket); sleep(5); continue;
			}

			if (client_socket == SOCKET_ERROR) {
				vega_log(INFO,"SOCKET_ERROR from %s:%d.\n",inet_ntoa(sinRemote.sin_addr),ntohs(sinRemote.sin_port));
				close(listeningSocket);sleep(5); continue;
			}else{
				pthread_t t_id;
				vega_log(INFO,"Accepted RX monitor connection from %s:%d.\n",inet_ntoa(sinRemote.sin_addr),ntohs(sinRemote.sin_port));	
				pthread_create(&t_id, NULL, tcp_monitor_rx_thread, (void*)client_socket);				
			}
		}	
	}// while (1)
	vega_log(INFO,"listening_monitor_rx_thread ended normally\n");
}

void* listening_monitor_tx_thread(void* arg)
{
	while (1)
	{
		vega_log(INFO,"create listening socket port %d in listening_monitor_tx_thread",IHM_LISTENING_MONITOR_TX_PORT);
		SOCKET listeningSocket = socket(PF_INET, SOCK_STREAM, 0);
		if (listeningSocket == INVALID_SOCKET) {
			vega_log(INFO,"Could not create listening socket");
			sleep(5); continue;
		}
		
		int one = 1;
		setsockopt(listeningSocket,SOL_SOCKET,SO_REUSEADDR,&one,sizeof(one));
		setsockopt(listeningSocket,SOL_SOCKET,SO_LINGER,&one,sizeof(one));		// attend la fin de l'emission lors du close()
 		
		struct sockaddr_in sinInterface;
		sinInterface.sin_family = AF_INET;
		sinInterface.sin_addr.s_addr = INADDR_ANY;
		sinInterface.sin_port = htons(IHM_LISTENING_MONITOR_TX_PORT);
		int nRet = bind(listeningSocket, (struct sockaddr*)&sinInterface, sizeof(sinInterface));	
		if (nRet == SOCKET_ERROR) {
			vega_log(INFO,"bind error:");
			close(listeningSocket);
			sleep(5); continue;
		}
		
		//LISTEN
		nRet = listen(listeningSocket, 0);
		if (nRet == SOCKET_ERROR) {
			vega_log(INFO,"listen error:");
			close(listeningSocket);
			sleep(5); continue;
		}

		while(INVALID_SOCKET!=listeningSocket)
		{		
			struct sockaddr_in sinRemote;
			int nAddrSize = sizeof(sinRemote);
			SOCKET client_socket = accept(listeningSocket, (struct sockaddr *)  &sinRemote, 		(unsigned int*)		&nAddrSize);	
			
			if (client_socket == INVALID_SOCKET) {
				vega_log(INFO,"Could not accept TCP connection client_socket:%d\n",client_socket);
				close(listeningSocket); sleep(5); continue;
			}

			if (client_socket == SOCKET_ERROR) {
				vega_log(INFO,"SOCKET_ERROR from %s:%d.\n",inet_ntoa(sinRemote.sin_addr),ntohs(sinRemote.sin_port));
				close(listeningSocket);sleep(5); continue;
			}else{
				pthread_t t_id;
				vega_log(INFO,"Accepted TX monitor connection from %s:%d.\n",inet_ntoa(sinRemote.sin_addr),ntohs(sinRemote.sin_port));	
				pthread_create(&t_id, NULL, tcp_monitor_tx_thread, (void*)client_socket);				
			}
		}	
	}// while (1)
	vega_log(INFO,"listening_monitor_rx_thread ended normally\n");
}
