#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "tst.h"
 
#define FSAMPLES 10000000
FILE* fp;
int srate,channel,bits;
int scount;


main()
{
	int i;
	unsigned char* head,* data;
	short left, right;
	srate=44100;
	channel=2;
	bits=16;
	head=malloc(36);
	data=malloc(8);
	writeriffhead(head, 36+8+FSAMPLES*channel*(bits/8));
	writepcmfmt(head+12, channel, srate, bits);
	writedatachunkhead(data, channel, bits, FSAMPLES);
	fp=fopen("d.wav","wb");
	fwrite(head, 36, 1, fp);
	fwrite(data, 8, 1, fp);
	free(head);
	free(data);
	for(i=0; i<FSAMPLES;i++)
	{
		left=(rand()-0x4000)*2;
		right=(rand()-0x4000)*2;
		fwrite(&left, 2, 1, fp);
		fwrite(&right, 2, 1, fp);
	}
	fclose(fp);
}