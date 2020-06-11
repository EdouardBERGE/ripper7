#include"library_v020.c"
#define __FILENAME__ "ripper7.c"

#define MAX_BUFFER_SIZE 256*1024*1024
#define HALF_BUFFER_SIZE MAX_BUFFER_SIZE/2

static int absolute_idx;
static int file_number=0;
static int memcard_id;
static char *global_buffer;

void SaveFile(char *extension,char *buffer,int filesize)
{
	#undef FUNC
	#define FUNC "SaveFile"
	char filename[PATH_MAX];
return;
	sprintf(filename,"RECOVER%08d.%s",file_number++,extension);
	FileRemoveIfExists(filename);
	FileWriteBinary(filename,buffer,filesize);
	FileWriteBinaryClose(filename);
	loginfo("[%s] saved",filename);
}

void SaveBigFile(char *extension, char *buffer, unsigned long long size)
{
	#undef FUNC
	#define FUNC "SaveBigFile"
	unsigned int translated_size;
	unsigned int remaining_size;
	unsigned int flushed_size;
	unsigned int cached_size;
	char filename[PATH_MAX];
	char *src,*dst;

	loginfo("SaveBigFile");
	
	cached_size=MAX_BUFFER_SIZE-absolute_idx;
	if (size>cached_size)
	{
		flushed_size=cached_size;
		remaining_size=size-cached_size;
	}
	else
	{
		flushed_size=size;
		remaining_size=0;
	}

	sprintf(filename,"RECOVER%08d.%s",file_number++,extension);
	FileRemoveIfExists(filename);

	FileWriteBinary(filename,buffer,flushed_size);
	if (remaining_size>0)
	{
		if (remaining_size>MAX_BUFFER_SIZE)
		{
			split_read(memcard_id,global_buffer,MAX_BUFFER_SIZE);
			FileWriteBinary(filename,global_buffer,MAX_BUFFER_SIZE);
			remaining_size-=MAX_BUFFER_SIZE;
		}
		else
		{
			split_read(memcard_id,global_buffer,remaining_size);
			FileWriteBinary(filename,global_buffer,remaining_size);
			remaining_size=0;
		}
		/* global_buffer is entirely flushed, get a new one */
		memset(global_buffer,0,MAX_BUFFER_SIZE);
		split_read(memcard_id,global_buffer,MAX_BUFFER_SIZE);
		absolute_idx=0;		
	}
	else
	{
		if (flushed_size<cached_size)
		{
			translated_size=cached_size-flushed_size;
			remaining_size=MAX_BUFFER_SIZE-translated_size;
			/* need to move the memory in a particular order to prevent from overwriting */
			dst=global_buffer;
			src=global_buffer+MAX_BUFFER_SIZE-translated_size;
			while (translated_size)
			{
				*dst=*src;
				dst++;
				src++;
				translated_size--;
			}
			/* add new datas to fill up the buffer entirely */
			memset(global_buffer+cached_size-flushed_size,0,remaining_size);
			split_read(memcard_id,global_buffer+cached_size-flushed_size,remaining_size);
			absolute_idx=0;		
		}
		else
		if (flushed_size==cached_size)
		{
			memset(global_buffer,0,MAX_BUFFER_SIZE);
			split_read(memcard_id,global_buffer,MAX_BUFFER_SIZE);
			absolute_idx=0;		
		}
	}

	FileWriteBinaryClose(filename);
	loginfo("[%s] saved",filename);
}

char *AIFFFourCCList[]={"COMM","FORM","INST","MARK","SKIP","SSND",NULL};
int ExtractAIFF(unsigned char *buffer)
{
	#undef FUNC
	#define FUNC "ExtractAIFF"
	int offset,i;
	unsigned long long size;

	if (strncmp(buffer,"FORM",4)) return 0;
	if (strncmp(buffer+8,"AIFF",4)) return 0;
	size=(unsigned)GetEndianINT(buffer+4,GET_BIG_ENDIAN)+8;
	i=0;
	while (AIFFFourCCList[i] && strncmp(buffer+12,AIFFFourCCList[i],4))
		i++;
	if (AIFFFourCCList[i])
	{
		loginfo("Found AIFF file size=%d",size);
		if (size<HALF_BUFFER_SIZE)
		{
			SaveFile("AIFF",buffer,size);
			return size-1;
		}
		else
		{
			SaveBigFile("AIFF",buffer,size);
			return -1;
		}
	}
	return 0;
}

int ExtractAU(unsigned char *buffer)
{
	#undef FUNC
	#define FUNC "ExtractAU"
	int offset;

	if (strncmp(buffer,".snd",4)) return 0;
	if (GetEndianINT(buffer+4,GET_BIG_ENDIAN)<24) return 0;
	if ((unsigned)GetEndianINT(buffer+8,GET_BIG_ENDIAN)==0xFFFFFFFF) return 0; /* unspecified size, cannot continue */
	
	offset=24+(unsigned)GetEndianINT(buffer+8,GET_BIG_ENDIAN);
	loginfo("Found AU file size=%d",offset);
	return offset-1;
}


