#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tst.h"

void writeriffhead(unsigned char* hdr, int flength)
{
	*(hdr)='R';
	*(hdr+1)='I';
	*(hdr+2)='F';
	*(hdr+3)='F';
	
	*(hdr+4)=(unsigned char)(flength&0xff);
	*(hdr+5)=(unsigned char)((flength&0xff00)>>8);
	*(hdr+6)=(unsigned char)((flength&0xff0000)>>16);
	*(hdr+7)=(unsigned char)((flength&0xff000000)>>24);

	*(hdr+8)='W';
	*(hdr+9)='A';
	*(hdr+10)='V';
	*(hdr+11)='E';

}
void writepcmfmt(unsigned char* hdr, int wchannel, int wsrate, int wbits)
{
	int Bps;
	
	Bps=wsrate*wchannel*(wbits/8);
	*(hdr)='f';
	*(hdr+1)='m';
	*(hdr+2)='t';
	*(hdr+3)=0x20;
	
	*(hdr+4)=0x10;
	*(hdr+5)=0; 
	*(hdr+6)=0; 
	*(hdr+7)=0; 
	 
	*(hdr+8)=1; 
	*(hdr+9)=0; 
	
	*(hdr+10)=(unsigned char)(wchannel&0xff);
	*(hdr+11)=(unsigned char)((wchannel&0xff00)>>8);
	
	*(hdr+12)=(unsigned char)(wsrate&0xff);
	*(hdr+13)=(unsigned char)((wsrate&0xff00)>>8);
	*(hdr+14)=(unsigned char)((wsrate&0xff0000)>>16);
	*(hdr+15)=(unsigned char)((wsrate&0xff000000)>>24);
	
	*(hdr+16)=(unsigned char)(Bps&0xff);
	*(hdr+17)=(unsigned char)((Bps&0xff00)>>8);
	*(hdr+18)=(unsigned char)((Bps&0xff0000)>>16);
	*(hdr+19)=(unsigned char)((Bps&0xff000000)>>24);
	
	*(hdr+20)=4; 
	*(hdr+21)=0; 
	
	*(hdr+22)=(unsigned char)(wbits&0xff);
	*(hdr+23)=(unsigned char)((wbits&0xff00)>>8);
}

void writedatachunkhead(unsigned char* hdr, int wchannel, int wbits, int nsamples)
{
	int size=wchannel*(wbits/8)*nsamples;
	*(hdr)='d';
	*(hdr+1)='a';
	*(hdr+2)='t';
	*(hdr+3)='a';
	
	*(hdr+4)=(unsigned char)(size&0xff);
	*(hdr+5)=(unsigned char)((size&0xff00)>>8);
	*(hdr+6)=(unsigned char)((size&0xff0000)>>16);
	*(hdr+7)=(unsigned char)((size&0xff000000)>>24);
	
} 

int parsewaveheader(unsigned char* wavhead, wavinfo* winfo)
{
	int whrate=0, whchannel=0, whbits=0;  
	if(*(wavhead)!='R' ||	*(wavhead+1)!='I' 
	|| *(wavhead+2)!='F' || *(wavhead+3)!='F')
		goto error;
		
	if(*(wavhead+8)!='W' ||	*(wavhead+9)!='A' 
	|| *(wavhead+10)!='V' || *(wavhead+11)!='E')
		goto error;		
	
	if(*(wavhead+12)!='f' ||	*(wavhead+13)!='m' 
	|| *(wavhead+14)!='t' ||    *(wavhead+15)!=' ')
		goto error;		
			
	whchannel=*(wavhead+22)+((int)(*(wavhead+23))<<8);
	whrate=*(wavhead+24)+((int)(*(wavhead+25))<<8)+((int)(*(wavhead+26))<<16)+((int)(*(wavhead+27))<<24);
	whbits=*(wavhead+34)+((int)(*(wavhead+35))<<8);
	winfo->srate=whrate;
	winfo->channel=whchannel;
	winfo->bits=whbits;
	return 1;
error:
	return 0;	
}

int finddatachunk(FILE* file)
{
	unsigned char data[5];
	unsigned char cdlen[5];
	int dlen;	
	int dafound=0;
	
	data[2]=0;
	while(fread(data,2,1,file)==1)
	{
		if(!strcmp(data, "ta"))
		{
			if(dafound)
			{
				//printf("data chunk found!\n");
				fread(cdlen, 4, 1, file);
				dlen=cdlen[0]+((int)(cdlen[1])<<8)+((int)(cdlen[2])<<16)+((int)(cdlen[3])<<24);
				return dlen;
			}
			else
				dafound=0;
		}
		else if(!strcmp(data, "da"))
		{
			dafound=1;
			continue;
		}
	}
	return 0;
}




