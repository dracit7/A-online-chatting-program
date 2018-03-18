// 本文件编译时需要手动设置debug multithreaded DLL运行库
#include <iostream>
#include <stdlib.h>
#include <process.h>
#include <winsock2.h>
#include <conio.h>
#include <string.h>
#pragma comment(lib,"ws2_32.lib")
#define PORT_USING 18090
using namespace std;
char user[32]={0};
char ouser[32]={0};
void error(char *message)
{
	cout<<"error:"<<message<<endl;
	exit(1);
}
unsigned WINAPI recvthread(void *param)
{
	char message[1024]={0};
	while(1)
	{
		if(recv(*(SOCKET*)param,message,sizeof(message),0)==SOCKET_ERROR)
		{
			Sleep(500);
			continue;
		}
		if(strlen(message)!=0)
			cout<<message<<endl;
	}
	return 0;
}
unsigned WINAPI sendthread(void *param)
{
	char message[1024]={0};
	while(1)
	{
		getch();
		cout<<user<<":";
		gets(message);
		if(send(*(SOCKET*)param,message,sizeof(message),0)==SOCKET_ERROR)
			return 1;
	}
	return 0;
}
void main()
{
	WSADATA wsadata;
	SOCKET clientsock;
	SOCKADDR_IN servaddr;
	int port=PORT_USING;
	if(WSAStartup(MAKEWORD(2,2),&wsadata))
		error("startup failed!");
	clientsock=socket(PF_INET,SOCK_STREAM,0);
	if(clientsock==INVALID_SOCKET)
		error("socket failed!");
	cout<<"Please input server IP:";
	char IP[32]={0};
	gets(IP);
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.S_un.S_addr=inet_addr(IP);
    servaddr.sin_port=htons(port);
    cout<<"Please input your user name:";
    gets(user);
    cout<<"connecting......"<<endl;
    if(connect(clientsock,(SOCKADDR*)&servaddr,sizeof(servaddr))==SOCKET_ERROR)
    {
    	closesocket(clientsock);
    	WSACleanup();
    	error("connect failed!");
    }
    cout<<"Successfully connected to server!"<<endl;
    cout<<"------------help------------"<<endl;
    cout<<"1.press \"enter\" to say something;"<<endl;
    cout<<"2.close the window to quit chatting;"<<endl;
    send(clientsock,user,sizeof(user),0);
    cout<<endl<<endl;
    HANDLE handles[2];
    handles[0]=(HANDLE)_beginthreadex(NULL,0,recvthread,&clientsock,0,NULL);
    handles[1]=(HANDLE)_beginthreadex(NULL,0,sendthread,&clientsock,0,NULL);
    WaitForMultipleObjects(2,handles,1,INFINITE);
    closesocket(clientsock);
    WSACleanup();
}