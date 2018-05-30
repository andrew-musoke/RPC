#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define SIZE 100 //message buffer
#define PORT 1525 //generated using "IPPORT_RESERVED + getuid()"
//FIX
typedef enum{ 
	OK,				/*operationsuccessful*/
	BAD,			/* unrecoverable error */
    WRONG_LENGTH,		/* bad message length supplied */
    CLOSE,
    BAD_TYPE,
    DIV_REM,
    DIV_ZERO
} Status;	
//FIX inlcude enum calcError
typedef struct {
enum {Request, Reply, Error, Stop, Ping} messageType; /* same size as an unsigned int */
unsigned int RPCId; 				/* unique identifier */ 
unsigned int procedureId; 			/*e.g.(1,2,3,4)for(+,-,*,/)*/ 
int arg1; 							/* argument/ return parameter */
int arg2;
int errorcodes; 							/* argument/ return parameter */
} RPCmessage; 						/* each int (and unsigned int) is 32 bits = 4 bytes */

typedef struct {
	unsigned int length;
	unsigned char data[SIZE];
} Message;

typedef struct sockaddr_in SocketAddress ;
//prototypes borrowed
void makeLocalSA(SocketAddress *, int port);
//prototypes required
Status arithmeticService(int op, int a, int b, int *result, int *remainder);
Status DoCalculation(int s, SocketAddress *clientSocketAddr, int errorcodes);
void marshal(RPCmessage *rm, Message *message);
void unMarshal(RPCmessage *rm, Message *message);
Status add(int a,int b,int *);
Status sub(int a,int b,int *);
Status mul(int a,int b,int *);
Status divide(int a,int b,int *,int *);
Status UDPsend(int s, Message *m, SocketAddress dest);
Status UDPreceive(int s, Message *r, SocketAddress *serverSA);
void printLine(void);

int main(int argc , char **argv){

	SocketAddress serverSocketAddr, clientSocketAddr;
	int senderSocket;
	//client socket
	if( (senderSocket= socket(AF_INET,SOCK_DGRAM,0))<0){
		perror("socket failed\n");
		return EXIT_FAILURE;
	}
	//server socket address for binding
	makeLocalSA(&serverSocketAddr,PORT);
	//binding
	if( bind(senderSocket, &serverSocketAddr, sizeof(SocketAddress))!= 0){
		perror("Bind failed\n"); 
		close (senderSocket);
		return EXIT_FAILURE;
	}
	//pass equation
	// Persistence loop
	int errorcodes;
	while(&free){
		Status status;
		status = DoCalculation(senderSocket, &clientSocketAddr, errorcodes);
		//error reporting
		switch(status){
			case BAD:
				errorcodes = 1; 
				DoCalculation(senderSocket, &clientSocketAddr, errorcodes);
				printLine();
				puts("FATAL ERROR!!"); break;
				goto EXITLOOP; 
			case BAD_TYPE:
				errorcodes = 5;
				printLine();
				puts("CLIENT ERROR!!: messageType returned is not request");
				DoCalculation(senderSocket, &clientSocketAddr, errorcodes);
				goto EXITLOOP; 
			case CLOSE: 
				goto EXITLOOP;
			default: errorcodes = 0; break;
		}
	}
	EXITLOOP:puts("\nServer Closed");
	printLine();
	close(senderSocket);
	exit(EXIT_SUCCESS);
}

