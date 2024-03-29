// reload.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <rude/config.h>

#include "ixpci.h"
#include "const.h"
#include "debug.h"
#include "conf.h"
#include "ini.h"
#include "misc.h"
#include <QMutex>

#define min(a, b)	((a) < (b) ? (a) : (b))

QLinkedList<vega_conference_t *> vega_conference_t::qlist;
QLinkedList<device_t *>			device_t::qlist_devices;
QLinkedList<CRM4 *>				CRM4::qlist_crm4;
QLinkedList<Radio *>			Radio::qlist_radio;
QMutex							qlist_conf_mutex;

//void* listening_ihm_tcp_thread(void* arg);

extern GROUP_T tab_appel_group[MAX_GROUPS+1]; 
extern int synchronize_remote_config;
static pthread_t t_id = 0; // id of tcp thread reading GUI commands

void load_groupes_section(const char* fname)
{	
	rude::Config config;
	if (config.load(fname) == false) {
		printf("rude cannot load %s file", fname);
		return ;
	}

	for ( int gnumber=1;gnumber<=10;gnumber++)
	{
		QString strgrpsection = QString("G%1").arg(gnumber);
		if ( true == config.setSection(qPrintable(strgrpsection),false) ) 
		{
			QString qstrmembers = config.getStringValue("members");
			QString strname= config.getStringValue("name");

			printf("G%d: strname:%s\n",gnumber, qPrintable(strname) );
			printf("strmembers:%s\n",qPrintable(qstrmembers) );

				if ( is_valid_group_number(gnumber) ) 
				{
					GROUP_T* ptr_G = &tab_appel_group[gnumber];
					ptr_G->nb_devices = 0; // empty the group before reloading

					char str_members[256]={0};
					strncpy(str_members,qPrintable(qstrmembers),sizeof(str_members));
					printf("ENTRY G%d str_members = %s\n", gnumber, str_members);

					char *elt = strtok(str_members, ",;\r\n:");			
					while (elt != NULL) 
					{
						int nd = atoi(elt);
						if ( device_t::is_valid_device_number(nd) )
						{
							//printf("GROUP:adding D%d to G%d (total:%d)\n", nd, NG, ptr_G->nb_devices);
							vega_log(INFO, "adding D%d to G%d (total:%d)...", nd, gnumber, ptr_G->nb_devices);

							
							if ( !is_device_number_in_group(nd,gnumber) && (ptr_G->nb_devices < MAX_PARTICIPANTS) )
							{
								ptr_G->tab_devices[ptr_G->nb_devices++] = nd;
								vega_log(INFO, "D%d added to G%d (total:%d)", nd, gnumber, ptr_G->nb_devices);
							}
						}
						elt = strtok(NULL, ",;\r\n:");
					}			  
				}else{
					vega_log(ERROR, "Error Group G%d", gnumber);
				}
			
		}//if ( true == config.setSection(strconfsection,false) ) 
	}//	for ( int cnumber=1;cnumber<=46;cnumber++)
}
	
int socket_printf(SOCKET s,const char *line,...)
{
	if ( 0==s) return 0;

	char buffer[1024]={0};
	va_list arglist;
	va_start(arglist, line);
	vsprintf(buffer, line, arglist);
	va_end(arglist);
	printf(buffer);

	int ret = send(s, buffer, strlen(buffer), MSG_DONTROUTE | MSG_NOSIGNAL);
	if (ret <= 0) 
	{
		printf("send returned error: %d\n", ret);
		switch(ret)
		{
			case EBADF: printf("EBADF:An invalid descriptor was specified"); break;
			case ENOTSOCK: printf("ENOTSOCK:The argument s is not a socket"); break;
			case EFAULT: printf("EFAULT:An invalid user space address was specified for a parameter"); break;
			case EMSGSIZE: printf("EMSGSIZE:The socket requires that message be sent atomically, and the size of the message to be sent made this impossible"); break;
			//case EAGAIN:
			case EWOULDBLOCK:
				printf("EWOULDBLOCK:The socket is marked non-blocking and the requested operation would block."); break;
			case ENOBUFS: printf("ENOBUFS:The output queue for a network interface was full. This generally indicates that the interface has stopped sending, but may be caused by transient congestion. (This cannot occur in Linux, packets are just silently dropped when a device queue overflows."); break;
			case EINTR : printf("EINTR:A signal occurred. "); break;
			case ENOMEM: printf("ENOMEM:No memory available"); break;
			case EINVAL: printf("EINVAL:Invalid argument passed"); break;		
			case EPIPE: printf("EPIPE"); break;
		}
	}else{
		printf("OK sent %d TCP bytes %s",ret,buffer);
		ret = send(s, "\r\n", 2, MSG_DONTROUTE | MSG_NOSIGNAL);
	}
	
    return ret;
	
} /* printf */

