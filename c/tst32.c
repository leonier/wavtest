#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "tst.h"
 FILE* fp;
 wavinfo wi;

main()
{
	unsigned char* wavhead;
	wavhead=malloc(36);
	fp=fopen("d.wav","rb");
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
	makedifferwav_32(fp, &wi);
	fclose(fp);
	free(wavhead);
}