void makedifferwav(FILE* fpi, wavinfo* wav)
{
	FILE *fpo;
	unsigned char* head,* data;
	unsigned char samp[2];
	short samp1=0,samp2=0,sampout=0;
	int sampdif=0;
	head=malloc(36);
	data=malloc(8);
	writeriffhead(head, 36+8+wav->datalen);
	writepcmfmt(head+12, wav->channel, wav->srate, wav->bits);
	writedatachunkhead(data, wav->channel, wav->bits, wav->samples);
	
	fpo=fopen("dd.wav","wb");
	fwrite(head, 36, 1, fpo);
	fwrite(data, 8, 1, fpo);
	free(head);
	free(data);

	fread(samp, 2,1, fpi);
	samp1=(samp[0]+((short)(samp[1])<<8));
	//sampout=samp1/2;
	sampout=samp1;
	samp[0]=sampout&0xff;
	samp[1]=(sampout&0xff00)>>8;
	fwrite(samp,2,1,fpo);
	
	while(fread(samp, 2,1,fpi))
	{
		samp2=((samp[0]+((short)(samp[1])<<8)));
		
		//sampout=samp2/2-samp1/2;
		sampout=samp2-samp1;
		samp[0]=sampout&0xff;
		samp[1]=(sampout&0xff00)>>8;
		fwrite(samp,2,1,fpo);
		samp1=samp2;
	}
	fclose(fpo);
}

void decodedifferwav(FILE* fpi, wavinfo* wav)
{
	FILE *fpo;
	unsigned char* head,* data;
	unsigned char samp[2];
	short samp1=0,samp2=0,sampo=0;
	head=malloc(36);
	data=malloc(8);
	writeriffhead(head, 36+8+wav->datalen);
	writepcmfmt(head+12, wav->channel, wav->srate, wav->bits);
	writedatachunkhead(data, wav->channel, wav->bits, wav->samples);
	
	fpo=fopen("dc.wav","wb");
	fwrite(head, 36, 1, fpo);
	fwrite(data, 8, 1, fpo);
	free(head);
	free(data);

	fread(samp, 2,1, fpi);
	samp1=(samp[0]+((short)(samp[1])<<8));
	//sampo=2*samp1;
	sampo=samp1;
	samp[0]=sampo&0xff;
	samp[1]=(sampo&0xff00)>>8;
	fwrite(samp, 2,1,fpo);
	
	while(fread(samp, 2,1,fpi))
	{
		samp2=((samp[0]+((short)(samp[1])<<8)));
		samp2+=samp1;
		//sampo=2*samp2;
		sampo=samp2;
		samp[0]=sampo&0xff;
		samp[1]=(sampo&0xff00)>>8;
		fwrite(samp,2,1,fpo);
		samp1=samp2;
	}
	fclose(fpo);
}


void makehalfwav(FILE* fpi, wavinfo* wav)
{
	FILE *fpo;
	unsigned char* head,* data;
	unsigned char samp[2];
	short samp1=0,samp2=0,sampdif=0;
	head=malloc(36);
	data=malloc(8);
	writeriffhead(head, 36+8+wav->datalen);
	writepcmfmt(head+12, wav->channel, wav->srate, wav->bits);
	writedatachunkhead(data, wav->channel, wav->bits, wav->samples);
	
	fpo=fopen("dh.wav","wb");
	fwrite(head, 36, 1, fpo);
	fwrite(data, 8, 1, fpo);
	free(head);
	free(data);

	fread(samp, 2,1, fpi);
	samp1=(samp[0]+((short)(samp[1]))<<8);
	samp1/=2;
	samp[0]=samp1&0xff;
	samp[1]=(samp1&0xff00)>>8;
	fwrite(samp, 2,1,fpo);
	
	while(fread(samp, 2,1,fpi))
	{
		samp2=((samp[0]+((short)(samp[1])<<8)));
		samp2/=2;
		samp[0]=samp2&0xff;
		samp[1]=(samp2&0xff00)>>8;
		fwrite(samp,2,1,fpo);
	}
	fclose(fpo);
}


