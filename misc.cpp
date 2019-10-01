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

#include "misc.h"
#include "debug.h"
#include "misc.h"

#define min(a, b)	((a) < (b) ? (a) : (b))

void restaure_options (void);
int install_options (void);

const char *strncpy_only_digit_az_AZ (char *dest, const char *src, size_t n /*taille maxi de dest */ )
{
  char *d = dest;
  if (0 == dest)
    return NULL;
  if (0 == src)
    {
      *dest = 0;
      return dest;
    }
  while (n-- && *src)
    {
      if ( ( (*src >= '0') && (*src <= '9') ) || 
		  ((*src >= 'a') && (*src <= 'z')) || 
		  ( *src ==' ') ||  // pdl 20090911 accepte aussi les espaces
		  ( *src =='.') ||  // pdl 20090911 accepte aussi les points
		  ((*src >= 'A') && (*src <= 'Z')) 
	
		  )
		{
		  *d++ = *src++;
		}
      else
	break;
    }
  *d = 0;
  return src;
}

double time_between_button(struct timeval* ptv)
{
	  struct timeval _now;	      
	  gettimeofday(&_now, NULL);		
	  double t1 =  (double)ptv->tv_sec;
	  double t2 =  (double)_now.tv_sec;
	  t2 += (double)_now.tv_usec/(1000*1000);
	  t1 += (double)ptv->tv_usec/(1000*1000);
	  //vega_log(INFO,"BUTTON within %f seconds\n", t2-t1);
	  /*if ( t2 - t1 < 1.0 ){				
		  vega_log(INFO,"IGNORED EVT_DEVICE_KEY_SELECT_CONFERENCE within %f seconds\n",t2-t1);
		  break;		
	  }*/
	  //gettimeofday(&d->_tstart_btn_conf, NULL); // memoriser la date a laquelle on prend en compte l'evt
	  return t2 - t1;
}

double tval(struct timeval* timeval1,struct timeval* timeval2)
{
    double t1, t2;

    t1 =  (double)timeval1->tv_sec 
		+ (double)timeval1->tv_usec/(1000*1000);
    
	t2 =  (double)timeval2->tv_sec + 
		  (double)timeval2->tv_usec/(1000*1000);
    return t2-t1;
}

int kbhit (int delay)
{
  struct pollfd pollfd;
  int ret;
  pollfd.fd = 0;
  pollfd.events = POLLIN;
  if ((ret = poll (&pollfd, 1, delay)) == -1)
    {
      if (errno != EINTR)
	{
	  perror ("<poll>");
	  fprintf (stderr, "err %d\n", errno);
	}
      else
	{
	  ret = 0;
	}
    }
  return ret;
}

/* Return TRUE if anything has been typed on stdin, FALSE otherwise */
int dos_kbhit (void)
{
  struct pollfd fds;		/* list of file descriptor info */
  /* set up the poll array */
  memset (&fds, 0, sizeof (fds));
  /* we're interested in reading from stdin */
  fds.fd = 0;			// console
  fds.events = POLLIN;
  if (poll (&fds, 1, 0) < 0)
    {
      return (-1);
    }
  if (fds.revents & POLLIN)
    {
      return (0);
    }
  return (-1);
}

int install_options (void)
{
  struct termios term;
  if (tcgetattr (STDIN_FILENO, &term) == -1)
    {
      perror ("tcgetattr");
    }
  term.c_lflag &= ~(ICANON | ECHO);
  term.c_cc[VMIN] = 1;
  term.c_cc[VTIME] = 0;
  if (tcsetattr (STDIN_FILENO, TCSAFLUSH, &term) == -1)
    {
      perror ("tcsetattr");
    }
  atexit (restaure_options);
  return 0;
}

void restaure_options (void)
{
  struct termios term;
  if (tcgetattr (STDIN_FILENO, &term) == -1)
    {
      perror ("tcgetattr");
    }
  term.c_lflag |= ICANON | ECHO;
  if (tcsetattr (STDIN_FILENO, TCSAFLUSH, &term) == -1)
    {
      perror ("tcsetattr");
    }
  return ;
}

void readline(FILE*fi, char*line, int maxlen, int bCutOffComments)  // todo: handle error while reading file!
{
    int pos = 0;
    while(!feof(fi)) 
	{
        if(!fread(&line[pos],1,1,fi))
            break;
        if(line[pos] == 13 || line[pos]==10)
            break;
		if(pos<maxlen-1)
			pos++;
    }
    line[pos]=0;    
    char* x = strchr(line,'#');
	if (bCutOffComments)
		if(x) {/* cut off comments */	
			*x=0;pos = x-line;
		}
    /* cut off trailing whitespace */
    while(pos>=1 && (line[pos-1]==32 || line[pos-1]==9)) 
	{
		pos--;
		line[pos]=0;
    }
	//printf("ligne lue: %s\n",line);
}

