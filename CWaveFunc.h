#pragma once
class CWaveFunc
{

public:

public:
	CWaveFunc();
	~CWaveFunc();

	void WaveGenerate(void);

	double Sin(UINT32 i, double Hz, double amplitude);
	double Triangle(UINT32 i, double Hz, double amplitude);


};

extern CWaveFunc clsWaveFunc;