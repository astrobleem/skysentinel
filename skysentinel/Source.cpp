#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <algorithm>


// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "4661"
using namespace std;

int __cdecl main(int argc, char **argv)
{
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
			
	const char start[] = { '\x06', '\x14', '\x00', '\x04', '\x00', '\x34', '\x11', '\x00', '\x00', '\x5D', '\0' };
	const char stop[] = { '\x06', '\x14', '\x00', '\x04', '\x00', '\x34', '\x11', '\x01', '\x00', '\x5E', '\0' };
	const char messagestatus[] = { '\x07', '\x14', '\x00', '\x05', '\x00', '\x34', '\x00', '\x00', '\x11', '\x27', '\x85' };
	
	char recvbuf[DEFAULT_BUFLEN] = { 0 };
	int iResult;
	int recvbuflen = DEFAULT_BUFLEN;

	// Validate the parameters
	if (argc != 3) {
		printf("usage: %s server-name command\n", argv[0]);
		return 1;
	}

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		// Connect to server.
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}

	// Send our command.
	string mystring = argv[2];
	transform(mystring.begin(), mystring.end(),
		mystring.begin(), ::tolower);

	if (mystring == "messagestatus")
	{
		iResult = send(ConnectSocket, messagestatus, sizeof(messagestatus), 0);
		iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
		iResult = shutdown(ConnectSocket, SD_SEND);
		cout << recvbuf[0] << endl;

		if (recvbuf[0] == 5)
		{
			cout << "ON\n";
			return 1;
		}
		if (recvbuf[0] == 0)
		{
			cout << "OFF\n";
			return 0;
		}

	}
	if (mystring == "start")
		iResult = send(ConnectSocket, start, sizeof(start), 0);
	if (mystring == "stop")
		iResult = send(ConnectSocket, stop, sizeof(stop), 0);

	if (iResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}

	iResult = shutdown(ConnectSocket, SD_SEND);

	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}

	// Receive until the peer closes the connection
	do {

		iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
		
		if (iResult > 0)
		{
		//	printf("Bytes received: %d\n", iResult);
		//	cout << "Data: " << recvbuf << endl;

		}
		else if (iResult == 0)
		{
	//		printf("Connection closed, no data received\n");
			//cout << "Data: " << recvbuf << endl;
		}
		else
			printf("recv failed with error: %d\n", WSAGetLastError());

	} while (iResult > 0);


	// cleanup
	//iResult = shutdown(ConnectSocket, SD_SEND);
	closesocket(ConnectSocket);
	WSACleanup();

	return 0;
}