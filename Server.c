#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include "Internal.h"
#include "GenericDoubleList.h"
#include "ListFunc.h"
#include "ListItr.h"
#include "Server.h"


#define ADDR "127.0.0.1"
#define MAGIC_NUMBER 123654789
#define FAIL -1
#define SUCCESS 0
#define MIN_SIZE_PORT 1024
#define MAX_SIZE_PORT 65535
#define MIN_SOCK 3
#define MAX_SOCK 1023

typedef struct sockaddr_in SocAdd;

struct Server
{
	List *m_list;
	int m_sock;
	size_t m_magicNumber;
	int m_countClient;
	fd_set m_master;
	fd_set m_temp;
	int m_activity;
	NewClient m_newClient;
	Fail m_fail;
	GotMessage m_gotMessage;
	CloseClient m_closeClient;
	int m_flag;
	void * m_contex;
};

static SE Connect(Serv* _server);
static SE CheckParameters(int _sock, void* _data, size_t _dataLen);
static SE CheckCurrClient(int _sock, GotMessage _gotMessage, void* _context);
static SE ForCheckClients(Serv* _server);
static void Destroy( void *_sock);


Serv* CreateServer( NewClient _newClient, Fail _fail, GotMessage _gotMessage, CloseClient _closeClient, int _port, SE* _statusRes, int _backLog, void* _context)
{
	SocAdd serverAdd;
	Serv *server;
	int sock ;
	int optval = 1  ;

	if(_statusRes == NULL)
	{
		return NULL;
	}
	if( _gotMessage == NULL )
	{
		*_statusRes = SERVER_MESSAGE_ERROR;
		return NULL;
	}
	if( _port < MIN_SIZE_PORT || _port > MAX_SIZE_PORT)
	{
		*_statusRes = SERVER_PORT_ERROR;
		return NULL;
	}
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if( sock < 0 ) 
	{
		perror("socket failed");
		*_statusRes = SERVER_SOCK_ERROR;
		return NULL;
	}
	memset(&serverAdd, 0, sizeof(serverAdd));
	serverAdd.sin_family = AF_INET;
	serverAdd.sin_addr.s_addr = INADDR_ANY;
	serverAdd.sin_port = htons(_port); 

	if( setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,&optval, sizeof(optval)) < 0) 
	{
		close(sock);
		perror("reuse failed");
		return NULL;
	}	
	if( bind(sock, (struct sockaddr  *) &serverAdd, sizeof(serverAdd)) < 0 )
	{
		close(sock);
		perror("bind failed");
		*_statusRes = SERVER_BIND_ERROR;
		return NULL;
	}
	if ( listen(sock, _backLog) < 0) 
	{
		close(sock);
		perror("listen failed");
		*_statusRes = SERVER_LISTEN_ERROR;
		return NULL;
	}
	if((server = (Serv*)malloc(sizeof(Serv))) == NULL)
	{
		close(sock);
		*_statusRes = SERVER_ALLOC_ERROR;
		return NULL;
	}
	if((server->m_list = ListCreate()) == NULL)
	{
		close(sock);
		free(server);
		*_statusRes = SERVER_LIST_ALLOC_ERROR;
		return NULL;
	}
	server->m_newClient = _newClient;
	server->m_fail = _fail;
	server->m_gotMessage = _gotMessage;
	server->m_closeClient = _closeClient;
	server->m_contex = _context;
	server->m_flag = 1;
	FD_ZERO(&server->m_master);
	FD_SET(sock, &server->m_master);
	server->m_sock = sock;
	server->m_countClient = 0;
	server->m_magicNumber = MAGIC_NUMBER;
	*_statusRes = SERVER_SUCCESS;
	return server;
}

SE SendToClient(Serv* _server, int _sock, char _data[], size_t _dataLen)
{
	int sentBytes;
	SE res;
	if((res = CheckParameters(_sock, _data, _dataLen)) != SERVER_SUCCESS)
	{
		return res;
	}
	sentBytes = send(_sock, _data, _dataLen , 0);
	if (sentBytes < 0) 
	{
		perror("sendto failed");
		/*RemoveAndClose(_server,_sock);*/
		return SERVER_SEND_ERROR;
	}
	return SERVER_SUCCESS;
}

SE StopRun(Serv* _server)
{
	if(_server == NULL)
	{
		return SERVER_UNITIALIZED_ERROR;
	}
	_server->m_flag = 0;
	return SERVER_SUCCESS;
}

