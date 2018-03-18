// 本文件编译时需要手动设置debug multithreaded DLL运行库
// 发送线程为发送信息到对应客户端的线程，接收线程为对应客户端发送信息
#include <iostream>
#include <stdlib.h>
#include <process.h>//支持多线程的头文件
#include <winsock2.h>
#include <string.h>
#pragma comment(lib,"ws2_32.lib")//预编译自动添加库文件
#define clientnum 2//客户端的数量
#define PORT_USING 18090//使用18090端口进行通信
using namespace std;
SOCKET serversock;//服务器套接字
SOCKADDR_IN clientaddr={0};//客户端的地址信息
int clientaddrlen=sizeof(clientaddr);
int status=0;//信息转发状态：0表示未转发，1表示已转发
int relink[clientnum]={0};//表示是否重连
HANDLE recvhandle[clientnum]={0},sendhandle[clientnum]={0};//线程句柄
struct client
{
	SOCKET clientsock;
	char message[1024];//用于储存将要发送给该用户的消息
	char user[32];//用户名
	char IP[32];//客户端IP地址
	int serial;//客户端的序号
};
client clients[clientnum]={0};
void error(char *message)
{
	cout<<"error:"<<message<<endl;
	exit(1);
}
unsigned WINAPI sendthread(void* param)
{
	int serial=*(int*)param;//传参
	char message[1024]={0};
	memcpy(message,clients[serial].message,sizeof(message));
// 将将要发送给对应客户端的消息复制到临时信息数组中
	sprintf(clients[serial].message,"\n%s:%s",clients[!serial].user,message);
// 用sprintf函数将消息格式化为用户名：消息内容的形式
	if(strlen(message)!=0&&status==0)//如果消息存在且尚未转发
		if(send(clients[serial].clientsock,clients[serial].message,sizeof(clients[serial].message),0)==SOCKET_ERROR)
		{
			cout<<"SEND failed!"<<endl;
			return 1;
		}//发送该条消息
	status=1;//设置发送状态为“已转发”
	return 0;
}//发送线程
unsigned WINAPI recvthread(void *param)
{
	int serial=*(int*)param;//传参
	char message[1024]={0};
	while(1)
	{
		memset(message,0,sizeof(message));//初始化临时信息数组
		if(recv(clients[serial].clientsock,message,sizeof(message),0)==SOCKET_ERROR)
			continue;//如果接受信息不成功则重复，接受的消息储存在临时信息数组中
		status=0;//设置发送状态为“未转发”
		memcpy(clients[!serial].message,message,sizeof(clients[!serial].message));
	// 将临时信息数组中的消息复制到发送对象的信息数组中
		sendhandle[serial]=(HANDLE)_beginthreadex(NULL,0,sendthread,&clients[!serial].serial,0,NULL);
	// 开启接收方的发送线程并将其句柄保存在发送方的sendhandle中
		WaitForSingleObject(sendhandle[serial],INFINITE);//等待发送结束
		CloseHandle(sendhandle[serial]);//回收发送线程句柄
	}
	return 0;
}//接收线程
unsigned WINAPI checkthread(void *param)
{
	while(1)
	{
		for(int j=0;j<clientnum;j++)
			if(clients[j].clientsock!=0&&send(clients[j].clientsock,"",sizeof(""),0)==SOCKET_ERROR)
			{//如果某一客户端的套接字未关闭且消息不通
				TerminateThread(recvhandle[j],0);//关闭该客户端的接收线程
				CloseHandle(recvhandle[j]);//回收句柄
				cout<<"disconnected from "<<clients[j].user<<endl;
				closesocket(clients[j].clientsock);//断开该客户端与服务器的连接
				memset(&clients[j],0,sizeof(clients[j]));//清理客户端信息
				clients[j].serial=j;//对客户端重新标号
			}
		Sleep(2000);//每隔两秒检查一次
	}
}//检查是否有人下线的线程
unsigned WINAPI acceptthread(void *param)
{
	int i=0;
	_beginthreadex(NULL,0,checkthread,NULL,0,NULL);//开启检查线程
	while(1)
	{
		while(i<2)//serversock依次向两个客户端发出accept请求
		{
			if(clients[i].clientsock!=0)//如果已经连接则跳过
			{
				++i;
				continue;
			}
			if((clients[i].clientsock=accept(serversock,(SOCKADDR*)&clientaddr,&clientaddrlen))==INVALID_SOCKET)
			{
				cout<<"accept failed!"<<endl;
				closesocket(serversock);
				WSACleanup();
				return -1;
			}
			relink[i]=1;//将该客户端状态设为重连
			recv(clients[i].clientsock,clients[i].user,sizeof(clients[i].user),0);
		// 接收该客户端传来的用户名
			cout<<clients[i].user<<" connected successfully!"<<endl;
			memcpy(clients[i].IP,inet_ntoa(clientaddr.sin_addr),sizeof(clients[i].IP));
		// 保存客户端的IP地址
			cout<<"IP:"<<clients[i].IP<<endl;
			i++;
		}
		i=0;
		if(clients[0].clientsock!=0&&clients[1].clientsock!=0)
	// 如果两个客户端都连接到了服务器
			for(int k=0;k<clientnum;k++)
			{
				if(relink[k])//如果k的状态是重连，则为其新开一个接收线程
					recvhandle[k]=(HANDLE)_beginthreadex(NULL,0,recvthread,&clients[k].serial,0,NULL);
				relink[k]=0;//将k的状态设为非重连
			}
		Sleep(3000);
	}
	return 0;
}
void main()
{
	for(int a=0;a<clientnum;a++)
		clients[a].serial=a;//对客户端进行标号
	WSADATA wsadata;
	SOCKADDR_IN servaddr;
	HANDLE accepthandle;//保存accept线程句柄值
	int port=PORT_USING;
	if(WSAStartup(MAKEWORD(2,2),&wsadata))
		error("startup failed!");
	serversock=socket(PF_INET,SOCK_STREAM,0);
	if(serversock==INVALID_SOCKET)
		error("socket failed!");
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.S_un.S_addr=htonl(INADDR_ANY);
    servaddr.sin_port=htons(port);
    if(bind(serversock,(SOCKADDR*) &servaddr,sizeof(servaddr))==SOCKET_ERROR)
    	error("bind failed!");
    if(listen(serversock,clientnum+1)==SOCKET_ERROR)
    	error("listen failed!");
    cout<<"waiting for connect......"<<endl;
    accepthandle=(HANDLE)_beginthreadex(NULL,0,acceptthread,NULL,0,0);
    WaitForSingleObject(accepthandle,INFINITE);//永久等待accept线程结束
    for(int j=0;j<clientnum;j++)
    	if(clients[j].clientsock!=INVALID_SOCKET)
    		closesocket(clients[j].clientsock);
    closesocket(serversock);
    WSACleanup();
}