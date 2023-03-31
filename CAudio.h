#pragma once

#include "CWaveData.h"
#include "CSoundCard.h"

#define AUDIO_BUFF_LENGTH		0x100000
#define AUDIO_BUFF_LENGTH_MASK	(AUDIO_BUFF_LENGTH - 1)

class CAudio
{
public:

	typedef float AUDIODATATYPE;

	AUDIODATATYPE *Buff;
	UINT BuffPos = 0;

	CSoundCard* m_SoundCard = NULL;

public:
	CAudio();
	~CAudio();

	void Init(void);
	void UnInit(void);

};

