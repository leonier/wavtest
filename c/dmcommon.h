#define DELTA 4096
#define QVALUE 16384
#define MAGICNUM 0x6d645f6c //"l_dm"
#define MODE_TYPE0 0x100
#define MODE_TYPE1 0x101
#define MODE_TYPE2 0x102

typedef struct dmarg{
	int magicnum;
	int delta;
	int mode;
}dmarg;
short dmp1bit(int e, int f, short delta);

short quant1bit(short f0, short f1);
