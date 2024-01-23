#include "chat_server.h"

CAccountServer m_clsAccountServer;

UINT ThreadListenMessagesFromClient(LPVOID pParam)
{	
	SOCKET sRecSocket = (SOCKET)pParam;
	while(true)
	{
		//false == 0 - everything is ok - we continue listening current socket
		if(m_clsAccountServer.ListenMessagesFromClientSocket(sRecSocket) == true)
		{
			// interrupt this thread, because socket stop sending messages to AS
			break;
		}
	}
	return 0;
}

UINT ThreadGeneral_ServerListeningAllTheClients(LPVOID pParam)
{	
	while(true)
	{
		m_clsAccountServer.WaitForNewClient();
	}
	return 0;
}

CAccountServer::CAccountServer()
{
	cout << "Proxy Networks Back Channel Server.\n";
	cout << "The Server receives messages from Stream Player and send it to \nProxy Network Renderer.\n";
	//constructor. init sockets.
	cout << "\nInit Server";
	m_bIsConnected = false;

	//default port
	int nPort = 8090;

	//get Port from ini file
	char buf[4096];
	FILE *fPort = fopen("port.ini","r");
	if(fPort == NULL)
	{
		cout<<"\nUnable to open port.ini file. Default Port 8090 will be used.";
	}
	else
	{
		while((fgets(buf,4096,fPort)) != NULL)
		{
			nPort = atoi(buf);
		}
		cout<<"\nSocket Port: "<<nPort;
		fclose(fPort);
	}

	WSADATA wsaData;

	sockaddr_in local;

	int wsaret=WSAStartup(0x101,&wsaData);

	if(wsaret!=0)
	{
		return;
	}

	local.sin_family=AF_INET; 
	local.sin_addr.s_addr=INADDR_ANY; 
	local.sin_port=htons((u_short)nPort); 

	//show server IP to console
	char* buffer="";
	gethostname (buffer,(int)strlen(buffer));
	HOSTENT* lpHostEnt = gethostbyname(buffer);
	const char* cc = inet_ntoa(*(LPIN_ADDR)*(lpHostEnt->h_addr_list));
	cout<<"\nServer IP:   "<<cc;

	m_ASSocketForListenClients=socket(AF_INET,SOCK_STREAM, IPPROTO_IP);

	if(m_ASSocketForListenClients==INVALID_SOCKET)
	{
		return;
	}

	if(bind(m_ASSocketForListenClients,(sockaddr*)&local,sizeof(local))!=0)
	{
		return;
	}

	if(listen(m_ASSocketForListenClients,10)!=0)
	{
		return;
	}

	m_bIsConnected = true;
	return;
}

CAccountServer::~CAccountServer()
{
	closesocket(m_ASSocketForListenClients);

	WSACleanup();
}

void CAccountServer::WaitForNewClient()
{
	sockaddr_in from;
	int fromlen=sizeof(from);

	Client NewClient;
	//here we wait for a new connection to Account Server socket
	NewClient.sSocket=accept(m_ASSocketForListenClients, (struct sockaddr*)&from, &fromlen);

	if(NewClient.sSocket != INVALID_SOCKET)
	{
		//add new client to member deque with all clients
		m_SocketClients.push_back(NewClient);
	}

	// and create one more thread for listening messages from this NewClient
	AfxBeginThread(ThreadListenMessagesFromClient,(void *)NewClient.sSocket);
}

int CAccountServer::SendMessageToListeningClients(string sMessage, string sGuid)
{
	int iStat = 0;
	list<Client>::iterator itl;

	if(m_SocketClients.size() == 0)
		return 0;

	for(itl = m_SocketClients.begin();itl != m_SocketClients.end();)
	{
		//let's check GUID of current listener
		//size_t compResult = itl->sGuid.compare(sGuid);
		//if (compResult == 0 && itl->bIsRenderer == true)
		{
			//yes, GUID is valid AND this is listener
			iStat = send(itl->sSocket,sMessage.c_str(),sMessage.size()+1,0);
			if(iStat == -1)
			{
				//m_SocketClients.remove(*itl);
				m_SocketClients.erase(itl);
			}
			else
			{
				itl++;
			}
		}
		//else
		{
			//increment iterator. this is not needed GUID
			itl++;
		}
	}

	if(iStat == -1)
	{
		return 1;
	}

	return 0;

}

