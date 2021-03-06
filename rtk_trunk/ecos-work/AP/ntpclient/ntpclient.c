/*
 * ntpclient.c - NTP client
 *
 * Copyright 1997, 1999, 2000, 2003  Larry Doolittle  <larry@doolittle.boa.org>
 * Last hack: July 5, 2003
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License (Version 2,
 *  June 1991) as published by the Free Software Foundation.  At the
 *  time of writing, that license was published by the FSF with the URL
 *  http://www.gnu.org/copyleft/gpl.html, and is incorporated herein by
 *  reference.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  Possible future improvements:
 *      - Double check that the originate timestamp in the received packet
 *        corresponds to what we sent.
 *      - Verify that the return packet came from the host we think
 *        we're talking to.  Not necessarily useful since UDP packets
 *        are so easy to forge.
 *      - Write more documentation  :-(
 *
 *  Compile with -D_PRECISION_SIOCGSTAMP if your machine really has it.
 *  There are patches floating around to add this to Linux, but
 *  usually you only get an answer to the nearest jiffy.
 *  Hint for Linux hacker wannabes: look at the usage of get_fast_time()
 *  in net/core/dev.c, and its definition in kernel/time.c .
 *
 *  If the compile gives you any flak, check below in the section
 *  labelled "XXXX fixme - non-automatic build configuration".
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>     /* gethostbyname */
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#ifdef _PRECISION_SIOCGSTAMP
#include <sys/ioctl.h>
#endif
#include <signal.h>
#include "../apmib/apmib.h"


//Brad add sync system time for dhcpd 2008/01/28 
#include <sys/stat.h>
//-------------------------
#define ENABLE_DEBUG

extern char *optarg;

/* XXXX fixme - non-automatic build configuration */
#ifdef linux
#include <sys/utsname.h>
#include <sys/time.h>
typedef u_int32_t __u32;
//#include <sys/timex.h>
#else
extern struct hostent *gethostbyname(const char *name);
extern int h_errno;
#define herror(hostname) \
	fprintf(stderr,"Error %d looking up hostname %s\n", h_errno,hostname)
typedef uint32_t __u32;
#endif

#define JAN_1970        0x83aa7e80      /* 2208988800 1970 - 1900 in seconds */
#define NTP_PORT (123)

#define NTPCLIENT_THREAD_PRIORITY 14
#define NTPCLIENT_THREAD_STACKSIZE 0x00002000

cyg_uint8  ntpclient_stack[NTPCLIENT_THREAD_STACKSIZE];
cyg_handle_t  ntpclient_thread;
cyg_thread  ntpclient_thread_object;

static char ntpclient_running=0;    // Boolean (running loop)
static char ntpclient_started=0;

#ifdef HAVE_SYSTEM_REINIT
static char ntpclient_cleaning=0; 
static cyg_sem_t ntpClient_sem_load;

#endif

#define TIME_ADJUST_INTERVAL (8 * 60 * 60 * 1000)
#define TIME_ADJUST_RETRY    (5 * 2 * 1000)
#define NTP_REPLY_TIMEOUT    1000


/* How to multiply by 4294.967296 quickly (and not quite exactly)
 * without using floating point or greater than 32-bit integers.
 * If you want to fix the last 12 microseconds of error, add in
 * (2911*(x))>>28)
 */
#define NTPFRAC(x) ( 4294*(x) + ( (1981*(x))>>11 ) )

/* The reverse of the above, needed if we want to set our microsecond
 * clock (via settimeofday) based on the incoming time in NTP format.
 * Basically exact.
 */
#define USEC(x) ( ( (x) >> 12 ) - 759 * ( ( ( (x) >> 10 ) + 32768 ) >> 16 ) )

/* Converts NTP delay and dispersion, apparently in seconds scaled
 * by 65536, to microseconds.  RFC1305 states this time is in seconds,
 * doesn't mention the scaling.
 * Should somehow be the same as 1000000 * x / 65536
 */
