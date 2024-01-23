//************************************************************************
// Boby Thomas Pazheparampil
// May 2006
// Implementation of CChatClient class and main.
//************************************************************************
#include "chat_client.h"

#define LISTENING_PORT 8100
//Global Message object
CChatClient CClientObj;


CChatClient::CChatClient()
{
	m_bIsConnected = false;
}

void CChatClient::Init(string sIpAddress, int iPort)
{

	m_sServerIPAddress = sIpAddress;
	m_iServerPort = iPort;
	struct hostent *hp;
	unsigned int addr;
	struct sockaddr_in server;
	

	WSADATA wsaData;
	int wsaret=WSAStartup(0x101,&wsaData);
	if(wsaret!=0)
	{
		return;
	}

	m_SClient=socket(AF_INET,SOCK_STREAM,IPPROTO_IP);
	if(m_SClient==INVALID_SOCKET)
	{
		return;
	}

	addr=inet_addr(m_sServerIPAddress.c_str());
	hp=gethostbyaddr((char*)&addr,sizeof(addr),AF_INET);
	
	if(hp==NULL)
	{
		closesocket(m_SClient);
		return;
	}

	server.sin_addr.s_addr=*((unsigned long*)hp->h_addr);
	server.sin_family=AF_INET;
	server.sin_port=htons(m_iServerPort);
	if(connect(m_SClient,(struct sockaddr*)&server,sizeof(server)))
	{
		closesocket(m_SClient);
		return;	
	}
	m_bIsConnected = true;
	return;
}

CChatClient::~CChatClient()
{
	if(m_bIsConnected)
		closesocket(m_SClient);
}

int CChatClient::SendMessageToServer(string sMessage)
{
	int iStat = 0;

	iStat = send(m_SClient,sMessage.c_str(),sMessage.size()+1,0);
	if(iStat == -1)
		return 1;

	return 0;

}

int CChatClient::ListenMessagesFromServer()
{
	char acRetData[4096];
	int iStat = 0;

	//we wait here for new messages from server
	iStat = recv(m_SClient,acRetData,4096,0);
	if(iStat == SOCKET_ERROR)
	{
		return 1;
	}
	cout<<"-From-Server->"<<acRetData<<"\n";
	CString csMessage(acRetData);
	if (csMessage.Find("Contacts List Changed") != -1)
	{
		string sMessageToServer = "Hello|I am iPad2|My UDID is 61ecac880ae4dad801f54b97d799ca3b4e52df27|Load Contacts List";
		SendMessageToServer(sMessageToServer);
	}

	return 0;
}

UINT ThreadReceiveMessageFromServer(LPVOID pParam)
{	
	while(1)
	{
		if(CClientObj.ListenMessagesFromServer())
			break;
	}
	return 0;
}

int main(int argc, char* argv[])
{
	char buf[4096];
	cout<<"This is a client TCP/IP application\nConnecting to port 8100\n";
	cout<<"\nCLIENT\n";
	cout<<"\n===============================================\n";

	//get server address
	FILE *fp = fopen("server.ini","r");
	if(fp == NULL)
	{
		cout<<"\nUnable to open server.ini. Please specify server IPsddress in server.ini";
		return 1; // main failed
	}
	string sServerAddress;
	while((fgets(buf,4096,fp)) != NULL)
	{
		if(buf[0] == '#')
			continue;
		sServerAddress = buf;
	}
	fclose(fp);

	if(sServerAddress.size() == 0)
	{
		cout<<"\nUnable to find server IPaddress in server.ini";
		cout<<"\nPlease set server IPaddress";
		cout<<"\nThis is Boby Signing off. BYE:";
		getch();
		return 0;
	}

	//get guid from ini file
	FILE *fg = fopen("guid.ini","r");
	if(fg == NULL)
	{
		cout<<"\nUnable to open guid.ini file";
		return 1; // main failed
	}
	string sGuid;
	while((fgets(buf,4096,fg)) != NULL)
	{
		sGuid = buf;
	}
	fclose(fg);

	CClientObj.Init(sServerAddress.c_str(),LISTENING_PORT);
	if(!CClientObj.IsConnected())
	{
		cout<<"\nUnable to connect to the IPaddress specified in server.ini";
		cout<<"\nPlease check server IPaddress";
		getch();
		return 0;	
	}
	AfxBeginThread(ThreadReceiveMessageFromServer,0);

	//get client type from ini file
	FILE *ft = fopen("type.ini","r");
	if(ft == NULL)
	{
		cout<<"\nUnable to open type.ini file";
		return 1; // main failed
	}
	string sType;
	while((fgets(buf,4096,ft)) != NULL)
	{
		sType = buf;
	}
	fclose(ft);

	cout<<"\nServer Address:"<<sServerAddress.c_str();
	cout<<"\nGUID:"<<sGuid.c_str()<<"\n";
	// send first init command for listener
	size_t compResult = sType.compare("PNR");
	if (compResult == 0)
	{
		// if PNR = Proxy Network Renderer
		//it should specify itself for the first time
		string sInitMessage;
		sInitMessage.append("PNR");
		sInitMessage.append(sGuid);
		CClientObj.SendMessageToServer(sInitMessage);
		cout<<"\nThis is PN Renderer. It listens server.\n";
		getch();
	}
	else
	{
		cout<<"\nThis is [Web] Client. It sends commands.\n";
		cout<<"\n===============================================\n";
		// if PNC = Proxy Network Client
		//it might send info wherever it want to
		while(gets(buf))
		{
			string sMessage;
			//sMessage.append("PNC");
			//sMessage.append("000.000.000.000"); //version
			//sMessage.append(sGuid);
			sMessage.append(buf);

			if(strlen(buf) == 0)
				break;
			if(CClientObj.SendMessageToServer(sMessage))
			{
				cout<<"Problem in connecting to server. Check whether server is running\n";
				break;
			}
		}
	}

	//getch();
	return 0;
}

