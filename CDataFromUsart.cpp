
#include "stdafx.h"

#include <iostream>
#include <process.h>

#include "public.h"
#include "CData.h"

#include "CDataFromUsart.h"

using namespace std;
using namespace DEVICES;

CDataFromUsart clsGetDataUsart;

void CDataFromUsart::UsartGetData(void)
{
	if (!clsGetDataUsart.InitPort(6))
	{
		cout << "initPort fail !" << endl;
	}
	else
	{
		cout << "initPort success !" << endl;
	}

	ListenThread(this);
}

/** 线程退出标志 */
bool CDataFromUsart::s_bExit = false;
/** 当串口无数据时,sleep至下次查询间隔的时间,单位:秒 */
const UINT SLEEP_TIME_INTERVAL = 5;

CDataFromUsart::CDataFromUsart(void)
	: m_hListenThread(INVALID_HANDLE_VALUE)
{
	m_hComm = INVALID_HANDLE_VALUE;
	m_hListenThread = INVALID_HANDLE_VALUE;

	InitializeCriticalSection(&m_csCommunicationSync);

}

CDataFromUsart::~CDataFromUsart(void)
{
	CloseListenTread();
	ClosePort();
	DeleteCriticalSection(&m_csCommunicationSync);
}

bool CDataFromUsart::InitPort(UINT portNo /*= 1*/, UINT baud /*= CBR_9600*/, char parity /*= 'N'*/,
	UINT databits /*= 8*/, UINT stopsbits /*= 1*/, DWORD dwCommEvents /*= EV_RXCHAR*/)
{
	/** 临时变量,将制定参数转化为字符串形式,以构造DCB结构 */
	char szDCBparam[256];
	//baud = 115200;
	baud = 115200;
	sprintf(szDCBparam, "baud=%d parity=%c data=%d stop=%d", baud, parity, databits, stopsbits);

	/** 打开指定串口,该函数内部已经有临界区保护,上面请不要加保护 */
	if (!openPort(portNo))
	{
		return false;
	}

	/** 进入临界段 */
	EnterCriticalSection(&m_csCommunicationSync);

	/** 是否有错误发生 */
	BOOL bIsSuccess = TRUE;

	/** 在此可以设置输入输出的缓冲区大小,如果不设置,则系统会设置默认值.
	*  自己设置缓冲区大小时,要注意设置稍大一些,避免缓冲区溢出
	*/
	/*if (bIsSuccess )
	{
	bIsSuccess = SetupComm(m_hComm,10,10);
	}*/
	bIsSuccess = SetupComm(m_hComm, 10000, 10000);

	/** 设置串口的超时时间,均设为0,表示不使用超时限制 */
	COMMTIMEOUTS  CommTimeouts;
	CommTimeouts.ReadIntervalTimeout = 0;
	CommTimeouts.ReadTotalTimeoutMultiplier = 0;
	CommTimeouts.ReadTotalTimeoutConstant = 0;
	CommTimeouts.WriteTotalTimeoutMultiplier = 0;
	CommTimeouts.WriteTotalTimeoutConstant = 0;
	if (bIsSuccess)
	{
		bIsSuccess = SetCommTimeouts(m_hComm, &CommTimeouts);
	}

	DCB  dcb;
	if (bIsSuccess)
	{
		// 将ANSI字符串转换为UNICODE字符串    
		DWORD dwNum = MultiByteToWideChar(CP_ACP, 0, szDCBparam, -1, NULL, 0);
		wchar_t* pwText = new wchar_t[dwNum];
		if (!MultiByteToWideChar(CP_ACP, 0, szDCBparam, -1, pwText, dwNum))
		{
			bIsSuccess = TRUE;
		}

		/** 获取当前串口配置参数,并且构造串口DCB参数 */
		BOOLEAN getC = GetCommState(m_hComm, &dcb);
		BOOLEAN buildC = BuildCommDCB(szDCBparam, &dcb);
		bIsSuccess = getC && buildC;
		/** 开启RTS flow控制 */
		dcb.fRtsControl = RTS_CONTROL_ENABLE;

		/** 释放内存空间 */
		delete[] pwText;
	}

	if (bIsSuccess)
	{
		/** 使用DCB参数配置串口状态 */
		bIsSuccess = SetCommState(m_hComm, &dcb);
	}

	/**  清空串口缓冲区 */
	PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	/** 离开临界段 */
	LeaveCriticalSection(&m_csCommunicationSync);

	return bIsSuccess == TRUE;
}