#define sec2u(x) ( (x) * 15.2587890625 )

struct ntptime {
	unsigned int coarse;
	unsigned int fine;
};

/* prototype for function defined in phaselock.c */
int contemplate_data(unsigned int absolute, double skew, double errorbar, int freq);

/* prototypes for some local routines */
static void send_packet(int usd);
int rfc1305print(uint32_t *data, struct ntptime *arrival);
void udp_handle(int usd, char *data, int data_len, struct sockaddr *sa_source, int sa_len);

/* variables with file scope
 * (I know, bad form, but this is a short program) */
static uint32_t incoming_word[325];
#define incoming ((char *) incoming_word)
#define sizeof_incoming (sizeof(incoming_word)*sizeof(uint32_t))
//static struct timeval time_of_send;
static int live=0;
static int set_clock=0;   /* non-zero presumably needs root privs */

/* when present, debug is a true global, shared with phaselock.c */
#ifdef ENABLE_DEBUG
//int debug=0;
#define DEBUG_OPTION "d"
#else
//#define debug 0
#define DEBUG_OPTION
#endif

int get_current_freq(void)
{
	/* OS dependent routine to get the current value of clock frequency.
	 */
#ifdef linux
	struct timex txc;
	txc.modes=0;
	if (__adjtimex(&txc) < 0) {
		perror("adjtimex"); exit(1);
	}
	return txc.freq;
#else
	return 0;
#endif
}

int set_freq(int new_freq)
{
	/* OS dependent routine to set a new value of clock frequency.
	 */
#ifdef linux
	struct timex txc;
	txc.modes = ADJ_FREQUENCY;
	txc.freq = new_freq;
	if (__adjtimex(&txc) < 0) {
		perror("adjtimex"); exit(1);
	}
	return txc.freq;
#else
	return 0;
#endif
}

static void send_packet(int usd)
{
	__u32 data[12];
	struct timeval now;
#define LI 0
#define VN 3
#define MODE 3
#define STRATUM 0
#define POLL 4 
#define PREC -6

//	if (debug) fprintf(stderr,"Sending ...\n");
	if (sizeof(data) != 48) {
		fprintf(stderr,"size error\n");
		return;
	}
	bzero((char *) data,sizeof(data));
	data[0] = htonl (
		( LI << 30 ) | ( VN << 27 ) | ( MODE << 24 ) |
		( STRATUM << 16) | ( POLL << 8 ) | ( PREC & 0xff ) );
	data[1] = htonl(1<<16);  /* Root Delay (seconds) */
	data[2] = htonl(1<<16);  /* Root Dispersion (seconds) */
	gettimeofday(&now,NULL);
	data[10] = htonl(now.tv_sec + JAN_1970); /* Transmit Timestamp coarse */
	data[11] = htonl(NTPFRAC(now.tv_usec));  /* Transmit Timestamp fine   */
	send(usd,data,48,0);
//	time_of_send=now;
}

void get_packet_timestamp(int usd, struct ntptime *udp_arrival_ntp)
{
	struct timeval udp_arrival;
#ifdef _PRECISION_SIOCGSTAMP
	if ( ioctl(usd, SIOCGSTAMP, &udp_arrival) < 0 ) {
		perror("ioctl-SIOCGSTAMP");
		gettimeofday(&udp_arrival,NULL);
	}
#else
	gettimeofday(&udp_arrival,NULL);
#endif
	udp_arrival_ntp->coarse = udp_arrival.tv_sec + JAN_1970;
	udp_arrival_ntp->fine   = NTPFRAC(udp_arrival.tv_usec);
}