void compresswavtohalf(FILE* fpi, wavinfo* wav)
{
	FILE *fpo;
	unsigned char* head,* data;
	unsigned char samp[2];
	short samp1=0,samp2=0,sampdif=0;
	head=malloc(36);
	data=malloc(8);
	writeriffhead(head, 36+8+wav->datalen);
	writepcmfmt(head+12, wav->channel, wav->srate, wav->bits);
	writedatachunkhead(data, wav->channel, wav->bits, wav->samples);
	
	fpo=fopen("dh.wav","wb");
	fwrite(head, 36, 1, fpo);
	fwrite(data, 8, 1, fpo);
	free(head);
	free(data);

	fread(samp, 2,1, fpi);
	samp1=(samp[0]+((short)(samp[1]))<<8);
	if(samp1>=16384)samp1=16384;
	if(samp1<=-16384)samp1=-16384;
	samp[0]=samp1&0xff;
	samp[1]=(samp1&0xff00)>>8;
	fwrite(samp, 2,1,fpo);
	
	while(fread(samp, 2,1,fpi))
	{
		samp2=((samp[0]+((short)(samp[1])<<8)));
		if(samp2>=16384)samp2=16384;
		if(samp2<=-16384)samp2=-16384;
		samp[0]=samp2&0xff;
		samp[1]=(samp2&0xff00)>>8;
		fwrite(samp,2,1,fpo);
	}
	fclose(fpo);
}

void makedifferwav_byte(FILE* fpi, wavinfo* wav)
{
	FILE *fpo;
	unsigned char* head,* data;
	unsigned char samp[2];
	unsigned char samp1=0,samp2=0,sampout=0;
	int sampdif=0;
	head=malloc(36);
	data=malloc(8);
	writeriffhead(head, 36+8+wav->datalen);
	writepcmfmt(head+12, wav->channel, wav->srate, wav->bits);
	writedatachunkhead(data, wav->channel, wav->bits, wav->samples);
	
	fpo=fopen("ddb.wav","wb");
	fwrite(head, 36, 1, fpo);
	fwrite(data, 8, 1, fpo);
	free(head);
	free(data);

	fread(&samp1, 1,1, fpi);
	//samp1=samp[0];
	//sampout=samp1/2;
	//
	sampout=samp1;
	//samp[0]=sampout;
	fwrite(&sampout,1,1,fpo);
	
	while(fread(&samp2, 1,1,fpi))
	{
		//sampout=samp2/2-samp1/2;
		sampout=samp2-samp1;
		//samp[0]=sampout;
		fwrite(&sampout,1,1,fpo);
		samp1=samp2;
	}
	fclose(fpo);
}

void decodedifferwav_byte(FILE* fpi, wavinfo* wav)
{
	FILE *fpo;
	unsigned char* head,* data;
	//unsigned char samp[2];
	unsigned char samp1=0,samp2=0,sampo=0;
	head=malloc(36);
	data=malloc(8);
	writeriffhead(head, 36+8+wav->datalen);
	writepcmfmt(head+12, wav->channel, wav->srate, wav->bits);
	writedatachunkhead(data, wav->channel, wav->bits, wav->samples);
	
	fpo=fopen("dcb.wav","wb");
	fwrite(head, 36, 1, fpo);
	fwrite(data, 8, 1, fpo);
	free(head);
	free(data);

	fread(&samp1, 1,1, fpi);
	//samp1=(samp[0]+((short)(samp[1])<<8));
	//sampo=2*samp1;
	sampo=samp1;
	//samp[0]=sampo&0xff;
	//samp[1]=(sampo&0xff00)>>8;
	fwrite(&sampo, 1,1,fpo);
	
	while(fread(&samp2, 1,1,fpi))
	{
		//samp2=((samp[0]+((short)(samp[1])<<8)));
		samp2+=samp1;
		//sampo=2*samp2;
		sampo=samp2;
		//samp[0]=sampo&0xff;
		//samp[1]=(sampo&0xff00)>>8;
		fwrite(&sampo,1,1,fpo);
		samp1=samp2;
	}
	fclose(fpo);
}

