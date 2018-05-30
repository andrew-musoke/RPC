#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define SIZE 20 //message buffer
#define PORT 1525 //generated using "IPPORT_RESERVED + getuid()"
#define TRY 3 // how many times client should attempt to resend.
typedef enum{ 
	OK,				/*operationsuccessful*/
	BAD,			/* unrecoverable error */
    WRONGLENGTH,		/* bad message length supplied */
    CLOSE,
    BAD_TYPE,
    BAD_ID,
    DIVZERO,
    DIVREM,
    PING,
    SERVER_CLOSED
} Status;	

typedef struct {
enum {Request, Reply,Error , Stop, Ping} messageType; 	/* same size as an unsigned int */
unsigned int RPCId; 				/* unique identifier */ 
unsigned int procedureId; 			/*e.g.(1,2,3,4)for(+,-,*,/)*/ 
int arg1; 							/* argument/ return parameter */
int arg2; 							/* argument/ return parameter */
int errorcodes;
} RPCmessage; 						/* each int (and unsigned int) is 32 bits = 4 bytes */

typedef struct {
	unsigned int length;
	unsigned char data[SIZE];
} Message;

typedef struct sockaddr_in SocketAddress ;
//prototypes borrowed
void makeLocalSA(SocketAddress *);
void makeDestSA(SocketAddress *, const char *hostname, int port);
//prototypes required
Status DoOperation(char a[] ,char b[], char op, int id, int s, SocketAddress receiverSocketAddr);
void marshal(RPCmessage *rm, Message *message);
void unMarshal(RPCmessage *rm, Message *message);
Status UDPsend(int s, Message *m, SocketAddress dest);
Status UDPreceive(int s, Message *r, SocketAddress *serverSA);
int testTimeout(int s);
void printLine(void);

int main(int argc , char *argv[]){
	if(argc != 2) {
		puts("Usage: ./UDPclient hostname \n");
		exit(EXIT_FAILURE);
	}

	SocketAddress senderSocketAddr, receiverSocketAddr;
	int senderSocket;
	//client socket
	if( (senderSocket= socket(AF_INET,SOCK_DGRAM,0))<0){
		perror("socket failed\n");
		return EXIT_FAILURE;
	}
	//server socket address
	makeDestSA(&receiverSocketAddr,argv[1],PORT);
	//sender Socket address for binding
	makeLocalSA(&senderSocketAddr);
	//binding
	if( bind(senderSocket, &senderSocketAddr, sizeof(SocketAddress))!= 0){
		perror("Bind failed\n"); 
		close (senderSocket);
		return EXIT_FAILURE;
	}
	//pass equation
	// Persistence loop
	int id = 0;				//initialize id
	while(&free){
		puts("enter equation eg: 2+2 | stop to close | ping");
		char arg1[SIZE],arg2[SIZE],operator,*p,*r;
		int i = 0;
		p = arg1; r = arg2;
		while(i<SIZE){
			*p = getchar();
		if(*p == '+'||*p == '*'||*p == '-'||*p == '/'){
				operator = *p;
				*p = 0;
				break;
			}else if(*p == '\n'){
				*p = 0;
				operator = 0;
				break;
			} 
			p++; 
			i++;
		}
		i = 0;
		while(i<SIZE){
			if(operator == 0) break;
			*r = getchar();
			if(*r == '\n'){
				*r = 0;
				break;
			} 
			r++; 
			i++;
		}
		++id;				//increment id every loop
		Status status;
		status = DoOperation(arg1 , arg2, operator, id, senderSocket, receiverSocketAddr);
		// error/status reporting
		switch(status){
			case BAD:
				puts("CLIENT CLOSED WITH UNRECOVERABLE ERROR: check usage and try again!");
				goto EXITLOOP; 
			case DIVZERO:
				printLine();
				puts("No division by zero! Usage for division: a/b | a > b > 0"); break; 
			case CLOSE: 
				goto EXITLOOP;
			case BAD_TYPE:
				printLine();
				puts("SERVER ERROR!!: messageType returned is not reply"); break;
			case BAD_ID: 
				printLine();
				puts("SERVER ERROR!!: reply ID differs from request ID"); break;
			case SERVER_CLOSED: 
				printLine();
				puts("SERVER CLOSED!!: check server and try again"); break;
			case PING: 
				puts("SERVER AWAKE:");
				printLine(); break;
			case WRONGLENGTH:   //FIX add divrem
				printLine();
				puts("Bad Length Supplied.");
				goto EXITLOOP; 
			default: continue;
		}
	}
	EXITLOOP: puts("\nClient Closed");
	printLine();
	close(senderSocket);
	exit(EXIT_SUCCESS);
}

