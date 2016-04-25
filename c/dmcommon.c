#include "dmcommon.h"
//prediction
short dmp1bit(int e, int f, short delta)
{
	int fr=0;
	if(e!=0)
		fr=f+delta/2;
	else
		fr=f-delta/2;
	if(fr<=32767&&fr>=-32768)
	{
		return (short)fr;
	}
	else
	{
		if(fr<-32768) return -32768;
		else return 32767;
	}
	//return (short)fr;
}


//1-bit quantize
short quant1bit(short f0, short f1)
{
	short sampout;
	if((f1-f0)>0)
		sampout=QVALUE;
	else
		sampout=0;
	return sampout;
}

