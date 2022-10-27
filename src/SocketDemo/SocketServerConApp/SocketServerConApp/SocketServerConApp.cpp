#include <iostream>
#include <winsock2.h>
#include <stdlib.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

//1k
const int DEFAULT_BUFLEN = 1024;
#define DEFAULT_PORT "60000"

int main()
{
	//WSAClose();
	//std::cout << "Hello World!\n";

	WSADATA wsaData;
	SOCKET listenSocket = INVALID_SOCKET;

	struct addrinfo* pAddr = nullptr;
	struct addrinfo hints;

	char pSendBuffer[DEFAULT_BUFLEN] = "Send : Hello , I am Jim.";
	char pRecvBuffer[DEFAULT_BUFLEN] = { 0 };

	int iResult = 0;
	int iSendResult = 0;

	int iRecvBufferLen = DEFAULT_BUFLEN;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (0 == iResult)
	{
		cout << "WSAStartup success" << endl;
	}
	else
	{
		WSACleanup();
		cout << "WSAStartup failed,press enter to exit..." << endl;
		system("pause");
		return 0;
	}


	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	iResult = getaddrinfo(nullptr, DEFAULT_PORT, &hints, &pAddr);
	if (0 == iResult)
	{
		cout << "getaddrinfo success" << endl;
	}
	else
	{
		WSACleanup();
		cout << "getaddrinfo failed , press enter to exit..." << endl;
		system("pause");
		return 0;
	}

	listenSocket = socket(pAddr->ai_family, pAddr->ai_socktype, pAddr->ai_protocol);
	if (INVALID_SOCKET == listenSocket)
	{

		freeaddrinfo(pAddr);
		WSACleanup();
		cout << "create listenSocket error :" << WSAGetLastError() << " , press enter to exit..." << endl;
		system("pause");
		return 0;
	}
	else
	{
		cout << "create listenSocket success" << endl;
	}

	iResult = bind(listenSocket, pAddr->ai_addr, (int)pAddr->ai_addrlen);
	if (INVALID_SOCKET == iResult)
	{

		freeaddrinfo(pAddr);
		closesocket(listenSocket);
		WSACleanup();
		cout << "bind error :" << WSAGetLastError() << " , press enter to exit..." << endl;
		system("pause");
		return 0;
	}
	else
	{
		cout << "bind success :" << endl;
	}

	freeaddrinfo(pAddr);

	iResult = listen(listenSocket, SOMAXCONN);
	if (INVALID_SOCKET == iResult)
	{
		closesocket(listenSocket);
		WSACleanup();
		cout << "listen error :" << WSAGetLastError() << " , press enter to exit..." << endl;
		system("pause");
		return 0;
	}
	else
	{
		cout << "listen success :" << endl;
	}



	while (true)
	{
		SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
		if (INVALID_SOCKET == clientSocket)
		{
			cout << "accept error " << WSAGetLastError() << endl;
			continue;
		}

		iResult = recv(clientSocket, pRecvBuffer, DEFAULT_BUFLEN, 0);
		if (0 < iResult)
		{
			if (0 == strcmp(pRecvBuffer, "exit"))
			{
				closesocket(clientSocket);
				WSACleanup();
				break;
			}

			cout << "received msg :" << pRecvBuffer << endl;
			iSendResult = send(clientSocket, pSendBuffer, DEFAULT_BUFLEN, 0);
			if (SOCKET_ERROR == iSendResult)
			{
				cout << "re send msg error :" << WSAGetLastError() << endl;
				continue;
			}
		}
		else if (0 == iResult)
		{
			cout << "Connection closing..." << endl;
			continue;
		}
		else
		{
			cout << "recv error :" << WSAGetLastError() << endl;
			continue;
		}
	}

	cout << "press enter to exit..." << endl;
	system("pause");
	return 0;
}
