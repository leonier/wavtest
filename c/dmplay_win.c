/*based on https://www.planet-source-code.com/vb/scripts/ShowCode.asp?txtCodeId=4422&lngWId=3 */

#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include "tst.h"
#include "dmcommon.h"
/*
* some good values for block size and count
*/
#define BLOCK_SIZE 8192
#define BLOCK_COUNT 20
/*
* function prototypes
*/
static void CALLBACK waveOutProc(HWAVEOUT, UINT, DWORD, DWORD, DWORD);
static WAVEHDR* allocateBlocks(int size, int count);
static void freeBlocks(WAVEHDR* blockArray);
static void writeAudio(HWAVEOUT hWaveOut, LPSTR data, int size);
/*
* module level variables
*/
static CRITICAL_SECTION waveCriticalSection;
static WAVEHDR* waveBlocks;
static volatile int waveFreeBlockCount;
static int waveCurrentBlock;

wavinfo* wi;
dmarg* dminfo;
int decodeDM(HANDLE hfile,  wavinfo* wav, dmarg* dminfo, unsigned char* buffer, int buffer_size);

int main(int argc, char* argv[])
{
	HWAVEOUT hWaveOut; /* device handle */
	HANDLE hFile;/* file handle */
	WAVEFORMATEX wfx; /* look this up in your documentation */
	unsigned char buffer[1024]; /* intermediate buffer for reading */
	int i;
	DWORD rbyte;
	/*
	* quick argument check
	*/
	if(argc != 2) {
		fprintf(stderr, "usage: %s <dm filename>)\n", argv[0]);
		ExitProcess(1);
	}
	/*
	* initialise the module variables
	*/
	waveBlocks = allocateBlocks(BLOCK_SIZE, BLOCK_COUNT);
	waveFreeBlockCount = BLOCK_COUNT;
	waveCurrentBlock= 0;
	InitializeCriticalSection(&waveCriticalSection);
	/*
	* try and open the file
	*/
	if((hFile = CreateFile(
	argv[1],
	GENERIC_READ,
	FILE_SHARE_READ,
	NULL,
	OPEN_EXISTING,
	0,
	NULL
	)) == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "%s: unable to open file '%s'\n", argv[0], argv[1]);
		ExitProcess(1);
	}

	//Read the dm Header infomation.
	dminfo=malloc(sizeof(dmarg));
	wi=malloc(sizeof(wavinfo));

	if(!ReadFile(hFile, dminfo, sizeof(dmarg), &rbyte, NULL))
	{
		printf("read dm file header error!\n");
		exit(0);
	}
	else
	{
		if(dminfo->magicnum!=MAGICNUM)
		{
			printf("not a dm file!\n");
			exit(0);
		}
		if(dminfo->delta<0x10000)
			printf("DM delta value %d\n",dminfo->delta);
		else
		{
			int deltal=dminfo->delta&0xffff;
			int deltar=(dminfo->delta-deltal)/0x10000;
			printf("DM delta value %d/%d\n",deltal,deltar);
		}
		if(dminfo->mode==MODE_TYPE1)
			printf("Channel Delta Mode\n");
		if(dminfo->mode==MODE_TYPE2)
			printf("L+R L-R Mode\n");
	}
	if(!ReadFile(hFile, wi, sizeof(wavinfo), &rbyte, NULL))
	{
		printf("read dm file header error!\n");
		exit(0);
	}
	else
	{
		printf("Sample rate %dHz, %d bits, %d channels\n",wi->srate,wi->bits,wi->channel);
		//printf("%d samples\n",wi->samples);
		printf("Length %d:%d\n",(wi->samples/wi->srate)/60,(wi->samples/wi->srate)%60);
	}
	/*
	* set up the WAVEFORMATEX structure.
	*/
	wfx.nSamplesPerSec = wi->srate; /* sample rate */
	wfx.wBitsPerSample = wi->bits; /* sample size */
	wfx.nChannels= wi->channel; /* channels*/
	wfx.cbSize = 0; /* size of _extra_ info */
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nBlockAlign = (wfx.wBitsPerSample * wfx.nChannels) >> 3;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
	/*
	* try to open the default wave device. WAVE_MAPPER is
	* a constant defined in mmsystem.h, it always points to the
	* default wave device on the system (some people have 2 or
	* more sound cards).
	*/
	if(waveOutOpen(
	&hWaveOut,
	WAVE_MAPPER,
	&wfx,
	(DWORD_PTR)waveOutProc,
	(DWORD_PTR)&waveFreeBlockCount,
	CALLBACK_FUNCTION
	) != MMSYSERR_NOERROR)
	{
		fprintf(stderr, "%s: unable to open wave mapper device\n", argv[0]);
		ExitProcess(1);
	}
	/*
	* playback loop
	*/
	printf("Started playing, press CTRL-C to exit!\n");
	while(1) {
		int i;
		short l,r;
		DWORD readBytes;
		//if(!ReadFile(hFile, buffer, sizeof(buffer), &readBytes, NULL))

		//memset(buffer,0,sizeof(buffer));
		if(!decodeDM(hFile, wi, dminfo, buffer, sizeof(buffer)))
			break;

		/*
		if(readBytes == 0)

		break;

		if(readBytes < sizeof(buffer)) {

		printf("at end of buffer\n");
		memset(buffer + readBytes, 0, sizeof(buffer) - readBytes);
		printf("after memcpy\n");

		}*/
		writeAudio(hWaveOut, buffer, sizeof(buffer));
	}
	/*
	* wait for all blocks to complete
	*/
	while(waveFreeBlockCount < BLOCK_COUNT)
	Sleep(10);

	/*
	* unprepare any blocks that are still prepared
	*/
	for(i = 0; i < waveFreeBlockCount; i++)
		if(waveBlocks[i].dwFlags &WHDR_PREPARED)
			waveOutUnprepareHeader(hWaveOut,&waveBlocks[i], sizeof(WAVEHDR));
	DeleteCriticalSection(&waveCriticalSection);
	freeBlocks(waveBlocks);
	waveOutClose(hWaveOut);
	CloseHandle(hFile);
	return 0;
}


