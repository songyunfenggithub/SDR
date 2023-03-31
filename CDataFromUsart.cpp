
#include "stdafx.h"

#include <iostream>
#include <process.h>

#include "public.h"
#include "CWaveData.h"

#include "CDataFromUsart.h"

using  namespace std;

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

/** �߳��˳���־ */
bool CDataFromUsart::s_bExit = false;
/** ������������ʱ,sleep���´β�ѯ�����ʱ��,��λ:�� */
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
	/** ��ʱ����,���ƶ�����ת��Ϊ�ַ�����ʽ,�Թ���DCB�ṹ */
	char szDCBparam[256];
	//baud = 115200;
	baud = 115200;
	sprintf(szDCBparam, "baud=%d parity=%c data=%d stop=%d", baud, parity, databits, stopsbits);

	/** ��ָ������,�ú����ڲ��Ѿ����ٽ�������,�����벻Ҫ�ӱ��� */
	if (!openPort(portNo))
	{
		return false;
	}

	/** �����ٽ�� */
	EnterCriticalSection(&m_csCommunicationSync);

	/** �Ƿ��д����� */
	BOOL bIsSuccess = TRUE;

	/** �ڴ˿���������������Ļ�������С,���������,��ϵͳ������Ĭ��ֵ.
	*  �Լ����û�������Сʱ,Ҫע�������Դ�һЩ,���⻺�������
	*/
	/*if (bIsSuccess )
	{
	bIsSuccess = SetupComm(m_hComm,10,10);
	}*/
	bIsSuccess = SetupComm(m_hComm, 10000, 10000);

	/** ���ô��ڵĳ�ʱʱ��,����Ϊ0,��ʾ��ʹ�ó�ʱ���� */
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
		// ��ANSI�ַ���ת��ΪUNICODE�ַ���    
		DWORD dwNum = MultiByteToWideChar(CP_ACP, 0, szDCBparam, -1, NULL, 0);
		wchar_t* pwText = new wchar_t[dwNum];
		if (!MultiByteToWideChar(CP_ACP, 0, szDCBparam, -1, pwText, dwNum))
		{
			bIsSuccess = TRUE;
		}

		/** ��ȡ��ǰ�������ò���,���ҹ��촮��DCB���� */
		BOOLEAN getC = GetCommState(m_hComm, &dcb);
		BOOLEAN buildC = BuildCommDCB(szDCBparam, &dcb);
		bIsSuccess = getC && buildC;
		/** ����RTS flow���� */
		dcb.fRtsControl = RTS_CONTROL_ENABLE;

		/** �ͷ��ڴ�ռ� */
		delete[] pwText;
	}

	if (bIsSuccess)
	{
		/** ʹ��DCB�������ô���״̬ */
		bIsSuccess = SetCommState(m_hComm, &dcb);
	}

	/**  ��մ��ڻ����� */
	PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	/** �뿪�ٽ�� */
	LeaveCriticalSection(&m_csCommunicationSync);

	return bIsSuccess == TRUE;
}

bool CDataFromUsart::InitPort(UINT portNo, const LPDCB& plDCB)
{
	/** ��ָ������,�ú����ڲ��Ѿ����ٽ�������,�����벻Ҫ�ӱ��� */
	if (!openPort(portNo))
	{
		return false;
	}

	/** �����ٽ�� */
	EnterCriticalSection(&m_csCommunicationSync);

	/** ���ô��ڲ��� */
	if (!SetCommState(m_hComm, plDCB))
	{
		return false;
	}

	/**  ��մ��ڻ����� */
	PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	/** �뿪�ٽ�� */
	LeaveCriticalSection(&m_csCommunicationSync);

	return true;
}

void CDataFromUsart::ClosePort()
{
	/** ����д��ڱ��򿪣��ر��� */
	if (m_hComm != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hComm);
		m_hComm = INVALID_HANDLE_VALUE;
	}
}