Status DoOperation(char a[] ,char b[], char op, int id, int s, SocketAddress receiverSocketAddr){
	Message message, reply;
	RPCmessage rpcMessage, rpcReply;
	rpcMessage.RPCId = id;				//assign Rpc ID
	rpcMessage.errorcodes = 0;
	Status status;
	rpcMessage.messageType = Request;
	message.length = SIZE;
	//check for chars stop and ping 
	if(strcmp(a,"stop") == 0 && op == 0){
		rpcMessage.arg1 = 0; rpcMessage.arg2 = 0; op = '+';
		rpcMessage.messageType = Stop;
	}else if(strcmp(a,"ping") == 0 && op == 0){
		rpcMessage.arg1 = 0; rpcMessage.arg2 = 0; op = '+';
		rpcMessage.messageType = Ping;
	}else{
		//char to int
		sscanf(a,"%u",&rpcMessage.arg1);
		sscanf(b,"%u",&rpcMessage.arg2);
	}
	switch(op){
		case '+': 
			rpcMessage.procedureId = 1;break;
		case '-': 
			rpcMessage.procedureId = 2;break;
		case '*': 
			rpcMessage.procedureId = 3;break;
		case '/': 
			rpcMessage.procedureId = 4;break;
		default:
			puts("Error in operator: Usage a (+|-|*|/) b");
	}
	
	//client retrial loop.
	int result, tryCount;
	result = tryCount = 0;
	while(result != 1 && tryCount < TRY){
		marshal(&rpcMessage, &message);
		status = UDPsend(s, &message, receiverSocketAddr);
		if (status == BAD){
			return BAD;
		}else if(status == OK){
			printLine();
			printf("\nsend to %s::%d success!\n\n",inet_ntoa(receiverSocketAddr.sin_addr),ntohs(receiverSocketAddr.sin_port));
			printLine();
		}
		//close without waiting for receive. //command from client
		if(rpcMessage.messageType == Stop){
			return CLOSE;
		}else if(op == 0)return BAD;
		//wait t seconds and move 
		if ((result = testTimeout(s)) == 1){
			status = UDPreceive(s, &reply, &receiverSocketAddr);
			if (status == BAD){
				return BAD;
			}else if(status == OK){
				printLine();
				printf("\nreply from %s::%d\n\n",inet_ntoa(receiverSocketAddr.sin_addr),ntohs(receiverSocketAddr.sin_port));
				printLine();
			}

			unMarshal(&rpcReply, &reply);
			// message validity checks
			if(rpcReply.RPCId != id)return BAD_ID;
			puts("unmarshalling");
			switch(rpcReply.messageType){
				//FIX 
				case Ping: return PING;
				case Reply: break;
				case Error:  puts("in default");break;
					puts("entered default");
					switch(rpcReply.errorcodes){
				// 0:no error| 1:bad | 2: div-zero | 3: div-rem | 4:wrong-length | 5:client bad-type
						case 1: 
							printLine();
							puts("SERVER CLOSED WITH UNRECOVERABLE ERROR");
							return SERVER_CLOSED;
						case 2: return DIVZERO;
						case 3: 
							printLine();
							printf("\tAnswer :\t %s%c%s = %d remainder %d\n",a,op,b,rpcReply.arg1,rpcReply.arg2);
							printLine(); return OK;
						case 4: return WRONGLENGTH;
						case 5: return BAD;
						
						} 
				default: return BAD_TYPE;
			}			
			printLine();
			printf("\tAnswer :\t %s%c%s = %d \n",a,op,b,rpcReply.arg1);
			printLine();
			return OK;
		}
		printf("Error in sending. Client retry number %d\n",++tryCount);
	}return SERVER_CLOSED;
}

void marshal(RPCmessage *rm, Message *message){
	//serializing
	rm->RPCId = htonl(rm->RPCId);
	rm->procedureId = htonl(rm->procedureId);
	rm->arg1 = htonl(rm->arg1);
	rm->arg2 = htonl(rm->arg2);
	rm->errorcodes = htonl(rm->errorcodes);

	memcpy(message->data,rm,sizeof(*rm));
	message->length = sizeof(*rm);
return;
}

void unMarshal(RPCmessage *rm, Message *message){
	memcpy(rm, message->data, sizeof(*rm));
	// deserializing
	rm->RPCId = ntohl(rm->RPCId);
	rm->procedureId = ntohl(rm->procedureId);
	rm->arg1 = ntohl(rm->arg1);
	rm->arg2 = ntohl(rm->arg2);
	rm->errorcodes = ntohl(rm->errorcodes);
return;
}

Status UDPsend(int s, Message *m, SocketAddress dest){
	int send_result = sendto(s,m->data,m->length,0, &dest, sizeof(dest));
	if (send_result<0){
		perror("send failed ");
		return BAD;
	}else return OK;
}

Status UDPreceive(int s, Message *r, SocketAddress *serverSA){
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
		return BAD;
	} else return OK;
}

int testTimeout(int s){
  int result;
  fd_set readset;
  struct timeval timeout;

  FD_ZERO(&readset);
  FD_SET(s, &readset);

  timeout.tv_sec = 5; 				//seconds wait
  timeout.tv_usec = 0; 				/* micro seconds*/
 
  if((result = select(s + 1, &readset, NULL, NULL, &timeout))<0){
    perror("Select fail:\n");		//some select error
    return (-1);
	}else if(result > 0){
    	return 1;					// pending request.
  	}else return 0;
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
	printf("************************************************************************************\n");
}



//errors server side
//remainder for dividing
//seperate arithmetic service
//seperate header files
// constrict code
















