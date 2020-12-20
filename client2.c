#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define UTC_NTP 2208988800U /* 1970 - 1900 */

void gettime64(uint32_t ts[]);
int die(const char *msg);
int useage(const char *path);
int openConnect(const char* server);
void requestTime(int fd);
void getReply(int fd);
int client(const char* server);


void gettime64(uint32_t ts[])
{
	struct timeval tv;
	gettimeofday(&tv, NULL);

	ts[0] = tv.tv_sec + UTC_NTP;
	ts[1] = (4294*(tv.tv_usec)) + ((1981*(tv.tv_usec))>>11);
}

int die(const char *msg)
{
	fputs(msg, stderr);
	exit(-1);
}

int useage(const char *path)
{
	printf("Useage:\n\t%s <server address>\n", path);
	return 1;
}


int openConnect(const char* server)
{
	int s;
	struct addrinfo *saddr;

	printf("Connecting to server: %s\n", server);
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s == -1) {
		die("Could not create socket.\n");
	}

	if (0 != getaddrinfo(server, "8123", NULL, &saddr)) {
		die("Ip Address Server does not match.\n");
	}

	if (connect(s, saddr->ai_addr, saddr->ai_addrlen) != 0) {
		die("Connect error\n");
	}

	freeaddrinfo(saddr);

	return s;
}

void requestTime(int fd)
{
	unsigned char buf[48] = {0};
	uint32_t tts[2];/* Transmit Timestamp */



	buf[0] = 0x23;

	gettime64(tts);
	(*(uint32_t *)&buf[40]) = htonl(tts[0]);
	(*(uint32_t *)&buf[44])= htonl(tts[1]);
	if (send(fd, buf, 48, 0) !=48 ) {
		die("Send error\n");
	}
}

void getReply(int fd)
{
	unsigned char buf[48];
	uint32_t *pt;

	uint32_t t1[2]; /* t1 = Originate Timestamp  */
        uint32_t t2[2]; /* t2 = Receive Timestamp @ Server */
        uint32_t t3[2]; /* t3 = Transmit Timestamp @ Server */
        uint32_t t4[2]; /* t4 = Receive Timestamp @ Client */

	double T1, T2, T3, T4;
	double tfrac = 4294967296.0;
	time_t curr_time;
	time_t diff_sec;

	if (recv(fd, buf, 48, 0) < 48) {
		die("Receive error\n");
	}
	gettime64(t4);
	pt = (uint32_t *)&buf[24];

	t1[0] = htonl(*pt++);
	t1[1] = htonl(*pt++);

	t2[0] = htonl(*pt++);
	t2[1] = htonl(*pt++);

	t3[0] = htonl(*pt++);
	t3[1] = htonl(*pt++);


	/* Transmit Timestamp @ Client */

	T1 = t1[0] + t1[1]/tfrac;
	T2 = t2[0] + t2[1]/tfrac;
	T3 = t3[0] + t3[1]/tfrac;
	T4 = t4[0] + t4[1]/tfrac;

	printf( "\ndelay = %lf\n" "offset = %lf\n\n",(T4-T1) - (T3-T2),((T2 - T1) + (T3 - T4)) /2);

	diff_sec = ((int32_t)(t2[0] - t1[0]) + (int32_t)(t3[0] - t4[0])) /2;
	curr_time = time(NULL) - diff_sec;
	printf("Current Time Display at Server:   %s\n", ctime(&curr_time));
}

int client(const char* server)
{
	int fd;

	fd = openConnect(server);
	requestTime(fd);
	getReply(fd);
	close(fd);

	return 0;
}


int main(int argc, char *argv[], char **env)
{
	if (argc < 2) {
		return useage(argv[0]);
	}

	return client(argv[1]);
}