static char *fourCCList[]={"3IV1", "3IV2", "8BPS", "AASC", "ABYR", "ADV1", "ADVJ", "AEMI", "AFLC", "AFLI", "AJPG", "AMPG", "ANIM", "AP41", "ASLC", "ASV1", "ASV2", "ASVX", "AUR2", "AURA", "avc1", "AVI ", "AVRn", "BA81", "BINK", "BLZ0", "BT20", "BTCV", "BW10", "BYR1", "BYR2", "CC12", "CDVC", "CFCC", "CGDI", "CHAM", "CJPG", "CMYK", "CPLA", "CRAM", "CSCD", "CTRX", "CVID", "CWLT", "CXY1", "CXY2", "CYUV", "CYUY", "D261", "D263", "davc", "DCL1", "DCL2", "DCL3", "DCL4", "DCL5", "DIV3", "DIV4", "DIV5", "DIVX", "divx", "DM4V", "dmb1", "LEAD", "DMB2", "DMK2", "DSVD", "DUCK", "dv25", "dv50", "DVAN", "DVCS", "DVE2", "dvh1", "dvhd", "dvsd", "Impl", "DVSD", "dvsl", "DVX1", "DVX2", "DVX3", "DX50", "vers", "DXGM", "DXTC", "DXTn", "EKQ0", "ELK0", "EM2V", "ES07", "ESCP", "ETV1", "ETV2", "ETVC", "FFV1", "FLJP", "FMP4", "FMVC", "FPS1", "FRWA", "FRWD", "FVF1", "GEOX", "GJPG", "GLZW", "GPEG", "GWLT", "H260", "H261", "H262", "H263", "H264", "H265", "H266", "H267", "H268", "H269", "HDYC", "HFYU", "HMCR", "HMRR", "i263", "IAN", "ICLB", "IGOR", "IJPG", "ILVC", "ILVR", "IPDV", "IR21", "IRAW", "ISME", "IV30", "IV32", "IV40", "IV50", "JBYR", "jpeg", "JPEG", "JPGL", "LEAD", "KMVC", "L261", "L263", "LBYR", "LCMW", "LCW2", "LEAD", "LGRY", "LJ11", "LJ22", "LJ2K", "LJ44", "Ljpg", "LMP2", "LMP4", "LSVC", "LSVM", "LSVX", "LZO1", "M261", "M263", "M4CC", "m4cc", "M4S2", "MC12", "MCAM", "MJ2C", "mJPG", "MJPG", "http", "http", "Pega", "Morg", "MMES", "MP2A", "MP2T", "MP2V", "MP42", "MP43", "MP4A", "MP4S", "MP4T", "mp4v", "MP4V", "MPEG", "MPG4", "MPGI", "MR16", "MRCA", "MRLE", "MSVC", "MSZH", "MTX1", "MTX2", "MTX3", "MTX4", "MTX5", "MTX6", "MTX7", "MTX8", "MTX9", "MVI1", "MVI2", "MWV1", "nAVI", "NDSC", "ndsm", "ndsp", "NDSP", "ndss", "NDSS", "NDXC", "NDXH", "NDXP", "NDXS", "NHVU", "NTN1", "NTN2", "NVDS", "NVHS", "NVS0", "NVS5", "NVT0", "NVT5", "PDVC", "PGVV", "PHMO", "PIM1", "PIM2", "pimj", "PIXL", "PJPG", "PVEZ", "PVMM", "PVW2", "QPEG", "qpeq", "raw ", "RGBT", "rle ", "RLE ", "RLE4", "RLE8", "RMP4", "RPZA", "RT21", "rv20", "rv30", "RV40", "RVX ", "s422", "YUV ", "SAN3", "SDCC", "SEDG", "SFMC", "SMC ", "SMP4", "SMSC", "SMSD", "smsv", "SP40", "SP44", "SP54", "SPIG", "SQZ2", "STVA", "STVB", "STVC", "STVX", "STVY", "SV10", "SVQ1", "SVQ3", "TLMS", "TLST", "TM20", "TM2X", "TMIC", "TMOT", "TR20", "TSCC", "TV10", "TVJP", "TVMJ", "TY0N", "TY2C", "TY2N", "UCOD", "ULTI", "v210", "V261", "V655", "VCR1", "VCR2", "VCR3", "VDCT", "VDOM", "VDOW", "VDTZ", "VGPX", "VIDS", "VIFP", "VIVO", "VIXL", "VLV1", "VP30", "VP31", "VP40", "VP50", "VP60", "VP61", "VP62", "VQC1", "VQC2", "VQJC", "vssv", "VUUU", "VX1K", "VX2K", "VXSP", "VYU9", "VYUY", "WBVC", "WHAM", "WINX", "WJPG", "WMV1", "WMV2", "WMV3", "WMVA", "WNV1", "WVC1", "x263", "X264", "XLV0", "XMPG", "XVID", "xvid", "XWV0", "XWV9", "XXAN", "Y16 ", "Y411", "Y41P", "Y444", "Y8  ", "YC12", "YUV8", "YUV9", "YUVP", "YUY2", "YUYV", "YV12", "YV16", "YV92", "ZLIB", "ZMBV", "ZPEG", "ZyGo", "ZYYY",NULL};


int ExtractAVI(unsigned char *buffer)
{
	#undef FUNC
	#define FUNC "ExtractAVI"
	int offset=0,i;
	unsigned long long size;
	unsigned long long fourcc;

	if (strncmp(buffer,"RIFF",4)) return 0;

	size=(unsigned long long)GetEndianINT(buffer+4,GET_LITTLE_ENDIAN)+8;
	
	i=0;
	while (fourCCList[i] && strncmp(fourCCList[i],buffer+8,4))
		i++;

	if (fourCCList[i])
	{
		loginfo("Found AVI file size=%d",size);
		if (size<HALF_BUFFER_SIZE)
		{
			SaveFile("AVI",buffer,size);
			return size-1;
		}
		else
		{
			SaveBigFile("AVI",buffer,size);
			return -1;
		}
	}
	
	return 0;
}