void process_tcp_command(SOCKET socket, char* buffer, int buf_len)
{
	vega_log(INFO,"->process_tcp_command buf_len: %d\n",buf_len);
	printf("process_tcp_command buf_len: %d\n",buf_len);

	if ( NULL == buffer ) return;
	if (buf_len == 0) return; 



	if( 0==strncmp(buffer, "PING", strlen("PING") ) ) // ex: "<PUT![general]  name=vega0001 ....>"
	{
		printf("PING from IHM received\n");
		vega_log(INFO,"PING from IHM received");

		socket_printf(socket,"<PONG>");
	}
	else if( 0==strncmp(buffer, "PUT", strlen("PUT") ) ) // ex: "<PUT![general]  name=vega0001 ....>"
	{
		printf("found %s !!!\n","PUT");
		vega_log(INFO,"found %s !!!\n","PUT");

		char fname_local[1024]={0};
		snprintf(fname_local, sizeof(fname_local)  - 1,"%s/%s", work_path, VEGACTRL_CONF);//"temp.conf") ;

		char horodate[128];
		time_t t = time(NULL);
		strftime(horodate, sizeof(horodate), "%Y-%m-%d_%Hh%Mm%Ss", localtime(&t));
		char fname_copy[1024]={0};
		snprintf(fname_copy, sizeof(fname_copy)  - 1,"%s/copie-%s-%s", work_path, horodate, VEGACTRL_CONF);//"temp.conf") ;

		char cmd[1024];
		sprintf(cmd,"cp %s %s", fname_local , fname_copy);
		int ret_cmd = system(cmd);
		vega_log(INFO,"ret%d:COPIE %s dans %s\n",ret_cmd,fname_local,fname_copy);

		char* ptrdebut = strchr(buffer,'[');
		if ( NULL ==ptrdebut ) {
			vega_log(INFO,"erreur buffer TCP recu (%d octets) ne contenant pas [",buf_len);
			return;
		}

		FILE *f = fopen(fname_local,"w"); 
		if ( f )  
		{
			printf("ECRASE %s \n",fname_local);
			vega_log(INFO,"ECRASE %s \n",fname_local);

			int saut = ptrdebut - buffer; // sauter jusqu au premier '['
			int nbwritten = fwrite(ptrdebut , 1, buf_len -saut, f);
			fclose(f);

			//if (nbwritten == buf_len - len ) 
			{
				// todo: regarder si c'est le meme nom de configuration que celle en cours!!!
				if (1){
					// remove first 
					vega_conference_t::update_remove_devices_from_conference(fname_local);
					// then add new devices
					vega_conference_t::reload_devices(fname_local);	
					
					// todo: changes on devices names and gains !!!
					CRM4::check_modif_crm4_device_section(fname_local);
				}

			}

			printf("----- TCP nbwritten = %d -------\n",nbwritten);
			vega_log(INFO,"%d octets ECRITS dans %s \n",nbwritten, fname_local);

		}
	}
	else if ( 0==strncmp(buffer,"RST", strlen("RST")))
	{
		//vega_alarm_log(EV_ALARM_RESTARTED_BY_GUI, -1 , -1, "ordre de redemarrage du PC de controle depuis ihm"); 
		close(socket);
		vega_log(INFO, "Restart requested from IHM");
		exit (-7); 
	}
	else if ( 0==strncmp(buffer,"GET", strlen("GET")))
	{
		vega_log(INFO, "%s requested from IHM","GET");
		printf("%s requested from IHM\n","GET");
		//socket_printf(socket,"<%s!","GET); 

		char fname_local[1024]={0};
		snprintf(fname_local, sizeof(fname_local)  - 1,"%s/%s", work_path, VEGACTRL_CONF) ;
		FILE* f = fopen( fname_local ,"r");
		if (f){
			//send(socket, "<", strlen("<"), MSG_DONTROUTE | MSG_NOSIGNAL);

			int nbsent = send(socket, "<GET|", strlen("<GET|"), MSG_DONTROUTE | MSG_NOSIGNAL);
			vega_log(INFO, "send %d/%d bytes returned OK\n", nbsent, strlen("<GET|"));
			printf("send %d/%d bytes returned OK\n", nbsent, strlen("<GET|"));
			usleep(1000);
			while(!feof(f)){
				char line[1024]={0};
				int nbread = fread(line,1,sizeof(line)-1,f);

				if ( nbread > 0 ) { // todo: faire un while not sent ici
					int nbsent = send(socket, line, nbread, MSG_DONTROUTE | MSG_NOSIGNAL);
					if (nbsent <= 0) {
						vega_log(INFO, "send %d/%d bytes error\n", nbsent,nbread);
					}else{
						vega_log(INFO, "send %d/%d bytes returned OK\n", nbsent, nbread);
						printf( "send %d/%d bytes returned OK\n", nbsent, nbread);
					}
					usleep(1000);
				}
			}
			usleep(1000);
			nbsent = send(socket, ">", 1, MSG_DONTROUTE | MSG_NOSIGNAL);
			vega_log(INFO, "send %d/%d bytes returned OK\n", nbsent, 1);
			printf( "send %d/%d bytes returned OK\n", nbsent, 1);
			fclose(f);
		}
	}else{
		socket_printf(socket,"<PONG>");
		printf("unhandled: 1er octet:%02x (%c)\n", buffer[0],  buffer[0]);
	} 
	printf("<---process_tcp_command\n");
	vega_log(INFO,"<-process_tcp_command buf_len: %d\n",buf_len);
}