bool CDataFromUsart::openPort(UINT portNo)
{
	/** �����ٽ�� */
	EnterCriticalSection(&m_csCommunicationSync);

	/** �Ѵ��ڵı��ת��Ϊ�豸�� */
	CHAR szPort[50];
	wsprintf(szPort, TEXT("COM%d"), portNo);

	/** ��ָ���Ĵ��� */
	m_hComm = CreateFile(szPort,  /** �豸��,COM1,COM2�� */
		GENERIC_READ | GENERIC_WRITE, /** ����ģʽ,��ͬʱ��д */
		0,                            /** ����ģʽ,0��ʾ������ */
		NULL,                         /** ��ȫ������,һ��ʹ��NULL */
		OPEN_EXISTING,                /** �ò�����ʾ�豸�������,���򴴽�ʧ�� */
		0,
		NULL);

	/** �����ʧ�ܣ��ͷ���Դ������ */
	if (m_hComm == INVALID_HANDLE_VALUE)
	{
		LeaveCriticalSection(&m_csCommunicationSync);
		return false;
	}

	/** �˳��ٽ��� */
	LeaveCriticalSection(&m_csCommunicationSync);

	return true;
}

bool CDataFromUsart::OpenListenThread()
{
	/** ����߳��Ƿ��Ѿ������� */
	if (m_hListenThread != INVALID_HANDLE_VALUE)
	{
		/** �߳��Ѿ����� */
		return false;
	}

	s_bExit = false;
	/** �߳�ID */
	UINT threadId;
	/** �����������ݼ����߳� */
	m_hListenThread = (HANDLE)_beginthreadex(NULL, 0, ListenThread, this, 0, &threadId);
	if (!m_hListenThread)
	{
		return false;
	}
	/** �����̵߳����ȼ�,������ͨ�߳� */
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
		/** ֪ͨ�߳��˳� */
		s_bExit = true;

		/** �ȴ��߳��˳� */
		Sleep(10);

		/** ���߳̾����Ч */
		CloseHandle(m_hListenThread);
		m_hListenThread = INVALID_HANDLE_VALUE;
	}
	return true;
}

