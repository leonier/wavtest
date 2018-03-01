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
	wavhead=malloc(36);
	if(argc<2) 
		return 0;
	wavhead=malloc(36);
	fp=fopen(argv[1],"rb");
	if(!fp)
		return 0;
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
		goto exit;
		
	cutwavlr(fp, fpo, &wi);
exit:
	fclose(fp);
	free(wavhead);
	free(ofn);
}