void* tcp_ihm_thread(void* data)
{
  SOCKET s = (SOCKET)data;

  int bBegOfCommandFound = false;		
  static char buffer[10*1024]={0};
  int pos = 0;
  int ret;

  
  vega_log(INFO,"->tcp_ihm_thread\n");
  //vega_event_log(EV_LOG_VEGA_GUI_STARTED, 0,0,0,"Ouverture de la connexion IHM <-> PC de control");	
  vega_alarm_log(EV_ALARM_CNX_IHM_CONTROL_UP,  0 , 0, "Reconnexion IHM <-> PC Controle"); 


  while (1)//pos < sizeof(buffer) -1) 
  {  
    char carcou;	
    //printf("attente\n");
    //printf("s = %d\n", s);

	fd_set rfds;
	struct timeval tv;
	int retval;

	while (1) 
	{
#if 1
		/* Surveiller stdin (fd 0) en attente d'entr�es */
		FD_ZERO(&rfds);
		FD_SET(s, &rfds);
		/* Pendant 5 secondes maxi */
		 tv.tv_sec = 15;
		 tv.tv_usec = 0;
		retval = select(s+1, &rfds, NULL, NULL, &tv);
		/* Considerer tv comme ind�fini maintenant ! */
		if (retval) 
		{
			ret = recv( s, &carcou, 1, 0  );			
			printf("received from IHM = %c !!! \n", carcou);
			 if (ret <= 0) 
			{
   
				printf ("Error reading from socket: ret=%d errno=%d pos=%d", ret, errno,pos);
      
				 if ( ret == EBADF) 
					printf("EBADF:An invalid descriptor was specified"); 
      
	

				 //vega_event_log(EV_LOG_VEGA_GUI_STOPPED, 0,0,0,"Fermeture de la connexion IHM <-> PC Controle"); 
				vega_alarm_log(EV_ALARM_CNX_IHM_CONTROL_DOWN,  0 , 0, "Deconnexion IHM <-> PC Controle"); 
      
				return NULL;
			}
			else if ( ret>0) 
			{				
	
				//printf("received one character !\n");
				if ( false==bBegOfCommandFound )				
				{
					if ( '<' == carcou) {
						printf("debut de trame TCP recu (0x%02X)\n",carcou);
						bBegOfCommandFound = true;
					}
				continue; // lire le prochain octet
				}
      
				if ( true==bBegOfCommandFound )
				{
				 //printf("%d bytes: %c \n",dwBytesRead,carcou);
					if ( '>' == carcou) 
					{

				  
						printf("fin de trame TCP recu (0x%02X)\n",carcou);
						printf("TCP : rx %d\n",pos);			
						vega_log(INFO,"TCP rx %d\n",pos);			
						bBegOfCommandFound = false;

						// traiter la commande recue avant de lire d'autres eventuels octets
						process_tcp_command(s,buffer,pos);
						memset(buffer,0,sizeof(buffer));		
						pos=0;
				
					}
					else{

					//For the standard ASCII character set, control characters are those between ASCII codes 0x00 (NUL) and 0x1f (US), 
					//plus 0x7f (DEL). Therefore, printable characters are all but these, although specific compiler implementations for
					// certain platforms may define additional control characters in the extended character set (above 0x7f).

						if ( ( '<' != carcou) && ( '>' != carcou)  &&
							( pos < sizeof(buffer)-1 ) 
						// && ( carcou=='\n' || carcou=='\r' || isprint(carcou) ) 
							)
						{
						buffer[pos++]=carcou;
					  //printf("%c",carcou);
						}
		
					}
				}
			
			}
		}else{
		
			fprintf(stderr, "KEEP ALIVE TIMEOUT !\n");
			close (s);

			//vega_event_log(EV_LOG_VEGA_GUI_STOPPED, 0,0,0,"Fermeture de la connexion IHM <-> PC Controle"); 
			vega_alarm_log(EV_ALARM_CNX_IHM_CONTROL_DOWN,  0 , 0, "Deconnexion IHM <-> PC Controle"); 

			return NULL;

		}
#endif

    }/*while 1*/
  }
  close(s);
  vega_log(INFO,"<-tcp_ihm_thread\n");
  return NULL;
}//tcp_ihm_thread


