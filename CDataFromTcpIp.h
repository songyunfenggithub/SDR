#pragma once

#include <stdio.h>  

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>  

#define FILTED_FORWORD_PAGE_SIZE			0x400	//1k 1024

class CDataFromTcpIp
{
public:
	HANDLE	hThreadGetData;
	HANDLE	hThreadControl;
	HANDLE	hThreadSendFiltedData;
	HANDLE	hThreadSendFFTData;

	bool	GetDataWorking = true;

	SOCKET hControlSocket;
	SOCKET hSendFiltedDataSocket;
	SOCKET hSendFFTDataSocket;

public:
	CDataFromTcpIp();
	~CDataFromTcpIp();

	void TcpIpThreadsStart(void);

	bool TcpIpGetData(void);
	bool TcpIpControl(void);
	bool TcpIpSendFiltedData(void);
	bool TcpIpSendFFTData(void);

	void SendFiltedData(char* buff, int len);
	void SendFFTData(char* buff, int len);
	void SendControlCMD(char* buff, int len);

	static LPTHREAD_START_ROUTINE CDataFromTcpIp::GetDataTcpIpThreadFun(LPVOID lp);
	static LPTHREAD_START_ROUTINE CDataFromTcpIp::ControlTcpIpThreadFun(LPVOID lp);
	static LPTHREAD_START_ROUTINE CDataFromTcpIp::SendFiltedDataTcpIpThreadFun(LPVOID lp);
	static LPTHREAD_START_ROUTINE CDataFromTcpIp::SendFFTDataTcpIpThreadFun(LPVOID lp);
};

extern CDataFromTcpIp clsGetDataTcpIp;