bool CDataFromUsart::InitPort(UINT portNo, const LPDCB& plDCB)
{
	/** 打开指定串口,该函数内部已经有临界区保护,上面请不要加保护 */
	if (!openPort(portNo))
	{
		return false;
	}

	/** 进入临界段 */
	EnterCriticalSection(&m_csCommunicationSync);

	/** 配置串口参数 */
	if (!SetCommState(m_hComm, plDCB))
	{
		return false;
	}

	/**  清空串口缓冲区 */
	PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	/** 离开临界段 */
	LeaveCriticalSection(&m_csCommunicationSync);

	return true;
}

void CDataFromUsart::ClosePort()
{
	/** 如果有串口被打开，关闭它 */
	if (m_hComm != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hComm);
		m_hComm = INVALID_HANDLE_VALUE;
	}
}

bool CDataFromUsart::openPort(UINT portNo)
{
	/** 进入临界段 */
	EnterCriticalSection(&m_csCommunicationSync);

	/** 把串口的编号转换为设备名 */
	CHAR szPort[50];
	wsprintf(szPort, TEXT("COM%d"), portNo);

	/** 打开指定的串口 */
	m_hComm = CreateFile(szPort,  /** 设备名,COM1,COM2等 */
		GENERIC_READ | GENERIC_WRITE, /** 访问模式,可同时读写 */
		0,                            /** 共享模式,0表示不共享 */
		NULL,                         /** 安全性设置,一般使用NULL */
		OPEN_EXISTING,                /** 该参数表示设备必须存在,否则创建失败 */
		0,
		NULL);

	/** 如果打开失败，释放资源并返回 */
	if (m_hComm == INVALID_HANDLE_VALUE)
	{
		LeaveCriticalSection(&m_csCommunicationSync);
		return false;
	}

	/** 退出临界区 */
	LeaveCriticalSection(&m_csCommunicationSync);

	return true;
}

bool CDataFromUsart::OpenListenThread()
{
	/** 检测线程是否已经开启了 */
	if (m_hListenThread != INVALID_HANDLE_VALUE)
	{
		/** 线程已经开启 */
		return false;
	}

	s_bExit = false;
	/** 线程ID */
	UINT threadId;
	/** 开启串口数据监听线程 */
	m_hListenThread = (HANDLE)_beginthreadex(NULL, 0, ListenThread, this, 0, &threadId);
	if (!m_hListenThread)
	{
		return false;
	}
	/** 设置线程的优先级,高于普通线程 */
	if (!SetThreadPriority(m_hListenThread, THREAD_PRIORITY_ABOVE_NORMAL))
	{
		return false;
	}

	return true;
}

bool CDataFromUsart::CloseListenTread()
{
	if (m_hListenThread != INVALID_HANDLE_VALUE)
	{
		/** 通知线程退出 */
		s_bExit = true;

		/** 等待线程退出 */
		Sleep(10);

		/** 置线程句柄无效 */
		CloseHandle(m_hListenThread);
		m_hListenThread = INVALID_HANDLE_VALUE;
	}
	return true;
}

UINT CDataFromUsart::GetBytesInCOM()
{
	DWORD dwError = 0;  /** 错误码 */
	COMSTAT  comstat;   /** COMSTAT结构体,记录通信设备的状态信息 */
	memset(&comstat, 0, sizeof(COMSTAT));

	UINT BytesInQue = 0;
	/** 在调用ReadFile和WriteFile之前,通过本函数清除以前遗留的错误标志 */
	if (ClearCommError(m_hComm, &dwError, &comstat))
	{
		BytesInQue = comstat.cbInQue; /** 获取在输入缓冲区中的字节数 */
	}

	return BytesInQue;
}
char buff[10000000];
int buffi = 0;
UINT WINAPI CDataFromUsart::ListenThread(void* pParam)
{
	/** 得到本类的指针 */
	CDataFromUsart* pSerialPort = reinterpret_cast<CDataFromUsart*>(pParam);
	char buff[1000000];
	// 线程循环,轮询方式读取串口数据    
	UINT BytesInQue = 0;
	while (Program_In_Process)
	{
		BytesInQue += pSerialPort->GetBytesInCOM();
		/** 如果串口输入缓冲区中无数据,则休息一会再查询 */
		if (BytesInQue == 0)
		{
			Sleep(SLEEP_TIME_INTERVAL);
			continue;
		}
		/** 读取输入缓冲区中的数据并输出显示 */
		UINT BytesReadBlock;
		if ((AdcDataI->Len << AdcDataI->MoveBit) - AdcDataI->CharPos < BytesInQue) {
			BytesReadBlock = (AdcDataI->Len << AdcDataI->MoveBit) - AdcDataI->CharPos;
			BytesInQue -= BytesReadBlock;
		}
		else
			BytesReadBlock = BytesInQue;
		UINT nBytesRead = pSerialPort->ReadBytesInQue((char*)(AdcDataI->Buff) + AdcDataI->CharPos, BytesReadBlock);
		//cout << " - " << nBytesRead << ":" << BytesInQue << endl;
		if (nBytesRead != BytesReadBlock) 
			cout << "Usart ReadByte Error\n" 
				<< "nBytesRead: " << nBytesRead << "  BytesReadBlock : " << BytesReadBlock << endl;

		static int count = 0;
		if (count++ % 100 == 0)
		{
			cout << endl << "StringToHex" << endl;
			StringToHex((char*)AdcDataI->Buff + AdcDataI->CharPos, nBytesRead);
		}

		AdcDataI->CharPos += nBytesRead;
		AdcDataI->Pos = AdcDataI->CharPos >> 2;
		if (AdcDataI->CharPos == (AdcDataI->Len << AdcDataI->MoveBit)) AdcDataI->CharPos = 0;
		AdcDataI->GetNew = true;

	}

	isGetDataExited = true;
	return 0;
}