void check_source(int data_len, struct sockaddr *sa_source, int sa_len)
{
	/* This is where one could check that the source is the server we expect */
#if 0
	if (debug) {
		struct sockaddr_in *sa_in=(struct sockaddr_in *)sa_source;
		printf("packet of length %d received\n",data_len);
		if (sa_source->sa_family==AF_INET) {
			printf("Source: INET Port %d host %s\n",
				ntohs(sa_in->sin_port),inet_ntoa(sa_in->sin_addr));
		} else {
			printf("Source: Address family %d\n",sa_source->sa_family);
		}
	}
#endif
}

double ntpdiff( struct ntptime *start, struct ntptime *stop)
{
	int a;
	unsigned int b;
	a = stop->coarse - start->coarse;
	if (stop->fine >= start->fine) {
		b = stop->fine - start->fine;
	} else {
		b = start->fine - stop->fine;
		b = ~b;
		a -= 1;
	}
	
	return a*1.e6 + b * (1.e6/4294967296.0);
}

/* Does more than print, so this name is bogus.
 * It also makes time adjustments, both sudden (-s)
 * and phase-locking (-l).  */
/* return value is number of microseconds uncertainty in answer */
int rfc1305print(uint32_t *data, struct ntptime *arrival)
{
/* straight out of RFC-1305 Appendix A */
	int li, vn, mode, stratum, poll, prec;
	int delay, disp, refid;
	struct ntptime reftime, orgtime, rectime, xmttime;
	double el_time,st_time,skew1,skew2;
	int freq;

#define Data(i) ntohl(((uint32_t *)data)[i])
	li      = Data(0) >> 30 & 0x03;
	vn      = Data(0) >> 27 & 0x07;
	mode    = Data(0) >> 24 & 0x07;
	stratum = Data(0) >> 16 & 0xff;
	poll    = Data(0) >>  8 & 0xff;
	prec    = Data(0)       & 0xff;
	if (prec & 0x80) prec|=0xffffff00;
	delay   = Data(1);
	disp    = Data(2);
	refid   = Data(3);
	reftime.coarse = Data(4);
	reftime.fine   = Data(5);
	orgtime.coarse = Data(6);
	orgtime.fine   = Data(7);
	rectime.coarse = Data(8);
	rectime.fine   = Data(9);
	xmttime.coarse = Data(10);
	xmttime.fine   = Data(11);
#undef Data

//	if (set_clock) {   /* you'd better be root, or ntpclient will crash! */
		struct timeval tv_set;
		/* it would be even better to subtract half the slop */
		tv_set.tv_sec  = xmttime.coarse - JAN_1970;
		/* divide xmttime.fine by 4294.967296 */
//		tv_set.tv_usec = USEC(xmttime.fine);
		
		cyg_libc_time_settime(tv_set.tv_sec);
		
//		if (settimeofday(&tv_set,NULL)<0) {
//			perror("settimeofday");
//			exit(1);
//		}
//		if (debug) {
//			printf("set time to %lu.%.6lu\n", tv_set.tv_sec, tv_set.tv_usec);
//		}
//	}
#if 0
	if (debug) {
	printf("LI=%d  VN=%d  Mode=%d  Stratum=%d  Poll=%d  Precision=%d\n",
		li, vn, mode, stratum, poll, prec);
	printf("Delay=%.1f  Dispersion=%.1f  Refid=%u.%u.%u.%u\n",
		sec2u(delay),sec2u(disp),
		refid>>24&0xff, refid>>16&0xff, refid>>8&0xff, refid&0xff);
	printf("Reference %u.%.10u\n", reftime.coarse, reftime.fine);
	printf("Originate %u.%.10u\n", orgtime.coarse, orgtime.fine);
	printf("Receive   %u.%.10u\n", rectime.coarse, rectime.fine);
	printf("Transmit  %u.%.10u\n", xmttime.coarse, xmttime.fine);
	printf("Our recv  %u.%.10u\n", arrival->coarse, arrival->fine);
	}
#endif
	el_time=ntpdiff(&orgtime,arrival);   /* elapsed */
	st_time=ntpdiff(&rectime,&xmttime);  /* stall */
	skew1=ntpdiff(&orgtime,&rectime);
	skew2=ntpdiff(&xmttime,arrival);
	freq=get_current_freq();
#if 0
	if (debug) {
	printf("Total elapsed: %9.2f\n"
	       "Server stall:  %9.2f\n"
	       "Slop:          %9.2f\n",
		el_time, st_time, el_time-st_time);
	printf("Skew:          %9.2f\n"
	       "Frequency:     %9d\n"
	       " day   second     elapsed    stall     skew  dispersion  freq\n",
		(skew1-skew2)/2, freq);
	}
#endif
	/* Not the ideal order for printing, but we want to be sure
	 * to do all the time-sensitive thinking (and time setting)
	 * before we start the output, especially fflush() (which
	 * could be slow).  Of course, if debug is turned on, speed
	 * has gone down the drain anyway. */
	if (live) {
		int new_freq;
		new_freq = contemplate_data(arrival->coarse, (skew1-skew2)/2,
			el_time+sec2u(disp), freq);
//		if (!debug && new_freq != freq) set_freq(new_freq);
		if (new_freq != freq) set_freq(new_freq);
	}
//	printf("%d %.5d.%.3d  %8.1f %8.1f  %8.1f %8.1f %9d\n",
//		arrival->coarse/86400, arrival->coarse%86400,
//		arrival->fine/4294967, el_time, st_time,
//		(skew1-skew2)/2, sec2u(disp), freq);
//	fflush(stdout);
	return(el_time-st_time);
}