int GIFCalcColorTableSize(unsigned char zegctr)
{
	#undef FUNC
	#define FUNC "GIFCalcColorTableSize"
	int offset=0;

	if (zegctr & 128)
	{
		offset=2<<(zegctr&7);
		offset*=3;
	}
	return offset;
}
/*
	http://www.matthewflickinger.com/lab/whatsinagif/bits_and_bytes.asp
*/
int ExtractGIF(unsigned char *buffer)
{
	#undef FUNC
	#define FUNC "ExtractGIF"
	int offset;

	if (strncmp(buffer,"GIF87a",6) && strncmp(buffer,"GIF89a",6)) return 0;
	offset=13+GIFCalcColorTableSize(buffer[10]);
	while (offset<HALF_BUFFER_SIZE)
	{
		switch (buffer[offset])
		{
			case 0x2C:
				offset+=10+GIFCalcColorTableSize(buffer[offset+9]);
				if (buffer[offset]>11) return 0;
				offset++;
			
				while (buffer[offset]!=0 && offset<HALF_BUFFER_SIZE) /* sub-block terminator */
				{
					offset+=1+buffer[offset];
				}
				offset++;
				break;
			case 0x21:
				if (buffer[offset+1]==0xF9)
				{
					if (buffer[offset+7]!=0) return 0;
					offset+=8;
				}
				else
				if (buffer[offset+1]==0xFE || buffer[offset+1]==0x01 || buffer[offset+1]==0xFF)
				{
					offset+=2;
					while (buffer[offset]!=0 && offset<HALF_BUFFER_SIZE) /* sub-block terminator */
					{
						offset+=1+buffer[offset];
					}
					offset++;
				}
				else
					return 0;
				break;
			case 0x3B:
				loginfo("Found GIF size=%d",offset+1);
				SaveFile("GIF",buffer,offset+1);
				return offset;
			default:
				return 0;				
		}
	}

	return 0;
}

/* this can only extract MP3 with ID3v2+ header format, not the old ones before 1998 */
int ExtractMP3(unsigned char *buffer)
{
	#undef FUNC
	#define FUNC "ExtractMP3"
	int offset,i;
	/* http://www.id3.org/id3v2-00 */
	if (strncmp(buffer,"ID3",3)) return 0;
	if (buffer[3]<2) return 0; /* major version must be 2 or greater */
	if (buffer[5]>0x7F || buffer[6]>0x7F || buffer[7]>0x7F || buffer[8]>0x7F) return 0;
	offset=512; /* purely speculative */
	while (offset<HALF_BUFFER_SIZE)
	{
		if (strncmp(buffer+offset,"TAG",3)==0)
		{
			for (i=3;i<125;i++)
				if ((buffer[offset+i]<0x20 || buffer[offset+i]>0x7F) && buffer[offset+i]!=0)
					break;
			if (i==125)
			{
				loginfo("Found MP3 (%s/%s/%s) size=%d",buffer+offset+3,buffer+offset+33,buffer+offset+63,offset+128);
				return offset+127;
			}
		}
		offset++;
	}
	return 0;
}


int ExtractZIP(unsigned char *buffer)
{
	#undef FUNC
	#define FUNC "ExtractZIP"
	int offset,cd_offset;

	/* checking header */
	if (buffer[0]!='P' || buffer[1]!='K') return 0;
	if (buffer[2]!=3 && buffer[2]!=5 && buffer[2]!=7) return 0;
	if (buffer[2]==3 && buffer[3]!=4) return 0;
	if (buffer[2]==7 && buffer[3]!=8) return 0;

	offset=30;
	while (offset<HALF_BUFFER_SIZE)
	{
		if (buffer[offset]=='P' && buffer[offset+1]=='K' && buffer[offset+2]==5 && buffer[offset+3]==6)
		{
			cd_offset=GetEndianINT(buffer+offset+16,GET_LITTLE_ENDIAN);
			/* central directory is always before the ending header */
			if (cd_offset<offset)
			if (buffer[cd_offset]=='P' && buffer[cd_offset+1]=='K' && buffer[cd_offset+2]==1 && buffer[cd_offset+3]==2)
			{
				offset+=22+GetEndianSHORTINT(buffer+offset+20,GET_LITTLE_ENDIAN);
				loginfo("Found ZIP packed size=%d",offset);
				SaveFile("ZIP",buffer,offset);
				return offset-1;
			}
		}
		offset++;
	}
	return 0;
}

int ExtractFLI(unsigned char *buffer)
{
	#undef FUNC
	#define FUNC "ExtractFLI"
	int offset;

	if (buffer[4]!=0x11 || buffer[5]!=0xAF || buffer[14]!=0 && buffer[14]!=3 || buffer[15]!=0) return 0; /* header check */
	if (GetEndianSHORTINT(buffer+6,GET_LITTLE_ENDIAN)>4000) return 0; /* no more than 4000 frames */

	offset=GetEndianINT(buffer,GET_LITTLE_ENDIAN);
	if (offset>HALF_BUFFER_SIZE) return 0;
	loginfo("Found FLI video file size=%d",offset);
	SaveFile("FLI",buffer,offset);
	return offset-1-4;
}

