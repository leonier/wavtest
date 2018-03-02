#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "tst.h"
  
int main(int argc, char* argv[])
{
	FILE* fp, *fpo;
	wavinfo wi;
	char* ofn;
	unsigned char* wavhead;
	int ismono=0,lr=0;
	wavhead=malloc(36);
	if(argc<2) 
	{
		printf("Usage: %s wavfile [m]\n", argv[0]);
		return 0;
	}
	if(argc>=3)
	{
		if(argv[2][0]=='m')
			ismono=1;
		else if(argv[2][0]=='l')
			lr=1;
	}
	
	wavhead=malloc(36);
	fp=fopen(argv[1],"rb");
	if(!fp)
	{
		printf("Open WAV file failed!\n");
		return 0;
	}
	fread(wavhead,36,1,fp);
	if(!parsewaveheader(wavhead,&wi))
	{
		printf("WAV header error!\n");
		exit(0);
	}
	printf("WAV sample rate %d\nWAV bits per sample %d\nWAV channels %d\n",wi.srate, wi.bits,wi.channel);
	wi.datalen=finddatachunk(fp);
	wi.samples=wi.datalen/((wi.bits/8)*wi.channel);
	printf("WAV data length %d\n",wi.datalen);
	ofn=(char*)malloc(strlen(argv[1])+5);
	sprintf(ofn, "%s.wav", argv[1]);
	fpo=fopen(ofn, "wb");
	if(!fpo)
	{
		printf("Open output WAV file %s failed!\n", ofn);
		goto exit;
	}
	if(!ismono)
		cutwavlr(fp, fpo, &wi, lr);
	else
		cutwavlr_mono(fp, fpo, &wi);
exit:
	fclose(fp);
	free(wavhead);
	free(ofn);
}