#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>

struct hostent *gethostbyname() ;
void printSA(struct sockaddr_in sa) ;
void makeDestSA(struct sockaddr_in * sa, char *hostname, int port) ;
void makeLocalSA(struct sockaddr_in *sa) ;
void receiver(int port) ;
void sender(char *message1, char *message2,char *machine, int port);

#define RECIPIENT_PORT 1234
#define SIZE 1000

/* main for sender and receiver - to send give s machine messag1 and message2
	- to receive give r
	*/
void main(int argc,char **argv)
{
	int port = RECIPIENT_PORT;

	if(argc <= 1){
		printf("Usage:s(end) ...or r(eceive) ??\n");
		exit();
	}
	if(*argv[1]  == 's'){
		if(argc <=2) {
			printf("Usage: s  machine message1 message2\n");
			exit();
		}
		sender(argv[3], argv[4], argv[2],port);
	}else if(*argv[1]  == 'r')
		receiver(port);
	else printf("send machine or receive??\n");
}

/*print a socket address */
void printSA(struct sockaddr_in sa)
{
	printf("sa = %d, %s, %d\n", sa.sin_family,
		inet_ntoa(sa.sin_addr), ntohs(sa.sin_port));
}

/* make a socket address for a destination whose machine and port
	are given as arguments */
void makeDestSA(struct sockaddr_in * sa, char *hostname, int port)
{
	struct hostent *host;

	sa->sin_family = AF_INET;
	if((host = gethostbyname(hostname))== NULL){
		printf("Unknown host name\n");
		exit(-1);
	}
	sa-> sin_addr = *(struct in_addr *) (host->h_addr);
	sa->sin_port = htons(port);
}

/* make a socket address using any of the addressses of this computer
for a local socket on any port */
void makeLocalSA(struct sockaddr_in *sa)
{
	sa->sin_family = AF_INET;
	sa->sin_port = htons(0);
	sa-> sin_addr.s_addr = htonl(INADDR_ANY);
}

/* make a socket address using any of the addressses of this computer
for a local socket on given port */
void makeReceiverSA(struct sockaddr_in *sa, int port)
{
	sa->sin_family = AF_INET;
	sa->sin_port = htons(port);
	sa-> sin_addr.s_addr = htonl(INADDR_ANY);
}

/*receive two messages via s new socket,
	print out the messages received and close the socket
	bind to  any of the addresses of this computer
	using port given as argument */
void receiver(int port)
{
	char message1[SIZE], message2[SIZE];
	struct sockaddr_in mySocketAddress, aSocketAddress;
	int s,aLength, n;
	int i;

	if((s = socket(AF_INET, SOCK_DGRAM, 0))<0) {
		perror("socket failed");
		return;
	}
	makeReceiverSA(&mySocketAddress, port);
	if( bind(s, (struct sockaddr *)&mySocketAddress, sizeof(struct sockaddr_in))!= 0){
		perror("Bind failed\n");
		close(s);
		return;
	}
	printSA(mySocketAddress);
	aLength = sizeof(aSocketAddress);
	aSocketAddress.sin_family = AF_INET;
	if((n = recvfrom(s, message1,  SIZE, 0, (struct sockaddr *)&aSocketAddress, &aLength))<0)
		perror("Receive 1") ;
	else{
		printSA(aSocketAddress);
		for(i=0; i<n; i++) printf("M:%d", message1[i]); 
		printf(" Received Message:(%s)length = %d \n",
			message1,n);
	}
	if((n = recvfrom(s, message2,  SIZE, 0, (struct sockaddr *)&aSocketAddress, &aLength))<0)
		perror("Receive 2");
	else {
		printf("Received Message:(%s)length = %d \n",
			message2,n);
	}
	close(s);
}


/*do send after receive ready, open socket
	bind socket to local internet port
		use any of the local computer's addresses
	send two messages with given lengths to machine and  port
	close socket
*/

void sender(char *message1, char *message2,char *machine, int port)
{
	int s, n;
	char message[SIZE];
	struct sockaddr_in mySocketAddress, yourSocketAddress;

	if(( s = socket(AF_INET, SOCK_DGRAM, 0))<0) {
		perror("socket failed");
		return;
	}
/*
	 if((x = setsockopt(s, SOL_SOCKET, SO_BROADCAST, &arg, sizeof(arg))<0)
		perror("setsockopt SO_BROADCAST---");
		exit(-1);
  */
	makeLocalSA(&mySocketAddress);
	if( bind(s, (struct sockaddr *)&mySocketAddress, sizeof(struct sockaddr_in))!= 0){
		perror("Bind failed\n");
		close (s);
		return;
	}
	printSA(mySocketAddress);
	makeDestSA(&yourSocketAddress,machine, port);
	printSA(yourSocketAddress);
	strcpy(message,message1);
	if( (n = sendto(s, message, strlen(message), 0, (struct sockaddr *)&yourSocketAddress,
		sizeof(struct sockaddr_in))) < 0)
		perror("Send failed\n");
	if(n != strlen(message)) printf("sent %d\n",n);
	strcpy(message,message2);
	if( (n = sendto(s, message, strlen(message), 0, (struct sockaddr *)&yourSocketAddress,
		sizeof(struct sockaddr_in))) < 0)
		perror("Send failed\n");
	if(n != strlen(message)) printf("sent %d\n",n);
	close(s);
}

#include <sys/time.h>
/* use select to test whether there is any input on descriptor s*/
int anyThingThere(int s)
{
	unsigned long read_mask;
	struct timeval timeout;
	int n;

	timeout.tv_sec = 10; /*seconds wait*/
	timeout.tv_usec = 0;        /* micro seconds*/
	read_mask = (1<<s);
	if((n = select(32, (fd_set *)&read_mask, 0, 0, &timeout))<0)
		perror("Select fail:\n");
	else printf("n = %d\n");
	return n;
}
