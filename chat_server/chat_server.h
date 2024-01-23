#include <Afxwin.h>
#include <stdio.h>
#include <winsock2.h>
#include <conio.h>
#include<list>
#include <iostream>
#include <deque>

using namespace std;

typedef enum tagEClientType
{
	eNotDefined = 0,
	eRenderer = 1,
	eProxyMaster = 2
} EClientType;

struct Client
{
	SOCKET sSocket;
	EClientType ctClientType;
	CString csClientID;
};

class CAccountServer
{
public:
	CAccountServer();
	~CAccountServer();
	bool IsConnected(){return m_bIsConnected;} // returns connection status
	void WaitForNewClient(); // Listen to client
	int SendMessageToListeningClients(string sMessage, string sGuid); // Send message to all clients.
	int ListenMessagesFromClientSocket(SOCKET sRecSocket); // receive message for a particular socket
private:
	bool m_bIsConnected; // true - connected false - not connected
	int m_iServerPort;
	list<Client> m_SocketClients; // All socket connected to client
	//std::deque<Client>	m_SocketClients;
	SOCKET m_ASSocketForListenClients; // socket listening for client calls
};
