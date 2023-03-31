#include "CAudio.h"


CAudio::CAudio()
{
	Init();
}

CAudio::~CAudio()
{
	UnInit();
}


void CAudio::Init(void)
{
	Buff = new CAudio::AUDIODATATYPE[AUDIO_BUFF_LENGTH];
	m_SoundCard = new CSoundCard;
}

void CAudio::UnInit(void)
{
	if (Buff != NULL) delete[] Buff;
	if (m_SoundCard != NULL) free(m_SoundCard);
}