void makedifferwav_32(FILE* fpi, wavinfo* wav)
{
	FILE *fpo;
	unsigned char* head,* data;
	unsigned char samp[4];
	int samp1=0,samp2=0,sampout=0;
	int sampdif=0;
	if(wav->channel!=2 || wav->bits!=16 )
		return;
	head=malloc(36);
	data=malloc(8);
	writeriffhead(head, 36+8+wav->datalen);
	writepcmfmt(head+12, wav->channel, wav->srate, wav->bits);
	writedatachunkhead(data, wav->channel, wav->bits, wav->samples);
	
	fpo=fopen("dd32.wav","wb");
	fwrite(head, 36, 1, fpo);
	fwrite(data, 8, 1, fpo);
	free(head);
	free(data);

	fread(samp, 4,1, fpi);
	samp1=(samp[0]+((int)(samp[1])<<8)+((int)(samp[2])<<16)+((int)(samp[3])<<24));
	//sampout=samp1/2;
	/*sampout=samp1;
	samp[0]=sampout&0xff;
	samp[1]=(sampout&0xff00)>>8;*/
	fwrite(samp,4,1,fpo);
	
	while(fread(samp, 4,1,fpi))
	{
		samp2=((samp[0]+((short)(samp[1])<<8))+((int)(samp[2])<<16)+((int)(samp[3])<<24));
		
		//sampout=samp2/2-samp1/2;
		sampout=samp2-samp1;
		samp[0]=sampout&0xff;
		samp[1]=(sampout&0xff00)>>8;
		samp[2]=(sampout&0xff0000)>>16;
		samp[3]=(sampout&0xff000000)>>24;
		
		fwrite(samp,4,1,fpo);
		samp1=samp2;
	}
	fclose(fpo);
}


void decodedifferwav_32(FILE* fpi, wavinfo* wav)
{
	FILE *fpo;
	unsigned char* head,* data;
	unsigned char samp[4];
	int samp1=0,samp2=0,sampo=0;
	head=malloc(36);
	data=malloc(8);
	writeriffhead(head, 36+8+wav->datalen);
	writepcmfmt(head+12, wav->channel, wav->srate, wav->bits);
	writedatachunkhead(data, wav->channel, wav->bits, wav->samples);
	
	fpo=fopen("dc32.wav","wb");
	fwrite(head, 36, 1, fpo);
	fwrite(data, 8, 1, fpo);
	free(head);
	free(data);

	fread(samp, 4,1, fpi);
	samp1=(samp[0]+((int)(samp[1])<<8)+((int)(samp[2])<<16)+((int)(samp[3])<<24));
	//sampo=2*samp1;
	/*sampo=samp1;
	samp[0]=sampo&0xff;
	samp[1]=(sampo&0xff00)>>8;*/
	fwrite(samp, 4,1,fpo);
	
	while(fread(samp, 4,1,fpi))
	{
		samp2=((samp[0]+((short)(samp[1])<<8))+((int)(samp[2])<<16)+((int)(samp[3])<<24));
		samp2+=samp1;
		//sampo=2*samp2;
		sampo=samp2;

		samp[0]=sampo&0xff;
		samp[1]=(sampo&0xff00)>>8;
		samp[2]=(sampo&0xff0000)>>16;
		samp[3]=(sampo&0xff000000)>>24;

		fwrite(samp,4,1,fpo);
		samp1=samp2;
	}
	fclose(fpo);
}

void makedifferwav_lr(FILE* fpi, wavinfo* wav)
{
	FILE *fpo;
	unsigned char* head,* data;
	unsigned char samp[4];
	short samp1l=0,samp1r=0,samp2l=0,samp2r=0;
	short sampoutl=0, sampoutr=0;
	
	if(wav->channel!=2 || wav->bits!=16 )
		return;
	head=malloc(36);
	data=malloc(8);
	writeriffhead(head, 36+8+wav->datalen);
	writepcmfmt(head+12, wav->channel, wav->srate, wav->bits);
	writedatachunkhead(data, wav->channel, wav->bits, wav->samples);
	
	fpo=fopen("ddlr.wav","wb");
	fwrite(head, 36, 1, fpo);
	fwrite(data, 8, 1, fpo);
	free(head);
	free(data);

	fread(samp, 4,1, fpi);
	samp1l=(samp[0]+((short)(samp[1])<<8));
	samp1r=(samp[2]+((short)(samp[3])<<8));
	fwrite(samp,4,1,fpo);
	
	while(fread(samp, 4,1,fpi))
	{
		samp2l=(samp[0]+((short)(samp[1])<<8));
		samp2r=(samp[2]+((short)(samp[3])<<8));
		//sampout=samp2/2-samp1/2;
		sampoutl=samp2l-samp1l;		
		sampoutr=samp2r-samp1r;
		samp[0]=sampoutl&0xff;
		samp[1]=(sampoutl&0xff00)>>8;
		samp[2]=sampoutr&0xff;
		samp[3]=(sampoutr&0xff00)>>8;
		
		fwrite(samp,4,1,fpo);
		samp1l=samp2l;
		samp1r=samp2r;
	}
	fclose(fpo);
}

