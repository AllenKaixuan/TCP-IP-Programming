#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <process.h> 
#include <time.h>

#define BUF_SIZE 100
#define MAX_CLNT 128
int clientCount=0;
char Message[BUF_SIZE];
SOCKET clntSocks[MAX_CLNT];
HANDLE hMutex; 


void SendMsg(char * msg, int len)   // 广播
{
	int i;
	WaitForSingleObject(hMutex, INFINITE);
	for(i=0; i<clientCount; i++)
		send(clntSocks[i], msg, len, 0);

	ReleaseMutex(hMutex);
    
}

unsigned WINAPI transferStation(void * arg)
{
	SOCKET hClntSock=*((SOCKET*)arg);
	int strLen=0, i;
	char msg[BUF_SIZE];
	
	while((strLen=recv(hClntSock, msg, sizeof(msg), 0))!=0)
    {
        SendMsg(msg, strLen);        //接受消息后转发
        if(strLen==-1) 
			return -1;
		msg[strLen]=0;
		fputs(msg, stdout);
    }
		
	
	WaitForSingleObject(hMutex, INFINITE); // 临界区加锁
	for(i=0; i<clientCount; i++)   
	{
		if(hClntSock==clntSocks[i])
		{
			while(i++<clientCount-1)
				clntSocks[i]=clntSocks[i+1];
			break;
		}
	}
	clientCount--;
	ReleaseMutex(hMutex);
	closesocket(hClntSock); // 关闭socket（线程）
	return 0;
}

void Error(char * message)
{
    printf("\n",message);	
	exit(1);
}



int main(int argc, char *argv[])
{
	WSADATA wsaData;
	SOCKET hServSock, hClntSock;
	SOCKADDR_IN servAddr, clntAddr;
	int clntAddrSize;
	HANDLE  hThread;
	if(argc!=2) {
		printf("Please enter port number.");
		return 0;
	}
	
	if(WSAStartup(MAKEWORD(2, 2), &wsaData)!=0)
		Error("WSAStartup() error."); 
  
	hMutex=CreateMutex(NULL, FALSE, NULL);
	hServSock=socket(PF_INET, SOCK_STREAM, 0);

	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family=AF_INET; 
	servAddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servAddr.sin_port=htons(atoi(argv[1]));
	
	if(bind(hServSock, (SOCKADDR*) &servAddr, sizeof(servAddr))==SOCKET_ERROR)
		Error("bind() error.");
	if(listen(hServSock, 5)==SOCKET_ERROR)
		Error("listen() error.");
	
	while(1)
	{
		clntAddrSize=sizeof(clntAddr);
		hClntSock=accept(hServSock, (SOCKADDR*)&clntAddr,&clntAddrSize);
		
		WaitForSingleObject(hMutex, INFINITE);  // 加锁，改变clientcount和clntSocks
		clntSocks[clientCount++]=hClntSock; // 客户端数组
		ReleaseMutex(hMutex); // 解锁
	
		hThread=(HANDLE)_beginthreadex(NULL, 0, transferStation, (void*)&hClntSock, 0, NULL);
		printf("Connected client IP: %s , %d come\n", inet_ntoa(clntAddr.sin_addr),clientCount);

	}
	closesocket(hServSock);
	WSACleanup();
	return 0;
}
	
