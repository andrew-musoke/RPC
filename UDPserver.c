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
#define PORT 1526 //generated using "int aPort=IPPORT_RESERVED + getuid();"
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
void makeLocalSA(SocketAddress *, int port);
//prototypes required
void GetRequest (Message *callMessage, int s, SocketAddress *clientSA);
void SendReply (Message *replyMessage, int s, SocketAddress clientSA);
void UDPsend(int s, Message *m, SocketAddress dest);
void UDPreceive(int s, Message *r, SocketAddress *clientSA);
void printLine(void);
const char bye[] = "You closed the server!!!";

void main(int argc, char const *argv[])
{
	Message callMessage;
	SocketAddress serverSocketAddr, clientSocketAddr;
	int senderSocket;
	//server socket
	if( (senderSocket= socket(AF_INET,SOCK_DGRAM,0))<0){
		perror("socket failed\n");
		return;
	}
	//server Socket address for binding
	makeLocalSA(&serverSocketAddr,PORT);
	//binding
	if( bind(senderSocket,&serverSocketAddr, sizeof(SocketAddress))!= 0){
		perror("Bind failed\n"); 
		close (senderSocket);
		return;
	}
	//Persistence loop
	while(strcmp(callMessage.data,"q") !=0){
		//find better way
		// to empty buffer
		unsigned char *p;
		for(p=callMessage.data; p<&callMessage.data[SIZE];p++){
			*p=0;
		}
		//receive message
		GetRequest (&callMessage, senderSocket, &clientSocketAddr);
		/*debug*/ //printf("receiverSA in main: %s ||%d",inet_ntoa(clientSocketAddr->sin_addr),ntohs(clientSocketAddr->sin_port)); 
		//send final notice to client
		if (strcmp(callMessage.data,"q")==0)
		{
			strcpy((char *)callMessage.data, bye);
			callMessage.length = strlen(bye);	
			SendReply (&callMessage, senderSocket, clientSocketAddr);
			break;
		}
	
		//send message
		SendReply (&callMessage, senderSocket, clientSocketAddr);
	}
	
	//doh
	printf("\n SERVER CLOSED\n");
	printLine();
	close(senderSocket);
	
	exit(EXIT_SUCCESS);
}

void GetRequest (Message *callMessage, int s, SocketAddress *clientSA){
	UDPreceive(s, callMessage, clientSA);
	/*debug*/ //printf("receive success!!!\n");
	return;
}

void SendReply (Message *replyMessage, int s, SocketAddress clientSocketAddr){
		UDPsend(s, replyMessage,clientSocketAddr);
	// close(s);
}

void UDPreceive(int s, Message *m, SocketAddress *clientSA){
	/*debug*/ //printf("before receive:%s | %d\n",m->data, m->length );
	unsigned int clientSALength = sizeof(SocketAddress);
	clientSA->sin_family = AF_INET;
	int receive_result = recvfrom(s, m->data, SIZE, 0, clientSA, &clientSALength); 
	m->length = strlen(m->data);
	//pass message
	if(receive_result<0){
		perror("receive 1");
		return ;
	}else{
		printLine();
		printf("\nmessage received from %s||%d    %s  || length: %d\n\n",inet_ntoa(clientSA->sin_addr),ntohs(clientSA->sin_port),m->data, m->length);
		printLine();
		return;
	}
}

void UDPsend(int s, Message *r, SocketAddress dest){
	int send_result = sendto(s,r->data,r->length,0, &dest, sizeof(dest));
	if (send_result<0){
		perror("send failed\n");
		return;
	}else{
		/*debug*/ //printf("message is: %s || %d\n",  r->data,send_result);
		printf("\nreply to %s||%d success!\n\n",inet_ntoa(dest.sin_addr),ntohs(dest.sin_port));
		printLine();
		return;
	}
}

void makeLocalSA(SocketAddress *sa, int localport){
	sa->sin_family = AF_INET;
	sa->sin_port = htons(localport);
	sa->sin_addr.s_addr = htonl(INADDR_ANY);
}

void printLine(){
	printf("************************************************************************************\n");
}




















