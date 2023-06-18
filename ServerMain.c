#include <stdio.h>
#include <string.h>
#include "Server.h"

static int Print(int _sockClientId, char _message[], int _messageLen, void* context)
{
	_message[_messageLen] = '\0';
	printf("Client: %s \n", _message);
	SendToClient(NULL, _sockClientId, "Wellcom from server", 19);
	return 0;
}

int main()
{
	int sock, i, con;
	Serv* server;
	SE* res;
	server = CreateServer(NULL, NULL, Print, NULL, 1234, res, 300, NULL);
	StartServer( server );
	ServerDestroy(server);
	return SERVER_SUCCESS;
}