int ExtractCRW(unsigned char *buffer)
{
	#undef FUNC
	#define FUNC "ExtractCRW"

	#define CRW_MAX_SIZE 18*1024*1024
	#define CRW_MIN_SIZE 1*1024*1024

	int crw_width,crw_height;
	int offset,tag_offset;
	int model_offset;
	int header_len;

	int flag_compressed=1;

	unsigned long long speculative_valuedatasize;
	int speculative_dircount;

	if (buffer[0]!='I' || buffer[1]!='I') return 0;
	if (strncmp(buffer+6,"HEAPCCDR",8)) return 0;
	if (GetEndianINT(buffer+2,GET_LITTLE_ENDIAN)!=26) return 0;
	header_len=26;

	/* as we don't know the CRW size, we will try every values in a reasonable range */
	offset=CRW_MIN_SIZE;
	while (offset<CRW_MAX_SIZE)
	{
		if (!strcmp(buffer+offset,"Canon") && !strncmp(buffer+offset+6,"Canon ",6))
			break;
		offset++;
		
	}
	if (offset==CRW_MAX_SIZE) return 0;
	model_offset=offset+6;

	/* the directory block structure must end after this internal TAG but we don't know where */
	while (offset<CRW_MAX_SIZE)
	{
		speculative_valuedatasize=GetEndianINT(buffer+offset,GET_LITTLE_ENDIAN);
		if (speculative_valuedatasize+header_len+2+10>offset) {offset++;continue;}
		speculative_dircount=GetEndianSHORTINT(buffer+speculative_valuedatasize+header_len,GET_LITTLE_ENDIAN);
		if (header_len+speculative_valuedatasize+2+speculative_dircount*10==offset)
		{
			loginfo("Found CRW (%s) size=%d",buffer+model_offset,offset+4);
			SaveFile("CRW",buffer,offset+4);
			return offset+4-1;
		}
		offset++;
	}
	logdebug("end of CRW not found...");
	return 0;
}

/* in loving memory of old DOS ripper free-software */
int ExtractPCX(unsigned char *buffer)
{
	#undef FUNC
	#define FUNC "ExtractPCX"
	int pcx_width,pcx_height;
	int rle,n,v;
	int offset;

	if (buffer[0]!=10) return 0;
	if (buffer[1]>5) return 0; /* version check */
	if (buffer[2]!=1) return 0; /* PCX supports only RLE encoding */
	if (buffer[3]!=1 && buffer[3]!=2 && buffer[3]!=4 && buffer[3]!=8) return 0; /* bits per plane */
	pcx_width=GetEndianSHORTINT(buffer+8,GET_LITTLE_ENDIAN)-GetEndianSHORTINT(buffer+4,GET_LITTLE_ENDIAN)+1; /* Xmax-Xmin+1 */
	pcx_height=GetEndianSHORTINT(buffer+10,GET_LITTLE_ENDIAN)-GetEndianSHORTINT(buffer+6,GET_LITTLE_ENDIAN)+1; /* Ymax-Ymin+1 */
	if (buffer[65]!=1 && buffer[65]!=3) return 0; /* nbplanes: 1 for palette mode and 3 for RGB mode */

	/* speculating with rle decoding */
	offset=128;rle=pcx_width*pcx_height*buffer[65];
	while (offset<HALF_BUFFER_SIZE && rle>0)
	{
		v=buffer[offset];
		if (v<192)
		{
			rle--;
			offset++;
		}
		else
		{
			rle-=v-192;
			offset+=2;
		}
	}
	if (rle!=0) return 0; /* must be exact! */

	/* if extended palette then check this */
	if (buffer[3]==8 && buffer[65]==1)
	{
		if (buffer[offset]!=12) return 0; /* palette TAG */
		offset+=769;
	}

	loginfo("Found PCX %dx%d size=%d",pcx_width,pcx_height,offset);
	SaveFile("PCX",buffer,offset);
	return offset-1;
}

int ExtractBMP(unsigned char *buffer)
{
	#undef FUNC
	#define FUNC "ExtractBMP"
	int bmp_width=0,bmp_height=0;
	int offset,datasize;

	if (buffer[0]!=0x42 || buffer[1]!=0x4D || buffer[6]!=0 || buffer[7]!=0 || buffer[8]!=0 || buffer[9]!=0) return 0; /* magic number */
	if (GetEndianINT(buffer+14,GET_LITTLE_ENDIAN)!=40) return 0; /* bitmap info header size be 40 */
	if (buffer[26]!=1 || buffer[27]!=0) return 0; /* number of planes, must be 1 */
	if (buffer[29]!=0 || buffer[28]!=1 && buffer[28]!=4 && buffer[28]!=8 && buffer[28]!=24 && buffer[28]!=32) return 0; /* bits per pixel */
	if (buffer[30]!=0 && buffer[30]!=1 && buffer[30]!=2 && buffer[30]!=3 && buffer[30]!=4 && buffer[30]!=5 || buffer[31]!=0 || buffer[32]!=0 || buffer[33]!=0) return 0; /* compression */

	bmp_width=GetEndianINT(buffer+18,GET_LITTLE_ENDIAN);	
	bmp_height=GetEndianINT(buffer+22,GET_LITTLE_ENDIAN);	
	offset=GetEndianINT(buffer+10,GET_LITTLE_ENDIAN);
	datasize=GetEndianINT(buffer+34,GET_LITTLE_ENDIAN);
	if (datasize==0) /* BMP is a crappy format, we hope to have a better chance with file size field */
	{
		offset=0;
		datasize=GetEndianINT(buffer+2,GET_LITTLE_ENDIAN);
		if (datasize==0)
			return 0;
	}

	/* crappy oversize check, just in case... */
	if (offset+datasize<40+bmp_width*bmp_height*5 && offset+datasize<HALF_BUFFER_SIZE)
	{
		loginfo("found BMP (%dx%d) size=%d",bmp_width,bmp_height,offset+datasize);
		SaveFile("BMP",buffer,offset+datasize);
		return offset+datasize-1;
	}
	return 0;
}