void* listening_ihm_tcp_thread(void* arg)
{
	while (1)
	{
		SOCKET listeningSocket = socket(PF_INET, SOCK_STREAM, 0);
		if (listeningSocket == INVALID_SOCKET) {
			perror("Could not create listening socket");
			return FALSE;
		}
		
		int one = 1;
		setsockopt(listeningSocket,SOL_SOCKET,SO_REUSEADDR,&one,sizeof(one));
		setsockopt(listeningSocket,SOL_SOCKET,SO_LINGER,&one,sizeof(one));		// attend la fin de l'emission lors du close()
 		
		// BIND	
		struct sockaddr_in sinInterface;
		sinInterface.sin_family = AF_INET;
		sinInterface.sin_addr.s_addr = INADDR_ANY;
		sinInterface.sin_port = htons(IHM_LISTENING_TCP_PORT);
		int nRet = bind(listeningSocket, (struct sockaddr*)&sinInterface, sizeof(sinInterface));	
		if (nRet == SOCKET_ERROR) {
			perror("bind error:");
			close(listeningSocket);
			return FALSE;
		}
		
		//LISTEN
		nRet = listen(listeningSocket, 0);
		if (nRet == SOCKET_ERROR) {
			perror("listen error:");
			close(listeningSocket);
			return FALSE;
		}

		while(INVALID_SOCKET!=listeningSocket)
		{		
			struct sockaddr_in sinRemote;
			int nAddrSize = sizeof(sinRemote);

			SOCKET client_socket;
			printf("try to accept TCP connection client_socket:%d\n",client_socket);


			client_socket = accept(listeningSocket, (struct sockaddr *)  &sinRemote, 		(unsigned int*)		&nAddrSize);	
			printf("accept TCP connection returned client_socket:%d\n",client_socket);

		
			
			if (client_socket == INVALID_SOCKET) {
				printf("Could not accept TCP connection client_socket:%d\n",client_socket);
				//sleep(10);
				perror("accept error"); // accept retourne immediatement 9 si cable coup�
					continue;
			//	break;
			}

	/* 		int flags = fcntl(client_socket, F_GETFL);	//printf("get sockets flags %x",flags); */
	/* 		flags &= ~O_NONBLOCK; // do block (default behavior) */
	/* 		//printf("set sockets flags %x (blocking socket)",flags); */
	/* 		fcntl (client_socket, F_SETFL, flags); */
			//int zero = 0;setsockopt(client,	SOL_SOCKET, SO_SNDBUF, (char *)&zero, sizeof(zero) );
	/* 		int one = 1; */
	/* 		setsockopt(client_socket,SOL_SOCKET,SO_REUSEADDR,&one,sizeof(one));		 */
	/* 		setsockopt(client_socket,SOL_SOCKET,SO_LINGER,&one,sizeof(one));		// attend la fin de l'emission lors du close() */
			//SO_SNDBUF fixe la taille du buffer d'�mission 
			//SO_SNDBUF et SO_RCVBUF sont respectivement des options permettant d'ajuster la taille des buffers allou�s 
			//pour l'�mission et la r�ception. La taille des buffers peut �tre augment�es pour des connexions avec un trafic important. Il y a des limites impos�es par le syst�me pour ces valeurs. 
		//	if (client_socket == SOCKET_ERROR) 
		//	{
		//		printf("SOCKET_ERROR from %s:%d.\n",inet_ntoa(sinRemote.sin_addr),ntohs(sinRemote.sin_port));
		//		return NULL;
		/*	}else*/
				
			{
				fprintf(stderr, "Accepted IHM connection from %s:%d.\n",inet_ntoa(sinRemote.sin_addr),ntohs(sinRemote.sin_port));	

				//vega_event_log(EV_LOG_VEGA_GUI_STARTED, 0,0,0,"tcp_ihm_thread started from %s:%d.\n",inet_ntoa(sinRemote.sin_addr),ntohs(sinRemote.sin_port));

				//pthread_create(&t_id, NULL, tcp_ihm_thread, (void*)client_socket);				
				tcp_ihm_thread((void*)client_socket);//, (void*)client_socket);	
				
				fprintf(stderr,"End of IHM connection from %s:%d.\n",inet_ntoa(sinRemote.sin_addr),ntohs(sinRemote.sin_port));	
				//exit (1);
			}
		}	

	}// while (1)
	printf("listening_pthread ended normally\n");
}

