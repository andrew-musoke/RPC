#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>


#define SIZE 1000 //message buffer
#define PORT 1526 //generated using "IPPORT_RESERVED + getuid()"
#define COUNT 100 //experiment loop
//FIX
typedef enum{ 
	OK,				/*operationsuccessful*/
	BAD,			/* unrecoverable error */
    WRONGLENGTH		/* bad message length supplied */
} Status;	

typedef struct {
	unsigned int length;
	unsigned char data[SIZE];
} Message;

typedef struct sockaddr_in SocketAddress ;
//prototypes borrowed
void makeLocalSA(SocketAddress *);
void makeDestSA(SocketAddress *, const char *hostname, int port);
//prototypes required
void DoOperation (Message *message,Message *reply, int s, SocketAddress receiverSocketAddr); 
void UDPsend(int s, Message *m, SocketAddress dest);
void UDPreceive(int s, Message *r, SocketAddress *serverSA);
void printLine(void);
void DoExperiment(int s, SocketAddress receiverSocketAddr);
int testTimeout(int s);

void main(int argc, char const *argv[]){	
	if(argc<2) {
		puts("Usage: ./UDPclient hostname \n");
		exit(EXIT_FAILURE);
	}

	Message message, reply;
	reply.length = SIZE;
	SocketAddress senderSocketAddr, receiverSocketAddr;
	int senderSocket;

	//client socket
	if( (senderSocket= socket(AF_INET,SOCK_DGRAM,0))<0){
		perror("socket failed\n");
		return;
	}
	//server socket address
	makeDestSA(&receiverSocketAddr,argv[1],PORT);
	//sender Socket address for binding
	makeLocalSA(&senderSocketAddr);
	//binding
	if( bind(senderSocket, &senderSocketAddr, sizeof(SocketAddress))!= 0){
		perror("Bind failed\n"); 
		close (senderSocket);
		return;
	}

	//pass message
	// Persistence loop
	while((strcmp(message.data,"q"))!= 0){
		//find better way
		puts("enter message");
		unsigned char *p,*r ;
		r = reply.data;
		for(p=message.data; p<&message.data[SIZE];p++){
			*p=0;
			*r++=0;
		}
		p = message.data;

		while (p<&message.data[SIZE]){
			*p= getchar();
			//if input is e do experiment then continue with program
			if ((strcmp(message.data,"e")) == 0){
				DoExperiment(senderSocket, receiverSocketAddr);
				break;
			}else if(*p == '\n'){
				*p =0;
				/*debug*///printf("inputArr::%s\n", inputArr);
				message.length = strlen(message.data);
				/*debug*/printf("message.data = %s  message.length = %d\n",message.data,message.length );
			DoOperation(&message,&reply, senderSocket, receiverSocketAddr);	
				break;
			}
			p++;
		}
	}
	puts("\nClient Closed");
	printLine();
	close(senderSocket);
	exit(EXIT_SUCCESS);
}

void DoOperation(Message *message, Message *reply, int s, SocketAddress receiverSocketAddr){
	UDPsend(s, message,receiverSocketAddr);
	//receives only from SA sent to.
	UDPreceive(s, reply, &receiverSocketAddr);
}

void UDPsend(int s, Message *m, SocketAddress dest){
	int send_result = sendto(s,m->data,m->length,0, &dest, sizeof(dest));
	if (send_result<0){
		perror("send failed ");
		return;
	}else {
		printLine();
		printf("\nsend to %s::%d %s success!\n\n",inet_ntoa(dest.sin_addr),ntohs(dest.sin_port),m->data);
		printLine();
		return;
	}
}

void UDPreceive(int s, Message *r, SocketAddress *serverSA){
	/*debug*/ //printf("before receive:%s | %d\n",m->data, m->length );

	unsigned int serverSALength = sizeof(serverSA);
	//char message[SIZE];
	serverSA->sin_family = AF_INET;
	int receive_result = recvfrom(s, r->data, SIZE, 0, serverSA, &serverSALength); 
	//pass reply
		//TODO message length
	r->length = strlen(r->data);
	/*debug*/ //printf("after receive:%s | %d\n",m->data, m->length );
	
	if(receive_result<0){
		perror("receive 1:");
		return;
	}else{
		printf("\nreply from %s::%d    %s  || length: %d\n\n",inet_ntoa(serverSA->sin_addr),ntohs(serverSA->sin_port),r->data, r->length);
		printLine();
		return;
	}
}

void makeLocalSA(SocketAddress *sa){
	sa->sin_family = AF_INET;
	sa->sin_port = htons(0);
	sa->sin_addr.s_addr = htonl(INADDR_ANY);
}

void makeDestSA(SocketAddress *sa, const char *hostname, int port){
	struct hostent *host;
	sa->sin_family = AF_INET;
	if((host=gethostbyname(hostname))==NULL){
		puts("Unknown hostname. Please try again.");
		puts("Usage: ./UDPclient hostname ");
		exit(EXIT_FAILURE);	
	}
	sa->sin_addr= *(struct in_addr *)(host->h_addr);
	sa->sin_port=htons(PORT);
}

void printLine(){
	puts("************************************************************************************");
}

int testTimeout(int s){
  int result;
  fd_set readset;
  struct timeval timeout;

  FD_ZERO(&readset);
  FD_SET(s, &readset);

  timeout.tv_sec = 2; //seconds wait
  timeout.tv_usec = 0; /* micro seconds*/
 
  if((result = select(s + 1, &readset, NULL, NULL, &timeout))<0){
    //some error
    perror("Select fail:\n");
    return (-1);
}else if(result > 0){
    // pending.
    return 1;
  }else return 0;
}

			void DoExperiment(int s, SocketAddress serverSA){
				Message em;
				Message er; 
				em.length = SIZE; er.length = SIZE;	
				float replyCount, sendCount, percent;
				int result;
				sendCount = replyCount = 0;
				char autoM[] = "";
				
				printf("size of autoM: %lu\n",sizeof(autoM) );
				for(int i=0; i < COUNT ; i++){
					memcpy(em.data, autoM, sizeof(autoM));
					UDPsend(s, &em, serverSA);
					sendCount++;

					if ((result = testTimeout(s)) == 1){
						UDPreceive(s, &er, &serverSA);
						replyCount++;
					}
				}
				percent = ((sendCount - replyCount)/sendCount)*100;
				printLine();
				puts("EXPERIMENT DONE!!\n");
				printf("MESSAGES SENT: %.1f \t\t\t MESSAGES RECEIVED: %.1f \n PERCENTAGE LOSS: %.2f percent \n",sendCount,replyCount,percent);
				printLine();
				return;
			}

/*
*	- A char array is initialised with a long string 
*	- Array is copied to message buffer and sent to server.
*	- increment sendcount
*	- wait a few seconds for reply 
*	- if reply then increment replycount otherwise skip
*	- Repeat COUNT times. which is 100 
*	- calculate the difference in counts and print as percentage.
*	- percentage shows how unreliable  UDP is
*
*/