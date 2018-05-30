#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define SIZE 1000 //message buffer
#define PORT 1525 //generated using "IPPORT_RESERVED + getuid()"
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

void makeLocalSA(SocketAddress *);
void makeDestSA(SocketAddress *, const char *hostname, int port);

Status DoOperation (Message *message,Message *reply, int s, SocketAddress receiverSocketAddr); 
Status UDPsend(int s, Message *m, SocketAddress dest);
void UDPreceive(int s, Message *r, SocketAddress *serverSA);
void printLine(void);

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

			if (*p == '\n'){
				*p =0;
				/*debug*///printf("inputArr::%s\n", inputArr);
				message.length = strlen(message.data);
				/*debug*/printf("message.data = %s  message.length = %d\n",message.data,message.length );
			if(DoOperation(&message,&reply, senderSocket, receiverSocketAddr)==OK){
				puts("success with this nonsense");
			}
				else if(DoOperation(&message,&reply, senderSocket, receiverSocketAddr)==BAD){
					puts("failed with this nonsense");
				}else puts("this is nonsense");	

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

Status DoOperation(Message *message, Message *reply, int s, SocketAddress receiverSocketAddr){
	Status status;
	if (UDPsend(s, message,receiverSocketAddr)==OK){
		return OK;
	}else return BAD;
	//receives only from SA sent to.
	UDPreceive(s, reply, &receiverSocketAddr);
}

Status UDPsend(int s, Message *m, SocketAddress dest){
	Status status;
	int send_result = sendto(s,m->data,m->length,0, &dest, sizeof(dest));
	if (send_result<0){
		perror("send failed ");
		return BAD;
	}else {
		printLine();
		printf("\nsend to %s::%d success!\n\n",inet_ntoa(dest.sin_addr),ntohs(dest.sin_port));
		printLine();
		return OK;
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
		printf("\nreply from %s||%d    %s  || length: %d\n\n",inet_ntoa(serverSA->sin_addr),ntohs(serverSA->sin_port),r->data, r->length);
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

/*
*	Pending issues;
*	- Well excercise 2 and 3 are still undone.
*	- look at the persistence loops in both client and server. see if you can make 
*		make it more effecient.
*	- If someone knows how to use enums. The types need to change from void to status
*		i didnt understand what and where type Status returns.
*	- Then the datagram loss experiment. I havent yet researched it. 
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*/ 

