/*
void stuff_net_addr(struct in_addr *p, char *hostname)
{
	struct hostent *ntpserver;
	ntpserver=gethostbyname(hostname);
	if (ntpserver == NULL) {
		herror(hostname);
		exit(1);
	}
	if (ntpserver->h_length != 4) {
		fprintf(stderr,"oops %d\n",ntpserver->h_length);
		exit(1);
	}
	memcpy(&(p->s_addr),ntpserver->h_addr_list[0],4);
}
*/

static void setup_receive(int usd, unsigned int interface, short port)
{
	struct sockaddr_in sa_rcvr;
	bzero((char *) &sa_rcvr, sizeof(sa_rcvr));
	sa_rcvr.sin_family=AF_INET;
	sa_rcvr.sin_addr.s_addr=htonl(interface);
	sa_rcvr.sin_port=htons(port);
	if (bind(usd,(struct sockaddr *) &sa_rcvr,sizeof(sa_rcvr)) == -1) {
		fprintf(stderr,"could not bind to udp port %d\n",port);
		perror("bind");
		exit(1);
	}
	listen(usd,3);
}

static void setup_transmit(int usd, struct in_addr *ntp_server, short port)
{
	struct sockaddr_in sa_dest;
	struct timeval timeout;
	int rc;
	bzero((char *) &sa_dest, sizeof(sa_dest));
	sa_dest.sin_family=AF_INET;
	sa_dest.sin_addr= (*ntp_server);

#if 0
	timeout.tv_sec = NTP_REPLY_TIMEOUT/1000;
	timeout.tv_usec = 0;
	
	rc = setsockopt(usd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
	if(rc)
	{
		diag_printf("Can't set socket option!\n");
		close(usd);
		exit(1);
	}
#endif

//	stuff_net_addr(&(sa_dest.sin_addr),host);
	sa_dest.sin_port=htons(port);
	if (connect(usd,(struct sockaddr *)&sa_dest,sizeof(sa_dest)) == -1) {
		perror("connect");
		exit(1);
	}
}


static void primary_loop(int usd, int num_probes, int interval, int goodness)
{
	fd_set fds;
	struct sockaddr sa_xmit;
	int i, pack_len, sa_xmit_len, probes_sent, error;
	struct timeval to;
	struct ntptime udp_arrival_ntp;

//	if (debug) printf("Listening...\n");

	probes_sent=0;
	sa_xmit_len=sizeof(sa_xmit);
	to.tv_sec=1;
	to.tv_usec=0;
	for (;;) {
#if 0
		FD_ZERO(&fds);
		FD_SET(usd,&fds);
		i=select(usd+1,&fds,NULL,NULL,&to);  /* Wait on read or error */
		if ((i!=1)||(!FD_ISSET(usd,&fds))) {
			if (i==EINTR) continue;
			if (i<0) perror("select");
			if (to.tv_sec == 0) {
				if (probes_sent >= num_probes &&
					num_probes != 0) break;
				send_packet(usd);
				++probes_sent;
				to.tv_sec=interval;
				to.tv_usec=0;
			}	
			continue;
		}
#endif
		

		send_packet(usd);

#ifdef HAVE_SYSTEM_REINIT
		if(ntpclient_cleaning==1)
		{			
			return;
		}
#endif
#ifdef HAVE_SYSTEM_REINIT
		//printf("%s:%d\n",__FUNCTION__,__LINE__);
		if(setsockopt(usd, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to))<0){
				printf("socket option  SO_RCVTIMEO not support\n");
				return;
			}
		while(1)
		{			
			if(ntpclient_cleaning==1)
			{
				
				return;
			}
			//printf("%s:%d\n",__FUNCTION__,__LINE__);
			pack_len=recvfrom(usd,incoming,sizeof_incoming,MSG_DONTWAIT,
			                  &sa_xmit,&sa_xmit_len);
			//printf("%s:%d pack_len=%d\n",__FUNCTION__,__LINE__,pack_len);
			if(pack_len==EWOULDBLOCK || pack_len== EAGAIN)
			{
				printf("%s:%d pack_len=%d\n",__FUNCTION__,__LINE__,pack_len);
				continue;
			}
			error = goodness+1;
			if (pack_len<0) {
				//perror("recvfrom");
			
			cyg_semaphore_timed_wait(&ntpClient_sem_load,cyg_current_time()+TIME_ADJUST_RETRY/10/5);
			} else if (pack_len>0 && (unsigned)pack_len<sizeof_incoming){
				get_packet_timestamp(usd, &udp_arrival_ntp);
				check_source(pack_len, &sa_xmit, sa_xmit_len);
				error = rfc1305print(incoming_word, &udp_arrival_ntp);
				printf("Get ntp time!\n");
				/* udp_handle(usd,incoming,pack_len,&sa_xmit,sa_xmit_len); */
				
				cyg_semaphore_timed_wait(&ntpClient_sem_load,cyg_current_time()+TIME_ADJUST_INTERVAL/10);
				break;
			} else {
				printf("Ooops.  pack_len=%d\n",pack_len);
				fflush(stdout);
				
				cyg_semaphore_timed_wait(&ntpClient_sem_load,cyg_current_time()+TIME_ADJUST_RETRY/10/5);
			}
			
		}
#else
		pack_len=recvfrom(usd,incoming,sizeof_incoming,MSG_DONTWAIT,
		                  &sa_xmit,&sa_xmit_len);

		error = goodness+1;
		if (pack_len<0) {
			perror("recvfrom");
		#ifdef	HAVE_SYSTEM_REINIT
		cyg_semaphore_timed_wait(&ntpClient_sem_load,cyg_current_time()+TIME_ADJUST_RETRY/10/5);
		#else
		sleep(TIME_ADJUST_RETRY/1000/5);
		#endif
		} else if (pack_len>0 && (unsigned)pack_len<sizeof_incoming){
			get_packet_timestamp(usd, &udp_arrival_ntp);
			check_source(pack_len, &sa_xmit, sa_xmit_len);
			error = rfc1305print(incoming_word, &udp_arrival_ntp);
			printf("Get ntp time!\n");
			/* udp_handle(usd,incoming,pack_len,&sa_xmit,sa_xmit_len); */
			#ifdef	HAVE_SYSTEM_REINIT
			cyg_semaphore_timed_wait(&ntpClient_sem_load,cyg_current_time()+TIME_ADJUST_INTERVAL/10);
			#else
			sleep(TIME_ADJUST_INTERVAL/1000);
			#endif
		} else {
			printf("Ooops.  pack_len=%d\n",pack_len);
			fflush(stdout);
			#ifdef	HAVE_SYSTEM_REINIT
			cyg_semaphore_timed_wait(&ntpClient_sem_load,cyg_current_time()+TIME_ADJUST_RETRY/10/5);
			#else
			sleep(TIME_ADJUST_RETRY/1000/5);
			#endif
		}
#endif
		diag_printf("probes_sent is %d, num_probes is %d\n", probes_sent, num_probes);
//		if ( error < goodness && goodness != 0) break;
//		if (probes_sent >= num_probes && num_probes != 0) break;
	}

}