int ExtractJPG(unsigned char *buffer)
{
	#undef FUNC
	#define FUNC "ExtractJPG"
	int jpg_width=0,jpg_height=0;
	int offset;

	/* magic number of the JPG */
	if (buffer[0]!=0xFF || buffer[1]!=0xD8 || buffer[2]!=0xFF)
		return 0;
	offset=2;
	while (offset<HALF_BUFFER_SIZE)
	{
		logdebug("chunk %X%X offset=%d",buffer[offset],buffer[offset+1],offset);
		if (buffer[offset]!=0xFF)
		{
			logdebug("invalid JPG chunk");
			return 0; /* invalid JPG chunk */
		}
		if (buffer[offset+1]==0xC0 || buffer[offset+1]==0xC2 && jpg_width==0)
		{
			jpg_height=GetEndianSHORTINT(&buffer[offset+5],GET_BIG_ENDIAN);
			jpg_width=GetEndianSHORTINT(&buffer[offset+7],GET_BIG_ENDIAN);
		}
		/* ECS segment that we wont decode must be followed by a 0xFF + non-zero value */
		if (buffer[offset+1]==0xDA)
		{
			offset++;
			while (offset<HALF_BUFFER_SIZE && (buffer[offset]!=0xFF || buffer[offset+1]==0))
				offset++;
			logdebug("ECS segment end at offset %d",offset);
		}
		/* finally */
		if (buffer[offset+1]==0xD9)
		{
			offset+=2;
			loginfo("found JPG (%dx%d) size=%d",jpg_width,jpg_height,offset);
			SaveFile("JPG",buffer,offset);
			return offset-1;
		}
		offset+=2+GetEndianSHORTINT(&buffer[offset+2],GET_BIG_ENDIAN);
	}
	return 0;
}

/* https://ccrma.stanford.edu/courses/422/projects/WaveFormat/ */

struct s_wav_header
{
	char ChunkID[4];
	int ChunkSize;
	char Format[4];

	char Subchunk1ID[4];
	int Subchunk1Size;
	short int AudioFormat;
	short int NumChannels;
	int SampleRate;
	int ByteRate;
	short int BlockAlign;
	short int BitsPerSample;

	char Subchunk2ID[4];
	int Subchunk2Size;

	/* data */
};

int ExtractWav(char *buffer)
{
	#undef FUNC
	#define FUNC "ExtractWav"
	struct s_wav_header *wav_header;
	int wav_size,alternate_wav_size;
	int wav_counter=0;
	char wav_output_name[1024];

	if (strncmp(buffer,"RIFF",4))
		return 0;
		
	wav_header=(struct s_wav_header *)buffer;

	/* doing some checks before writing... */

	if (strncmp(wav_header->Format,"WAVE",4)) return 0;
	if (strncmp(wav_header->Subchunk1ID,"fmt ",4)) return 0;
	if (strncmp(wav_header->Subchunk2ID,"data",4)) return 0;

	if (wav_header->NumChannels!=1 && wav_header->NumChannels!=2) return 0;
	if (wav_header->SampleRate>96000 || wav_header->SampleRate<4000) return 0;
	if (wav_header->BitsPerSample!=8 && wav_header->BitsPerSample!=16 && wav_header->BitsPerSample!=24) return 0;

	wav_size=wav_header->ChunkSize+8;
	alternate_wav_size=36+wav_header->Subchunk2Size+8;
	if (wav_size!=alternate_wav_size) return 0;
	alternate_wav_size=4+8+wav_header->Subchunk1Size+8+wav_header->Subchunk2Size+8;
	if (wav_size!=alternate_wav_size) return 0;
	
	loginfo("found WAV file PCM(%d/%d/%s) size=%d",wav_header->SampleRate,wav_header->BitsPerSample,wav_header->NumChannels==1?"mono":"stereo",wav_size);
	if (wav_size>=HALF_BUFFER_SIZE)
	{
		SaveBigFile("WAV",buffer,wav_size);
		return -1;
	}
	/*
	FileWriteBinary(wav_output_name,buffer,wav_size);
	FileWriteBinaryClose(wav_output_name);
	*/
	SaveFile("WAV",buffer,wav_size);
	return wav_size-1;
}