UINT CDataFromUsart::ReadBytesInQue(char* Buff, UINT BytesInQue)
{
	bool bResult;
	DWORD BytesRead = 0;
	if (m_hComm == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	/** 临界区保护 */
	EnterCriticalSection(&m_csCommunicationSync);

	/** 从缓冲区读取一个字节的数据 */
	bResult = ReadFile(m_hComm, Buff, BytesInQue, &BytesRead, NULL);
	if ((!bResult))
	{
		/** 获取错误码,可以根据该错误码查出错误原因 */
		DWORD dwError = GetLastError();
		cout << "Err " << dwError << endl;

		/** 清空串口缓冲区 */
		PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);
		LeaveCriticalSection(&m_csCommunicationSync);

		return 0;
	}

	/** 离开临界区 */
	LeaveCriticalSection(&m_csCommunicationSync);

	return BytesRead;

}

bool CDataFromUsart::ReadChar(char& cRecved)
{
	BOOL  bResult = TRUE;
	DWORD BytesRead = 0;
	if (m_hComm == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	/** 临界区保护 */
	EnterCriticalSection(&m_csCommunicationSync);

	/** 从缓冲区读取一个字节的数据 */
	bResult = ReadFile(m_hComm, &cRecved, 1, &BytesRead, NULL);
	if ((!bResult))
	{
		/** 获取错误码,可以根据该错误码查出错误原因 */
		DWORD dwError = GetLastError();

		/** 清空串口缓冲区 */
		PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);
		LeaveCriticalSection(&m_csCommunicationSync);

		return false;
	}

	/** 离开临界区 */
	LeaveCriticalSection(&m_csCommunicationSync);

	return (BytesRead == 1);

}

void CDataFromUsart::ReadData(uint8* buff)
{
	// 线程循环,轮询方式读取串口数据  
	while (!this->Readexit)
	{
		UINT BytesInQue = this->GetBytesInCOM();
		/** 如果串口输入缓冲区中无数据,则休息一会再查询 */
		if (BytesInQue == 0)
		{
			Sleep(SLEEP_TIME_INTERVAL);
			continue;
		}

		memset(buff, 0x00, sizeof(buff));
		/** 读取输入缓冲区中的数据并输出显示 */
		char cRecved = 0x00;
		int count = 0;
		do
		{
			cRecved = 0x00;
			if (this->ReadChar(cRecved) == true)
			{
				if (Readexit)
				{
					break;
				}
				cout << (unsigned)cRecved << " ";
				continue;
			}
		} while (--BytesInQue);
	}
}

bool CDataFromUsart::WriteData(unsigned char* pData, unsigned int length)
{
	BOOL   bResult = TRUE;
	DWORD  BytesToSend = 0;
	if (m_hComm == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	/** 临界区保护 */
	EnterCriticalSection(&m_csCommunicationSync);

	/** 向缓冲区写入指定量的数据 */
	bResult = WriteFile(m_hComm, pData, length, &BytesToSend, NULL);
	if (!bResult)
	{
		DWORD dwError = GetLastError();
		/** 清空串口缓冲区 */
		PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);
		LeaveCriticalSection(&m_csCommunicationSync);

		return false;
	}

	/** 离开临界区 */
	LeaveCriticalSection(&m_csCommunicationSync);

	return true;
}

LPTHREAD_START_ROUTINE CDataFromUsart::GetDataUsartThreadFun(LPVOID lp)
{
	OPENCONSOLE_SAVED;
	clsGetDataUsart.UsartGetData();
	//CLOSECONSOLE;
	return 0;
}