void do_replay(void)
{
	char line[100];
	int n, day, freq, absolute;
	float sec, el_time, st_time, disp;
	double skew, errorbar;
	int simulated_freq = 0;
	unsigned int last_fake_time = 0;
	double fake_delta_time = 0.0;

	while (fgets(line,sizeof(line),stdin)) {
		n=sscanf(line,"%d %f %f %f %lf %f %d",
			&day, &sec, &el_time, &st_time, &skew, &disp, &freq);
		if (n==7) {
			fputs(line,stdout);
			absolute=day*86400+(int)sec;
			errorbar=el_time+disp;
//			if (debug) printf("contemplate %u %.1f %.1f %d\n",
//				absolute,skew,errorbar,freq);
			if (last_fake_time==0) simulated_freq=freq;
			fake_delta_time += (absolute-last_fake_time)*((double)(freq-simulated_freq))/65536;
//			if (debug) printf("fake %f %d \n", fake_delta_time, simulated_freq);
			skew += fake_delta_time;
			freq = simulated_freq;
			last_fake_time=absolute;
			simulated_freq = contemplate_data(absolute, skew, errorbar, freq);
		} else {
			fprintf(stderr,"Replay input error\n");
			exit(2);
		}
	}
}

/*
void usage(char *argv0)
{
	fprintf(stderr,
	"Usage: %s [-c count] [-d] [-g goodness] -h hostname [-i interval]\n"
	"\t[-l] [-p port] [-r] [-s] \n",
	argv0);
}
*/