/***

	Try to guess if the data is a TIFF file

	TIFF format can be in little-endian or big-endian mode
	so we have to deal with this (GetEndianINT, GetEndianSHORTINT, ...)

	specs: http://partners.adobe.com/public/developer/en/tiff/TIFF6.pdf

	extended TIFF/EP SubIFD
	http://www.awaresystems.be/imaging/tiff/tifftags/subifds.html

*/
int ExtractTIFF(unsigned char *buffer)
{
	#undef FUNC
	#define FUNC "ExtractTIFF"

	int i,idx,c,zemode;
	int zeoffset,maxoffset;
	int dng_flag=0;
	int nb_ifd;
	int ifd_field_tag,ifd_field_type,ifd_field_values,ifd_field_offset;
	int jpg_width=0,jpg_height=0;
	char *tag_type;

	int tiff_bits_per_sample=-1;
	int tiff_image_width=-1;
	int tiff_image_height=-1;
	int tiff_sample_per_pixel=-1;
	int tiff_data_size=0;

	int tiff_jpeg_offset;
	int tiff_jpeg_size;
	int tiff_strip_offset;
	int tiff_strip_size;
	int strip_offset_tab;
	int strip_offset_size;
	int strip_size_tab;
	int strip_size_size;
	int tab_idx;
	int offset;

	int tiff_max_offset=0;
	char *tiff_model_name;
	char *tiff_extension;

	/* TIFF/EP extension */
	int subifd_tab=0,subifd_count=0;

	idx=1;
	c=buffer[0];
	if (c==0x49 && buffer[idx]==0x49 && buffer[idx+1]==0x2A && buffer[idx+2]==0x00)
	{
		zemode=GET_LITTLE_ENDIAN;
	}
	else
	if (c==0x4D && buffer[idx]==0x4D && buffer[idx+1]==0x00 && buffer[idx+2]==0x2A)
	{
		zemode=GET_BIG_ENDIAN;
	}
	else
		return 0;
	/* at this point, we have a little chance to get a real TIFF file */
	logdebug("maybe a TIFF file in %s-endian mode",zemode==GET_LITTLE_ENDIAN?"little":"big");
	/* get offset of the first supposed Image File Directory (IFD) 
	   this offset may be at the end of the file so we have to
	   guess if the value is correct or not */
	zeoffset=GetEndianINT(&buffer[idx+3],zemode);
	/* limit TIFF size to 128Mbytes */
	if (zeoffset>7 && zeoffset<HALF_BUFFER_SIZE)
	{
		while (zeoffset || subifd_count!=0)
		{
			/* TIFF/EP extension, we must follow SubIFD */
			if (zeoffset==0)
			{
				zeoffset=GetEndianINT(&buffer[subifd_tab],zemode);
				subifd_tab+=4;
				subifd_count--;
				if (zeoffset>HALF_BUFFER_SIZE)
					return 0;
			}

			nb_ifd=GetEndianSHORTINT(&buffer[zeoffset],zemode);
			logdebug("found %d entries. Checking...",nb_ifd);

			tiff_strip_offset=0;
			tiff_strip_size=0;
			strip_offset_tab=0;
			strip_size_tab=0;
			tiff_jpeg_size=0;
			tiff_jpeg_offset=0;

			for (idx=zeoffset+2;idx<zeoffset+2+nb_ifd*12;idx+=12)
			{
				ifd_field_tag=GetEndianSHORTINT(&buffer[idx],zemode);
				ifd_field_type=GetEndianSHORTINT(&buffer[idx+2],zemode);
				ifd_field_values=GetEndianINT(&buffer[idx+4],zemode);
				ifd_field_offset=GetEndianINT(&buffer[idx+8],zemode);
				tag_type=NULL;
				switch (ifd_field_type)
				{
					case 1:tag_type="byte";break;
					case 2:tag_type="ascii";break;
					case 3:tag_type="short";break;
					case 4:tag_type="long";break;
					case 5:tag_type="rational";break;
					/* TIFF 6.0 specs */
					case 6:tag_type="sbyte";break;
					case 7:tag_type="undefined";break;
					case 8:tag_type="sshort";break;
					case 9:tag_type="slong";break;
					case 10:tag_type="srational";break;
					case 11:tag_type="float";break;
					case 12:tag_type="double";break;
					/* TIFF-EP specs */
					case 13:tag_type="SubIFD";break;
					default:logdebug("wrong datatype, this is not a valid TIFF file");return 0;
				}

				logdebug("tag:%X type:%s count:%d offset:%d",ifd_field_tag,tag_type,ifd_field_values,ifd_field_offset);
				if (strcmp(tag_type,"ascii")==0 && ifd_field_tag==0x110)
				{
					if (buffer[ifd_field_offset+ifd_field_values-1]==0)
					{
						logdebug("%s",&buffer[ifd_field_offset]);
						tiff_model_name=&buffer[ifd_field_offset];
					}
					else
					{
						logdebug("wrong ascii field, this is not a valid TIFF file");
						return 0;
					}
				}
				switch (ifd_field_tag)
				{
					case 0x14A:
						    subifd_count=ifd_field_values;
						    if (subifd_count>1)
							subifd_tab=ifd_field_offset;
						    else
							subifd_tab=idx+8; /* direct value */
						    logdebug("Main SubIFD found! count=%d",subifd_count);
						    break;
					case 0xC612:dng_flag=1;break;
					/* the four most interesting tags (offset/size) */
					case 0x201:
						if (ifd_field_type==4 && ifd_field_values==1)
							tiff_jpeg_offset=ifd_field_offset;
						else
						{
							logdebug("JPEGOffset must be LONG type! This is not a valid TIFF file");
							return 0;
						}
						break;
					case 0x202:
						if (ifd_field_type==4 && ifd_field_values==1)
							tiff_jpeg_size=ifd_field_offset;
						else
						{
							logdebug("JPEGSize must be LONG type! This is not a valid TIFF file");
							return 0;
						}
						break;
					case 0x111:
						if (ifd_field_type==4 || ifd_field_type==3)
						{
							if (ifd_field_values==1)
								tiff_strip_offset=ifd_field_offset;
							else
							{
								strip_offset_tab=ifd_field_offset;
								strip_offset_size=ifd_field_values;
							}
						}
						else
						{
							logdebug("StripOffset is neither SHORT nor LONG type! This is not a valid TIFF file");
							return 0;
						}
						break;
					case 0x117:
						if (ifd_field_type==4 || ifd_field_type==3)
						{
							if (ifd_field_values==1)
								tiff_strip_size=ifd_field_offset;
							else
							{
								strip_size_tab=ifd_field_offset;
								strip_size_size=ifd_field_values;
							}
						}
						else
						{
							logdebug("StripByteCount is neither SHORT nor LONG type! This is not a valid TIFF file");
							return 0;
						}
						break;
						

					case 0x100:tiff_image_width=ifd_field_offset;logdebug("width=%d",tiff_image_width);break;
					case 0x101:tiff_image_height=ifd_field_offset;logdebug("height=%d",tiff_image_height);break;
					case 0x115:tiff_sample_per_pixel=ifd_field_offset;break;
					case 0x102:
						if (ifd_field_type!=3)
						{
							logdebug("BitsPerSample tag is not SHORT type! This is not a valid TIFF file");
							return 0;
						}
						tiff_bits_per_sample=0;
						for (i=0;i<ifd_field_values;i++)
						{
							tiff_bits_per_sample+=GetEndianSHORTINT(&buffer[ifd_field_offset+i*2],zemode);
						}
						logdebug("bits per sample: %d",tiff_bits_per_sample);
						
				}
			}
			if (tiff_strip_offset && tiff_strip_size)
			{
				logdebug("unique offset/size for the strip");
				if (tiff_strip_size+tiff_strip_offset>tiff_max_offset)
					tiff_max_offset=tiff_strip_size+tiff_strip_offset;
			}
			else
			if (strip_offset_tab && strip_size_tab)
			{
				logdebug("We have an array for the strips");
				if (strip_offset_size!=strip_size_size)
				{
					logdebug("invalid strip array (count of tags 0x111 and 0x117 are different)");
					return 0;
				}
				for (tab_idx=0;tab_idx<strip_offset_size;tab_idx++)
				{
					tiff_strip_offset=GetEndianINT(buffer+strip_offset_tab+tab_idx*4,zemode);
					tiff_strip_size=GetEndianINT(buffer+strip_size_tab+tab_idx*4,zemode);
					if (tiff_strip_size+tiff_strip_offset>tiff_max_offset)
						tiff_max_offset=tiff_strip_size+tiff_strip_offset;
				}
			}
			/* Nikon, Pentax like this one... */
			if (tiff_jpeg_size+tiff_jpeg_offset>tiff_max_offset)
			{
				logdebug("Found a JPG (probably a thumbnail) in the TIFF size=%d",tiff_jpeg_size);
				tiff_max_offset=tiff_jpeg_size+tiff_jpeg_offset;
			}

			zeoffset=GetEndianINT(&buffer[idx],zemode);
			if (zeoffset!=0)
				logdebug("There is a new IFD following at offset %d...",zeoffset);
		}
		if (tiff_max_offset==0)
			return 0;

		tiff_extension="TIFF";
		if (strstr(tiff_model_name,"Canon")!=NULL)
			tiff_extension="CR2";
		else
		if (strstr(tiff_model_name,"NIKON")!=NULL)
			tiff_extension="NEF";
		else
		if (strstr(tiff_model_name,"PENTAX")!=NULL && !dng_flag)
			tiff_extension="PEF";
		else
		if (strstr(tiff_model_name,"PENTAX")!=NULL && dng_flag)
			tiff_extension="DNG";
		else
		if (strstr(tiff_model_name,"DSLR-A")!=NULL && !dng_flag)
			tiff_extension="ARW";
		else
		if (strstr(tiff_model_name,"DSC-R")!=NULL && !dng_flag)
			tiff_extension="SR2";

		/* Patch for DNG with RAW data saved as JPG-Block without width & height description */
		offset=tiff_max_offset;
		while ((buffer[offset]==0xFF && buffer[offset+1]==0xD8 && buffer[offset+2]==0xFF 
		|| buffer[offset+1]==0xFF && buffer[offset+2]==0xD8 && buffer[offset+3]==0xFF)
			&& offset<HALF_BUFFER_SIZE)
		{
			logdebug("extended JPG block check");
			if (buffer[offset]==0xFF)
				offset=tiff_max_offset+2;
			else
				offset=tiff_max_offset+3;

			while (offset<HALF_BUFFER_SIZE)
			{
				logdebug("chunk %X%X offset=%d",buffer[offset],buffer[offset+1],offset);
				if (buffer[offset]!=0xFF)
				{
					logdebug("invalid JPG chunk");
					break;
				}
				if (buffer[offset+1]==0xC0 || buffer[offset+1]==0xC2 && jpg_width==0)
				{
					jpg_height=GetEndianSHORTINT(&buffer[offset+5],GET_BIG_ENDIAN);
					jpg_width=GetEndianSHORTINT(&buffer[offset+7],GET_BIG_ENDIAN);
				}
				/* ECS segment that we wont decode must be followed by a 0xFF + non-zero value */
				if (buffer[offset+1]==0xDA)
				{
					offset++;
					while (offset<HALF_BUFFER_SIZE && (buffer[offset]!=0xFF || buffer[offset+1]==0))
						offset++;
					logdebug("ECS segment end at offset %d",offset);
				}
				/* finally */
				if (buffer[offset+1]==0xD9 && jpg_height==0 && jpg_width==0)
				{
					offset+=2;
					logdebug("found JPG block");
					tiff_max_offset=offset;
					break;
				}
				offset+=2+GetEndianSHORTINT(&buffer[offset+2],GET_BIG_ENDIAN);
			}
		}

		loginfo("%s file found %d bytes (%s)",tiff_extension,tiff_max_offset,tiff_model_name);
		SaveFile(tiff_extension,buffer,tiff_max_offset);
	}
	else
		return 0;

	/* ecriture du fichier */

	return tiff_max_offset-1;
}

