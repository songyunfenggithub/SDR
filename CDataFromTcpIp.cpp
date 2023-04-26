
#include "stdafx.h"
#include <iostream>
#include "string.h"

#include "public.h"
#include "CDataFromTcpIp.h"
#include "CData.h"
#include "CWaveFilter.h"
#include "CWaveFFT.h"
#include "CWinFFT.h"
#include "CWinSpectrum.h"
#include "CWinMain.h"

#pragma comment(lib,"ws2_32.lib")  

#define BUFFSIZE 8192
//#define SERVER_TCPIP_ADDRESS					"192.168.0.106"
#define SERVER_TCPIP_ADDRESS					"192.168.0.111"
//#define SERVER_TCPIP_ADDRESS					"192.168.137.1"
#define SERVER_TCPIP_CONTROL_PORT				5675
#define SERVER_TCPIP_SEND_FILTERED_DATA_PORT	5676
#define SERVER_TCPIP_SEND_FFT_DATA_PORT			5677
#define SERVER_TCPIP_GET_DATA_PORT				5678

using namespace std;

CDataFromTcpIp clsGetDataTcpIp;

CDataFromTcpIp::CDataFromTcpIp()
{

}

CDataFromTcpIp::~CDataFromTcpIp()
{

}

void CDataFromTcpIp::TcpIpThreadsStart(void)
{
	clsGetDataTcpIp.hThreadGetData			= CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CDataFromTcpIp::GetDataTcpIpThreadFun, NULL, 0, NULL);
	clsGetDataTcpIp.hThreadControl			= CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CDataFromTcpIp::ControlTcpIpThreadFun, NULL, 0, NULL);
	clsGetDataTcpIp.hThreadSendFiltedData	= CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CDataFromTcpIp::SendFiltedDataTcpIpThreadFun, NULL, 0, NULL);
	clsGetDataTcpIp.hThreadSendFFTData		= CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CDataFromTcpIp::SendFFTDataTcpIpThreadFun, NULL, 0, NULL);
}

LPTHREAD_START_ROUTINE CDataFromTcpIp::GetDataTcpIpThreadFun(LPVOID lp)
{
	OPENCONSOLE;
	clsGetDataTcpIp.TcpIpGetData();
	//CLOSECONSOLE;

	return 0;
}

