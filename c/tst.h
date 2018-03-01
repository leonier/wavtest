typedef struct wavinfo
{
		int srate;
		int channel;
		int bits;
		int datalen;
		int samples;
}wavinfo;

int parsewaveheader(unsigned char* wavhead, wavinfo* winfo);
int finddatachunk(FILE* file);
void writedatachunkhead(unsigned char* hdr, int wchannel, int wbits, int nsamples);
void writepcmfmt(unsigned char* hdr, int wchannel, int wsrate, int wbits);
void writeriffhead(unsigned char* hdr, int flength);

void makedifferwav_lr(FILE* fpi, wavinfo* wav);
void decodedifferwav_lr(FILE* fpi, wavinfo* wav);
void decodedifferwav_32(FILE* fpi, wavinfo* wav);
void makedifferwav_32(FILE* fpi, wavinfo* wav);
void decodedifferwav_byte(FILE* fpi, wavinfo* wav);
void makedifferwav_byte(FILE* fpi, wavinfo* wav);
void compresswavtohalf(FILE* fpi, wavinfo* wav);
void makehalfwav(FILE* fpi, wavinfo* wav);
void decodedifferwav(FILE* fpi, wavinfo* wav);
void makedifferwav(FILE* fpi, wavinfo* wav);

void cutwavlr(FILE* fpi, FILE* fpo, wavinfo* wav);
void cutwavlr_mono(FILE* fpi, FILE* fpo, wavinfo* wav);