int split_read(int zeid,char *buffer, int size)
{
	#define FRAG_BLOCK 16384
	int curbloc=0,rkb=0,rmb=0,rrr;
	
	while (curbloc<size)
	{
		if (rmb!=rkb/64)
		{
			rmb=rkb/1024;
			printf("reading %dMb     \r",rmb);fflush(stdout);
		}
		if (curbloc+FRAG_BLOCK<=size)
		{
			rrr=read(zeid,buffer+curbloc,FRAG_BLOCK);
			if (rrr!=FRAG_BLOCK)
				size=0;
		}
		else
		{
			rrr=read(zeid,buffer+curbloc,size-curbloc);
			if (rrr!=size-curbloc)
				size=0;
		}
		curbloc+=rrr;
		rkb+=16;
	}
	return curbloc;
}
void ExtractFiles(char *filename)
{
	#undef FUNC
	#define FUNC "ExtractFiles"

	char *ph="/-\\|";
	int w=0,p,op=-1;
	unsigned char *buffer;
	int maxidx;
	int page_size,endofcard;
	long long size,rsize;

	memcard_id=open64(filename,O_RDONLY,0);
	if (memcard_id==-1)
	{
		logerr("cannot open [%s]",filename);
		logerr("(%d) %s",errno,strerror(errno));
		if (errno==13)
		{
			logerr("You must be root to run this program!");
		}
		exit(ABORT_ERROR);
	}
	size=lseek(memcard_id,0,SEEK_END);
	lseek(memcard_id,0,SEEK_SET);
	if (size<1000*1000)
		loginfo("size=%ldkb",size/1024);
	else
	if (size<1000*1000*1000)
		loginfo("size=%ldMb",size/1000/1000);
	else
		loginfo("size=%ldGb",size/1000/1000/1000);

	/* we dont care about the real size of the file */
	buffer=global_buffer=MemCalloc(MAX_BUFFER_SIZE);
	/* first read, we fill the whole buffer */
	rsize=split_read(memcard_id,buffer,MAX_BUFFER_SIZE);
	endofcard=0;
	while (rsize || endofcard)
	{
		absolute_idx=0;
		while (absolute_idx<HALF_BUFFER_SIZE)
		{
			switch (buffer[absolute_idx])
			{
				case 0x0A:absolute_idx+=ExtractPCX(&buffer[absolute_idx]);break;
				case 0x11:if (absolute_idx>4) absolute_idx+=ExtractFLI(&buffer[absolute_idx-4]);break;
				case 0x2E:absolute_idx+=ExtractAU(&buffer[absolute_idx]);break;	/* '.' */
				case 0x42:absolute_idx+=ExtractBMP(&buffer[absolute_idx]);	/* 'B' */
				case 0x46:absolute_idx+=ExtractAIFF(&buffer[absolute_idx]);	/* 'F' */
				case 0x47:absolute_idx+=ExtractGIF(&buffer[absolute_idx]);	/* 'G' */
				case 0x49:absolute_idx+=ExtractTIFF(&buffer[absolute_idx]);	/* 'I' */
					  absolute_idx+=ExtractCRW(&buffer[absolute_idx]);
					  absolute_idx+=ExtractMP3(&buffer[absolute_idx]);
					  break;
				case 0x4D:absolute_idx+=ExtractTIFF(&buffer[absolute_idx]);break;	/* 'M' */
				case 0x50:absolute_idx+=ExtractZIP(&buffer[absolute_idx]);break;	/* 'P' */
				case 0x52:absolute_idx+=ExtractWav(&buffer[absolute_idx]);	/* 'R' */
					  absolute_idx+=ExtractAVI(&buffer[absolute_idx]);break;
				case 0xFF:absolute_idx+=ExtractJPG(&buffer[absolute_idx]);break;
			}
			absolute_idx++;
		}
		memcpy(buffer,buffer+HALF_BUFFER_SIZE,HALF_BUFFER_SIZE);
		memset(buffer+HALF_BUFFER_SIZE,0,HALF_BUFFER_SIZE);
		rsize=split_read(memcard_id,buffer+HALF_BUFFER_SIZE,HALF_BUFFER_SIZE);
		if (!rsize && !endofcard)
			endofcard++;
		else
		if (!rsize && endofcard)
			endofcard=0;
	}
	MemFree(buffer);
}