bool CDataFromTcpIp::TcpIpGetData(void)
{
	WSADATA wsadata;
	int err = 0;
	if ((err = WSAStartup(MAKEWORD(2, 2), &wsadata)) != 0) {
		printf("startup error : %d\n", err);
		return 0;
	}
	SOCKADDR_IN serv_addr, client_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	memset(&client_addr, 0, sizeof(client_addr));
	serv_addr.sin_addr.S_un.S_addr = inet_addr(SERVER_TCPIP_ADDRESS); //INADDR_ANY;
	serv_addr.sin_port = htons(SERVER_TCPIP_GET_DATA_PORT);
	serv_addr.sin_family = AF_INET;
	SOCKET hListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (bind(hListenSocket, (SOCKADDR*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
	{
		LPVOID lpMsgBuf;
		DWORD dw = WSAGetLastError();
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0, NULL);
		printf(TEXT("bind SERVER_TCPIP_GET_DATA_PORT %d error :%d, %s\n"),
			SERVER_TCPIP_GET_DATA_PORT, dw, lpMsgBuf);
		LocalFree(lpMsgBuf);
		closesocket(hListenSocket);
		WSACleanup();
		return 0;
	}
	if (SOCKET_ERROR == listen(hListenSocket, 10)) {
		printf(TEXT("listen error :%d\n"), WSAGetLastError());
		closesocket(hListenSocket);
		WSACleanup();
	}
	printf(TEXT("TcpIpGetData\t\t server start at %d port.\r\n"), SERVER_TCPIP_GET_DATA_PORT);
	int client_len = sizeof(client_addr);
	while (GetDataWorking)
	{
		SOCKET hRecvSocket = accept(hListenSocket, (SOCKADDR*)&client_addr, &client_len);
		printf("TcpIpGetData\t\t%s:%d connected.\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
		fd_set read_set, except_set, rset, eset;
		FD_ZERO(&read_set);
		FD_ZERO(&except_set);
		FD_SET(hRecvSocket, &read_set);
		FD_SET(hRecvSocket, &except_set);
		struct timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		int n = 0, len = 0;
		char buf[BUFFSIZE];
		while (GetDataWorking)
		{
			rset = read_set;
			eset = except_set;
			n = select(0, &rset, NULL, &eset, &timeout);
			if (0 == n) {
				puts("TcpIpGetData timeout~");
				closesocket(hRecvSocket);
				break;
			}
			else if (SOCKET_ERROR == n) {
				printf("TcpIpGetData error : %d\n", WSAGetLastError());
				closesocket(hRecvSocket);
				break;
			}
			else {
				//OOB 数据
				if (FD_ISSET(hRecvSocket, &eset)) {
					len = recv(hRecvSocket, buf, BUFFSIZE - 1, MSG_OOB);
					if (0 == len) {
						puts("on recv oob data , peer closed");
						closesocket(hRecvSocket);
						break;
					}
					printf("oob data len :%d\n", len);
					buf[len] = 0;
					printf("oob data buf :%s\n", buf);
				}
				//普通数据
				if (FD_ISSET(hRecvSocket, &rset)) {
					if((DATA_BUFFER_LENGTH << DATA_BYTE_TO_POSITION_MOVEBIT) == clsData.AdcGetCharLength) clsData.AdcGetCharLength = 0;
					int m = (DATA_BUFFER_LENGTH << DATA_BYTE_TO_POSITION_MOVEBIT) - clsData.AdcGetCharLength;
					if (m > 1024) m = 1024;
					n = recv(hRecvSocket, (char*)clsData.AdcBuff + clsData.AdcGetCharLength, m, MSG_WAITALL);
					if (0 == n) {
						printf("on recv nornal data , peer closed\n");
						printf("Rc:0\n");
						closesocket(hRecvSocket);
						break;
					}
					//StringToHex((char*)(clsData.AdcBuff) + clsData.AdcGetCharLength, 20);
					//printf("%d\n", len);
					if (n > 0) {
						if ((n % 4) != 0) {
							printf("Rc:%d\n", n);
							//closesocket(hRecvSocket);
							//break;
						}
						clsData.AdcGetCharLength += n;
						if (clsData.AdcGetCharLength >= (DATA_BUFFER_LENGTH << DATA_BYTE_TO_POSITION_MOVEBIT))
							clsData.AdcGetCharLength -= DATA_BUFFER_LENGTH << DATA_BYTE_TO_POSITION_MOVEBIT;
						clsData.AdcPos = clsData.AdcGetCharLength >> DATA_BYTE_TO_POSITION_MOVEBIT;
						//printf("pos:%d\n", clsData.AdcPos);
						clsData.AdcGetNew = true;
					}
					else Sleep(100);
				}
			}
		}
	}
	closesocket(hListenSocket);
	WSACleanup();

	return 0;
}

LPTHREAD_START_ROUTINE CDataFromTcpIp::ControlTcpIpThreadFun(LPVOID lp)
{
	OPENCONSOLE;
	clsGetDataTcpIp.TcpIpControl();
	//CLOSECONSOLE;

	return 0;
}

bool CDataFromTcpIp::TcpIpControl(void)
{
	WSADATA wsadata;
	int err = 0;
	if ((err = WSAStartup(MAKEWORD(2, 2), &wsadata)) != 0) {
		printf("startup error : %d\n", err);
		return 0;
	}
	SOCKADDR_IN serv_addr, client_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	memset(&client_addr, 0, sizeof(client_addr));
	serv_addr.sin_addr.S_un.S_addr = inet_addr(SERVER_TCPIP_ADDRESS); //INADDR_ANY;
	serv_addr.sin_port = htons(SERVER_TCPIP_CONTROL_PORT);
	serv_addr.sin_family = AF_INET;
	SOCKET hListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (bind(hListenSocket, (SOCKADDR*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
	{
		LPVOID lpMsgBuf;
		DWORD dw = WSAGetLastError();
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0, NULL);
		printf(TEXT("bind SERVER_TCPIP_CONTROL_PORT %d error :%d, %s\n"),
			SERVER_TCPIP_CONTROL_PORT, dw, lpMsgBuf);
		LocalFree(lpMsgBuf);		
		closesocket(hListenSocket);
		WSACleanup();
		return 0;
	}
	if (SOCKET_ERROR == listen(hListenSocket, 10)) {
		printf(TEXT("listen error :%d\n"), WSAGetLastError());
		closesocket(hListenSocket);
		WSACleanup();
	}
	printf(TEXT("TcpIpControl\t\t server start at %d port.\r\n"), SERVER_TCPIP_CONTROL_PORT);
	int client_len = sizeof(client_addr);
	while (GetDataWorking)
	{
		SOCKET hControlSocket = accept(hListenSocket, (SOCKADDR*)&client_addr, &client_len);
		printf("TcpIpControl\t\t%s:%d connected.\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
		fd_set read_set, except_set, rset, eset;
		FD_ZERO(&read_set);
		FD_ZERO(&except_set);
		FD_SET(hControlSocket, &read_set);
		FD_SET(hControlSocket, &except_set);
		struct timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		int n = 0, len = 0;
		char buf[BUFFSIZE];
		while (GetDataWorking)
		{
			rset = read_set;
			eset = except_set;
			n = select(0, &rset, NULL, &eset, &timeout);
			if (0 == n) {
				puts("TcpIpControl timeout~");
				closesocket(hControlSocket);
				break;
			}
			else if (SOCKET_ERROR == n) {
				printf("TcpIpControl error : %d\n", WSAGetLastError());
				closesocket(hControlSocket);
				break;
			}
			else {
				//OOB 数据
				if (FD_ISSET(hControlSocket, &eset)) {
					len = recv(hControlSocket, buf, BUFFSIZE - 1, MSG_OOB);
					if (0 == len) {
						puts("on recv oob data , peer closed");
						closesocket(hControlSocket);
						break;
					}
					printf("oob data len :%d\n", len);
					buf[len] = 0;
					printf("oob data buf :%s\n", buf);
				}
				//普通数据
				if (FD_ISSET(hControlSocket, &rset)) {
					len = recv(hControlSocket, buf, FORWARD_CMD_LENGTH, 0);
					if (0 == len) {
						puts("on recv nornal data , peer closed");
						closesocket(hControlSocket);
						break;
					}
					if (len != FORWARD_CMD_LENGTH) {
						printf("recv control cmd data error %d / %d\r\n", len, FORWARD_CMD_LENGTH);
					}
					//switch (buf[0])
					switch (100)
					{
					case FORWARD_CMD_TYPE_FILTER_SETTING:
						printf("FORWARD_CMD_TYPE_FILTER_SETTING\r\n");
						StringToHex(buf, FORWARD_CMD_LENGTH);
						clsWaveFilter.GetCMDFilter(buf);
						break;
					case FORWARD_CMD_TYPE_FILTER_RECORE:
						printf("FORWARD_CMD_TYPE_FILTER_RECORE\r\n");
						StringToHex(buf, FORWARD_CMD_LENGTH);
						//clsWaveFilter.build_core();
						break;
					case FORWARD_CMD_TYPE_FILTER_DESC_SETTING:
						printf("FORWARD_CMD_TYPE_FILTER_DESC_SETTING\r\n");
						StringToHex(buf, FORWARD_CMD_LENGTH);
						clsWaveFilter.GetCMDFilterDesc(buf);
						break;
					case FORWARD_CMD_TYPE_FILTER_DESC_RECORE:
						printf("FORWARD_CMD_TYPE_FILTER_DESC_RECORE\r\n");
						StringToHex(buf, FORWARD_CMD_LENGTH);
						printf("%s\r\n", clsWaveFilter.FilterCoreDesc);
						clsWaveFilter.FilterCoreDesc[strlen(clsWaveFilter.FilterCoreDesc) - 1] = '\0';
						printf("%s\r\n", clsWaveFilter.FilterCoreDesc);
						clsWaveFilter.setFilterCoreDesc(&clsWaveFilter.rootFilterInfo, clsWaveFilter.FilterCoreDesc);
						clsWaveFilter.ParseCoreDesc(&clsWaveFilter.rootFilterInfo);
						clsWaveFilter.FilterCoreAnalyse(clsWinMain.m_FilterWin, &clsWaveFilter.rootFilterInfo);
						break;
					case FORWARD_CMD_TYPE_FFT_SET:
						printf("FORWARD_CMD_TYPE_FFT_SET\r\n");
						StringToHex(buf, FORWARD_CMD_LENGTH);
						/*
						clsWaveFFT.FFTSize = *&buf[4];
						clsWaveFFT.FFTStep = *&buf[8];
						clsWaveFFT.Init();
						clsWinFFT.Init();
						clsWinSpect.Init();
						*/
						break;

					case FORWARD_CMD_TYPE_FFT_GET_SIGNAL:
						printf("FORWARD_CMD_TYPE_FFT_GET_SIGNAL\r\n");
						StringToHex(buf, FORWARD_CMD_LENGTH);
						clsWaveFFT.ForwardSignal = *&buf[4];
						clsWaveFFT.ForwardFFTlog = *&buf[8];
						break;

					case FORWARD_CMD_TYPE_FFT_SIGNAL_SAMPLE_RATE:
						printf("FORWARD_CMD_TYPE_FFT_SIGNAL_SAMPLE_RATE\r\n");
						StringToHex(buf, FORWARD_CMD_LENGTH);
						break;

					default:
						break;
					}
				}
			}
		}
	}
	closesocket(hListenSocket);
	WSACleanup();

	return 0;
}

LPTHREAD_START_ROUTINE CDataFromTcpIp::SendFiltedDataTcpIpThreadFun(LPVOID lp)
{
	OPENCONSOLE;
	clsGetDataTcpIp.TcpIpSendFiltedData();
	//CLOSECONSOLE;

	return 0;
}

void CDataFromTcpIp::SendFiltedData(char *buff, int len) 
{
	try {
		int n;
		if ((n = send(hSendFiltedDataSocket, buff, len, 0)) < 0)
		{
			perror("SendFiltedData error!\r\n");
			//exit(1);
		}
	}
	catch (int& e) {
		printf("Exception SendFiltedData %d", e);
	}
}

void CDataFromTcpIp::SendFFTData(char* buff, int len)
{
	int n;
	if ((n = send(hSendFFTDataSocket, buff, len, 0)) < 0)
	{
		perror("SendFFTData error!\r\n");
		//exit(1);
	}
}

void CDataFromTcpIp::SendControlCMD(char* buff, int len)
{
	int n;
	if ((n = send(hControlSocket, buff, len, 0)) < 0)
	{
		perror("SendControlCMD error!\r\n");
		//exit(1);
	}
}

bool CDataFromTcpIp::TcpIpSendFiltedData(void)
{
	WSADATA wsadata;
	int err = 0;
	if ((err = WSAStartup(MAKEWORD(2, 2), &wsadata)) != 0) {
		printf("startup error : %d\n", err);
		return 0;
	}
	SOCKADDR_IN serv_addr, client_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	memset(&client_addr, 0, sizeof(client_addr));
	serv_addr.sin_addr.S_un.S_addr = inet_addr(SERVER_TCPIP_ADDRESS); //INADDR_ANY;
	serv_addr.sin_port = htons(SERVER_TCPIP_SEND_FILTERED_DATA_PORT);
	serv_addr.sin_family = AF_INET;
	SOCKET hListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (bind(hListenSocket, (SOCKADDR*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
	{
		LPVOID lpMsgBuf;
		DWORD dw = WSAGetLastError();
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0, NULL);
		printf(TEXT("bind SERVER_TCPIP_SEND_FILTERED_DATA_PORT %d error :%d, %s\n"), 
			SERVER_TCPIP_SEND_FILTERED_DATA_PORT, dw, lpMsgBuf);
		LocalFree(lpMsgBuf);
		closesocket(hListenSocket);
		WSACleanup();
		return 0;
	}
	if (SOCKET_ERROR == listen(hListenSocket, 10)) {
		printf(TEXT("listen error :%d\n"), WSAGetLastError());
		closesocket(hListenSocket);
		WSACleanup();
	}
	printf(TEXT("TcpIpSendFiltedData\t server start at %d port.\r\n"), SERVER_TCPIP_SEND_FILTERED_DATA_PORT);
	int client_len = sizeof(client_addr);
	while (GetDataWorking)
	{
		hSendFiltedDataSocket = accept(hListenSocket, (SOCKADDR*)&client_addr, &client_len);
		printf("TcpIpSendFiltedData\t%s:%d connected.\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
		
		/*
		fd_set read_set, except_set, rset, eset;
		FD_ZERO(&read_set);
		FD_ZERO(&except_set);
		FD_SET(hSendFiltedDataSocket, &read_set);
		FD_SET(hSendFiltedDataSocket, &except_set);
		struct timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		int n = 0, len = 0;
		char buf[BUFFSIZE];
		while (GetDataWorking)
		{
			rset = read_set;
			eset = except_set;
			n = select(0, &rset, NULL, &eset, &timeout);
			if (0 == n) {
				puts("timeout~");
				closesocket(hSendFiltedDataSocket);
				break;
			}
			else if (SOCKET_ERROR == n) {
				printf("error : %d\n", WSAGetLastError());
				closesocket(hSendFiltedDataSocket);
				break;
			}
			else {
				//OOB 数据
				if (FD_ISSET(hSendFiltedDataSocket, &eset)) {
					len = recv(hSendFiltedDataSocket, buf, BUFFSIZE - 1, MSG_OOB);
					if (0 == len) {
						puts("on recv oob data , peer closed");
						closesocket(hSendFiltedDataSocket);
						break;
					}
					printf("oob data len :%d\n", len);
					buf[len] = 0;
					printf("oob data buf :%s\n", buf);
				}
				//普通数据
				if (FD_ISSET(hSendFiltedDataSocket, &rset)) {
					len = recv(hSendFiltedDataSocket, (char*)(clsData.AdcBuff) + clsData.AdcGetCharLength, 1024, 0);
					if (0 == len) {
						puts("on recv nornal data , peer closed");
						closesocket(hSendFiltedDataSocket);
						break;
					}
					clsData.AdcGetCharLength += len;
					clsData.AdcPos = clsData.AdcGetCharLength >> 1;
					clsData.AdcGetCharLength = true;
				}
			}
		}
		*/
	}
	closesocket(hListenSocket);
	WSACleanup();

	return 0;
}

LPTHREAD_START_ROUTINE CDataFromTcpIp::SendFFTDataTcpIpThreadFun(LPVOID lp)
{
	OPENCONSOLE;
	clsGetDataTcpIp.TcpIpSendFFTData();
	//CLOSECONSOLE;

	return 0;
}

bool CDataFromTcpIp::TcpIpSendFFTData(void)
{
	WSADATA wsadata;
	int err = 0;
	if ((err = WSAStartup(MAKEWORD(2, 2), &wsadata)) != 0) {
		printf("startup error : %d\n", err);
		return 0;
	}
	SOCKADDR_IN serv_addr, client_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	memset(&client_addr, 0, sizeof(client_addr));
	serv_addr.sin_addr.S_un.S_addr = inet_addr(SERVER_TCPIP_ADDRESS); //INADDR_ANY;
	serv_addr.sin_port = htons(SERVER_TCPIP_SEND_FFT_DATA_PORT);
	serv_addr.sin_family = AF_INET;
	SOCKET hListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (bind(hListenSocket, (SOCKADDR*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
	{
		LPVOID lpMsgBuf;
		DWORD dw = WSAGetLastError();
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0, NULL);
		printf(TEXT("bind SERVER_TCPIP_SEND_FFT_DATA_PORT %d error :%d, %s\n"),
			SERVER_TCPIP_SEND_FFT_DATA_PORT, dw, lpMsgBuf);
		LocalFree(lpMsgBuf);
		closesocket(hListenSocket);
		WSACleanup();
		return 0;
	}
	if (SOCKET_ERROR == listen(hListenSocket, 10)) {
		printf(TEXT("listen error :%d\n"), WSAGetLastError());
		closesocket(hListenSocket);
		WSACleanup();
	}
	printf(TEXT("TcpIpSendFFTData\t server start at %d port.\r\n"), SERVER_TCPIP_SEND_FFT_DATA_PORT);
	int client_len = sizeof(client_addr);
	while (GetDataWorking)
	{
		hSendFFTDataSocket = accept(hListenSocket, (SOCKADDR*)&client_addr, &client_len);
		printf("TcpIpSendFFTData\t%s:%d connected.\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
		/*
		fd_set read_set, except_set, rset, eset;
		FD_ZERO(&read_set);
		FD_ZERO(&except_set);
		FD_SET(hRecvSocket, &read_set);
		FD_SET(hRecvSocket, &except_set);
		struct timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		int n = 0, len = 0;
		char buf[BUFFSIZE];
		while (GetDataWorking)
		{
			rset = read_set;
			eset = except_set;
			n = select(0, &rset, NULL, &eset, &timeout);
			if (0 == n) {
				puts("timeout~");
				closesocket(hRecvSocket);
				break;
			}
			else if (SOCKET_ERROR == n) {
				printf("error : %d\n", WSAGetLastError());
				closesocket(hRecvSocket);
				break;
			}
			else {
				//OOB 数据
				if (FD_ISSET(hRecvSocket, &eset)) {
					len = recv(hRecvSocket, buf, BUFFSIZE - 1, MSG_OOB);
					if (0 == len) {
						puts("on recv oob data , peer closed");
						closesocket(hRecvSocket);
						break;
					}
					printf("oob data len :%d\n", len);
					buf[len] = 0;
					printf("oob data buf :%s\n", buf);
				}
				//普通数据
				if (FD_ISSET(hRecvSocket, &rset)) {
					len = recv(hRecvSocket, (char*)(clsData.AdcBuff) + clsData.AdcGetCharLength, 1024, 0);
					if (0 == len) {
						puts("on recv nornal data , peer closed");
						closesocket(hRecvSocket);
						break;
					}
					clsData.AdcGetCharLength += len;
					clsData.AdcPos = clsData.AdcGetCharLength >> 1;
					clsData.AdcGetCharLength = true;
				}
			}
		}
		*/
	}
	closesocket(hListenSocket);
	WSACleanup();

	return 0;
}
