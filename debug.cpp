#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include "debug.h"
#include "alarmes.h"
#include "conf.h"
#include <QTextStream>

extern void set_alarm_dry_contact(int on_off);

QTextStream outStream;

int verbose=0;
int display_source_filename=0;
int log_file_line=0;
int loglvl = _LOG_DEBUG;
//static pthread_mutex_t mutex_vega_log = PTHREAD_MUTEX_INITIALIZER;
/*    QString firstName( "Joe" );
    QString lastName( "Bloggs" );
    QString fullName;
    fullName = QString( "First name is '%1', last name is '%2'" )
               .arg( firstName )
               .arg( lastName );

*/

void vega_log(int lvl, const char *file, int line, const char *function, const char* fmt, ...)
{	
	if ( 0==fmt ) return;
	if ( 0==fmt[0] ) return;
	if ( '\n'==fmt[0] ) return;
	if (lvl < loglvl) 	return;
	if ( ! fmt ) return;

	static int init_done = 0;
	if ( ! init_done ) {
		init_done = 1;
		//pthread_mutex_init (&mutex_vega_log, NULL);
	}

	//pthread_mutex_lock(&mutex_vega_log);
	{
		struct timeval tv;
		struct tm* ptm;
		char time_string[40];
		long milliseconds;
		int nb=0;
		char log_filename[128];
		char horodate[128];
		char temp[1024]={0};
		time_t t = time(NULL);
		va_list args;

		gettimeofday (&tv, NULL);
		ptm = localtime (&tv.tv_sec);
		strftime (time_string, sizeof (time_string), "%Y-%m-%d %H:%M:%S", ptm);
		milliseconds = tv.tv_usec / 1000;

		va_start(args, fmt);
		nb = vsprintf(temp, fmt, args);
		va_end (args);

		//while ( temp && temp[0] && temp[strlen(temp)-1] == '\n' )// oter tous les '\n' en fin de chaine
			//temp[strlen(temp)-1] = 0;

		strftime(horodate, sizeof(horodate), "%Y%m%d", localtime(&t));


		sprintf(log_filename,_ROOT_LOG_NAME_"-%s.log",horodate);
		//sprintf(log_filename,"./traces-vega-%s.log",horodate);

		FILE* fout=fopen(log_filename,"a+t") ;
		if ( NULL==fout ) {
			fprintf(stderr,"cannot create %s\n",log_filename );
		}else{

			strftime(horodate, sizeof(horodate), "%H:%M:%S", localtime(&t));
			
			if ( log_file_line ) 
				fprintf(fout,"%s.%03ld %s(%d) %s\n",horodate, milliseconds,file,line, temp);
			else 
				fprintf(fout,"%s.%03ld %s\n",horodate, milliseconds, temp);
			
			if ( verbose && display_source_filename ) 
				printf("%s.%03ld - %s - f:%s - %s\n",horodate,milliseconds,file, function, temp);
			else if ( verbose ) 
				printf("%s - %s\n",horodate, temp);
			
			//fsync(fout);
			fclose(fout);
		}
	}
	//pthread_mutex_unlock(&mutex_vega_log);

}

void vega_event_log(EV_LOG_T type_event, int d1, int d2, int C, const char* fmt, ...)
{	// flush in a file
  if ( ! fmt ) return;

  struct timeval tv;
  struct tm* ptm;
  char time_string[40];
  long milliseconds;
  int nb=0;
  char horodate[128];
  char temp[1024]={0};
  time_t t = time(NULL);
  va_list args;
  
  gettimeofday (&tv, NULL);
  ptm = localtime (&tv.tv_sec);
  strftime (time_string, sizeof (time_string), "%Y-%m-%d %H:%M:%S", ptm);
  milliseconds = tv.tv_usec / 1000;
  
  if ( fmt ) {
	  va_start(args, fmt);
	  //nb = vsnprintf(temp, sizeof(temp) , fmt, args);
	  nb = vsprintf(temp, fmt, args);
	  va_end (args);
  }
  // oter tous les '\n' en fin de chaine
  while ( temp && temp[0] && temp[strlen(temp)-1] == '\n' )
    temp[strlen(temp)-1] = 0;
  
  strftime(horodate, sizeof(horodate), "%Y%m%d", localtime(&t));
  
  char log_filename[128];
  sprintf(log_filename,"%s/%s.log", log_path, _ROOT_EVENT_NAME_ );

  FILE* fout=fopen(log_filename,"a+t") ;

  if ( fout ) 
  {

    strftime(horodate, sizeof(horodate), "%Y-%m-%d %H:%M:%S", localtime(&t));
    //strftime(horodate, sizeof(horodate), "%H:%M:%S", localtime(&t));
	fprintf(fout,"%s|%d|%02d|%02d|%02d|%02d|%s\n",horodate, time(NULL), type_event, d1, d2, C, temp);
	fprintf(stdout,"%s|%d|%02d|%02d|%02d|%02d|%s\n",horodate, time(NULL), type_event, d1, d2, C, temp);
    
	fflush(fout);
    fclose(fout);

  }
}