void decodedifferwav_lr(FILE* fpi, wavinfo* wav)
{
	FILE *fpo;
	unsigned char* head,* data;
	unsigned char samp[4];
	short samp1l=0,samp2l=0,samp1r=0,samp2r=0;
	int sampo=0;
	head=malloc(36);
	data=malloc(8);
	writeriffhead(head, 36+8+wav->datalen);
	writepcmfmt(head+12, wav->channel, wav->srate, wav->bits);
	writedatachunkhead(data, wav->channel, wav->bits, wav->samples);
	
	fpo=fopen("dclr.wav","wb");
	fwrite(head, 36, 1, fpo);
	fwrite(data, 8, 1, fpo);
	free(head);
	free(data);

	fread(samp, 4,1, fpi);
	samp1l=(samp[0]+((short)(samp[1])<<8));
	samp1r=(samp[2]+((short)(samp[3])<<8));

	fwrite(samp, 4,1,fpo);
	
	while(fread(samp, 4,1,fpi))
	{
		samp2l=(samp[0]+((short)(samp[1])<<8));
		samp2r=(samp[2]+((short)(samp[3])<<8));
		samp2l+=samp1l;
		samp2r+=samp1r;
		
		//sampo=2*samp2;

		samp[0]=samp2l&0xff;
		samp[1]=(samp2l&0xff00)>>8;
		samp[2]=samp2r&0xff;
		samp[3]=(samp2r&0xff00)>>8;

		fwrite(samp,4,1,fpo);
		samp1l=samp2l;
		samp1r=samp2r;
	}
	fclose(fpo);
}

void cutwavlr(FILE* fpi, FILE* fpo, wavinfo* wav)
{
	unsigned char* head,* data;
	unsigned char samp[4];
	short samp1=0,samp2=0;
	if(!fpi || !fpo) 
		return;
	if(wav->channel!=2) 
		return;
	head=malloc(36);
	data=malloc(8);
	writeriffhead(head, 36+8+wav->datalen);
	writepcmfmt(head+12, wav->channel, wav->srate, wav->bits);
	writedatachunkhead(data, wav->channel, wav->bits, wav->samples);
	
	fwrite(head, 36, 1, fpo);
	fwrite(data, 8, 1, fpo);
	free(head);
	free(data);

	while(fread(samp, 4,1,fpi))
	{
		samp1=samp[0]+((short)samp[1]<<8);
		samp2=samp[2]+((short)samp[3]<<8);
		
		samp[0]=samp1&0xff;
		samp[1]=(samp1&0xff00)>>8;
		samp[2]=(~samp2)&0xff;
		samp[3]=((~samp2)&0xff00)>>8;
		fwrite(samp,4,1,fpo);
	}
	fclose(fpo);
}

void cutwavlr_mono(FILE* fpi, FILE* fpo, wavinfo* wav)
{
	unsigned char* head,* data;
	unsigned char samp[4];
	short samp1=0,samp2=0,sampo=0;
	if(!fpi || !fpo) 
		return;
	if(wav->channel!=2) 
		return;
	head=malloc(36);
	data=malloc(8);
	writeriffhead(head, 36+8+wav->datalen);
	writepcmfmt(head+12, wav->channel, wav->srate, wav->bits);
	writedatachunkhead(data, wav->channel, wav->bits, wav->samples);
	
	fwrite(head, 36, 1, fpo);
	fwrite(data, 8, 1, fpo);
	free(head);
	free(data);

	while(fread(samp, 4,1,fpi))
	{
		samp1=samp[0]+((short)samp[1]<<8);
		samp2=samp[2]+((short)samp[3]<<8);
		sampo=samp1/2+samp2/2;
		samp[0]=sampo&0xff;
		samp[1]=(sampo&0xff00)>>8;
		samp[2]=(~sampo)&0xff;
		samp[3]=((~sampo)&0xff00)>>8;
		fwrite(samp,4,1,fpo);
	}
	fclose(fpo);
}

