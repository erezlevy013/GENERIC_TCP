#ifndef __SERVER_H__
#define __SERVER_H__

#include <stdlib.h>
#define SIZE_BUFFER 1024


typedef struct Server Serv;

typedef enum Server_Error
{
	SERVER_FAIL = -1,
    SERVER_SUCCESS = 0,
	SERVER_PORT_ERROR,
	SERVER_MESSAGE_ERROR,
	SERVER_SOCK_ERROR,
	SERVER_BIND_ERROR,
	SERVER_LISTEN_ERROR,
	SERVER_SEND_ERROR,
	SERVER_UNITIALIZED_ERROR,
	SERVER_DATA_LEN_ERROR,				
	SERVER_RECV_ERROR,
	SERVER_ACCEPT_ERROR,
	SERVER_ACTIVITY_ERROR,
	SERVER_LIST_ALLOC_ERROR,
	SERVER_ALLOC_ERROR
}SE;

typedef int(*NewClient)(int  _sockClientId);
typedef int(*Fail)(int _sockClientId, SE _res);
typedef int(*GotMessage)(int  _sockClientId, char _message[], int _messageLen, void* context);
typedef void(*CloseClient)(int  _sockClientId);

/** 
 * @brief Create a generic server
 * @param _newClient: Function that get sock of client and create new client. 
 * @param _fail: Function that get sock of client and type of error.
 * @param _gotMessage: Function that get new request from client.
 * @param _closeClient: Function that get a close sock of client. 
 * @param _port: num between 1024 to 65536
 * @param _statusRes: save the error/success of the function.
 * @param _backLog: num between 1 to 1000 ,  queue size of OS.
 * @param _context: optional variable
 * @retval  NULL if errors.
 * @retval Pointer to Server if success
 */
Serv* CreateServer( NewClient _newClient, Fail _fail, GotMessage _gotMessage, CloseClient _closeClient, int _port, SE* _statusRes, int _backLog, void* _context);

/** 
 * @brief Function sending message to client
 * @param _server: Pointer to Server . 
 * @param _sock: sock of client.
 * @param _data: message.
 * @param _dataLen: message size. 
 * @retval SERVER_SEND_ERROR if the sending failed.
 * @retval SERVER_UNITIALIZED_ERROR if _server = NULL.
 * @retval SERVER_SOCK_ERROR if num is not between 3 to 1024.
 * @retval ERVER_DATA_LEN_ERROR if _dataLen = 0.
 * @retval SERVER_SUCCESS if success.
 */
SE SendToClient(Serv* _server, int _sock, char _data[], size_t _dataLen);

/** 
 * @brief Stop the running server
 * @param _server: Pointer to Server . 
 * @retval SERVER_UNITIALIZED_ERROR if _server = NULL.
 * @retval SERVER_SUCCESS if success.
 */
SE StopRun(Serv* _server);

/** 
 * @brief start the server.
 * @param _server: Pointer to Server . 
 * @retval SERVER_UNITIALIZED_ERROR if _server = NULL.
 * @retval SERVER_SUCCESS if success.
 */
SE StartServer(Serv* _server);

/** 
 * @brief close and free all the socks.
 * @param _server: Pointer to Server . 
 */
void ServerDestroy(Serv* _server);

#endif /* __SERVER_H__ */