void writeAudio(HWAVEOUT hWaveOut,LPSTR data, int size)
{
	WAVEHDR* current;
	int remain;
	current = &waveBlocks[waveCurrentBlock];
	while(size > 0) {
		/*
		* first make sure the header we're going to use is unprepared
		*/
		if(current->dwFlags & WHDR_PREPARED)
			waveOutUnprepareHeader(hWaveOut,current, sizeof(WAVEHDR));

		if(size < (int)(BLOCK_SIZE-current->dwUser)) {
			memcpy((current->lpData) +(current->dwUser), data, size);
			current->dwUser += size;
			break;
		}
		remain = BLOCK_SIZE - current->dwUser;
		memcpy(current->lpData + current->dwUser, data, remain);
		size -= remain;
		data += remain;
		current->dwBufferLength = BLOCK_SIZE;
		waveOutPrepareHeader(hWaveOut, current, sizeof(WAVEHDR));
		waveOutWrite(hWaveOut, current, sizeof(WAVEHDR));
		EnterCriticalSection(&waveCriticalSection);
		waveFreeBlockCount--;
		LeaveCriticalSection(&waveCriticalSection);
		/*
		* wait for a block to become free
		*/
		while(!waveFreeBlockCount)
			Sleep(10);
		/*
		* point to the next block
		*/
		waveCurrentBlock++;
		waveCurrentBlock %= BLOCK_COUNT;
		current = &waveBlocks[waveCurrentBlock];
		current->dwUser = 0;
	}
}


WAVEHDR* allocateBlocks(int size, int count)
{
	unsigned char* buffer;
	int i;
	WAVEHDR* blocks;
	DWORD totalBufferSize = (size + sizeof(WAVEHDR)) * count;
	/*
	* allocate memory for the entire set in one go
	*/
	if((buffer = HeapAlloc(
		GetProcessHeap(),
		HEAP_ZERO_MEMORY,
		totalBufferSize
		)) == NULL)

	{
		fprintf(stderr, "Memory allocation error\n");
		ExitProcess(1);
	}
	/*
	* and set up the pointers to each bit
	*/
	blocks = (WAVEHDR*)buffer;
	buffer += sizeof(WAVEHDR) * count;
	for(i = 0; i < count; i++) {
		blocks[i].dwBufferLength = size;
		blocks[i].lpData = buffer;
		buffer += size;
	}
	return blocks;
}
void freeBlocks(WAVEHDR* blockArray)
{
	/*
	* and this is why allocateBlocks works the way it does
	*/
	HeapFree(GetProcessHeap(), 0, blockArray);
}