int CAccountServer::ListenMessagesFromClientSocket(SOCKET sRecSocket)
{
	//cout <<inet_ntoa(from.sin_addr) <<":"<<cMessageFromClient<<"\r\n";
	char cMessageFromClient[4096] = {0};
	int iStat;
	list<Client>::iterator itl;

	//here we wait messages from client
	iStat = recv(sRecSocket,cMessageFromClient,4096,0);

	if(iStat == SOCKET_ERROR || iStat == 0)
	{
		//if we have message "socket closed" - remove it from members list.
		if (m_SocketClients.size())
		{
			list<Client>::iterator itl;
			for(itl = m_SocketClients.begin();itl != m_SocketClients.end();)
			{
				if (itl->sSocket == sRecSocket)
				{
					m_SocketClients.erase(itl);
					return 1;
				}
				else
				{
					itl++;
				}
			}
		}
		return 1;
	}
	else
	{
		//we receive correct message from client
		//cout <<cMessageFromClient<<"\n";
		string sType;
		string sVersion;
		string sGuid;
		string sUsefulMessage;
		string sMessage(cMessageFromClient);
		int nMesLenght = sMessage.length();
		CString csMessage(cMessageFromClient);

		//parse message
		sType	= sMessage.substr(0, 3); 
		sVersion	= sMessage.substr(3, 15); 
		sGuid	= sMessage.substr(18, 36); 
		sUsefulMessage = sMessage.substr(54, nMesLenght);
		cout <<sType.c_str()<<sVersion.c_str()<<sGuid.c_str()<<":"<<sUsefulMessage.c_str()<<"\n";

		//let's change current socket attributes, like GUID and type
		for(itl = m_SocketClients.begin();itl != m_SocketClients.end();)
		{
			if (itl->sSocket == sRecSocket)
			{
				//client ask server about SIP Account
				if (csMessage.Find("Give me SIP-account") != -1)
				{
					if (csMessage.Find("UDID") != -1)
					{
						//Hello|I am iPad2|My UDID is 61ecac880ae4dad801f54b97d799ca3b4e52df27|Give me SIP-account
						itl->csClientID = csMessage.Mid(csMessage.Find("My UDID is ") + 11, 40);
						itl->ctClientType = eProxyMaster;
					} else if (csMessage.Find("HOST ID") != -1)
					{
						//Hello|I am Renderer|My HOST ID is 3f36a64f-719c-4758-b885-13225ec0c6fe|Give me SIP-account
						itl->csClientID = csMessage.Mid(csMessage.Find("My HOST ID is ") + 14, 36);
						itl->ctClientType = eRenderer;
					} else
					{
						ATLASSERT(itl->ctClientType);
					}
				}
				break;
			}
			else
			{
				itl++;
			}
		}

		//if (sUsefulMessage.compare("") != 0)
		//{
		//	//do not resend an empty messages then Renderer send init message
		//	SendMessageToListeningClients(sUsefulMessage, sGuid);
		//}
		return 0;
	}
	return 0;
}

int main(int argc, char* argv[])
{
	char buf[4096];

	//cout << "Connects to the server PC port 8090.\n";
	cout << "\n=================================================\n";

	if(!m_clsAccountServer.IsConnected())
	{
		cout<<"\nFailed to initialize server socket";
		cout<<"\nThis is Body signing off : Bye";
		getch();
		return 1;
	}
	AfxBeginThread(ThreadGeneral_ServerListeningAllTheClients, 0);

	// this logic gets message from console and send it to all listeners
	// not needed in future
	//while(gets(buf))
	//{
	//	if(strlen(buf) == 0)
	//		break;
	//	if(m_clsAccountServer.SendMessageToListeningClients(buf))
	//	{
	//		cout<<"Problem in connecting to server. Check whether server is running\n";
	//		break;
	//	}
	//}

	getch();

	return 0;
}