SE StartServer(Serv* _server)
{
	int activity;
	if(_server == NULL)
	{
		return SERVER_UNITIALIZED_ERROR;
	}
	while(_server->m_flag)
	{
		SE res;
		_server->m_temp = _server->m_master;
		activity = select(1024, &_server->m_temp, NULL, NULL, NULL); 
		if(activity < 0)
		{

			return FAIL;
		}
		else if(activity > 0)
		{
			_server->m_activity = activity;
			if(FD_ISSET(_server->m_sock, &_server->m_temp))
			{
				 /* check new client*/
				if( (res = Connect(_server)) != SERVER_SUCCESS && _server->m_fail != NULL )
				{
					(*_server->m_fail)( -1 , res);
				}
				_server->m_activity--;
			}
			if(_server->m_activity > 0)
			{
				ForCheckClients(_server);
			}
		}
	}
	return SERVER_SUCCESS;
}

void ServerDestroy(Serv* _server)
{
	if(_server == NULL || _server->m_magicNumber != MAGIC_NUMBER)
	{
		return;
	}
	_server->m_magicNumber = 0;
	ListDestroy(&_server->m_list, Destroy);
	close(_server->m_sock);
	free(_server);
}


static SE Connect(Serv* _server)
{
	unsigned int clientAdd_len;
	int client_sock, flags;
	int *  client;
	SocAdd clientAdd;
	
	clientAdd_len = sizeof(clientAdd);
	client_sock = accept(_server->m_sock, (struct sockaddr  *) &clientAdd, &clientAdd_len);
	if( client_sock < 0)
	{
		return SERVER_ACCEPT_ERROR;
	}
	if(_server->m_countClient >= 1000)
	{
		close(client_sock);
		return SERVER_FAIL;
	}
	if((client = (int*)malloc(sizeof(int))) == NULL)
	{
		perror("malloc failed");
		close(client_sock);
		return SERVER_ALLOC_ERROR;
	}
	*client = client_sock;
	if(ListPushHead(_server->m_list, client) != LIST_SUCCESS) 
	{
		close(*client);
		free(client);
		return SERVER_LIST_ALLOC_ERROR;
	}
	_server->m_countClient++;
	FD_SET(client_sock, &_server->m_master);
	return SERVER_SUCCESS;
}

static SE CheckParameters(int _sock, void* _data, size_t _dataLen)
{
	if( _data == NULL)
	{
		return SERVER_UNITIALIZED_ERROR;
	}
	if( _sock < MIN_SOCK  || _sock > MAX_SOCK )
	{
		return SERVER_SOCK_ERROR;
	}
	if( _dataLen == 0)
	{
		return SERVER_DATA_LEN_ERROR;
	}
	return SERVER_SUCCESS;
}

static SE CheckCurrClient(int _sock, GotMessage _gotMessage, void* _context)
{
	char buffer[ SIZE_BUFFER];
	ssize_t readBytes; /* int */
	readBytes = recv( _sock, buffer, SIZE_BUFFER, 0 );
	if( readBytes < 0)
	{
		return FAIL;
	}
	else if( readBytes == 0 )
	{
		return FAIL;
	}
	(*_gotMessage)(_sock, buffer, readBytes, _context);
	return SERVER_SUCCESS;
}

static SE ForCheckClients(Serv* _server)
{
	ListItr begin, end, temp;
	int *cliSock;
	
	begin = ListItrBegin(_server->m_list);
	end = ListItrEnd(_server->m_list);
	while(begin != end && _server->m_activity > 0)
	{
		cliSock = (int*)ListItrGet(begin);
		if(FD_ISSET(*cliSock, &_server->m_temp))
		{
			_server->m_activity--;
			if(CheckCurrClient(*cliSock, _server->m_gotMessage ,_server->m_contex) == FAIL )
			{
				close(*cliSock);
				if( _server->m_closeClient != NULL)
				{
					(*_server->m_closeClient)(*cliSock);
				}
				FD_CLR(*cliSock, &_server->m_master);
				free(cliSock);
				temp = begin;
				begin = ListItrNext(begin);
				ListItrRemove(temp);
				_server->m_countClient--;
				continue;
			}
		}
		begin = ListItrNext(begin);
		
	}
	return SERVER_SUCCESS;
}

static void Destroy( void *_sock)
{
	close(*(int*)_sock);
	free(_sock);
}