static void CALLBACK waveOutProc(
	HWAVEOUT hWaveOut,
	UINT uMsg,
	DWORD dwInstance,
	DWORD dwParam1,
	DWORD dwParam2
)
{
	/*
	* pointer to free block counter
	*/
	int* freeBlockCounter = (int*)dwInstance;
	/*
	* ignore calls that occur due to opening and closing the
	* device.
	*/
	if(uMsg != WOM_DONE)
		return;
	EnterCriticalSection(&waveCriticalSection);
	(*freeBlockCounter)++;
	LeaveCriticalSection(&waveCriticalSection);
}

//read and decode DM stream.
int decodeDM(HANDLE hFile,  wavinfo* wav, dmarg* dminfo, unsigned char* buffer, int buffer_size)
{
	unsigned int* fullbuf; 
	int i,rc,pos;
	int rsamples;
	
	int wcount=0;
	unsigned int wbuf=0;
	unsigned int bmask=0x80000000;
	
	static int samp1l=0,samp1r=0;
	int sampl=0,sampr=0;
	int valuel=0,valuer=0;
	
	rsamples=(8*buffer_size)/((wav->bits)*(wav->channel));
//	printf("buffer_size %d, load  %d samples\n",buffer_size, rsamples);	
	switch(dminfo->mode)
	{
	int deltal, deltar;
	case MODE_TYPE0:
		if(dminfo->delta<65536)
		{
			deltal=deltar=dminfo->delta;
		}
		else
		{
			deltal=dminfo->delta&0xffff;
			deltar=(dminfo->delta-deltal)/0x10000;
		}
		for(i=0; i<rsamples; i++)
		{
			if(!wcount)
			{
				if(!ReadFile(hFile, &wbuf, sizeof(unsigned int), &rc, NULL))
					return 0;		
				if(!rc)
					return 0;
			}
				
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
		

			sampl=dmp1bit(sampl,samp1l,deltal);
			sampr=dmp1bit(sampr,samp1r,deltar);
	//		printf("%d %d %d\n",wcount,sampl,sampr);	
			
			pos=i*((wav->bits*wav->channel)/8);
			buffer[pos]=sampl&0xff;	
			buffer[pos+1]=(sampl&0xff00)>>8;
			buffer[pos+2]=sampr&0xff;
			buffer[pos+3]=(sampr&0xff00)>>8;

			samp1l=sampl;
			samp1r=sampr;
		}
		break;
	case MODE_TYPE1:
		for(i=0; i<rsamples*2; i++)
		{
			if(!wcount)
			{
				if(!ReadFile(hFile, &wbuf, sizeof(unsigned int), &rc, NULL))
					return 0;		
				if(!rc)
					return 0;
			}
		
			sampl=(wbuf&bmask)?1:0;
			bmask=bmask>>1;

			wcount++;
			if(wcount>31)
			{
				wcount=0;
				bmask=0x80000000;
			}
		
			sampl=dmp1bit(sampl,samp1l,dminfo->delta);
			
			pos=i*((wav->bits)/8);
			buffer[pos]=sampl&0xff;	
			buffer[pos+1]=(sampl&0xff00)>>8;

			samp1l=sampl;
		}
		break;	
	case MODE_TYPE2:
		for(i=0; i<rsamples; i++)
		{
			if(!wcount)
			{
				if(!ReadFile(hFile, &wbuf, sizeof(unsigned int), &rc, NULL))
					return 0;		
				if(!rc)
					return 0;
			}

		
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
			
			pos=i*((wav->bits*wav->channel)/8);
			*(buffer+pos)=valuel&0xff;	
			*(buffer+pos+1)=(valuel&0xff00)>>8;
			*(buffer+pos+2)=valuer&0xff;
			*(buffer+pos+3)=(valuer&0xff00)>>8;

			samp1l=sampl;
			samp1r=sampr;
		}
		break;
	}
	return 1;
}