UINT CDataFromUsart::GetBytesInCOM()
{
	DWORD dwError = 0;  /** ������ */
	COMSTAT  comstat;   /** COMSTAT�ṹ��,��¼ͨ���豸��״̬��Ϣ */
	memset(&comstat, 0, sizeof(COMSTAT));

	UINT BytesInQue = 0;
	/** �ڵ���ReadFile��WriteFile֮ǰ,ͨ�������������ǰ�����Ĵ����־ */
	if (ClearCommError(m_hComm, &dwError, &comstat))
	{
		BytesInQue = comstat.cbInQue; /** ��ȡ�����뻺�����е��ֽ��� */
	}

	return BytesInQue;
}
char buff[10000000];
int buffi = 0;
UINT WINAPI CDataFromUsart::ListenThread(void* pParam)
{
	/** �õ������ָ�� */
	CDataFromUsart* pSerialPort = reinterpret_cast<CDataFromUsart*>(pParam);
	char buff[1000000];
	// �߳�ѭ��,��ѯ��ʽ��ȡ��������    
	UINT BytesInQue = 0;
	while (Program_In_Process)
	{
		BytesInQue += pSerialPort->GetBytesInCOM();
		/** ����������뻺������������,����Ϣһ���ٲ�ѯ */
		if (BytesInQue == 0)
		{
			Sleep(SLEEP_TIME_INTERVAL);
			continue;
		}
		/** ��ȡ���뻺�����е����ݲ������ʾ */
		UINT BytesReadBlock;
		if ((DATA_BUFFER_LENGTH << 2) - clsWaveData.AdcGetCharLength < BytesInQue) {
			BytesReadBlock = (DATA_BUFFER_LENGTH << 2) - clsWaveData.AdcGetCharLength;
			BytesInQue -= BytesReadBlock;
		}
		else
			BytesReadBlock = BytesInQue;
		UINT nBytesRead = pSerialPort->ReadBytesInQue((char*)(clsWaveData.AdcBuff) + clsWaveData.AdcGetCharLength, BytesReadBlock);
		//cout << " - " << nBytesRead << ":" << BytesInQue << endl;
		if (nBytesRead != BytesReadBlock) 
			cout << "Usart ReadByte Error\n" 
				<< "nBytesRead: " << nBytesRead << "  BytesReadBlock : " << BytesReadBlock << endl;

		static int count = 0;
		if (count++ % 100 == 0)
		{
			cout << endl << "StringToHex" << endl;
			StringToHex((char*)clsWaveData.AdcBuff + clsWaveData.AdcGetCharLength, nBytesRead);
		}

		clsWaveData.NumPerSecInProcess += nBytesRead;
		clsWaveData.AdcGetCharLength += nBytesRead;
		clsWaveData.AdcPos = clsWaveData.AdcGetCharLength >> 2;
		if (clsWaveData.AdcGetCharLength == (DATA_BUFFER_LENGTH << 2)) clsWaveData.AdcGetCharLength = 0;
		clsWaveData.AdcGetNew = true;

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

	/** �ٽ������� */
	EnterCriticalSection(&m_csCommunicationSync);

	/** �ӻ�������ȡһ���ֽڵ����� */
	bResult = ReadFile(m_hComm, Buff, BytesInQue, &BytesRead, NULL);
	if ((!bResult))
	{
		/** ��ȡ������,���Ը��ݸô�����������ԭ�� */
		DWORD dwError = GetLastError();
		cout << "Err " << dwError << endl;

		/** ��մ��ڻ����� */
		PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);
		LeaveCriticalSection(&m_csCommunicationSync);

		return 0;
	}

	/** �뿪�ٽ��� */
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

	/** �ٽ������� */
	EnterCriticalSection(&m_csCommunicationSync);

	/** �ӻ�������ȡһ���ֽڵ����� */
	bResult = ReadFile(m_hComm, &cRecved, 1, &BytesRead, NULL);
	if ((!bResult))
	{
		/** ��ȡ������,���Ը��ݸô�����������ԭ�� */
		DWORD dwError = GetLastError();

		/** ��մ��ڻ����� */
		PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);
		LeaveCriticalSection(&m_csCommunicationSync);

		return false;
	}

	/** �뿪�ٽ��� */
	LeaveCriticalSection(&m_csCommunicationSync);

	return (BytesRead == 1);

}

void CDataFromUsart::ReadData(uint8* buff)
{
	// �߳�ѭ��,��ѯ��ʽ��ȡ��������  
	while (!this->Readexit)
	{
		UINT BytesInQue = this->GetBytesInCOM();
		/** ����������뻺������������,����Ϣһ���ٲ�ѯ */
		if (BytesInQue == 0)
		{
			Sleep(SLEEP_TIME_INTERVAL);
			continue;
		}

		memset(buff, 0x00, sizeof(buff));
		/** ��ȡ���뻺�����е����ݲ������ʾ */
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

	/** �ٽ������� */
	EnterCriticalSection(&m_csCommunicationSync);

	/** �򻺳���д��ָ���������� */
	bResult = WriteFile(m_hComm, pData, length, &BytesToSend, NULL);
	if (!bResult)
	{
		DWORD dwError = GetLastError();
		/** ��մ��ڻ����� */
		PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);
		LeaveCriticalSection(&m_csCommunicationSync);

		return false;
	}

	/** �뿪�ٽ��� */
	LeaveCriticalSection(&m_csCommunicationSync);

	return true;
}

LPTHREAD_START_ROUTINE CDataFromUsart::GetDataUsartThreadFun(LPVOID lp)
{
	OPENCONSOLE;
	clsGetDataUsart.UsartGetData();
	CLOSECONSOLE;
	return 0;
}