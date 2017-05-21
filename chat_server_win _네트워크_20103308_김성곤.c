/*
 * chat_server_win.c
 * VidualStudio에서 console프로그램으로 작성시 multithread-DLL로 설정하고
 *  소켓라이브러리(ws2_32.lib)를 추가할 것.
 */

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <process.h>

#define BUFSIZE 100
#define NAMESIZE 20

DWORD WINAPI ClientConn(void *arg);
void SendMSG(char* message, int len);
void ErrorHandling(char *message);

int clntNumber=0;
SOCKET clntSocks[10];
HANDLE hMutex;
char nameList[NAMESIZE][NAMESIZE];
int numOfmem = 0;

int main(int argc, char **argv)
{
	char name[NAMESIZE];
  WSADATA wsaData;
  SOCKET servSock;
  SOCKET clntSock;

  SOCKADDR_IN servAddr;
  SOCKADDR_IN clntAddr;
  int clntAddrSize;

  HANDLE hThread;
  DWORD dwThreadID;

  if(argc!=2){
    printf("Usage : %s <port>\n", argv[0]);
    exit(1);
  }
  if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) /* Load Winsock 2.2 DLL */
	  ErrorHandling("WSAStartup() error!");

  hMutex=CreateMutex(NULL, FALSE, NULL);
  if(hMutex==NULL){
	  ErrorHandling("CreateMutex() error");
  }

  servSock=socket(PF_INET, SOCK_STREAM, 0);   
  if(servSock == INVALID_SOCKET)
    ErrorHandling("socket() error");
 
  memset(&servAddr, 0, sizeof(servAddr));
  servAddr.sin_family=AF_INET;
  servAddr.sin_addr.s_addr=htonl(INADDR_ANY);
  servAddr.sin_port=htons(atoi(argv[1]));

  if(bind(servSock, (SOCKADDR*) &servAddr, sizeof(servAddr))==SOCKET_ERROR)
    ErrorHandling("bind() error");

  if(listen(servSock, 5)==SOCKET_ERROR)
    ErrorHandling("listen() error");

  while(1){
	  clntAddrSize=sizeof(clntAddr);
	  clntSock=accept(servSock, (SOCKADDR*)&clntAddr, &clntAddrSize);
	  if(clntSock==INVALID_SOCKET)
		  ErrorHandling("accept() error");

	  WaitForSingleObject(hMutex, INFINITE);
	  clntSocks[clntNumber++]=clntSock;
	  ReleaseMutex(hMutex);
	  printf("새로운 연결, 클라이언트 IP : %s \n", inet_ntoa(clntAddr.sin_addr));

	  hThread = (HANDLE)_beginthreadex(NULL, 0, ClientConn, (void*)clntSock, 0, (unsigned *)&dwThreadID);
	  if(hThread == 0) {
		  ErrorHandling("쓰레드 생성 오류");
	  }
  }

  WSACleanup();
  return 0;
}

DWORD WINAPI ClientConn(void *arg)
{
	SOCKET clntSock = (SOCKET)arg;
	int strLen = 0;
	char message[BUFSIZE];
	int i;
	char copyMessage[BUFSIZE];
	char copyMessage2[BUFSIZE];

  while ((strLen = recv(clntSock, message, BUFSIZE, 0)) != 0) {
	  strcpy(copyMessage, message);
	  strcpy(copyMessage2, message);
	  
	  if (strstr(copyMessage, "님이 가입하셨습니다") != NULL) {
		  strtok(copyMessage2, "님");
		  strcat(copyMessage2, "\n");
		  strcpy(nameList[numOfmem++], copyMessage2);
		  strtok(copyMessage2, "\n");
		  printf("%s님이 @@join 요청\n", copyMessage2);
	  }
	  if (strstr(copyMessage, "님이 탈퇴하셨습니다") != NULL) {
		  strtok(copyMessage2, "님");
		  strcat(copyMessage2, "\n");
		  for (i = 0; i < numOfmem; i++) {
			  if (strstr(copyMessage2, nameList[i]) != NULL) {
				  for (; i < numOfmem - 1; i++) {
					  strcpy(nameList[i], nameList[i + 1]);
				  }
				  strtok(copyMessage2, "\n");
				  printf("%s님이 @@out 요청\n", copyMessage2);
				  numOfmem--;
				  break;
			  }
		  }
	  }
	  SendMSG(message, strLen);
  }

  WaitForSingleObject(hMutex, INFINITE);
  for(i=0; i<clntNumber; i++){   // 클라이언트 연결 종료시
    if(clntSock == clntSocks[i]){
		for (; i < clntNumber - 1; i++) {
			clntSocks[i] = clntSocks[i + 1];
		}
        break;
    }
  }
  clntNumber--;
  ReleaseMutex(hMutex);

  closesocket(clntSock);
  return 0;
}

void SendMSG(char* message, int len)
{
	int i;
	int j;
	int k;
	char messageTemp[120];
	char messageTemp2[120];
	char messageTemp3[120];
	char receiveName[20] = "[";
	char *find;
	char *findMessage;
	char senderName[125];
	char tempName[20];
	char *findEnd;

	WaitForSingleObject(hMutex, INFINITE);
	strcpy(messageTemp, message);
	strcpy(messageTemp2, message);
	strcpy(messageTemp3, message);
	if (strchr(message, '&')) {
		if(strstr(message, "list") != 0) {
			strtok(messageTemp, "]");
			strcat(messageTemp, "]");
			strcat(messageTemp, "\n");
			for (i = 0; i < numOfmem; i++) {
				if (!strcmp(messageTemp, nameList[i])) {
					j = i;
					break;
				}
			}
			strtok(messageTemp, "\n");
			printf("%s님이 @@member 요청\n", messageTemp);
			send(clntSocks[j], "\nuser list\n", 11, 0);
			for (i = 0; i < numOfmem; i++) {
				send(clntSocks[j], nameList[i], strlen(nameList[i]), 0);
			}
			send(clntSocks[j], "\n", 1, 0);
			
		} else if(strstr(message, "p2p") != 0) {
			find = strtok(messageTemp, "]");
			find = strtok(NULL, "[");
			find = strtok(NULL, "]");
			strcat(find, "]");
			strcat(receiveName, find);
			strcat(receiveName, "\n");
			for (i = 0; i < numOfmem; i++) {
				if (!strcmp(receiveName, nameList[i])) {
					j = i;
					break;
				}
			}
			strtok(messageTemp2, "]");
			strcat(messageTemp2, "]");
			strcat(messageTemp2, "\n");
			for (i = 0; i < numOfmem; i++) {
				if (!strcmp(messageTemp2, nameList[i])) {
					k = i;
					break;
				}
			}
			strcpy(senderName, nameList[k]);
			strtok(senderName, "\n");
			strcpy(tempName, senderName);
			strcat(senderName, "님의 귓속말 ");
			findMessage = strtok(messageTemp3, "]");
			findMessage = strtok(NULL, "]");
			findMessage = strtok(NULL, "]");
			strcat(senderName, findMessage);
			strcat(senderName, "\n");
			printf("%s님이 @@talk 요청\n", tempName);
			send(clntSocks[j], senderName, len, 0);
			}
	}
	else {
		for (i = 0; i < clntNumber; i++)
			send(clntSocks[i], message, len, 0);
	}
	ReleaseMutex(hMutex);	
}

void ErrorHandling(char *message)
{
  fputs(message, stderr);
  fputc('\n', stderr);
  exit(1);
}