void vega_alarm_log(EV_LOG_T type_alarm, int board, int device, const char* fmt, ...)
{	// flush in a file
   
	switch(type_alarm)
	{
	case EV_ALARM_AGA_BOARD_PLUGGED:
		set_alarm_dry_contact(0);
		break;
	case EV_ALARM_AGA_BOARD_UNPLUGGED:
		set_alarm_dry_contact(1);
		break;

	case EV_ALARM_ASLT_BOARD_PLUGGED:
		set_alarm_dry_contact(0);
		break;
	case EV_ALARM_ASLT_BOARD_UNPLUGGED:
		set_alarm_dry_contact(1);
		break;

	case EV_ALARM_BOARD_ANOMALY:
		set_alarm_dry_contact(1);
		break;
	case EV_ALARM_END_BOARD_ANOMALY:
		set_alarm_dry_contact(0);
		break;

	case EV_ALARM_DEVICE_ANOMALY:
		set_alarm_dry_contact(1);
		break;
	case EV_ALARM_END_DEVICE_ANOMALY:
		set_alarm_dry_contact(0);
		break;

	
	case EV_ALARM_CNX_ALPHACOM_CONTROL_DOWN:
		set_alarm_dry_contact(1);
		break;

	case EV_ALARM_CNX_ALPHACOM_CONTROL_UP:
		set_alarm_dry_contact(0);
		break;

	case EV_ALARM_POWER_DOWN:
		set_alarm_dry_contact(1);
		break;
	case EV_ALARM_POWER_UP:
		set_alarm_dry_contact(0);
		break;
	//case EV_ALARM_CONFIGURATION_RELOADED:	break;
	//case EV_ALARM_RESTARTED_BY_GUI:	break;

	default:
		break;
	
	}

  struct timeval tv;
  struct tm* ptm;
  char time_string[40];
  long milliseconds;
  int nb=0;
  char horodate[128];
  char temp[1024]={0};
  time_t t = time(NULL);
  va_list args;
  
  gettimeofday (&tv, NULL);
  ptm = localtime (&tv.tv_sec);
  strftime (time_string, sizeof (time_string), "%Y-%m-%d %H:%M:%S", ptm);
  milliseconds = tv.tv_usec / 1000;
  
  if ( fmt ) {
	  va_start(args, fmt);
	  //nb = vsnprintf(temp, sizeof(temp) , fmt, args);
	  nb = vsprintf(temp, fmt, args);
	  va_end (args);
  }
  
  // oter tous les '\n' en fin de chaine
  while ( temp && temp[0] && temp[strlen(temp)-1] == '\n' )
    temp[strlen(temp)-1] = 0;
  
  strftime(horodate, sizeof(horodate), "%Y%m%d", localtime(&t));
  
  char log_filename[128];
  sprintf(log_filename,"%s/%s.log", log_path, _ROOT_ALARM_NAME_ );

  FILE* fout=fopen(log_filename,"a+t") ;
  if ( NULL==fout ) {
	  fprintf(stderr,"vega_alarm_log cannot create %s\n",log_filename );
  }else{

	strftime(horodate, sizeof(horodate), "%Y-%m-%d %H:%M:%S", localtime(&t));
	//strftime(horodate, sizeof(horodate), "%H:%M:%S", localtime(&t));
	fprintf(fout,"%s|%d|%02d|%02d|%02d|%s\n",horodate, time(NULL), type_alarm, board, device, temp);
	fprintf(stdout,"%s|%d|%02d|%02d|%02d|%s\n",horodate, time(NULL), type_alarm, board, device, temp);

	fflush(fout);
	fclose(fout);

	}
}

void critical_dump()
{
	return;
	struct timeval tv;
	struct tm* ptm;

	char time_string[40];
	long milliseconds;
	char horodate[128];
	char filename[1024]={0};
	time_t t = time(NULL);

	gettimeofday (&tv, NULL);
	ptm = localtime (&tv.tv_sec);
	strftime (time_string, sizeof (time_string), "%Y-%m-%d %H:%M:%S", ptm);
	milliseconds = tv.tv_usec / 1000;

	strftime(horodate, sizeof(horodate), "%Y%m%d", localtime(&t));
	sprintf(filename,_ROOT_CRITICAL_NAME_"-%s-%d.log",horodate,getpid());
  
	FILE* fout=fopen(filename,"a+t") ;
	
	if ( NULL==fout ) {
		fprintf(stderr,"cannot create %s\n",filename );
		return;
	}

	dump_conferences(fout);

	foreach(device_t* d,device_t::qlist_devices)
	//int i;for (i = 0; i <  g_list_length(devices) ; i++) 
	{
		//device_t* d = (device_t*)g_list_nth_data(devices, i);
		d->fprint_mixers( fout);
	}

	CRM4::dump_devices(fout);
	dump_timeslots(fout,1);
	dump_group(fout);
	/*if (alarms) {
	  alarms_dump(fout, alarms->alarms);
	}*/

	fclose(fout);
}