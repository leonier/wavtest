#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "tst.h"
#include "dmcommon.h"

FILE* fp;
wavinfo* wi;
dmarg* dminfo;
void decodedm(FILE* fpi, wavinfo* wav, dmarg* dminfo);

main()
{
	dminfo=malloc(sizeof(dmarg));
	wi=malloc(sizeof(wavinfo));
	fp=fopen("test.dm","rb");
	fread(dminfo,sizeof(dmarg),1,fp);
	if(dminfo->magicnum!=MAGICNUM)
	{
		printf("not a dm file!\n");
		exit(0);
	}
	printf("DM delta value %d\n",dminfo->delta);
	if(dminfo->mode==MODE_TYPE1)
		printf("Channel Delta Mode\n");
	if(dminfo->mode==MODE_TYPE2)
		printf("L+R L-R Mode\n");
	fread(wi,sizeof(wavinfo),1,fp);
	printf("Sample rate %dHz, %d bits, %d channels\n",wi->srate,wi->bits,wi->channel);
	printf("%d samples\n",wi->samples);
	decodedm(fp,wi,dminfo);
	fclose(fp);
	free(wi);
	free(dminfo);
}

void decodedm(FILE* fpi, wavinfo* wav, dmarg* dminfo)
{
	FILE *fpo;
	unsigned char* head,* data;
	unsigned char samp[4];
	int samp1l=0,samp1r=0;
	int sampl=0,sampr=0;
	int valuel=0,valuer=0;
	
	int delta,i;

	int wcount=0;
	unsigned int wbuf=0;
	unsigned int bmask=0x80000000;
	
	head=malloc(36);
	data=malloc(8);
	writeriffhead(head, 36+8+wav->datalen);
	writepcmfmt(head+12, wav->channel, wav->srate, wav->bits);
	writedatachunkhead(data, wav->channel, wav->bits, wav->samples);
	
	fpo=fopen("dedm_raw.wav","wb");
	if(!fpo)
	{
		printf("Open destination file failed!\n");
		return;
	}
	fwrite(head, 36, 1, fpo);
	fwrite(data, 8, 1, fpo);
	free(head);
	free(data);
	
	switch(dminfo->mode)
	{
	case MODE_TYPE0:
		for(i=0; i<wav->samples; i++)
		{
			if(!wcount)
				fread(&wbuf,sizeof(unsigned int),1,fpi);
		
			sampl=(wbuf&bmask)?1:0;
			bmask=bmask>>1;

			sampr=(wbuf&bmask)?1:0;	
			bmask=bmask>>1;	
			wcount+=2;
			if(wcount>31)
			{
				wcount=0;
				bmask=0x80000000;
			}
		
			sampl=dmp1bit(sampl,samp1l,dminfo->delta);
			sampr=dmp1bit(sampr,samp1r,dminfo->delta);
		
			samp[0]=sampl&0xff;
			samp[1]=(sampl&0xff00)>>8;
			samp[2]=sampr&0xff;
			samp[3]=(sampr&0xff00)>>8;

			fwrite(samp,4,1,fpo);
			samp1l=sampl;
			samp1r=sampr;
		}
		break;
	case MODE_TYPE1:
		for(i=0; i<(wav->samples)*2; i++)
		{
			if(!wcount)
				fread(&wbuf,sizeof(unsigned int),1,fpi);
		
			sampl=(wbuf&bmask)?1:0;
			bmask=bmask>>1;

			wcount++;
			if(wcount>31)
			{
				wcount=0;
				bmask=0x80000000;
			}
		
			sampl=dmp1bit(sampl,samp1l,dminfo->delta);
		
			samp[0]=sampl&0xff;
			samp[1]=(sampl&0xff00)>>8;

			fwrite(samp,2,1,fpo);
			samp1l=sampl;
		}
		break;	
	case MODE_TYPE2:
		for(i=0; i<wav->samples; i++)
		{
			if(!wcount)
				fread(&wbuf,sizeof(unsigned int),1,fpi);
		
			sampl=(wbuf&bmask)?1:0;
			bmask=bmask>>1;

			sampr=(wbuf&bmask)?1:0;	
			bmask=bmask>>1;	
			wcount+=2;
			if(wcount>31)
			{
				wcount=0;
				bmask=0x80000000;
			}
		
			sampl=dmp1bit(sampl,samp1l,dminfo->delta);
			sampr=dmp1bit(sampr,samp1r,dminfo->delta);
			
			valuel=(sampl+sampr);
			valuer=(sampr-sampl);
			samp[0]=valuel&0xff;
			samp[1]=(valuel&0xff00)>>8;
			samp[2]=valuer&0xff;
			samp[3]=(valuer&0xff00)>>8;

			fwrite(samp,4,1,fpo);
			samp1l=sampl;
			samp1r=sampr;
		}
		break;
	}
	fclose(fpo);
}