/*
//Brad add sync system time for dhcpd 2008/01/28 
int getPid(char *filename)
{
	struct stat status;
	char buff[100];
	FILE *fp;

	if ( stat(filename, &status) < 0)
		return -1;
	fp = fopen(filename, "r");
	if (!fp) {
        	printf("Can not open file :%s\n", filename);
		return -1;
   	}
	fgets(buff, 100, fp);
	fclose(fp);

	return (atoi(buff));
}

void Inform_DHCPD(void)
{
	char tmpBuf[100];
	int pid;
	memset(tmpBuf, '\0', 100);
	sprintf(tmpBuf, "%s/%s.pid", "/var/run", "udhcpd");
	pid = getPid(tmpBuf);
	if ( pid > 0)
		kill(pid, SIGUSR2);
	usleep(1000);
}
//-----------------------------------
*/

void getNtpServer(struct in_addr *ntp_server)
{
	int ntp_id;

	apmib_get(MIB_NTP_SERVER_ID,(void *)&ntp_id);
	if(ntp_id == 0)
		apmib_get(MIB_NTP_SERVER_IP1,(void *)ntp_server);
	else if(ntp_id == 1)
		apmib_get(MIB_NTP_SERVER_IP2,(void *)ntp_server);
	else
	{
		diag_printf("Can't get NTP Server IP!\n");
		exit(1);
	}
}