/***************************************
	semi-generic body of program
***************************************/

/*
	Usage
	display the mandatory parameters
*/
void Usage()
{
	#undef FUNC
	#define FUNC "Usage"
	
	printf("ripper7.exe v2.0 - extract multimedia files from aggregated file or memory card\n");
	printf("Image format detected: TIFF,JPG,CRW,CR2,NEF,PEF,ARW,SR2,DNG,PCX,BMP,GIF\n");
	printf("Audio format detected: MP3,WAV,AIFF,AU\n");
	printf("Video format detected: AVI,FLI\n");
	printf("creation: 2011-07-19 Edouard BERGE\n");
	printf("modification: 2011-07-23 Edouard BERGE\n");
	printf("usage:\n");
	printf("-h                      display help\n");
	printf("-d                      debug mode\n");
	printf("-i <input_file>         set the name of the input file\n");
	printf("                        or the location of your memory card\n");
	printf("\n");
	exit(ABORT_ERROR);
}

/*
	GetParametersFromCommandLine	
	retrieve parameters from command line and fill pointers to file names
*/
void GetParametersFromCommandLine(int argc, char **argv, char **fin)
{
	#undef FUNC
	#define FUNC "GetParametersFromCommandLine"
	int i;
	
	for (i=1;i<argc;i++)
	{
		if (argv[i][0]=='-')
		{
			switch(argv[i][1])
			{
				case 'D':
				case 'd':setdebug(1);break;
				case 'H':Usage();
				case 'h':Usage();
				case 'i':if (i+1<argc)
					{
						*fin=argv[i+1];
						i++;
						loginfo("input file: %s",*fin);
						break;
					}
					Usage();
				default:
					Usage();				
			}
		}
		else
			Usage();
	}
	/* no file specified in command line, ask for usb */
	if (!*fin)
	{
		logerr("You must specify a filename or the location of your memory card: ");
		Usage();
	}
}

/*
	main
	
	check parameters
	execute the main processing
*/
void main(int argc, char **argv)
{
	#undef FUNC
	#define FUNC "main"
	char *inputfile=NULL;
	GetParametersFromCommandLine(argc,argv,&inputfile);
	ExtractFiles(inputfile);
	CloseLibrary();
	exit(0);
}