Status DoCalculation(int s, SocketAddress *clientSocketAddr, int err){
	Message reply, request;
	RPCmessage rpcReply, rpcRequest;
	request.length = reply.length = sizeof(rpcRequest);
	Status status;
	status = UDPreceive(s, &request, clientSocketAddr);
	if (status == BAD){
		return BAD;
	}else if(status == OK){
		printLine();
		printf("\nrequest from %s::%d\n\n",inet_ntoa(clientSocketAddr->sin_addr),ntohs(clientSocketAddr->sin_port));
	}
	unMarshal(&rpcRequest, &request);
	rpcReply.RPCId = rpcRequest.RPCId;
	rpcReply.errorcodes = err;
	switch(rpcRequest.messageType){
		case Stop:
			return CLOSE;
		case Ping:
			rpcRequest.arg1 = rpcRequest.arg2 = 0;
			rpcRequest.procedureId = 1;
			rpcReply.messageType = Ping; break;
		case Request:
			rpcReply.messageType = Reply; break;
		default : 
			puts("ERROR!!: messageType is not request"); 
			return BAD_TYPE;
	}
	//portable arithmetic service as requested
	status = arithmeticService(rpcRequest.procedureId, rpcRequest.arg1, rpcRequest.arg2, &rpcReply.arg1, &rpcReply.arg2);
	switch(status){
		case BAD: return BAD;
		case DIV_ZERO:
				err = 2;
				printLine();
				puts("CLIENT ERROR!!: dividing by zero"); break;
		case DIV_REM:
				err = 3;
		case WRONG_LENGTH:
				err = 4;
				printLine();
				puts("CLIENT ERROR!!: result to long. Truncated"); break;
		default: break;
	}
	if(err > 0){
		rpcReply.messageType = Error;
		rpcReply.errorcodes = err;
	}
	//FIX wronglength if multiplication results in number longer than int
	marshal(&rpcReply, &reply);
	status=UDPsend(s, &reply, *clientSocketAddr);
	if (status == BAD){
		return BAD;
	}else if(status == OK){
		printLine();
		printf("\nreply to %s::%d success!\n\n",inet_ntoa(clientSocketAddr->sin_addr),ntohs(clientSocketAddr->sin_port));
		printLine();
	}
	return OK;
}
void unMarshal(RPCmessage *rm, Message *message){
	memcpy(rm, message->data,sizeof(*rm));
	// deserialization
	rm->RPCId = ntohl(rm->RPCId);
	rm->procedureId = ntohl(rm->procedureId);
	rm->arg1 = ntohl(rm->arg1);
	rm->arg2 = ntohl(rm->arg2);
	rm->errorcodes = ntohl(rm->errorcodes);
return;
}
void marshal(RPCmessage *rm, Message *message){
	// serialization
	rm->RPCId = htonl(rm->RPCId);
	rm->procedureId = htonl(rm->procedureId);
	rm->arg1 = htonl(rm->arg1);
	rm->arg2 = htonl(rm->arg2);
	rm->errorcodes = htonl(rm->errorcodes);
	
	memcpy(message->data, rm,sizeof(*rm));
return;
}

Status arithmeticService(int op, int a, int b, int *result, int *remainder){
	switch(op){
		case 1:
			*result = a + b;
			return OK;
		case 2:
			*result = a - b;
			return OK;
		case 3:
			*result =(unsigned int)(a * b);
			return OK;
		case 4:
			if (b == 0) return DIV_ZERO;
			if ((a%b)!= 0){
				*result = a / b;
				*remainder = a % b;
				return DIV_REM;
			}else{
				*result = a / b;
				return OK;
			}
	}puts("Error in operator switch"); return BAD;
}		

Status UDPreceive(int s, Message *m, SocketAddress *clientSA){
	unsigned int clientSALength = sizeof(SocketAddress);
	clientSA->sin_family = AF_INET;
	int receive_result = recvfrom(s, m->data, sizeof(m->data), 0, clientSA, &clientSALength); 
	m->length = strlen(m->data);
	//pass message
	if(receive_result<0){
		perror("receive ERROR: ");
		return BAD;
	}else return OK;
}

Status UDPsend(int s, Message *r, SocketAddress dest){
	int send_result = sendto(s,r->data,r->length,0, &dest, sizeof(dest));
	if (send_result<0){
		perror("send failed\n");
		return BAD;
	}else return OK;
}

void makeLocalSA(SocketAddress *sa, int port){
	sa->sin_family = AF_INET;
	sa->sin_port = htons(port);
	sa->sin_addr.s_addr = htonl(INADDR_ANY);
}

void printLine(){
	puts("************************************************************************************");
}



