int _ntpclient_main() {
	int usd;  /* socket */
//	int c;
	/* These parameters are settable from the command line
	   the initializations here provide default behavior */
	short int udp_local_port=0;   /* default of 0 means kernel chooses */
	int cycle_time=600;           /* seconds */
	int probe_count=0;            /* default of 0 means loop forever */
	/* int debug=0; is a global above */
	int goodness=0;
//	char *hostname=NULL;          /* must be set */
	int replay=0;                 /* replay mode overrides everything */

	struct in_addr ntp_server;
	
	ntpclient_running = 1;

	getNtpServer(&ntp_server);
//	diag_printf("the host is %s\n", inet_ntoa(ntp_server));
	
	if (replay) {
		do_replay();
		exit(0);
	}

#if 0
	if (debug) {
		printf("Configuration:\n"
		"  -c probe_count %d\n"
		"  -d (debug)     %d\n"
		"  -g goodness    %d\n"
		"  -h hostname    %s\n"
		"  -i interval    %d\n"
		"  -l live        %d\n"
		"  -p local_port  %d\n"
		"  -s set_clock   %d\n",
		probe_count, debug, goodness, hostname, cycle_time,
		live, udp_local_port, set_clock );
	}
#endif

	/*
	// create the pid file
	{
		FILE *fp = NULL;
		pid_t mypid;
		char *pid_file = "/var/run/ntpclient.pid";
		
		if ((fp = fopen(pid_file, "w")) == NULL) {
			fprintf(stderr, "could not create pid file: %s \n", pid_file);
			exit(1);
		}
	
		mypid = getpid();
		fprintf(fp, "%d\n", (int)mypid);
		fclose(fp);
	}
	*/
	cycle_time = 1;
	/* Startup sequence */
	if ((usd=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP))==-1)
		{perror ("socket");exit(1);}

	setup_receive(usd, INADDR_ANY, udp_local_port);

	setup_transmit(usd, &ntp_server, NTP_PORT);
	
	sleep(5);
	primary_loop(usd, probe_count, cycle_time, goodness);
	close(usd);
//Brad add sync system time for dhcpd 2008/01/28 	
//	sprintf(tmpBuf, "echo \"%ld\" > /tmp/dhcpd_unix", sys_time_orig);
//	system(tmpBuf);
//	Inform_DHCPD();
//-----------------------------------------------	
#ifdef HAVE_SYSTEM_REINIT
		ntpclient_cleaning=0;
#endif

	return 0;
}

int start_ntpclient()
{
//	diag_printf("start ntpclient!\n");
	if (ntpclient_started==1)
	{
		diag_printf("Ntpclient has already been startup\n");
		return(-1);
	}	
	if (ntpclient_running==0)
	{
#ifdef HAVE_SYSTEM_REINIT
		cyg_semaphore_init(&ntpClient_sem_load,0);
#endif
		cyg_thread_create(NTPCLIENT_THREAD_PRIORITY,
		_ntpclient_main,
		0,
		"ntpclient",
		ntpclient_stack,
		sizeof(ntpclient_stack),
		&ntpclient_thread,
		&ntpclient_thread_object);
		
		diag_printf("Starting NTP Client thread...\n");
		cyg_thread_resume(ntpclient_thread);
		ntpclient_started=1;
		return(0);
	}
	else
	{
		diag_printf("Ntpclient is already running\n");
		return(-1);
	}
}

#ifdef HAVE_SYSTEM_REINIT
void clean_ntpclient()
{
	if(ntpclient_running)
	{
		ntpclient_cleaning=1;
		
		cyg_semaphore_post(&ntpClient_sem_load);
		while(ntpclient_cleaning){
			cyg_thread_delay(20);
		}
		cyg_semaphore_destroy(&ntpClient_sem_load);

		ntpclient_running=0;
		ntpclient_started=0;
		cyg_thread_kill(ntpclient_thread);
		cyg_thread_delete(ntpclient_thread);
	}
}

#endif


