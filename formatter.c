#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/select.h>


#define IDLE_TIMEOUT		200
#define TIME_BUF_SIZE		1000
#define PRINT_BUF_SIZE		1000
#define MAC_ADDR_LEN		(17)
#define TIMESTAMP_FORMAT	"%y-%m-%d %H:%M:%S"

char printbuf[PRINT_BUF_SIZE];
char macAddrStr[18] = "00:00:00:00:00:00";	/* 12 alphanumeric chars, 5 semicolon delimiters, and null terminator */

char * currTime(const char * format);
ssize_t readLine(int fd, void * buffer, size_t n, int to);

void hello(void) {

	fprintf(stderr, "<<new connection>>, %s, pid: %d, mac: %s, remote: %s:%s \r\n",
                                currTime(TIMESTAMP_FORMAT),
                                getpid(),
                                macAddrStr,
                                getenv("TCPREMOTEIP"),
                                getenv("TCPREMOTEPORT"));

}

void printLine(char * p) {

	//	fprintf(stderr, "\033[1;31mbold red text\033[0m\n");
	fprintf(stderr, "    %s, pid: %d, mac: %s, remote: %s:%s, %s \r\n",
				currTime(TIMESTAMP_FORMAT),
				getpid(),
				macAddrStr,
				getenv("TCPREMOTEIP"),
				getenv("TCPREMOTEPORT"),
				p); 	/* print to stderr */	
}

void goodbye(char * p) {

	fprintf(stderr, "<<connection closed>>, %s, pid: %d, mac: %s, remote: %s:%s,  %s \r\n",
				currTime(TIMESTAMP_FORMAT),
				getpid(),
				macAddrStr,
				getenv("TCPREMOTEIP"),
				getenv("TCPREMOTEPORT"),
				p);
}

char * currTime(const char *format)
{
	static char buf[TIME_BUF_SIZE];
	time_t t;
	size_t s;
	struct tm *tm;
	/* Nonreentrant */
	t = time(NULL);
	tm = localtime(&t);
	if (tm == NULL)
		return NULL;
	s = strftime(buf, TIME_BUF_SIZE, (format != NULL) ? format : "%c", tm);
		return (s == 0) ? NULL : buf;
}

/** this function may return a lot of errors **/
// return 	-1	EINVAL 		invalid parameter
//			EBADF		bad file descriptor for select()
//			ETIMEDOUT	timeout
//			
ssize_t readLine(int fd, void *buffer, size_t n, int to)
{

	ssize_t numRead;			/* # of bytes fetched by last read() */
	size_t totRead;				/* Total bytes read so far, aka, buffer pointer */
	char *buf;
	char ch;
	int ret;

	/* for select() */	
	fd_set	set;
	struct timeval timeout;
	
	FD_ZERO(&set);
	FD_SET(0, &set);
	timeout.tv_sec = to;			/* hard coded */
	timeout.tv_usec = 0;


	if (n <= 0 || buffer == NULL || to <= 0) {
		errno = EINVAL;			/* invalid parameters */
		return -1;
	}

	buf = buffer;				/* No pointer arithmetic on "void *" */
	totRead = 0;

	for (;;) {
		for(;;) {

			ret = select(fd + 1, &set, NULL, NULL, &timeout);	/* to timeout the blocking read*/

			if (ret == -1) {
				if (errno == EINTR) 
					continue;

				else {			/* must be EBADF, bad file descriptor, according to TLPI */
					return -1; 		
				}
			} else if (ret == 0) {

				errno = ETIMEDOUT;
				return -1;
			} else {		/* ret  should be one */	
				break;		/* data available */
			}
		}
		
		numRead = read(fd, &ch, 1);	

		if (numRead == -1) {
			if (errno == EINTR)	/* Interrupted --> restart read() */
				continue;	
			else
				return -1;	/* Some other error, man read */
		} else if (numRead == 0) {	/* EOF */
			if (totRead == 0)
				return 0;	/* No bytes read; return 0 */
			else
				break; 		/* Some bytes read; add '\0' */

		} else {			/* 'numRead' must be 1 if we get here */

			if (buf == buffer && (ch == '\r' || ch == '\n')) 	/* discard leading \r or \n */
				continue;
	
			if (totRead < n - 1) { 	/* Discard > (n - 1) bytes */

				if (ch == '\r' || ch == '\n') {

					*buf = '\0';
					return totRead;
				}

				totRead++;
				*buf++ = ch;
			} 							
			
			if (ch == '\r' || ch == '\n')	break;
		}	
	}

	*buf = '\0';
	return totRead;
}

bool isValidMacAddrStr(char* str) {
	
	return true;				/* TODO place holder */
}

void main(int argc, char **argv) {

	char linebuf[1024];				/* buffer for readline */
	ssize_t numRead = 0;				/* return for readline */
	char * p;					/* one-time char pointer */

	hello();

	for (;;) {

		numRead = readLine(0, linebuf, sizeof(linebuf), IDLE_TIMEOUT);	/* TODO timeout should be argument */
					
		if (numRead == -1) {

			if (errno == ETIMEDOUT) {
				
				
			}

			snprintf(printbuf, PRINT_BUF_SIZE, "readLine() error, errno: %d", errno);
			goodbye(printbuf);
			exit(EXIT_FAILURE);

		}
		else if (numRead == 0) {

			snprintf(printbuf, PRINT_BUF_SIZE, "readLine() reaches EOF (connection disconnect remotely)");
			goodbye(printbuf);
			exit(EXIT_SUCCESS);
		}

		if (linebuf == strstr(linebuf, "PING")) {	/* check ping */

			if (	numRead >= (5 /* ping and space */ + MAC_ADDR_LEN + 2 /* \r\n */) &&
				isValidMacAddrStr(&linebuf[5]) ) {
			
				strncpy(macAddrStr, &linebuf[5], MAC_ADDR_LEN);
			}

			fprintf(stdout, "PONG\r\n");		/* reply anyway */
			printLine("ping ... pong ...");
			continue;
		}

		printLine(linebuf);		
	}
}

