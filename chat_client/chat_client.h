//************************************************************************
// Boby Thomas Pazheparampil
// May 2006
// Implementation of CChatClient class and main.
//************************************************************************

#include <Afxwin.h>
#include <stdio.h>
#include <winsock2.h>
#include <conio.h> 

#include <iostream>

using namespace std;


class CChatClient
{
public:
	CChatClient();
	~CChatClient();
	void Init(string sIpAddress, int iPort);
	int SendMessageToServer(string sMessage);
	int ListenMessagesFromServer();
	bool IsConnected(){return m_bIsConnected;}
private:
	bool m_bIsConnected; // true - connected false - not connected
	string m_sServerIPAddress;
	int m_iServerPort;
	SOCKET m_SClient; // socket connected to server
};