DWORD GetPrivateProfileString(const char* section,const char* lpKeyName,const char* lpDefault, char* lpReturnedString, DWORD nSize, const char* filename)
{
	FILE*fi=0;
    char line[256];	
	int bSectionFound=false;
	int ret_section_found = false;
	DWORD ret=0;
	
	//printf("GetPrivateProfileString '%s' dans [%s]...\n",lpKeyName,section);
	// default values:
	strncpy(lpReturnedString,lpDefault,nSize);
	ret=strlen(lpReturnedString);

	fi=fopen(filename, "rb");
    while(fi && !feof(fi) && !ret_section_found)
    {
		char* equal;
		char* debutsection;
		char* finsection;
		readline(fi, line, sizeof(line),1);
		if ( ( debutsection = strchr(line, '[') ) && ( finsection = strchr(line, ']') ) &&( finsection > debutsection ) )
		{
			char temp[64]={0};
			memcpy (temp,debutsection+1, min((finsection - debutsection-1),sizeof(temp)) ) ;
			temp[finsection - debutsection]=0;			
			if (0==strcasecmp(temp, section))
			{
				bSectionFound = true;
				//printf("[%s] found in %s\n",section,filename);
			}else{
				if (true==bSectionFound) 
					ret_section_found = true;
				else
					bSectionFound = false;
			}
		}	
		
		if (bSectionFound)
		{			
			if( (equal = strchr(line, '=') ) )
			{				
				char* x ;
				*equal = 0; equal++;	
				// supprimer les espaces	//g_strstrip(equal);
				while ( ' '==*equal) equal++;
				if ( equal && (0!=*equal) && (strlen(equal)>2)) while ( ' '==equal[strlen(equal)-1] ) equal[strlen(equal)-1]=0;

				if( (x = strchr(line,'#') ) ) 
				{// # -> commentaire
					x++;
					//printf("COMMENT:|%s=%s|\n",x,equal);
				}else
					if( (x = strchr(line,';')) ) 
					{// ; -> commentaire
						x++;
						//printf("COMMENT:|%s=%s|\n",x,equal);
					}else{
						x = line;
						
						//printf("(1)x=-%s-\n",x);
						while ( ' '==*x) x++;						
						//printf("(2)x=-%s-\n",x);
						
						if ( x && (0!=*x) && (strlen(x)>1)) 
							while ( ' '==x[strlen(x)-1] ) 
							{
								x[strlen(x)-1]=0;
								//printf("(3)x=-%s-\n",x);	
							}
						
						//printf("%s=%s\n",x,equal);
						if (0 == strcasecmp(lpKeyName,x) ){
							//debug_printf("FOUND:[%s]%s=%s.\n",section,x,equal);
							strncpy(lpReturnedString,equal,nSize);
							ret=strlen(lpReturnedString);
							ret_section_found = true;
						}
					}
			}			
		}
	}
	if (fi) fclose(fi);
	//printf("GetPrivateProfileString returned %d\n",ret);
	return ret_section_found;
}


int GetPrivateProfileStringKeyFromValue(const char* section_rech, const char* value_rech,char* clef_rech, unsigned int len_clef_rech, const char* filename)
{
    char line[256]={0};
	int bSectionFound=false;	
	int ret_section_found = false;

	if ( !section_rech )		return false;
	if ( 0 == strlen(section_rech) )		return false;
	if ( !value_rech )		return false;
	if ( 0 == strlen(value_rech) )		return false;
	if ( !clef_rech )		return false;
	if ( 0 == len_clef_rech )		return false;
	if ( 0 == filename )		return false;	
	
	FILE*fi=fopen(filename, "rb");
    while(fi && !feof(fi) && !ret_section_found)
    {		
		readline(fi, line, 256,1);
		char*  debutsection;char* finsection;
		if ( ( debutsection = strchr(line, '[') ) && 
			( finsection = strchr(line, ']') ) &&
			( finsection > debutsection ) )
		{
			char temp[256]={0};
			memcpy (temp,debutsection+1, min((finsection - debutsection-1),sizeof(temp)) ) ;
			temp[finsection - debutsection]=0;			
			if (!strcasecmp(temp, section_rech)){
				bSectionFound = true;
				//printf("%s FOUND\n",temp);
			}else{
				if (true==bSectionFound)
					ret_section_found= true; // fin de la bonne section
				bSectionFound = false;
			}
		}
		if (bSectionFound)
		{
			char* equal;
			if( (equal = strchr(line, '=')  ) )
			{
				*equal = 0; equal++;	//if (!strcasecmp(line, "4321"))		
				if ( (line) && (equal) && (0==strcasecmp(equal,value_rech)) ){
					strncpy(clef_rech, line, len_clef_rech);
					//printf("GetPrivateProfileStringKeyFromValue:%s\n",line);
					ret_section_found = true;
				}
			}
		}		
	}
	if (fi) fclose(fi);
	return ret_section_found;
}
