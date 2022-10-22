#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <process.h> 
#include <time.h>
	
#define BUF_SIZE 100
#define NAME_SIZE 20
#define TIME_SIZE 20

char name[NAME_SIZE];
char msg[BUF_SIZE];
char globalTime[TIME_SIZE];

void getTime()
{
    time_t currentTime;
    time(&currentTime);

    struct tm *p;
    p = gmtime(&currentTime);

    snprintf(globalTime, 20, "%d-%d-%d %d:%d:%d", 1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday, 8 + p->tm_hour, p->tm_min, p->tm_sec);
    
}
	
// 将发送与读取分为两个线程
unsigned WINAPI SendMsg(void * arg)   // send 服务器线程
{
	SOCKET hSock=*((SOCKET*)arg);
	char Messages[TIME_SIZE+NAME_SIZE+BUF_SIZE];
	while(1) 
	{
		fgets(msg, BUF_SIZE, stdin);  // 获取消息

        getTime(); //时间

		if(!strcmp(msg,"exit\n")) 
		{
            char *tempMessage = " exit.\n";
            sprintf(Messages,"%s %s %s",globalTime,name, tempMessage);
            send(hSock, Messages, strlen(Messages), 0);
			closesocket(hSock);
			exit(0);
		}
		sprintf(Messages,"%s %s %s",globalTime,name, msg);
       
		send(hSock, Messages, strlen(Messages), 0);
	}
	return 0;
}
	
unsigned WINAPI RecvMsg(void * arg)   // read 服务器线程
{
	int hSock=*((SOCKET*)arg);
	char Messages[NAME_SIZE+BUF_SIZE];
	int strLen;
	while(1)
	{
		strLen=recv(hSock, Messages, TIME_SIZE+NAME_SIZE+BUF_SIZE-1, 0);
		if(strLen==-1) 
			return -1;
		Messages[strLen]=0;
		fputs(Messages, stdout);
	}
	return 0;
}
	
void ErrorHandling(char *msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}
	

	
int main(int argc, char *argv[])
{
	WSADATA wsaData;
	SOCKET hSock;
	SOCKADDR_IN servAdr;
	HANDLE hSndThread, hRcvThread;
	if(argc!=4) {
		printf("Please enter port number.");
		return 0;
	}
	if(WSAStartup(MAKEWORD(2, 2), &wsaData)!=0)
		ErrorHandling("WSAStartup() error."); 

	sprintf(name, "[%s]", argv[3]);
	hSock=socket(PF_INET, SOCK_STREAM, 0);
	
	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family=AF_INET;
	servAdr.sin_addr.s_addr=inet_addr(argv[1]);
	servAdr.sin_port=htons(atoi(argv[2]));
	  
	if(connect(hSock, (SOCKADDR*)&servAdr, sizeof(servAdr))==SOCKET_ERROR)
		ErrorHandling("connect() error.");
	
	hSndThread=(HANDLE)_beginthreadex(NULL, 0, SendMsg, (void*)&hSock, 0, NULL);
	hRcvThread=(HANDLE)_beginthreadex(NULL, 0, RecvMsg, (void*)&hSock, 0, NULL);

	WaitForSingleObject(hSndThread, INFINITE);
	WaitForSingleObject(hRcvThread, INFINITE);
	closesocket(hSock);
	WSACleanup();
	return 0;
}
	

