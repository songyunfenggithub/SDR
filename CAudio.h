#pragma once

#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include <commdlg.h>

#define SOUNDCARD_BUFF_DATA_BIT		16
#define SOUNDCARD_STEP_LENGTH		0x1000
#define SOUNDCARD_BUFF_LENGTH		0x100000
#define SOUNDCARD_BUFF_LENGTH_MASK	(SOUNDCARD_BUFF_LENGTH - 1)

//#define SOUNDCARD_SAMPLE	44100L
#define SOUNDCARD_SAMPLE	11025L

#define SOUNDCARD_WAVEHDR_DEEP		0x8
#define SOUNDCARD_WAVEHDR_DEEP_MASK	(SOUNDCARD_WAVEHDR_DEEP - 1)
namespace METHOD {
	class cuda_CFilter;
}
using namespace METHOD;

class CData;

namespace DEVICES {

	class CAudio
	{
	public:

		typedef struct INDATATAG
		{
			BOOL		fInOpen;
			HWAVEIN		hWaveIn;
			WAVEHDR		WaveInHdr1;
			WAVEHDR		WaveInHdr2;
		} INDATA, * PINDATA;

		INDATA InData;

		typedef struct OUTDATATAG
		{
			BOOL		fOutOpen;
			HWAVEOUT	hWaveOut;
			WAVEHDR		WaveOutHdr1;
			WAVEHDR		WaveOutHdr2;
		} OUTDATA, * POUTDATA;

		OUTDATA	OutData;

		typedef short AUDIODATATYPE;

		CData* inData = NULL;
		//	AUDIODATATYPE inBuff[SOUNDCARD_BUFF_LENGTH];
		//	UINT	inBuffPos = 0;
		UINT	inBeginPos = 0, inEndPos = 0;;

		CData* inDataFiltted = NULL;
		//	AUDIODATATYPE inFilttedBuff[SOUNDCARD_BUFF_LENGTH];
		//	UINT inFilttedPos = 0;

		//	AUDIODATATYPE outBuff[SOUNDCARD_BUFF_LENGTH];
		//	UINT	outBuffPos = 0;
		CData* outData = NULL;
		UINT	outBeginPos = 0, outEndPos = 0;

		CData* outDataFiltted = NULL;
		//	AUDIODATATYPE outFilttedBuff[SOUNDCARD_BUFF_LENGTH];
		//	UINT outFilttedPos = 0;

		cuda_CFilter* cudafilter = NULL;

		WAVEFORMATEX	FormatEx;
		HWAVEOUT		hWaveOut;
		WAVEHDR			WaveOutHdrs[SOUNDCARD_WAVEHDR_DEEP];
		UINT			waveHDRPos, waveHDRUsed;
		bool			boutOpened = false;

		UINT uSampleRate = SOUNDCARD_SAMPLE;
		UINT *SampleRate = &uSampleRate;
		float Am_zoom = 10.0;

		bool bPlay = true;

		HANDLE hThread = NULL;
		bool Doing = false;

	public:
		CAudio();
		~CAudio();

		void Init(void);
		void UnInit(void);

		void OpenIn(UINT pos, UINT endPos);
		void CloseIn(void);

		void OpenOut(UINT dwBeginPos, UINT dwEndPos);
		void PauseOut(void);
		void CloseOut(void);

		void GeneratorWave(void);

		void StartOut(void);
		void StopOut(void);
		INT WriteToOut(UINT pos);
		INT WriteToOut(void* buff);
		static void CALLBACK waveOutProc2(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);

		static void CALLBACK waveInProc(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
		static void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);

		void Thread_Audio_Out_Func(void);
		static LPTHREAD_START_ROUTINE Thread_Audio_Out(LPVOID lp);
	};
}

extern DEVICES::CAudio mAudio;
