/***********************************************************
	         mini-library 2011-07
	         Edouard  BERGE v0.20

      !!! DO NOT INCLUDE THIS FILE, USE library.h !!!	

changes v020:
- Now we may disable the memory wrapper (for performance)
- Linux include bugfix
- Bugfixed GetEndianMode

changes v019:
- TxtRegMatch, TxtGlobMatch
- DirMatchEntry

changes v018:
- stricmp when not supported by architecture
- KeyGetString, KeyGetChoice
- TxtCleanSpecialChar, TxtDeleteSpecialChar, TxtReplaceSpecialChar

changes v017:
- Some bugfixes with va_list usage (Linux)
- Bugfix with programs starting with a Realloc of a NULL pointer and no debug mode
- Better flushing method in _internal_put2log

changes v016:
- Linux compliant
- 64bits compliant
- SystemInfo to display some details about running architecture
- GetAutoEndian to know if the current architecture is big-endian or little-endian
- GetEndianSHORTINT, GetEndianINT, GetEndianLONGLONG to retrieve values regardless of endianness mode
- PutEndianSHORTINT, PutEndianINT, PutEndianLONGLONG to put values in memory regardless of endianness mode
- TxtIsFloat, TxtIsDouble, TxtIsLongDouble
- Some bugfixes in _internal_TxtIsFloat (used for float, double and long double check)

changes v015:
- MemTrace, MemUntrace may use variable label
- Secured writes to text buffer in put2file, ExecuteSysCmd, MemTrace
- MemDump
- TxtGetSubstr
- TxtIsDouble

changes v014:
- Bugfixed FileGetStat error processing
- Harmonisation of errno management
- CloseLibrary checks for opened monitors
- MemTrace, MemUntrace may use label
- _internal_put2log flush when logtype change (to avoid conflicts when both stderr and stdout are redirected together)

changes v013:
- MemTrace, MemUntrace
- Check the memory when freeing memory

changes v012:
- TxtStrReplace is clever with zero size strings
- ChangeDir, GetDir
- ExecuteSysCmdGetLimit
- ExecuteSysCmdBypassError
- ExecuteSysCmd, ExecuteSysCmdBypassError now compute the maximum command line limit
- All filesystem related functions now check for PATH_MAX before processing

changes v011:
- FileChmod
- MakeDirWithRights, MakeDir
- ExecuteSysCmd system limit check (lower for rex compatibility)
- Trim functions now can handle tabs
- ParameterParseFieldList now can handle reversed interval

changes v010:
- Bugfixed MemFree (memory was not freed in the v007, v008, v009)
- Bugfixed Memory monitor leak
- FileRemoveIfExists
- DirRemoveIfExists
- General clean up, comments, internal prototypes harmonisation

changes v009:
- Bugfixed memory leak in CheckDateFormat
- Bugfixed CSVGetAllFields when separator was the very last char of a string (may occur when trimming strings)
- XMLLookForFieldByName, XMLLookForFieldByValue
- FieldArrayAddDynamicValue
- ExecuteSysCmd
- DirRemove now can remove non empty directories recursively

changes v008:
- MemRealloc is more flexible (may realloc to zero size, as a free)
- XML library (parsing, management, file operation)
- ConvertDate (convert any date format to any other)
- Leap year fix in CheckDateFormat
- TxtIsInteger

changes v007:
- Automatic memory check (full in debug_mode, light otherwise)
- TxtIsInvisible function
- TxtTrimEnd, TxtTrimStart, TxtTrimSpace, TxtTrim functions
- Bugfixed LookForFile (memory of callers is now preserved)

changes v006:
- Automatic memory monitor
- CloseLibrary function (check of unfreed memory, unfreed system pointers)
- Bugfixed memory leaks in FileClose and DirClose

changes v005:
- Datetime functions
- Directory reading functions
- File stat function
- Log on file function
- XML specific read function
	
changes v004:
- CSV field parsing functions
- TxtStrDup function
	
changes v003:
- Log functions can handle multiple arguments (very useful)
- Library functions do not use buffers for logs anymore...
- INTERNAL_ERROR is now 7 instead of 1 like ABORT_ERROR
- default DEBUG_MODE to 0
	
changes v002:
- ParameterParseFieldList now can handle interval
- File functions now can handle many file simultaneously

initial release v001:
- Log functions
- Memory management functions
- Text functions (substr, replace)
- File read&write functions
- Parameters array functions

************************************************************/

#define __FILENAME__ "library_v020.c"

#define LIBRARY_NO_MEMORY_WRAPPER 0

#ifndef DEBUG_MODE
static int DEBUG_MODE=0;
#endif

/* default is an AIX host */
#define OS_AIX 1
#ifdef OS_LINUX
#undef OS_AIX
#endif

/* includes for all OS */
#include<string.h>
#include<stdlib.h>
#include<stdarg.h>
#include<dirent.h>
#include<stdio.h>
#include<errno.h>
#include<ctype.h>
#include<time.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>
#include<fcntl.h>
#include<regex.h>

/* OS specific includes */
#ifdef OS_AIX
#include<curses.h>
#endif

#ifdef OS_LINUX
#include<wait.h>
#include<signal.h>
#include<limits.h>
#define ARG_MAX 131072
#endif

extern char **environ;

#define INTERNAL_ERROR 7
#define ABORT_ERROR 1

#define MAX_LINE_BUFFER 16384

#define REG_MATCH             0

#define GET_ENDIAN_UNDEFINED -1
#define GET_LITTLE_ENDIAN     0
#define GET_BIG_ENDIAN        1
#define GET_AUTO_ENDIAN       2

#define PUT_ENDIAN_UNDEFINED  GET_ENDIAN_UNDEFINED
#define PUT_LITTLE_ENDIAN     GET_LITTLE_ENDIAN
#define PUT_BIG_ENDIAN        GET_BIG_ENDIAN
#define PUT_AUTO_ENDIAN       GET_AUTO_ENDIAN


/* statics */

static int _static_library_endian_mode=GET_ENDIAN_UNDEFINED;

static int _static_library_memory_used=0;
static int _static_library_memory_used_max=0;
static int _static_library_nbfile_opened=0;
static int _static_library_nbfile_opened_max=0;
static int _static_library_nbdir_opened=0;
static int _static_library_nbdir_opened_max=0;

static int _static_library_memory_monitor_top=0;
static int _static_library_memory_monitor_max=0;
static int _static_library_memory_operation=0;
static int _static_library_memory_aggregate=0;

static char _static_library_txtcleanconvtable[256];

/***
	LOG functions
	
	output:
	- the type of log (info, warning, error)
	- the source filename
	- the line of the log
	- the C function
	- the error message
*/

#define FUNC ""

/***
	_internal_put2log
	
	internal use, do not call this function
*/
enum e_logtype {
LOG_START,
LOG_STDOUT,
LOG_STDERR
};

#ifndef stricmp
/***
	stricmp
	
	make an strcmp comparison case insensitive
*/
int stricmp(char *str1,char *str2)
{
	int idx;
	int c1,c2;
	
	if (!str1) return (!str2)?0:-(*str2);
	if (!str2) return *str1;

	while ((c1=tolower(*str1))==(c2=tolower(*str2)) && *str1)
	{
		str1++;
		str2++;
	}
	
	return c1-c2;
}

#endif

void _internal_put2log(char *logtype, char *cursource, int curline, char *curfunc, ...)
{
	#undef FUNC
	#define FUNC "_internal_put2log"
	
	static int last_std=LOG_START;
	int cur_std;
	char *format;
	va_list argptr;

	va_start(argptr,curfunc);
	format=va_arg(argptr,char *);

	if (strcmp(logtype,"INFO ")==0 || strcmp(logtype,"WARN ")==0 || strcmp(logtype,"MONITOR ")==0 || strcmp(logtype,"DEBUG ")==0)
		cur_std=LOG_STDOUT;
	else
		cur_std=LOG_STDERR;
		
	/* when logtype changes from previous value, we flush the previous output buffer */
	if (cur_std!=last_std)
	{
		switch (last_std)
		{
			case LOG_STDOUT:fflush(stdout);break;
			case LOG_START:fflush(stderr);break;
			default:break;
		}
		last_std=cur_std;
	}	

	if (strcmp(logtype,"DEBUG ")==0)
	{
		if (DEBUG_MODE)
		{
			fprintf(stdout,"%s (%s) L%d - %s - ",logtype,cursource,curline,curfunc);
			vfprintf(stdout,format,argptr);
			fprintf(stdout,"\n");
		}
	}
	else
	if (strcmp(logtype,"INFO ")==0 || strcmp(logtype,"WARN ")==0 || strcmp(logtype,"MONITOR ")==0)
	{
		fprintf(stdout,"%s (%s) L%d - %s - ",logtype,cursource,curline,curfunc);
		vfprintf(stdout,format,argptr);
		fprintf(stdout,"\n");
	}
	else
	{
		fprintf(stderr,"%s (%s) L%d - %s - ",logtype,cursource,curline,curfunc);
		vfprintf(stderr,format,argptr);
		fprintf(stderr,"\n");
	}
	
	va_end(argptr);
}

/* prototypes needed for _internal_put2file */
void FileWriteLine(char *filename, char *buffer);
void *_MemRealloc(void *ptr, int size, char *cursource,int curline, char *curfunc);
void _MemFree(void *ptr, char *cursource,int curline, char *curfunc);
#define MemRealloc(ptr,size) _MemRealloc(ptr,size,__FILENAME__,__LINE__,FUNC)
#define MemFree(ptr) _MemFree(ptr,__FILENAME__,__LINE__,FUNC)
/***
	logfile
	
	log into a file
	internal use, do not call this function
	
	as we must use a temporary buffer before writing to a file
	we must compute the final string size to be sure that we
	have enough memory for it
*/
void _internal_put2file(char *ferrname, char *format, ...)
{
	#undef FUNC
	#define FUNC "logfile"
	
	static char *errbuffer=NULL;
	static int errbuffer_size=0;
	int zelen;
	va_list argptr;
	
	if (format==NULL)
	{
		FileWriteLine(ferrname,NULL);
		errbuffer_size=0;
		/* conditionnal algo may close file without having opened */
		if (errbuffer)
		{
			MemFree(errbuffer);
			errbuffer=NULL;
		}
	}
	else
	{
		/* check that our temporary buffer is big enough for string plus carriage return */
		va_start(argptr,format);
		zelen=vsnprintf(NULL,0,format,argptr)+3;
		/* we reallocate for a bigger size only */
		if (zelen>errbuffer_size)
			errbuffer=MemRealloc(errbuffer,zelen);
		/* reinit the va_list */
		va_end(argptr);
		va_start(argptr,format);
		vsnprintf(errbuffer,zelen,format,argptr);
		va_end(argptr);
		strcat(errbuffer,"\n");
		FileWriteLine(ferrname,errbuffer);
	}
}

/*** LOG macros send useful informations to the put2log function */
#define loginfo(...) _internal_put2log("INFO ",__FILENAME__,__LINE__,FUNC,__VA_ARGS__)
#define logdebug(...) _internal_put2log("DEBUG ",__FILENAME__,__LINE__,FUNC,__VA_ARGS__)
#define logwarn(...) _internal_put2log("WARN ",__FILENAME__,__LINE__,FUNC,__VA_ARGS__)
#define logerr(...) _internal_put2log("ERROR ",__FILENAME__,__LINE__,FUNC,__VA_ARGS__)
#define logfile(...) _internal_put2file(__VA_ARGS__)
#define logfileclose(ferrname) _internal_put2file(ferrname,NULL)
#define _internal_put2fileclose(ferrname) _internal_put2file(ferrname,NULL)
#define setdebug(a) DEBUG_MODE=a


/***
	the memory_monitor structure stores for every alloc the
	location in the source of the memory call (function, line)
	as some additionnal informations like the size and the
	auto-incremental number of memory operation.
	
	in addition to that, when allocating memory, we allocate
	more than requested to put an header before the memory
	block and a footer, after. Both are filled with checksum
	and memory codes.
	
	in debug mode, every allocation is checked with those
	headers and footers to prevent segmentation fault and
	make the debuging easier.
	
	there is also a user memory monitor to trace memory operations
	of selected memory areas. The user define a memory zone to
	monitor, then every MemCheck, memory is compared to a backup
	and a warning is done when modifications are encountered.
	
*/

#define MEM_MONITOR_FRAG_ELEMENT 20000
/* #define MEM_CHECK_FILLER_SIZE 96 */
#define MEM_CHECK_FILLER_SIZE 224
#define WAY_MALLOC 0
#define WAY_REALLOC 1
#define WAY_FREE 2

struct s_memory_monitor
{
	char *ptr;
	int size;
	char *cursource;
	char *curfunc;
	int curline;
	int mem_op;
};
struct s_memory_monitor *mem_monitor=NULL;

struct s_memory_check_header
{
	unsigned char filler[MEM_CHECK_FILLER_SIZE];
	unsigned long checksum[4]; /* may be 32 or 64 bits depending on the architecture */
};
struct s_memory_check_footer
{
	unsigned long checksum[4]; /* may be 32 or 64 bits depending on the architecture */
	unsigned char filler[MEM_CHECK_FILLER_SIZE];
};

struct s_memory_trace
{
	char *name;
	char *ptr;
	char *backup;
	int size;
	struct s_memory_trace *next;
};
static struct s_memory_trace *memory_trace_ROOT=NULL;

/* need some memory related prototypes cause of cross definitions */
void _internal_MemTrace(char *cursource,int curline, char *curfunc,char *ptr,int size,...);
void _internal_MemUntrace(char *cursource,int curline, char *curfunc,void *ptr,...);
#define MemUntrace(...) _internal_MemUntrace(__FILENAME__,__LINE__,FUNC , ## __VA_ARGS__)
#define MemTrace(...) _internal_MemTrace(__FILENAME__,__LINE__,FUNC , __VA_ARGS__)
char *_internal_strdup(char *in_str,char *cursource,int curline, char *curfunc);
#define TxtStrDup(in_str) _internal_strdup(in_str,__FILENAME__,__LINE__,FUNC)

/***
	MemDump
	
	dump memory in hexa/text on stdout
	
	input: pointer to memory
	       size of the dump in bytes
	
	_internal_MemDump does not make any check about memory rights
	_internal_MemDumpChecked does a check and allow only malloced memory to be dumped
	MemDump is the library exported function
	
*/
void _internal_MemDump(unsigned char *ptr,int sizedump)
{
	#undef FUNC
	#define FUNC "_internal_MemDump"
	
	int i,j,lim1,lim2;

	fflush(stdout);	
	fflush(stderr);	
	for (i=0;i<sizedump;i+=16)
	{
		lim1=i+8;
		lim2=i+16;
		if (lim1>sizedump) lim1=sizedump;
		if (lim2>sizedump) lim2=sizedump;
		
		printf("%8lX - ",(long)ptr+i);
		for (j=i;j<lim1;j++)
			printf("%02X ",*(ptr+j));
		for (j=lim1;j<i+8;j++)
			printf("   ");
		printf(" ");
		for (j=lim1;j<lim2;j++)
			printf("%02X ",*(ptr+j));
		for (j=lim2;j<i+16;j++)
			printf("   ");
		printf(" | ");
		for (j=i;j<lim2;j++)
			if ((*(ptr+j))>=32 && (*(ptr+j))<128)
				printf("%c",(*(ptr+j)));
			else
				printf(".");
		printf("\n");
	}	
	fflush(stdout);	
}
void _internal_MemDumpChecked(char *ptr,int sizedump, char *cursource, int curline, char *curfunc)
{
	#undef FUNC
	#define FUNC "MemDump"
	int i,ok=0;
	
	_internal_put2log("ERROR ",cursource,curline,curfunc,"dumping %X:%d",ptr,sizedump);
	for (i=0;i<_static_library_memory_monitor_top && !ok;i++)
	{
		if (mem_monitor[i].ptr!=NULL)
		{
			if (ptr>=mem_monitor[i].ptr && ptr<mem_monitor[i].ptr+mem_monitor[i].size)
				ok=1;
		}
	}
	if (!ok)
	{
		_internal_put2log("ERROR ",cursource,curline,curfunc,"You cannot dump an area that was not previously alloced");
		exit(INTERNAL_ERROR);
	}
	_internal_MemDump(ptr,sizedump);
}
#define MemDump(ptr,sizedump) _internal_MemDumpChecked(ptr,sizedump,__FILENAME__,__LINE__,FUNC)

/***
	_MemCheck
	
	perform a check of all memory allocated blocks
	
	fullcheck:
	0 perform only one memory check
	1 perform a full memory check (when using DEBUG_MODE or macro call)
	
	in addition to that, a monitor check may be performed if the user enabled it with MemTrace
	
	internal use, do not call this function
	please use the macro MemCheck()
*/
void _MemCheck(int fullcheck,char *cursource,int curline, char *curfunc)
{
	#undef FUNC
	#define FUNC "_MemCheck"
	static int cur_check_idx=0;
	struct s_memory_check_header *mem_header;
	struct s_memory_check_footer *mem_footer;
	struct s_memory_trace *mem_trace;
	char *realptr;
	int i,j,ok,istart,iend,dump_size;

	/** memory monitor check **/
	mem_trace=memory_trace_ROOT;
	while (mem_trace!=NULL)
	{
		/* check if there was some active memory user monitor when freeing memory */
		ok=0;
		for (i=0;i<_static_library_memory_monitor_top && !ok;i++)
		{
			if (mem_monitor[i].ptr<=mem_trace->ptr && mem_trace->ptr<mem_monitor[i].ptr+mem_monitor[i].size)
			{
				ok=1;
			}
		}
		if (!ok)
		{
			_internal_put2log("MONITOR",cursource,curline,curfunc,"Cannot monitor anymore the block [%8X:%d:%s] cause it was freed!",mem_trace->ptr,mem_trace->size,mem_trace->name);
			MemUntrace(mem_trace->ptr);
			/* au plus simple */
			mem_trace=memory_trace_ROOT;
			continue;
		}
		
		/* monitor the memory */
		if (memcmp(mem_trace->ptr,mem_trace->backup,mem_trace->size)!=0)
		{
			
			/* warn about the change, then update memory backup */
			_internal_put2log("MONITOR",cursource,curline,curfunc,"memory block [%8X:%d:%s] memory changed!",mem_trace->ptr,mem_trace->size,mem_trace->name);
			if (mem_trace->size<128)
			{
				dump_size=mem_trace->size;
				_internal_put2log("MONITOR",cursource,curline,curfunc,"from:",mem_trace->ptr,mem_trace->size,mem_trace->name);
			}
			else
			{
				dump_size=128;
				_internal_put2log("MONITOR",cursource,curline,curfunc,"from: (dump is limited to 128 bytes)",mem_trace->ptr,mem_trace->size,mem_trace->name);
			}
			_internal_MemDump(mem_trace->backup,dump_size);
			_internal_put2log("MONITOR",cursource,curline,curfunc,"to:",mem_trace->ptr,mem_trace->size,mem_trace->name);
			_internal_MemDump(mem_trace->ptr,dump_size);
			
			memcpy(mem_trace->backup,mem_trace->ptr,mem_trace->size);
		}
		mem_trace=mem_trace->next;
	}
	
	/** memory check **/
	
	if (!fullcheck)
	{
		/* we want to control only one but used cell */
		istart=cur_check_idx;
		while (istart<_static_library_memory_monitor_top && mem_monitor[istart].ptr==NULL)
			istart++;
		if (istart>=_static_library_memory_monitor_top)
			istart=cur_check_idx=0;
		
		iend=cur_check_idx+1;
		/* we wont check the first alloc if it is a realloc on NULL pointer */
		if (iend>_static_library_memory_monitor_top)
			iend=_static_library_memory_monitor_top;
	}
	else
	{
		istart=0;
		iend=_static_library_memory_monitor_top;
	}	
	
	for (j=istart;j<iend;j++)
	{
		if (mem_monitor[j].ptr!=NULL)
		{
			realptr=mem_monitor[j].ptr-sizeof(struct s_memory_check_header);
			mem_header=(struct s_memory_check_header *)realptr;
			mem_footer=(struct s_memory_check_footer *)(mem_monitor[j].ptr+mem_monitor[j].size);
			
			for (i=0;i<MEM_CHECK_FILLER_SIZE;i++)
			{
				if (mem_header->filler[i]!=0xA5)
				{
					/* This is a direct call to put2log, specific to memory management! Do not copy this! */
					_internal_put2log("ERROR",cursource,curline,curfunc,"memory block %8lX header check failed!",(long)mem_monitor[j].ptr);
					_internal_put2log("ERROR",cursource,curline,curfunc,"dump of the header");
					_internal_MemDump((char*)mem_header,sizeof(struct s_memory_check_header));
					_internal_put2log("ERROR",cursource,curline,curfunc,"dump of data after header");
					_internal_MemDump((char*)mem_monitor[j].ptr,64);
					exit(INTERNAL_ERROR);
				}
				if (mem_footer->filler[i]!=0x5A)
				{
					/* This is a direct call to put2log, specific to memory management! Do not copy this! */
					_internal_put2log("ERROR",cursource,curline,curfunc,"memory block %8lX footer check failed!",(long)mem_monitor[j].ptr);
					_internal_put2log("ERROR",cursource,curline,curfunc,"dump of data before footer");
					_internal_MemDump((char*)mem_footer-64,64);
					_internal_put2log("ERROR",cursource,curline,curfunc,"dump of footer");
					_internal_MemDump((char*)mem_footer,sizeof(struct s_memory_check_footer));
					exit(INTERNAL_ERROR);
				}
			}
			for (i=0;i<4;i++)
			{
				if (mem_header->checksum[i]!=(unsigned long)realptr*(i+1))
				{
					/* This is a direct call to put2log, specific to memory management! Do not copy this! */
					_internal_put2log("ERROR",cursource,curline,curfunc,"memory block %8X header check checksum failed!",mem_monitor[j].ptr);
					_internal_put2log("ERROR",cursource,curline,curfunc,"dump of the header");
					_internal_MemDump((char*)mem_header,sizeof(struct s_memory_check_header));
					_internal_put2log("ERROR",cursource,curline,curfunc,"dump of data after header");
					_internal_MemDump((char*)mem_monitor[j].ptr,64);
					exit(INTERNAL_ERROR);
				}
				if (mem_footer->checksum[i]!=(unsigned long)realptr*(i+1))
				{
					/* This is a direct call to put2log, specific to memory management! Do not copy this! */
					_internal_put2log("ERROR",cursource,curline,curfunc,"memory block %8X footer check checksum failed!",mem_monitor[j].ptr);
					_internal_put2log("ERROR",cursource,curline,curfunc,"dump of data before footer");
					_internal_MemDump((char*)mem_footer-64,64);
					_internal_put2log("ERROR",cursource,curline,curfunc,"dump of footer");
					_internal_MemDump((char*)mem_footer,sizeof(struct s_memory_check_footer));
					exit(INTERNAL_ERROR);
				}
			}
		}
	}
}

/***
	_MemMonitor

	monitor memory alloc and free (the adresses are the users adresses, not the real ptr values)
	
	when alloc, informations are stored in a dynamic array, grouping all memory requests
	when realloc, informations are updated
	when free, we check that the pointer is in the array, then we delete informations	
	
	when memory_way equals WAY_MALLOC
	we store ptr in the table, newptr is not used
	when memory_way equals WAY_REALLOC
	we look for ptr in the table then we store the newptr value
	when memory_way equals WAY_FREE
	we look for ptr in the table then we delete the value. newptr is not used
	
	other parameters give debug informations
		
	internal use, do not call this function
*/
void _MemMonitor(void *ptr, void *newptr, int size, char *cursource,int curline, char *curfunc,int memory_way)
{
	#undef FUNC
	#define FUNC "_MemMonitor"
	
	char *waylibs[3]={"Malloc ","Realloc","Free   "};
	static int last_memory_monitor_idx=0;
	int memory_monitor_idx;
	
	_static_library_memory_operation++;
	
	if (memory_way==WAY_REALLOC)
		logdebug("DUMP - %s (%8lX->%8lX;%d) %s %s L%d op:%d - mem used=%d",waylibs[memory_way],(long)ptr,(long)newptr,size,cursource,curfunc,curline,_static_library_memory_operation,_static_library_memory_used);
	else
		logdebug("DUMP - %s (%8lX;%d) %s %s L%d op:%d - mem used=%d",waylibs[memory_way],(long)ptr,size,cursource,curfunc,curline,_static_library_memory_operation,_static_library_memory_used);
	
	/* all structs full, need more memory */
	if (_static_library_memory_monitor_top==_static_library_memory_monitor_max)
	{
		_static_library_memory_monitor_max+=MEM_MONITOR_FRAG_ELEMENT;
		logdebug("DUMP - Reallocating memory monitor structure to %d",_static_library_memory_monitor_max);
		mem_monitor=realloc(mem_monitor,_static_library_memory_monitor_max*sizeof(struct s_memory_monitor));
		if (!mem_monitor)
		{
			logerr("not enough memory to realloc memory monitor structure (%d elements eq %dkb) - memory in used=%dkb",_static_library_memory_monitor_max,_static_library_memory_monitor_max*sizeof(struct s_memory_monitor)/1024,_static_library_memory_used/1024);
			exit(INTERNAL_ERROR);
		}
	}
	/* init top cell struct to ZERO */
	memset(&mem_monitor[_static_library_memory_monitor_top],0,sizeof(struct s_memory_monitor));
	
	switch (memory_way)
	{
		case WAY_MALLOC:
			/* is the last block used free? */
			if (mem_monitor[last_memory_monitor_idx].ptr==NULL)
				memory_monitor_idx=last_memory_monitor_idx;
			else
			if (last_memory_monitor_idx>0 && mem_monitor[last_memory_monitor_idx-1].ptr==NULL)
				memory_monitor_idx=last_memory_monitor_idx-1;
			else
			if (last_memory_monitor_idx<_static_library_memory_monitor_top && mem_monitor[last_memory_monitor_idx+1].ptr==NULL)
				memory_monitor_idx=last_memory_monitor_idx+1;
			else
				memory_monitor_idx=_static_library_memory_monitor_top;

			/* not found the easiest way, try to get a free block in the full list */
			if (memory_monitor_idx==_static_library_memory_monitor_top)
			{
				memory_monitor_idx=0;
				while (mem_monitor[memory_monitor_idx].ptr!=NULL && memory_monitor_idx<_static_library_memory_monitor_top)
					memory_monitor_idx++;
			}
				
			if (memory_monitor_idx==_static_library_memory_monitor_top)
				_static_library_memory_monitor_top++;
			
			mem_monitor[memory_monitor_idx].ptr=ptr;
			mem_monitor[memory_monitor_idx].size=size;
			mem_monitor[memory_monitor_idx].cursource=cursource;
			mem_monitor[memory_monitor_idx].curfunc=curfunc;
			mem_monitor[memory_monitor_idx].curline=curline;
			mem_monitor[memory_monitor_idx].mem_op=_static_library_memory_operation;
			_static_library_memory_used+=size;
			_static_library_memory_aggregate+=size;
			break;
		case WAY_REALLOC:
			memory_monitor_idx=0;
			while (mem_monitor[memory_monitor_idx].ptr!=ptr && memory_monitor_idx<_static_library_memory_monitor_top)
				memory_monitor_idx++;
			if (memory_monitor_idx==_static_library_memory_monitor_top)
			{
				/* This is a direct call to put2log, specific to memory management! Do not copy this! */
				_internal_put2log("ERROR",cursource,curline,curfunc,"you cannot realloc a memory block that wasn't previously alloc!");
				exit(INTERNAL_ERROR);
			}
			/* update monitor block */
			_static_library_memory_used+=size-mem_monitor[memory_monitor_idx].size;
			_static_library_memory_aggregate+=size;
			
			mem_monitor[memory_monitor_idx].ptr=newptr;
			mem_monitor[memory_monitor_idx].size=size;
			mem_monitor[memory_monitor_idx].cursource=cursource;
			mem_monitor[memory_monitor_idx].curfunc=curfunc;
			mem_monitor[memory_monitor_idx].curline=curline;
			mem_monitor[memory_monitor_idx].mem_op=_static_library_memory_operation;
			break;
		case WAY_FREE:
			memory_monitor_idx=0;
			while (mem_monitor[memory_monitor_idx].ptr!=ptr && memory_monitor_idx<_static_library_memory_monitor_top)
				memory_monitor_idx++;
			if (memory_monitor_idx==_static_library_memory_monitor_top)
			{
				/* This is a direct call to put2log, specific to memory management! Do not copy this! */
				_internal_put2log("ERROR",cursource,curline,curfunc,"you cannot free a memory block that wasn't previously alloc!");
				exit(INTERNAL_ERROR);
			}
			_static_library_memory_used-=mem_monitor[memory_monitor_idx].size;
			/* reset monitor block */
			memset(&mem_monitor[memory_monitor_idx],0,sizeof(struct s_memory_monitor));
			/* keep this id for next alloc */
			last_memory_monitor_idx=memory_monitor_idx;
			break;
		default:
			logerr("Unknown memory_way method! This should not happened!");
			exit(INTERNAL_ERROR);
	}	
	if (_static_library_memory_used>_static_library_memory_used_max)
		_static_library_memory_used_max=_static_library_memory_used;
}


/***
	Memory management functions
	- Check NULL pointer
	- Display usefull debug informations when things go wrong
	
	Warning: log messages use direct call to put2log, this is
	specific to memory management because we need to know which
	function calls the memory function! Do not copy this behaviour!

	MemRealloc(ptr,size) like realloc
	MemMalloc(size)      like malloc
	MemCalloc(size)      like calloc
	MemFree(ptr)         like free
	MemCheck()           check memory allocations for overriding bugs
	
	You do not have to check returned pointer. If things go wrong
	the program will be halted and display an error message

*/

/***
	_MemFree
	internal use, do not call this function
	please use the macro MemFree(ptr)

*/
void _MemFree(void *ptr, char *cursource,int curline, char *curfunc)
{
	if (!ptr)
	{
		/* This is a direct call to put2log, specific to memory management! Do not copy this! */
		_internal_put2log("ERROR",cursource,curline,curfunc,"cannot free NULL pointer");
		exit(INTERNAL_ERROR);
	}
	
	free(ptr-sizeof(struct s_memory_check_header));
	_MemMonitor(ptr,NULL,0,cursource,curline,curfunc,WAY_FREE);
	ptr=NULL;
	
	/* make a memory check like in allocation routine */
	if (DEBUG_MODE)
		_MemCheck(1,cursource,curline,curfunc);
	else
		_MemCheck(0,cursource,curline,curfunc);
	
}

/***
	_MemRealloc
	internal use, do not call this function
	please use the macro MemRealloc(ptr,size)

*/
void *_MemRealloc(void *ptr, int size, char *cursource,int curline, char *curfunc)
{
	struct s_memory_check_header *mem_header;
	struct s_memory_check_footer *mem_footer;
	char *newptr,*realptr;
	int realsize;
	int i;
	/* if debug mode enabled, perform a full check every operations */
	if (DEBUG_MODE)
		_MemCheck(1,cursource,curline,curfunc);
	else
		_MemCheck(0,cursource,curline,curfunc);

	/* realloc of size 0, we must free and quit */
	if (size==0 && ptr!=NULL)
	{
		_MemFree(ptr,cursource,curline,curfunc);
		return NULL;
	}

	/* we malloc more than requested, to store memory check structures */
	realsize=size+sizeof(struct s_memory_check_header)+sizeof(struct s_memory_check_footer);

	/* more compliant because on posix system, realloc can do his job on an input NULL pointer */
	if (ptr!=NULL)
	{
		int memchkidx=0;
		while (mem_monitor[memchkidx].ptr!=ptr && memchkidx<_static_library_memory_monitor_top)
			memchkidx++;
		if (memchkidx==_static_library_memory_monitor_top)
		{
			/* This is a direct call to put2log, specific to memory management! Do not copy this! */
			_internal_put2log("ERROR",cursource,curline,curfunc,"you cannot realloc a memory block that wasn't previously alloc!");
			exit(INTERNAL_ERROR);
		}
		realptr=realloc(ptr-sizeof(struct s_memory_check_header),realsize);
	}
	else
		realptr=malloc(realsize);
		
	if (!realptr)
	{
		/* This is a direct call to put2log, specific to memory management! Do not copy this! */
		_internal_put2log("ERROR",cursource,curline,curfunc,"not enough memory to allocate %d byte%s (operation %d cumul %d)",size,(size>1)?"s":"",_static_library_memory_operation,_static_library_memory_aggregate);
		exit(ABORT_ERROR);
	}
	
	newptr=realptr+sizeof(struct s_memory_check_header);
	
	/* add checksum control to structures */
	mem_header=(struct s_memory_check_header *)realptr;
	mem_footer=(struct s_memory_check_footer *)(newptr+size);
	for (i=0;i<MEM_CHECK_FILLER_SIZE;i++)
	{
		mem_header->filler[i]=0xA5;
		mem_footer->filler[i]=0x5A;
	}
	for (i=0;i<4;i++)
	{
		mem_header->checksum[i]=(unsigned long)realptr*(i+1);
		mem_footer->checksum[i]=(unsigned long)realptr*(i+1);
	}
	
	if (ptr==NULL)
		_MemMonitor(newptr,NULL,size,cursource,curline,curfunc,WAY_MALLOC);
	else
		_MemMonitor(ptr,newptr,size,cursource,curline,curfunc,WAY_REALLOC);
		
	return newptr;
}
/***
	_MemCalloc
	internal use, do not call this function
	please use the macro MemCalloc(size)

*/
void *_MemCalloc(int size, char *cursource,int curline, char *curfunc)
{
	char *newptr;
	
	newptr=_MemRealloc(NULL,size,cursource,curline,curfunc);
	memset(newptr,0,size);
	return newptr;
}

/*** Exported memory management prototypes */

/*** no memory trick functions */
#if LIBRARY_NO_MEMORY_WRAPPER==1
#define MemRealloc(ptr,size) realloc(ptr,size)
#define MemMalloc(size) malloc(size)
#define MemCalloc(size) calloc(size,1);
#define MemFree(ptr) free(ptr)
#define MemCheck() while (0);
#else
/*** patched memory functions */
#define MemRealloc(ptr,size) _MemRealloc(ptr,size,__FILENAME__,__LINE__,FUNC)
#define MemMalloc(size) _MemRealloc(NULL,size,__FILENAME__,__LINE__,FUNC)
#define MemCalloc(size) _MemCalloc(size,__FILENAME__,__LINE__,FUNC)
#define MemFree(ptr) _MemFree(ptr,__FILENAME__,__LINE__,FUNC)
#define MemCheck() _MemCheck(1,__FILENAME__,__LINE__,FUNC)
#endif


/***
	MemTrace
	_internal_MemTrace
	
	store a trace order in the global structure
	input: ptr, size
	
	then <size> bytes will be monitored for change at <ptr> adress
	an optionnal end parameter with the macro can define a monitor name (with arguments to include some counters)
*/
void _internal_MemTrace(char *cursource,int curline, char *curfunc,char *ptr,int size,...)
{
	#undef FUNC
	#define FUNC "MemTrace"
	
	struct s_memory_trace *newtrace,*curtrace;
	char *memtracename;
	int i,ok=0,zelen;
	char *zename;
	
	va_list argptr;
	va_start(argptr,size);
	zename=va_arg(argptr,char *);
	if (zename!=NULL)
	{
		zelen=vsnprintf(NULL,0,zename,argptr)+1;
		va_end(argptr);
		memtracename=MemMalloc(zelen);
		va_start(argptr,size);
		zename=va_arg(argptr,char *);
		vsprintf(memtracename,zename,argptr);
	}		
	else
		memtracename=TxtStrDup("");
	va_end(argptr);

	logdebug("monitoring %lX:%d",ptr,size);
	for (i=0;i<_static_library_memory_monitor_top && !ok;i++)
	{
		if (mem_monitor[i].ptr!=NULL)
		{
			if (ptr>=mem_monitor[i].ptr && ptr<mem_monitor[i].ptr+mem_monitor[i].size)
				ok=1;
		}
	}
	if (!ok)
	{
		_internal_put2log("ERROR ",cursource,curline,curfunc,"You cannot monitor an area that was not previously alloced");
		exit(INTERNAL_ERROR);
	}	
	
	newtrace=MemCalloc(sizeof(struct s_memory_trace));
	newtrace->name=memtracename;
	newtrace->ptr=ptr;
	newtrace->size=size;
	newtrace->backup=MemMalloc(size);
	memcpy(newtrace->backup,ptr,size);
	
	curtrace=memory_trace_ROOT;
	if (curtrace!=NULL)
	{
		/* check for duplicate name */
		if (strlen(newtrace->name))
		{
			if (!strcmp(curtrace->name,newtrace->name))
			{
				_internal_put2log("ERROR ",cursource,curline,curfunc,"You cannot use the same monitor name twice or more [%s]",newtrace->name);
				exit(INTERNAL_ERROR);
			}			
		}
		
		while (curtrace->next!=NULL)
			curtrace=curtrace->next;
		curtrace->next=newtrace;
	}
	else
		memory_trace_ROOT=newtrace;
}

/***
	MemUntrace
	_internal_MemUntrace
	
	remove a trace order from the global structure
	input: ptr
	the <ptr> adress wont be monitored anymore
	an optionnal end parameter with the macro can define a monitor name (with arguments to include some counters)
*/
void _internal_MemUntrace(char *cursource,int curline, char *curfunc, void *ptr,...)
{
	#undef FUNC
	#define FUNC "MemUntrace"
	
	struct s_memory_trace *zetrace,*curtrace,*seektrace;
	char *zename,*memtracename;
	int zelen;
	va_list argptr;

	va_start(argptr,ptr);
	zename=va_arg(argptr,char *);
	if (ptr==NULL && zename!=NULL)
	{
		zelen=vsnprintf(NULL,0,zename,argptr)+1;
		va_end(argptr);
		memtracename=MemMalloc(zelen);
		va_start(argptr,ptr);
		zename=va_arg(argptr,char *);
		vsprintf(memtracename,zename,argptr);
		zename=memtracename;
		logdebug("unmonitoring %X:%s",ptr,zename);
	}
	else
	{
		zename=NULL;
		logdebug("unmonitoring %X",ptr);
	}
	va_end(argptr);
	
	curtrace=memory_trace_ROOT;
	if (curtrace!=NULL)
	{
		if (ptr!=NULL)
		{
			while (curtrace!=NULL && curtrace->ptr!=ptr)
			{
				curtrace=curtrace->next;
			}
		}
		else
		if (zename!=NULL)
		{
			while (curtrace!=NULL && strcmp(curtrace->name,zename))
			{
				curtrace=curtrace->next;
			}
		}
		else
		{
			_internal_put2log("ERROR ",cursource,curline,curfunc,"You must specify a pointer or a name to remove the trace...");
			exit(INTERNAL_ERROR);
		}		
		
		if (!curtrace)
		{
			_internal_put2log("ERROR ",cursource,curline,curfunc,"trace %8X not found! Exiting",ptr);
			exit(INTERNAL_ERROR);
		}
	}
	else
	{
		_internal_put2log("ERROR ",cursource,curline,curfunc,"There is no trace to remove! Exiting");
		exit(INTERNAL_ERROR);
	}
	
	if (curtrace==memory_trace_ROOT)
	{
		memory_trace_ROOT=curtrace->next;
	}
	else
	{
		zetrace=memory_trace_ROOT;
		while (zetrace->next!=curtrace)
			zetrace=zetrace->next;
		zetrace->next=curtrace->next;
	}
	if (zename)
		MemFree(zename);
	MemFree(curtrace->name);
	MemFree(curtrace->backup);
	MemFree(curtrace);
}



/***
	KeyGetString
	
*/
char *KeyGetString(char *format, ...)
{
	#undef FUNC
	#define FUNC "KeyGetString"
	
	char buffer[PATH_MAX+1];
	va_list argptr;
	int c=0,idx=0;;
	
	va_start(argptr,format);
	vprintf(format,argptr);
	va_end(argptr);
	fflush(stdout);
	
	while (c!=0x0A && idx<PATH_MAX)
		buffer[idx++]=c=getchar();
		
	buffer[idx-1]=0;
	return TxtStrDup(buffer);
}


enum e_keychoices {
CHOICE_YES=1,
CHOICE_NO=2,
CHOICE_ALL=4,
CHOICE_QUIT=8,
};

/***
	KeyGetChoice
	
		
*/
int KeyGetChoice(int choice, char *format, ...)
{
	#undef FUNC
	#define FUNC "KeyGetYesNo"
	
	va_list argptr;
	char question[PATH_MAX]; /* big enough to put all options */
	char *buffer;
	int c=-1;
	char sep[2];
	
	if (!choice)
		return 0;
	
	va_start(argptr,format);
	vprintf(format,argptr);
	va_end(argptr);
	strcpy(sep,"");	
	strcpy(question,"(");
	if (choice & CHOICE_YES) {strcat(question,"Yes");strcpy(sep,"/");}
	if (choice & CHOICE_NO) {strcat(question,sep);strcat(question,"No");strcpy(sep,"/");}
	if (choice & CHOICE_ALL) {strcat(question,sep);strcat(question,"All");strcpy(sep,"/");}
	if (choice & CHOICE_QUIT) {strcat(question,sep);strcat(question,"Quit");strcpy(sep,"/");}
	strcat(question,")");
	
	buffer=KeyGetString("%s:",question);
	while ((stricmp(buffer,"Yes") || !(choice & CHOICE_YES))
	 && (stricmp(buffer,"No") || !(choice & CHOICE_NO))
	 && (stricmp(buffer,"All") || !(choice & CHOICE_ALL))
	 && (stricmp(buffer,"Quit") || !(choice & CHOICE_QUIT)))
	{
		MemFree(buffer);
		buffer=KeyGetString("Please enter %s:",question);
	}
	if (stricmp(buffer,"Yes")==0) c=CHOICE_YES;
	if (stricmp(buffer,"No")==0) c=CHOICE_NO;
	if (stricmp(buffer,"All")==0) c=CHOICE_ALL;
	if (stricmp(buffer,"Quit")==0) c=CHOICE_QUIT;
	MemFree(buffer);
	return c;
}


/***
	TxtIsInteger
	
	return 1 if the string is a pure integer (positive or negative)
	else return 0
*/
int TxtIsInteger(char *in_str)
{
	#undef FUNC
	#define FUNC "TxtIsInteger"
	
	int idx;
	
	if (!in_str[0])
		return 0;
	if (in_str[0]=='-' && in_str[1]==0)
		return 0;
	if (in_str[0]=='-' || (in_str[0]>='0' && in_str[0]<='9'))
	{
		idx=1;
		while (in_str[idx]>='0' && in_str[idx]<='9')
			idx++;
		if (in_str[idx]==0)
			return 1;
	}
	return 0;
}
/***
	TxtIsFloat
	TxtIsDouble
	TxtIsLongDouble
	
	return 1 if the string is a double (positive or negative)
	else return 0
	
	limits are 1.7E+308 and 1.7E-308 (long double)
*/
int _internal_TxtIsFloat(char *in_str, double mantmax, int expmax)
{
	#undef FUNC
	#define FUNC "_internal_TxtIsFloat"
	
	int idx,zepoint=0,zeexp=0;
	int valexp=0,backidx;
	char *expptr=NULL;
	double valmant,mul=10.0;
	
	if (in_str==NULL)
	{
		logerr("first parameter cannot be NULL!");
		exit(INTERNAL_ERROR);		
	}
	if (!in_str[0])
		return 0;
	if ((in_str[0]=='+' || in_str[0]=='-') && in_str[1]==0)
		return 0;
	if (in_str[0]=='+' ||in_str[0]=='-' || (in_str[0]>='0' && in_str[0]<='9'))
	{
		if (in_str[0]=='+' || in_str[0]=='-')
			idx=1;
		else
			idx=0;
		while (in_str[idx]>='0' && in_str[idx]<='9' || in_str[idx]=='.')
		{
			/* the dot must be before the exponent */
			if (in_str[idx]=='.') {if (expptr) return 0; else zepoint++;}
			else
			if (in_str[idx+1]=='E' && (in_str[idx+2]=='+' || in_str[idx+2]=='-') && (in_str[idx+3]>='0' && in_str[idx+3]<='9'))
				{expptr=&in_str[idx+3];zeexp++;idx+=3;valexp=atoi(expptr);}
			else
			if (in_str[idx+1]=='E') /* invalid exponent format */
				return 0;
			idx++;
		}
		backidx=idx;
		if (zepoint>1 || zeexp>1) return 0;
		if (valexp>expmax) return 0;
		if (valexp==expmax)
		{
			valmant=0;
			zepoint=0;
			if (in_str[0]=='-' || in_str[0]=='+') idx=1; else idx=0;
			while (in_str[idx]!='E')
			{
				if (in_str[idx]>='0' && in_str[idx]<='9' && !zepoint) valmant=valmant*mul+in_str[idx]-'0';
				else
				if (in_str[idx]=='.') zepoint++;
				else
				if (in_str[idx]>='0' && in_str[idx]<='9') {valmant=valmant+(in_str[idx]-'0')/mul;mul=mul*10.0;}
				else
				{
					logerr("Internal decoding error");
					exit(INTERNAL_ERROR);
				}				
				idx++;
			}
			if (valmant>mantmax)
				return 0;			
		}
		
		if (in_str[backidx]==0)
			return 1;
	}
	return 0;
}

/* doubles and floats have the same limits but not long doubles */
#define TxtIsFloat(in_str)      _internal_TxtIsFloat(in_str,1.0,37)
#define TxtIsDouble(in_str)     _internal_TxtIsFloat(in_str,1.0,37)
#define TxtIsLongDouble(in_str) _internal_TxtIsFloat(in_str,1.7,308)

/***
	TxtIsInvisible
	
	check for spaces and carriage returns
	
	return 1 if the text is invisible, 0 if not
*/
int TxtIsInvisible(char *in_str)
{
	#undef FUNC
	#define FUNC "TxtIsInvisible"

	int idx=0;
	
	while (in_str[idx]!=0)
	{
		if (in_str[idx]!=0x0D && in_str[idx]!=0x0A && in_str[idx]!=' ' && in_str[idx]!=0x0C)
			return 0;
		else
			idx++;
	}
	return 1;
}

/***
	TxtStrDup
	
	make a secure strdup of a string
*/
char *_internal_strdup(char *in_str,char *cursource,int curline, char *curfunc)
{
	#undef FUNC
	#define FUNC "TxtStrDup"
	
	char *out_str;
	
	out_str=_MemRealloc(NULL,strlen(in_str)+1,cursource,curline,curfunc);
	strcpy(out_str,in_str);
	return out_str;
}
#define TxtStrDup(in_str) _internal_strdup(in_str,__FILENAME__,__LINE__,FUNC)

/***
	TxtSubstr copy a substring from a string to a new alloc and free string
	TxtGetSubstr copy a substring from a string to a new alloc	
	
	in_str:    reference string
	start_idx: start copy from offset start_idx
	len:       copy len characters.
	           may be relative to the end with a negative value
	           
	out:       the substring, dynamically malloced
	
	note: in_str MUST BE previously malloced for TxtSubstr
*/
char *TxtGetSubstr(char *in_str, int start_idx, int len)
{
	#undef FUNC
	#define FUNC "TxtGetSubstr"
	
	char *out_str;
	int sl;
	
	if (in_str==NULL)
		return NULL;

	/*logdebug("substr(\"%s\",%d,%d)",in_str,start_idx,len);*/
	
	sl=strlen(in_str);
	if (!sl || start_idx<0 || start_idx>=sl)
		return NULL;
		
	/* relative to the end */
	if (len<=0)
	{
		/*logdebug("len<=0\nlen=%d-%d-%d",sl,start_idx,len);*/
		len=sl-start_idx+len;
		/*logdebug("len=0%d",len);*/
	}
	
	if (len<0 || start_idx+len>sl)
		return NULL;

	out_str=MemMalloc(len+1);
	strncpy(out_str,in_str+start_idx,len);
	out_str[len]=0;
	/*logdebug("out_str=\"%s\"\n",out_str);*/
	return out_str;
}
char *TxtSubstr(char *in_str, int start_idx, int len)
{
	#undef FUNC
	#define FUNC "TxtSubstr"
	
	char *out_str;
	
	out_str=TxtGetSubstr(in_str,start_idx,len);
	MemFree(in_str);
	return out_str;
}

/***
	TxtReplace
	
	input:
	in_str:     string where replace will occur
	in_substr:  substring to look for
	out_substr: replace substring
	recurse:    loop until no in_substr is found
	
	note: in_str MUST BE previously malloced if out_substr is bigger than in_substr
*/
char *TxtReplace(char *in_str, char *in_substr, char *out_substr, int recurse)
{
	#undef FUNC
	#define FUNC "TxtReplace"
	
	char *str_look,*m1,*m2;
	char *out_str;
	int i,sl,l1,l2,dif,cpt;

	if (in_str==NULL)
		return NULL;
		
	sl=strlen(in_str);
	l1=strlen(in_substr);
	/* empty string, nothing to do except return empty string */
	if (!sl || !l1)
		return in_str;
		
	l2=strlen(out_substr);
	dif=l2-l1;
	
	/*if (recurse)	
		logdebug("replace in recurse mode");
	else
		logdebug("replace in normal mode");*/
		
	/* replace string is small or equal in size, we dont realloc */
	if (dif<=0)
	{
		/* we loop while there is a replace to do */
		str_look=strstr(in_str,in_substr);
		while (str_look!=NULL)
		{
			/*logdebug(str_look);*/
			
			/* we copy the new string if his len is not null */
			if (l2)
				memcpy(str_look,out_substr,l2);
			/* only if len are different */
			if (l1!=l2)
			{
				/* we move the end of the string byte per byte
				   because memory locations overlap. This is
				   faster than memmove */
				m1=str_look+l1;
				m2=str_look+l2;
				while (*m1!=0)
				{
					*m2=*m1;
					m1++;m2++;
				}
				/* we must copy the EOL */
				*m2=*m1;
			}
			/* look for next replace */
			if (!recurse)
				str_look=strstr(str_look+l2,in_substr);
			else
				str_look=strstr(in_str,in_substr);
		}
		out_str=in_str;
	}
	else
	{
		/* we need to count each replace */
		cpt=0;
		str_look=strstr(in_str,in_substr);
		while (str_look!=NULL)
		{
			cpt++;
			str_look=strstr(str_look+l1,in_substr);
		}
		/* is there anything to do? */
		if (cpt)
		{
			/*logdebug("found!");*/
			/* we realloc to a size that will fit all replaces */
			out_str=MemRealloc(in_str,sl+1+dif*cpt);
			str_look=strstr(out_str,in_substr);
			while (str_look!=NULL && cpt)
			{
				/*logdebug("before");
				logdebug(out_str);
				logdebug(str_look);*/
				
				/* as the replace string is bigger we
				   have to move memory first from the end */
				m1=out_str+sl;
				m2=m1+dif;
				sl+=dif;
				while (m1!=str_look+l1-dif)
				{
					*m2=*m1;
					m1--;m2--;
				}
				/*logdebug("intermediate");
				logdebug(out_str);*/
				
				/* then we copy the replace string (can't be NULL in this case) */
				memcpy(str_look,out_substr,l2);
				
				/*logdebug("after");
				logdebug(out_str);
				logdebug(str_look);*/
				
				/* look for next replace */
				if (!recurse)
					str_look=strstr(str_look+l2,in_substr);
				else
					str_look=strstr(in_str,in_substr);
					
				/* to prevent from naughty overlap */
				cpt--;
			}
			if (str_look!=NULL)
			{
				logerr("overlapping replace string (%s/%s), you can't use this one!",in_substr,out_substr);
				exit(ABORT_ERROR);
			}
		}
		else
			out_str=in_str;
	}
	
	/*logdebug(out_str);*/
	return out_str;
}

/***
	TxtCleanSpecialChar
	TxtDeleteSpecialChar
	TxtReplaceSpecialChar
	_internal_TxtCleanInitTable
	_internal_TxtChangeSpecialChar
	
	clean a string from spaces, accented characters and other special chars and replace them with underscore
	delete from a string every spaces, accented chars and other special chars
*/

#define TXT_UNDEFINED_SPECIAL_CHAR 0
#define TXT_CLEAN_SPECIAL_CHAR 1
#define TXT_DELETE_SPECIAL_CHAR 2
#define TXT_REPLACE_LIST_SPECIAL_CHAR 3

int _internal_TxtCleanInitTable(int zemode, va_list argptr)
{
	#undef FUNC
	#define FUNC "_internal_TxtCleanInitTable"
	/* must be unsigned cause in_replace will be offset */
	unsigned char *in_replace,*out_replace;
	char default_char;
	int i,l1,l2;

	/* common to all methods */
	_static_library_txtcleanconvtable[0]=0;
	
	switch(zemode)
	{
		case TXT_CLEAN_SPECIAL_CHAR:default_char='_';break;
		case TXT_DELETE_SPECIAL_CHAR:default_char=0;break;
		case TXT_REPLACE_LIST_SPECIAL_CHAR:
			in_replace=va_arg(argptr,unsigned char *);
			out_replace=va_arg(argptr,unsigned char *);
			l1=strnlen(in_replace,255);
			l2=strnlen(out_replace,255);
			if (l1==255 || l2==255)
			{
				logerr("replace substring musn't be longer than 255 chars");
				exit(INTERNAL_ERROR);
			}
			if (l1!=l2)
			{
				logerr("replace substrings must be the same size (%d!=%d)",l1,l2);
				logerr("in =[%s]\nout=[%s]",in_replace,out_replace);
				exit(INTERNAL_ERROR);
			}
			if (!l1 || !l2)
			{
				logerr("replace substring musn't start with zero value");
				exit(INTERNAL_ERROR);
			}
			/* default */
			for (i=1;i<256;i++)
				_static_library_txtcleanconvtable[i]=i;
			/* user define */
			for (i=0;i<l1;i++)
				_static_library_txtcleanconvtable[in_replace[i]]=out_replace[i];
			
			return TXT_UNDEFINED_SPECIAL_CHAR;
			
		default:logerr("unsupported cleaning method");exit(INTERNAL_ERROR);
	}
	
	for (i=1;i<256;i++)
		_static_library_txtcleanconvtable[i]=default_char;
	for (i='a';i<='z';i++)
		_static_library_txtcleanconvtable[i]=i;
	for (i='A';i<='Z';i++)
		_static_library_txtcleanconvtable[i]=i;
	for (i='0';i<='9';i++)
		_static_library_txtcleanconvtable[i]=i;
	_static_library_txtcleanconvtable['.']='.';
	_static_library_txtcleanconvtable['-']='-';
	return zemode;
}

char *_internal_TxtChangeSpecialChar(char *in_str, int zemode, ...)
{
	#undef FUNC
	#define FUNC "_internal_TxtChangeSpecialChar"
	static int init=TXT_UNDEFINED_SPECIAL_CHAR;
	va_list argptr;
	char convtmp[2];
	char convrep[2];
	int i;
	
	convtmp[1]=0;
	convrep[1]=0;
	
	if (init!=zemode)
	{
		va_start(argptr,zemode);
		init=_internal_TxtCleanInitTable(zemode,argptr);
		va_end(argptr);
	}
	
	for (i=1;i<256;i++)	
	{
		if (_static_library_txtcleanconvtable[i]!=i)
		{
			convtmp[0]=i;
			convrep[0]=_static_library_txtcleanconvtable[i];
			TxtReplace(in_str,convtmp,convrep,1);
		}
	}
	return in_str;
}
#define TxtCleanSpecialChar(in_str) _internal_TxtChangeSpecialChar(in_str,TXT_CLEAN_SPECIAL_CHAR)
#define TxtDeleteSpecialChar(in_str) _internal_TxtChangeSpecialChar(in_str,TXT_DELETE_SPECIAL_CHAR)
#define TxtReplaceSpecialChar(in_str,in_replace,out_replace) _internal_TxtChangeSpecialChar(in_str,TXT_REPLACE_LIST_SPECIAL_CHAR,in_replace,out_replace)


/******** trim functions ***********/

/***
	TxtTrimEnd
	
	trim end of string
*/
char * TxtTrimEnd(char *in_str)
{
	#undef FUNC
	#define FUNC "TxtTrimEnd"

	int idx=strlen(in_str)-1;
	
	while (in_str[idx]>=0)
	{
		if (in_str[idx]==0x0D || in_str[idx]==0x0C || in_str[idx]==0x0A || in_str[idx]==' ' || in_str[idx]=='\t')
			in_str[idx--]=0;
		else
			return;
	}
	return in_str;
}

/***
	TxtTrimStart
	
	trim start of string
*/
char * TxtTrimStart(char *in_str)
{
	#undef FUNC
	#define FUNC "TxtTrimStart"

	int l=strlen(in_str);
	char *m1,*m2;
	int idx;
	
	idx=0;
	while (in_str[idx]==' ' || in_str[idx]=='\t')
		idx++;

	/* faster than memmove */	
	m1=in_str;
	m2=in_str+idx;
	while (*m2!=0)
	{
		*m1=*m2;
		m1++;m2++;
	}
	/* we must copy the EOL */
	*m1=*m2;
	
	return in_str;
}

/*** macro that replace double or more spaces by a single one */
#define TxtTrimSpace(in_str) TxtReplace(in_str, "  "," ",1)
#define TxtTrimTab(in_str) TxtReplace(in_str, "\t\t","\t",1)

/***
	TxtTrim
	
	trim start, end of string, then multiple spaces
*/
char * TxtTrim(char *in_str)
{
	#undef FUNC
	#define FUNC "TxtTrim"
	
	TxtTrimEnd(in_str);
	TxtTrimStart(in_str);
	TxtTrimSpace(in_str);
	TxtTrimTab(in_str);
	
	return in_str;
}

/***
	Regular expression matching function
s

	struct s_reg_machine is designed to handle
	in the future a caching system of regexp
	
*/


struct s_reg_machine
{
	regex_t *preg;
	char *regexp;
	int reghash;
	struct s_reg_machine *next;
};

/***
	_internal_regerror
	
	the POSIX regerror needs a static buffer (why?)
	so it is simple to wrap the function with this
	
	we wont free the memory allocated for the buffer
	because we ALWAYS quit in internal error after
	calling this function
*/
char *_internal_regerror(int regexp_error,regex_t *preg)
{
	#undef FUNC
	#define FUNC "_internal_regerror"
	
	char *buffer;
	buffer=MemCalloc(MAX_LINE_BUFFER+1);
	regerror(regexp_error,preg,buffer,MAX_LINE_BUFFER);
	/* buffer overflow security */
	if (strnlen(buffer,MAX_LINE_BUFFER)==MAX_LINE_BUFFER)
		buffer[MAX_LINE_BUFFER]=0;
	return buffer;
}

/***
	_internal_compile_regexp
	
	compile the regular expression then fill our
	regular expression pool structure
*/
struct s_reg_machine * _internal_compile_regexp(char *regexp)
{
	#undef FUNC
	#define FUNC "_internal_compile_regexp"

	struct s_reg_machine *current_regexp;
	regex_t *preg;
	int err;
	
	/* we malloc this cause we don't known how to free it, except with regfree */
	preg=malloc(sizeof(regex_t));
	
	if ((err=regcomp(preg,regexp,REG_EXTENDED|REG_NOSUB))!=0)
	{
		logerr("cannot compile regexp [%s]",regexp);
		logerr("%s",_internal_regerror(err,current_regexp->preg));
		exit(INTERNAL_ERROR);
	}
	current_regexp=MemCalloc(sizeof(struct s_reg_machine));
	current_regexp->preg=preg;
	current_regexp->regexp=TxtStrDup(regexp);
	
	return current_regexp;
}

/***
	TxtRegMatch
	
	make a regular expression match on in_str with regexp
	
	return REG_MATCH if match
	return REG_NOMATCH if not
	
	warning, this is a pure regular match so if you want to
	match a whole string, use ^ at the beginning of your
	regexp, and $ at the end.
	
	or simply use TxtGlobMatch
*/
int TxtRegMatch(char *in_str, char *regexp)
{
	#undef FUNC
	#define FUNC "TxtRegMatch"
	
	struct s_reg_machine *current_regexp;
	int err;
	
	current_regexp=_internal_compile_regexp(regexp);
	
	err=regexec(current_regexp->preg,in_str,0,NULL,0);
	if (err!=0 && err!=REG_NOMATCH)
	{
		logerr("regexp execution error with [%s]",regexp);
		logerr("%s",_internal_regerror(err,current_regexp->preg));
		exit(INTERNAL_ERROR);
	}
	
	/* free the memory */
	regfree(current_regexp->preg);
	MemFree(current_regexp->regexp);
	MemFree(current_regexp);
	return err;
}

/***
	TxtGlobMatch
	
	like TxtRegMatch bug match the whole string
	by adding constraints to the regular expression
	
	useful to match filenames (globing)
*/
int TxtGlobMatch(char *in_str, char *pattern)
{
	#undef FUNC
	#define FUNC "TxtGlobMatch"
	return fnmatch(pattern,in_str,0);
}

/************************** File operation ******************************/


/***
	s_fileid
	structure used by FileReadLine and FileWriteLine to manage multiple files at a time
*/
struct s_fileid
{
	FILE *file_id;
	char *filename;
	char opening_type[4];
	int cpt;
	struct s_fileid *next;
};

static struct s_fileid *fileidROOT=NULL;

/***
	FileGetStructFromName
	
	input: filename
	output: file structure
		NULL if not found
*/
struct s_fileid *FileGetStructFromName(char *filename)
{
	#undef FUNC
	#define FUNC "FileGetStructFromName"
	struct s_fileid *curfile;

	if (!filename)
	{
		logerr("filename must not be NULL");
		exit(ABORT_ERROR);
	}

	if (strnlen(filename,PATH_MAX)==PATH_MAX)
	{
		logerr("cannot open this file because the argument size is bigger than PATH_MAX (%d)",PATH_MAX);
		exit(ABORT_ERROR);
	}
	
	/* try to find the file in the list */
	curfile=fileidROOT;
	while (curfile!=NULL)
	{
		if (!strcmp(curfile->filename,filename))
			break;
		else
			curfile=curfile->next;
	}
	return curfile;
}

/***
	FileOpen function
	
	open a file in any mode (r,w,a,r+,w+,a+)
	check if the file is already open
	check for conflicts	
*/
FILE *FileOpen(char *filename, char *opening_type)
{
	#undef FUNC
	#define FUNC "FileOpen"
	struct s_fileid *curfile;
	
	/* check parameters coherency */
	if (strlen(opening_type)>3)
	{
		logerr("illegal opening type (too long)");
		exit(ABORT_ERROR);
	}	
	if (strcmp(opening_type,"a") && strcmp(opening_type,"w") && strcmp(opening_type,"r")
	 && strcmp(opening_type,"a+")  && strcmp(opening_type,"w+") && strcmp(opening_type,"r+"))
	{
		logerr("illegal opening type [%s]\nallowed options are: r,w,a,r+,w+,a+",opening_type);
		exit(ABORT_ERROR);
	}		
	
	curfile=FileGetStructFromName(filename);
	
	/* if curfile is NULL then the file is not opened yet */
	if (!curfile)
	{
		_static_library_nbfile_opened++;
		if (_static_library_nbfile_opened>_static_library_nbfile_opened_max)
			_static_library_nbfile_opened_max=_static_library_nbfile_opened;
		curfile=MemCalloc(sizeof(struct s_fileid));
		curfile->filename=MemMalloc(strlen(filename)+1);
		strcpy(curfile->filename,filename);
		strcpy(curfile->opening_type,opening_type);
		curfile->next=fileidROOT;
		curfile->cpt=0;
		fileidROOT=curfile;
	}
	else
	{
		if (strcmp(curfile->opening_type,opening_type))
		{
			logerr("You can't open this file in [%s] mode cause it's already open in [%s] mode",opening_type,curfile->opening_type);
			exit(ABORT_ERROR);
		}		
		return curfile->file_id;
	}
	
	curfile->file_id=fopen(filename,opening_type);
	if (!curfile->file_id)
	{
		int cpt=0;
		logerr("failed to open file [%s] with mode [%s]",filename,opening_type);
		
		curfile=fileidROOT;
		while (curfile!=NULL)
		{
			cpt++;
			curfile=curfile->next;
		}
		if (cpt>10)
			logerr("You have %d files opened, this may be a system limitation");
		logerr("anyway, check empty space and permissions");
		exit(ABORT_ERROR);
	}
	else
	{
		logdebug("opening file [%s] in %s mode",filename,opening_type);
	}
	
	return curfile->file_id;
}

/***
	FileGetStructFromID
	
	retrieve the file structure from the tree, with his ID
*/
struct s_fileid *FileGetStructFromID(FILE *file_id)
{
	#undef FUNC
	#define FUNC "FileGetStructFromID"
	struct s_fileid *curfile;
	
	curfile=fileidROOT;
	while (curfile!=NULL)
	{
		if (curfile->file_id==file_id)
			break;
		else
			curfile=curfile->next;
	}
	if (!curfile)
	{
		logerr("ID requested for an unknown file! (was supposed to be opened)");
		exit(INTERNAL_ERROR);
	}
	
	return curfile;
}

/***
	FileClose function
	
	check for closing return code
	free the memory file structure
*/
void FileClose(FILE *file_id)
{
	#undef FUNC
	#define FUNC "FileClose"
	struct s_fileid *curfile;
	struct s_fileid *unlinkcell;
	
	curfile=FileGetStructFromID(file_id);
	if (fclose(curfile->file_id))
	{
		logerr("error while closing file [%s]",curfile->filename);
	}	

	/* unlink the cell from ROOT */
	if (curfile==fileidROOT)
	{
		fileidROOT=curfile->next;
	}
	else
	{
		unlinkcell=fileidROOT;
		while (unlinkcell->next!=curfile)
			unlinkcell=unlinkcell->next;
		unlinkcell->next=curfile->next;
	}
	MemFree(curfile->filename);
	MemFree(curfile);
	_static_library_nbfile_opened--;
}

/***
	FileAddCPT
	
	add n to counter when reading or writing in a file
*/
void FileAddCPT(FILE *file_id, int n)
{
	#undef FUNC
	#define FUNC "FileAddCPT"
	struct s_fileid *curfile;
	
	curfile=FileGetStructFromID(file_id);
	curfile->cpt+=n;
}
#define FileIncCPT(file_id) FileAddCPT(file_id,1)

/***
	FileGetCPT
	
	Get file counter information
	input: file_id
*/
int FileGetCPT(FILE *file_id)
{
	#undef FUNC
	#define FUNC "FileGetCPT"
	struct s_fileid *curfile;
	
	curfile=FileGetStructFromID(file_id);
	return curfile->cpt;
}
/***
	FileGetCPTFromName
	
	Get file counter information
	input: filename
*/
int FileGetCPTFromName(char *filename)
{
	#undef FUNC
	#define FUNC "FileGetCPTFromName"
	struct s_fileid *curfile;
	
	curfile=FileGetStructFromName(filename);
	if (!curfile)
	{
		logerr("You requested a counter for a file that is not opened! [%s]",filename);
		exit(INTERNAL_ERROR);
	}
	return curfile->cpt;
}

/***
	_fgetsXML
	
	This function is an fgets replacement to read XML file provided without carriage return
	Behaviour is the same as fgets, BUT carriage return are added to XML end tag
	
	future evolution: DATA segment management
*/
char *_internal_fgetsXML(char *buffer, int maxlen, FILE *f)
{
	#undef FUNC
	#define FUNC "_internal_fgetsXML"
	int idx=0,l=0,end=0,eol=0,trigger_end=0;
	int opentag=0;
	int cc,first=1;
	
	/*logdebug("do");*/
	
	do
	{
		cc=fgetc(f);
		while (first & (cc==0x0D || cc==0x0A)) cc=fgetc(f);
		first=0;
		
		if (cc==EOF)
			end=1;
		else
		if (cc==0x0D || cc==0x0A)
		{
			end=1;
			buffer[idx++]=0x0D;
			buffer[idx++]=0x0A;
			/*logdebug("(cc==0x0D || cc==0x0A)");*/
			eol=1;
			
			while (cc==0x0D || cc==0x0A) cc=fgetc(f);
		}
		else
		{
			/*logdebug("buffer[idx++]=%c (%d)",cc,cc);*/
			buffer[idx++]=cc;
			
			switch (cc)
			{
				case '<':
					/*logdebug("opentag=1");*/
					opentag=1;
					break;
				case '>':
					/*logdebug("opentag=0");*/
					opentag=0;
					if (trigger_end)
					{
						end=1;
						buffer[idx++]=0x0D;
						buffer[idx++]=0x0A;
					}
					break;
				case '/':
					/*logdebug("if (opentag) trigger_end=1; %s",opentag?"YES":"NO");*/
					if (opentag)
						trigger_end=1;
					break;
			}
		}
		
		if (idx>=MAX_LINE_BUFFER-3)
		{
			logerr("XML tag is too long! More than %d characters\n",MAX_LINE_BUFFER);
			exit(INTERNAL_ERROR);
		}		
	}
	while (!end);
	/*logdebug("while (!end);");*/

	buffer[idx]=0;
	
	/* EOF on first read char */
	if (cc==EOF && !idx)
		return NULL;
	else
		return buffer;
}

/***
	FileReadLine/FileReadLineXML function
	
	input:
	- filename

	output:
	- a static buffer with the line read
	
	this function can handle many file simultaneously
	just use different filenames without regarding to opened/closed pointer
	the opened handles are automatically closed when the end of the file is reached	
	you are only limited by the system, not by the code
	
	the XML version ass carriage returns after closing tag
	this can be usefull for parsing when reading XML file in a single line
*/
#define RAW_READING 0
#define XML_READING 1

char *_internal_fgetsmulti(char *filename, int read_mode)
{
	#undef FUNC
	#define FUNC "_internal_fgetsmulti"
	static char buffer[MAX_LINE_BUFFER+1]={0};
	FILE *last_id=NULL;
	char * (*_file_get_string)(char *, int, FILE *);
	
	last_id=FileOpen(filename,"r");
	
	switch (read_mode)
	{
		case RAW_READING:_file_get_string=fgets;break;
		case XML_READING:_file_get_string=_internal_fgetsXML;break;
		default:logerr("Unknown read mode! (%d)",read_mode);
	}	
	
	if (_file_get_string(buffer,MAX_LINE_BUFFER,last_id)!=NULL)
	{
		FileIncCPT(last_id);
		if (strnlen(buffer,MAX_LINE_BUFFER)==MAX_LINE_BUFFER)
		{
			logerr("line %d is too long! More than %d characters\n",FileGetCPT(last_id),MAX_LINE_BUFFER);
			exit(INTERNAL_ERROR);
		}
		return buffer;
	}
	else
	{
		/* End of file, we close the handle */
		logdebug("%d line(s) read from %s",FileGetCPT(last_id),filename);
		FileClose(last_id);
		return NULL;
	}
}
#define FileReadLine(filename) _internal_fgetsmulti(filename,RAW_READING)
#define FileReadLineXML(filename) _internal_fgetsmulti(filename,XML_READING)

/***

	FileWriteLine function
	
	input:
	- filename
	- a buffer with the line to write
	
	the function do not close the file until a NULL buffer is sent.
	It prevents from too much write flushes
	
	you can use it with many files simultaneously but do not forget
	to close your files if you use this function in a loop of filenames
	or the system will warn you
*/

void FileWriteLine(char *filename, char *buffer)
{
	#undef FUNC
	#define FUNC "FileWriteLine"
	FILE *last_id=NULL;
	
	last_id=FileOpen(filename,"a+");

	if (buffer!=NULL)
	{	
		fputs(buffer,last_id);
		FileIncCPT(last_id);
	}
	else
	{
		/* NULL buffer sent, this means End of file, we close the handle */
		logdebug("%d line(s) written to %s",FileGetCPT(last_id),filename);
		FileClose(last_id);
	}
}
#define FileWriteLineClose(filename) FileWriteLine(filename,NULL)

/***
	FileReadBinary function
	
	read n bytes into buffer
	return number of byte really read
	
	as other File functions, you can use it with many files simultaneously
*/
int FileReadBinary(char *filename,char *data,int n)
{
	#undef FUNC
	#define FUNC "FileReadBinary"
	FILE *last_id=NULL;
	int nn;
	
	last_id=FileOpen(filename,"r");

	if (data==NULL)
	{
		FileClose(last_id);
		return 0;
	}
	
	nn=fread(data,1,n,last_id);
	FileAddCPT(last_id,nn);
	if (nn!=n)
	{
		/* if eof is set, this is not an error */
		if (feof(last_id))
		{
			logdebug("%d byte(s) read from %s",FileGetCPT(last_id),filename);
		}
		else
		if (ferror(last_id))
		{
			if (!nn)
				logerr("cannot read %s",filename);
			else
				logerr("error during reading of %s",filename);
			exit(ABORT_ERROR);
		}
		else
		{
			logerr("error during read of %s (but no error neither eof flag set)",filename);
			exit(INTERNAL_ERROR);
		}				
		FileClose(last_id);
	}
	return nn;
}

/***
	FileWriteBinary function
	
	write n bytes from buffer to file
	return number of byte really written
	
	as other File functions, you can use it with many files simultaneously
*/
int FileWriteBinary(char *filename,char *data,int n)
{
	#undef FUNC
	#define FUNC "FileWriteBinary"
	FILE *last_id=NULL;
	int nn;
	
	last_id=FileOpen(filename,"a+");
	if (data!=NULL)
	{	
		nn=fwrite(data,1,n,last_id);
		FileAddCPT(last_id,nn);
		/* we must always write the same amount of data */
		if (n!=nn)
		{
			if (ferror(last_id))
			{
				if (!nn)
					logerr("cannot write %s",filename);
				else
					logerr("error during write of %s (%d byte(s))",filename,FileGetCPT(last_id));
				exit(ABORT_ERROR);
			}
			else
			{
				logerr("error during write of %s (but no error neither eof flag set) %d byte(s) written",filename,FileGetCPT(last_id));
				exit(INTERNAL_ERROR);
			}
		}
	}
	else
	{
		/* NULL buffer sent, this means End of file, we close the handle */
		logdebug("%d byte(s) written to %s",FileGetCPT(last_id),filename);
		FileClose(last_id);
	}
	return nn;
}
/*** macro to close binary files */
#define FileReadBinaryClose(filename) FileReadBinary(filename,NULL,0)
#define FileWriteBinaryClose(filename) FileWriteBinary(filename,NULL,0)


/***
	FileChmod
	
*/
void FileChmod(char *filename, mode_t zemode)
{
	#undef FUNC
	#define FUNC "FileChmod"

	if (!filename)
	{
		logerr("filename cannot be NULL!");
		exit(ABORT_ERROR);
	}
	/* checked after by the system but... */
	if (strnlen(filename,PATH_MAX)==PATH_MAX)
	{
		logerr("cannot chmod this file or directory because the argument size is bigger than PATH_MAX (%d)",PATH_MAX);
		exit(ABORT_ERROR);
	}
	
	logdebug("chmod file %s with rights %o",filename,zemode);
	if (chmod(filename,zemode)!=0)
	{
		logerr("file: %s mode:%o",filename,zemode);
		switch (errno)
		{
			case EACCES:logerr("Search permission is denied on a component of the path prefix.");break;
			case ELOOP:logerr("A loop exists in symbolic links encountered during resolution of the path argument.");break;
			case ENAMETOOLONG:logerr("The length of the path argument exceeds maximum path value or a pathname component is longer than name maximum value.");break;
			case ENOTDIR:logerr("A component of the path prefix is not a directory.");break;
			case ENOENT:logerr("A component of path does not name an existing file or path is an empty string.");break;
			case EPERM:logerr("The effective user ID does not match the owner of the file and the process does not have appropriate privileges.");break;
			case EROFS:logerr("The named file resides on a read-only file system.");break;
			case EINTR:logerr("A signal was caught during execution of the function.");break;
			case EINVAL:logerr("The value of the mode argument is invalid.");break;
			default:logerr("Unknown error %d during chmod: %s",errno,strerror(errno));
		}
		exit(ABORT_ERROR);
	}	
}
/***
	MakeDirWithRights
	
	to simplify directories creation management, we override the umask
*/
void MakeDirWithRights(char *dirname, mode_t zemode)
{
	#undef FUNC
	#define FUNC "MakeDirWithRights"
	
	if (!dirname)
	{
		logerr("directory name must not be NULL");
		exit(ABORT_ERROR);
	}
	/* apparently not checked by the system so... */
	if (strnlen(dirname,PATH_MAX)==PATH_MAX)
	{
		logerr("cannot create this directory because the argument size is bigger than PATH_MAX (%d)",PATH_MAX);
		exit(ABORT_ERROR);
	}
	
	logdebug("make directory %s with rights %o",dirname,zemode);
	if (mkdir(dirname,zemode)!=0)
	{
		logerr("dir: %s mode:%o",dirname,zemode);
		switch (errno)
		{
			case EACCES:logerr("Write permission is denied for the parent directory in which the new directory is to be added.");break;
			case EEXIST:logerr("An entry named %s already exists.",dirname);break;
			case EMLINK:logerr("The parent directory has too many links.");break;
			case ENOSPC:logerr("The file system doesn't have enough room to create the new directory.");break;
			case EROFS:logerr("The parent directory of the directory being created is on a read-only file system and cannot be modified.");break;
			default:logerr("Unknown error %d during mkdir: %s",errno,strerror(errno));
		}
		exit(ABORT_ERROR);
	}
	/* because i do not want to deal with umask for this function */
	FileChmod(dirname,zemode);
}
#define MakeDir(dirname) MakeDirWithRights(dirname,S_IRWXU|S_IRWXG|S_IRWXO);



/***
	FileGetStat
	
	Get file statistics (size, timestamp, etc.)

struct stat {
    dev_t     st_dev;      ID of device containing file 
    ino_t     st_ino;      inode number 
    mode_t    st_mode;     protection 
    nlink_t   st_nlink;    number of hard links 
    uid_t     st_uid;      user ID of owner 
    gid_t     st_gid;      group ID of owner 
    dev_t     st_rdev;     device ID (if special file) 
    off_t     st_size;     total size, in bytes 
    blksize_t st_blksize;  blocksize for filesystem I/O 
    blkcnt_t  st_blocks;   number of blocks allocated 
    time_t    st_atime;    time of last access 
    time_t    st_mtime;    time of last modification 
    time_t    st_ctime;    time of last status change 
};

S_IFMT  0170000  bitmask for the file type bitfields  
S_IFSOCK  0140000  socket  
S_IFLNK  0120000  symbolic link  
S_IFREG  0100000  regular file  
S_IFBLK  0060000  block device  
S_IFDIR  0040000  directory  
S_IFCHR  0020000  character device  
S_IFIFO  0010000  FIFO  
S_ISUID  0004000  set UID bit  
S_ISGID  0002000  set-group-ID bit (see below)  
S_ISVTX  0001000  sticky bit (see below)  
S_IRWXU  00700  mask for file owner permissions  
S_IRUSR  00400  owner has read permission  
S_IWUSR  00200  owner has write permission  
S_IXUSR  00100  owner has execute permission  
S_IRWXG  00070  mask for group permissions  
S_IRGRP  00040  group has read permission  
S_IWGRP  00020  group has write permission  
S_IXGRP  00010  group has execute permission  
S_IRWXO  00007  mask for permissions for others (not in group)  
S_IROTH  00004  others have read permission  
S_IWOTH  00002  others have write permission  
S_IXOTH  00001  others have execute permission 
*/

/***
	FileGetStat
	
	input: filename
	return the stat structure of a given file
*/
struct stat *FileGetStat(char *filename)
{
	#undef FUNC
	#define FUNC "FileGetStat"
	struct stat *filestat;

	if (!filename)
	{
		logerr("filename must not be NULL");
		exit(ABORT_ERROR);
	}
	/* check after by the system but... */
	if (strnlen(filename,PATH_MAX)==PATH_MAX)
	{
		logerr("cannot open this file because the argument size is bigger than PATH_MAX (%d)",PATH_MAX);
		logerr("[%s]",filename);
		exit(ABORT_ERROR);
	}
	
	filestat=MemMalloc(sizeof(struct stat));
	if (stat(filename,filestat))
	{
		logerr("stat %s failed",filename);
		switch (errno)
		{
			case EACCES:logerr("Search permission is denied for one of the directories in the path prefix of path.");break;
			case EBADF:logerr("filedes is bad.");break;
			case EFAULT:logerr("Bad address.");break;
			case ELOOP:logerr("Too many symbolic links encountered while traversing the path.");break;
			case ENAMETOOLONG:logerr("File name too long.");break;
			case ENOENT:logerr("A component of the path path does not exist, or the path is an empty string.");break;
			case ENOMEM:logerr("Out of memory (i.e. kernel memory).");break;
			case ENOTDIR:logerr("A component of the path is not a directory.");break;
			default:logerr("Unknown error %d during stat: %s",errno,strerror(errno));
		}
		exit(ABORT_ERROR);
	}
	return filestat;
}

/***
	FileGetSize
	
	input: filename
	output: the size in bytes of the file
*/
long long FileGetSize(char *filename)
{
	#undef FUNC
	#define FUNC "FileGetSize"
	struct stat *filestat;
	long long nn;
	
	filestat=FileGetStat(filename);
	nn=filestat->st_size;
	MemFree(filestat);
	logdebug("size of %s = %d (%dkb)",filename,nn,nn/1024);
	return nn;
}
/***
	FileGetMode
	
	input: filename
	output: the mode_t structure of the file
*/
mode_t FileGetMode(char *filename)
{
	#undef FUNC
	#define FUNC "FileGetMode"
	struct stat *filestat;
	mode_t nn;
	
	filestat=FileGetStat(filename);
	nn=filestat->st_mode;
	MemFree(filestat);
	return nn;
}
/***
	macro to retrieve some commons informations
	
	file is a socket
	file is a link
	file is regular
	file is a block device
	file is a dir
	file is a character device
	file is a fifo
*/
#define FileIsSocket(filename) ((FileGetMode(filename) & S_IFSOCK)==S_IFSOCK)
#define FileIsLink(filename) ((FileGetMode(filename) & S_IFLNK)==S_IFLNK)
#define FileIsRegular(filename) ((FileGetMode(filename) & S_IFREG)==S_IFREG)
#define FileIsBlockDevice(filename) ((FileGetMode(filename) & S_IFBLK)==S_IFBLK)
#define FileIsDir(filename) ((FileGetMode(filename) & S_IFDIR)==S_IFDIR)
#define FileIsCharacterDevice(filename) ((FileGetMode(filename) & S_IFCHR)==S_IFCHR)
#define FileIsFifo(filename) ((FileGetMode(filename) & S_IFIFO)==S_IFIFO)

/***
	FileGetUID
	
	input: filename
	output: the uid of the file
*/
uid_t FileGetUID(char *filename)
{
	#undef FUNC
	#define FUNC "FileGetUID"
	struct stat *filestat;
	uid_t nn;
	
	filestat=FileGetStat(filename);
	nn=filestat->st_uid;
	MemFree(filestat);
	return nn;
}

/***
	FileGetGID
	
	input: filename
	output: the gid of the file
*/
gid_t FileGetGID(char *filename)
{
	#undef FUNC
	#define FUNC "FileGetGID"
	struct stat *filestat;
	gid_t nn;
	
	filestat=FileGetStat(filename);
	nn=filestat->st_gid;
	MemFree(filestat);
	return nn;
}


/*****************  XML related functions *****************/

struct s_xml_field
{
	char *xml_name;
	char *xml_value;
	
	struct s_xml_field **child;
	struct s_xml_field *parent;
	int nb_child;
	int opened_marker;
	int auto_closed;
};

/***
	XMLDumpTreeLinear
	
	follow an XML tree and display all fields, with identation
	this function does not use the stack, it can handle huge structures
*/
void XMLDumpTreeLinear(struct s_xml_field *zefield)
{
	#undef FUNC
	#define FUNC "XMLDumpTreeLinear"
	struct s_xml_field *zechild=NULL;
	int i,fini=0,zetab=0;

	while (!fini)
	{
		if (zetab<0)
		{
			logerr("zetab<0 it must not happened!");
			exit(INTERNAL_ERROR);
		}
		
		/* zechild==NULL, this is the first call for this field */	
		if (zechild==NULL)
		{
			loginfo("%*.*s %s=[%s]",zetab,zetab,"",zefield->xml_name,zefield->xml_value);
			if (zefield->nb_child)
			{
				/*XMLDumpTreeLinear(zefield->child[0],NULL,zetab+1);*/
				zefield=zefield->child[0];
				zechild=NULL;
				zetab++;
				continue;
			}
		}
				
		for (i=0;i<zefield->nb_child;i++)
			if (zefield->child[i]==zechild)
				break;
		i++;
		if (i<zefield->nb_child)
		{
				zefield=zefield->child[i];
				zechild=NULL;
				zetab++;
				continue;
		}
		else
		{
			if (zefield->parent)
			{
				/*XMLDumpTreeLinear(zefield->parent,zefield,zetab-1);*/
				zechild=zefield;
				zefield=zefield->parent;
				zetab--;
				continue;
			}
			else
				fini=1;
		}
	}
}
/***
	XMLDumpTreeRecurse
	
	follow an XML tree and display all fields, with identation
*/
void XMLDumpTreeRecurse(struct s_xml_field *zefield,int zetab)
{
	#undef FUNC
	#define FUNC "XMLDumpTreeRecurse"
	int i;
	
	loginfo("%*.*s %s=[%s](%d)",zetab,zetab,"",zefield->xml_name,zefield->xml_value,zefield->opened_marker);
	
	for (i=0;i<zefield->nb_child;i++)
		XMLDumpTreeRecurse(zefield->child[i],zetab+1);
}
/*#define XMLDumpTree(zefield) XMLDumpTreeRecurse(zefield,0)*/
#define XMLDumpTree(zefield) XMLDumpTreeLinear(zefield)

/***
	XMLSetField
	
	put a name and a value in a XML field structure
	
	input: field structure
	       name
	       text value
*/
void XMLSetField(struct s_xml_field *zefield,char *zename, char *zevalue)
{
	#undef FUNC
	#define FUNC "XMLSetField"

	MemFree(zefield->xml_name);
	MemFree(zefield->xml_value);
	zefield->xml_name=TxtStrDup(zename);
	zefield->xml_value=TxtStrDup(zevalue);
}

/***
	XMLSetField
	
	set the state of a XML field (open/closed)
	
	input: field structure
	       state (0 or 1)
*/
void XMLSetFieldState(struct s_xml_field *zefield, int state)
{
	#undef FUNC
	#define FUNC "XMLSetField"

	zefield->opened_marker=state;
}
/*** macro to open and close a field */
#define XMLOpenField(zefield) XMLSetFieldState(zefield,1)
#define XMLCloseField(zefield) XMLSetFieldState(zefield,0)

/*** 
	XMLCreateField
	
	create a new XML field structure, featuring a name and a text value

*/
struct s_xml_field *XMLCreateField(char *zename, char *zevalue)
{
	#undef FUNC
	#define FUNC "XMLCreateField"
	
	struct s_xml_field *curfield;
	
	curfield=MemCalloc(sizeof(struct s_xml_field));
	
	curfield->xml_name=TxtStrDup(zename);
	curfield->xml_value=TxtStrDup(zevalue);
	curfield->child=NULL;
	curfield->parent=NULL;
	curfield->nb_child=0;
	curfield->opened_marker=1;
	
	return curfield;
}
/*** macro to duplicate a XML field */
#define XMLDuplicateField(zefield) XMLCreateField(zefield->xml_name,zefield->xml_value)

/***
	XMLFreeField
	
	free the memory of a XML field
	
	input: XML field structure
*/
void XMLFreeField(struct s_xml_field *xml_field)
{
	#undef FUNC
	#define FUNC "XMLFreeField"
	
	int i;
	
	/* free all childs */
	for (i=0;i<xml_field->nb_child;i++)
		XMLFreeField(xml_field->child[i]);
	if (i)
		MemFree(xml_field->child);
		
	MemFree(xml_field->xml_name);
	MemFree(xml_field->xml_value);
	MemFree(xml_field);
}
/***
	XMLAddChild
	
	add a XML child to another XML field
	
	input: XML field structure of the parent
	       XML field structure of the child
*/
void XMLAddChild(struct s_xml_field *parent,struct s_xml_field *child)
{
	#undef FUNC
	#define FUNC "XMLAddChild"
	
	parent->child=MemRealloc(parent->child,sizeof(struct s_xml_field *)*(parent->nb_child+1));
	parent->child[parent->nb_child]=child;
	parent->nb_child++;
	
	child->parent=parent;
}
/***
	XMLDeleteChild
	
	delete a child from his parent

	input: XML field structure of the parent
	       XML field structure of the child
*/
void XMLDeleteChild(struct s_xml_field *parent,struct s_xml_field *child)
{
	#undef FUNC
	#define FUNC "XMLDeleteChild"
	
	int i,ok=0;
	
	for (i=0;i<parent->nb_child;i++)
		if (parent->child[i]==child)
		{
			XMLFreeField(child);
			parent->nb_child--;
			if (i!=parent->nb_child)
				parent->child[i]=parent->child[parent->nb_child];
			parent->child=MemRealloc(parent->child,sizeof(struct s_xml_field *)*(parent->nb_child+1));
			ok=1;
		}
	if (!ok)
	{
		logerr("XML child was not found");
		logerr("parent: <%s>%s</%s>",parent->xml_name,parent->xml_value,parent->xml_name);
		logerr("child : <%s>%s</%s>",child->xml_name,child->xml_value,child->xml_name);
		exit(INTERNAL_ERROR);
	}	
}

/***
	XMLPurgeMarker

	this function delete every XML marker of a string and
	replace markers by spaces then feature a full trim of the string
	
	example	before: <cursorx>pouet 56</cursorx>gnagna<plop><xml=youpi>glapi</plop>
	example after : pouet 56 gnagna glapi
	
	input: a string
	output: the pointer to this string, modified
*/
char *XMLPurgeMarker(char *in_str)
{
	#undef FUNC
	#define FUNC "XMLPurgeMarker"
	
	int idx,l;
	int marker;
	
	idx=0;
	l=strlen(in_str);
	while (in_str[idx]!=0)
	{
		if (in_str[idx]=='<')
			marker=1;

		if (marker)
		{
			if (in_str[idx]=='>')
				marker=0;
			in_str[idx]=' ';
		}
		idx++;
	}
	TxtTrim(in_str);
	return in_str;
}

/***
	XMLLookForFieldByName
	
	return the first child of a XML root with the requested name
	
	input: XML field structure of the root
	       XML name of the field
*/
struct s_xml_field *XMLLookForFieldByName(struct s_xml_field *zefield, char *xmlname)
{
	#undef FUNC
	#define FUNC "XMLLookForFieldByName"
	
	int idx;
	
	for (idx=0;idx<zefield->nb_child;idx++)
		if (!strcmp(zefield->child[idx]->xml_name,xmlname))
			return zefield->child[idx];
		else
		if ((zefield=XMLLookForFieldByName(zefield->child[idx],xmlname))!=NULL)
			return zefield;
	return NULL;
}

/***
	XMLLookForFieldByValue
	
	return the first child of a XML root with the requested value
*/
struct s_xml_field *XMLLookForFieldByValue(struct s_xml_field *zefield, char *xmlvalue)
{
	#undef FUNC
	#define FUNC "XMLLookForFieldByValue"
	
	int idx;
	
	for (idx=0;idx<zefield->nb_child;idx++)
		if (!strcmp(zefield->child[idx]->xml_value,xmlvalue))
			return zefield->child[idx];
		else
		if ((zefield=XMLLookForFieldByValue(zefield->child[idx],xmlvalue))!=NULL)
			return zefield;
	return NULL;
}

/***
	XMLLoadFile

	load an entire xml file into a XML tree
	
	input: name of the xml file
	output: xml root structure
*/
struct s_xml_field *XMLLoadFile(char *filename)
{
	#undef FUNC
	#define FUNC "XMLLoadFile"
	
	struct s_xml_field *xmlroot,*curfield,*child;
	char value_buffer[MAX_LINE_BUFFER];
	char marker_buffer[512];
	char *buffer;
	int mdx,idx,start,end;
	
	xmlroot=XMLCreateField("XML_ROOT","");
	xmlroot->auto_closed=1;
	curfield=xmlroot;
	
	while ((buffer=FileReadLineXML(filename))!=NULL)
	{
		idx=0;
		end=strlen(buffer);
		while (idx<end)
		{
			/* look for marker, or fill the current xml field */
			start=idx;
			while (idx<end && buffer[idx]!='<') idx++;
			
			strncpy(value_buffer,buffer+start,idx-start);
			value_buffer[idx-start]=0;
			TxtTrimEnd(value_buffer);
			
			/*loginfo("value_buffer=[%s]",value_buffer);*/
			/* already filled by data, we must concat */
			if (curfield->xml_value[0]!=0)
			{
				if (strlen(value_buffer))
				{
					/*loginfo("concat [%s] into [%s](%s)",value_buffer,curfield->xml_name,curfield->xml_value);*/
					curfield->xml_value=MemRealloc(curfield->xml_value,strlen(curfield->xml_value)+2+idx-start);
					strcat(curfield->xml_value," ");
					strcat(curfield->xml_value,TxtTrimEnd(value_buffer));
				}
			}
			else
			{
				/*loginfo("copy [%s] into [%s](%s)",value_buffer,curfield->xml_name,curfield->xml_value);*/
				curfield->xml_value=MemRealloc(curfield->xml_value,strlen(value_buffer)+1);
				strcpy(curfield->xml_value,value_buffer);
			}
		
			/* closing marker */
			if (buffer[idx]=='<' && buffer[idx+1]=='/')
			{
				idx+=2;
				mdx=0;
				/* no extra parameters on closing marker */
				while (buffer[idx]!='>' && idx<end)
					marker_buffer[mdx++]=buffer[idx++];
				idx++;
				marker_buffer[mdx]=0;
				/*loginfo("closing [%s]",marker_buffer);*/
				
				if (strcmp(curfield->xml_name,marker_buffer))
				{
					logerr("cannot close the marker [%s] within [%s]",marker_buffer,curfield->xml_name);
					exit(ABORT_ERROR);
				}
				/* close field then switch back to his parent (or root) */
				XMLCloseField(curfield);
				curfield=curfield->parent;
			}
			else
			if (buffer[idx]=='<')
			{
				/* at this point, this may be an opening marker or an auto-closing marker */
				idx++;
				mdx=0;
				while (buffer[idx]!='>' && buffer[idx]!=' ' && idx<end)
					marker_buffer[mdx++]=buffer[idx++];
				marker_buffer[mdx]=0;
				/* to jump over marker parameters */
				while (buffer[idx]!='>' && idx<end)
					idx++;
				idx++;
				
				/* auto-closing marker */	
				if (strstr(marker_buffer,"/"))
				{
					/*loginfo("auto-closing [%s]",marker_buffer);*/
					TxtReplace(marker_buffer,"/","",0);
					child=XMLCreateField(marker_buffer,"");
					XMLCloseField(child);
					child->auto_closed=1;
					XMLAddChild(curfield,child);
				}
				else /* opening marker */
				{
					/*loginfo("opening [%s]",marker_buffer);*/
					child=XMLCreateField(marker_buffer,"");
					XMLAddChild(curfield,child);
					curfield=child;
				}			
			}
			else
			if (idx<end)
			{
				logerr("must not reach this point, surely an internal error processing...");
				exit(INTERNAL_ERROR);
			}
		}
	}
	
	/* the last field must be the xmlroot */
	if (curfield!=xmlroot)
	{
		logerr("this XML document is invalid (probably unclosed markers)");
		exit(ABORT_ERROR);
	}	
	
	XMLCloseField(curfield);
	return xmlroot;
}	

/***
	XMLWriteFile
	
	follow a XML tree and write it to a file
	this function does not use the stack, it can handle huge structures
*/
void XMLWriteFile(char *filename,struct s_xml_field *zefield)
{
	#define XML_INTERNAL_TAB_SPACES 3
	#undef FUNC
	#define FUNC "XMLWriteFile"
	struct s_xml_field *xmlroot=zefield;
	struct s_xml_field *zechild=NULL;
	int i,fini=0,zetab=0,first=1;
 
	while (!fini)
	{
		if (zetab<0)
		{
			logerr("zetab<0 it must not happened!");
			exit(INTERNAL_ERROR);
		}
		
		/* zechild==NULL, this is the first call for this field */	
		if (zechild==NULL)
		{
			if (zefield->auto_closed)
				_internal_put2file(filename,"%*.*s<%s/>",zetab,zetab,"",zefield->xml_name);
			else
				_internal_put2file(filename,"%*.*s<%s>%s",zetab,zetab,"",zefield->xml_name,zefield->xml_value);
			
			if (zefield->nb_child)
			{
				zefield=zefield->child[0];
				zechild=NULL;
				if (!first)
					zetab+=XML_INTERNAL_TAB_SPACES;
				first=0;
				continue;
			}
		}
				
		for (i=0;i<zefield->nb_child;i++)
			if (zefield->child[i]==zechild)
				break;
		i++;
		if (i<zefield->nb_child)
		{
				zefield=zefield->child[i];
				zechild=NULL;
				zetab+=XML_INTERNAL_TAB_SPACES;
				continue;
		}
		else
		{
			if (zefield->parent)
			{
				_internal_put2file(filename,"%*.*s</%s>",zetab,zetab,"",zefield->xml_name);
				zechild=zefield;
				zefield=zefield->parent;
				if (zefield!=xmlroot)
					zetab-=XML_INTERNAL_TAB_SPACES;
				continue;
			}
			else
				fini=1;
		}
	}
	_internal_put2fileclose(filename);
}



/***
	ParameterParseFieldList
	input : a string representing a list of fields	
	this may be 1,2,3,7,5,12 (only positives numbers) but also
	an interval like 6-24 (ascending or descending) or a mix like 5,20-15,2
	
	output: an allocated integer array of fields, terminated by -1 value
*/
int *ParameterParseFieldList(char *fieldlist)
{
	#undef FUNC
	#define FUNC "ParameterParseFieldList"
	static int terminator=-1;
	char *buffer,*fieldbuffer;
	int cl,cpt,i,startidx,endidx;
	int *field;

	if (fieldlist==NULL)
	{
		field=MemMalloc(sizeof(int));
		field[0]=-1;
		return field;
	}	
	
	/* parse fieldlist */
	buffer=fieldlist;
	/* +1 cause we need the ending character to get the last field*/
	fieldbuffer=buffer;
	cl=strlen(buffer)+1;
	cpt=1;
	i=0;
	while (i<cl)
	{
		if (buffer[i]=='-')
		{
			buffer[i]=0;
			startidx=atoi(fieldbuffer);
			buffer[i]='-';
			fieldbuffer=&buffer[i+1];
			i++;
			while (buffer[i]!=',' && buffer[i]!=0 && buffer[i]!=0x0D && buffer[i]!=0x0A) i++;
			buffer[i]=0;
			endidx=atoi(fieldbuffer);
			if (endidx<startidx)
				cpt+=startidx-endidx+1;
			else
				cpt+=endidx-startidx+1;
			
			fieldbuffer=&buffer[i+1];
		}
		
		if (buffer[i]==',')
		{
			cpt++;
			fieldbuffer=&buffer[i+1];
		}
		i++;
	}
	field=MemMalloc((cpt+1)*sizeof(int));
	
	buffer=fieldlist;
	fieldbuffer=buffer;
	i=cpt=0;
	while (i<cl)
	{
		if (buffer[i]=='-')
		{
			buffer[i]=0;
			startidx=atoi(fieldbuffer);
			fieldbuffer=&buffer[i+1];
			i++;
			while (buffer[i]!=',' && buffer[i]!=0 && buffer[i]!=0x0D && buffer[i]!=0x0A) i++;
			buffer[i]=0;
			endidx=atoi(fieldbuffer);
			if (endidx<startidx)
				while (startidx>=endidx)
				{
					field[cpt++]=startidx;
					startidx--;
				}
			else
				while (startidx<=endidx)
				{
					field[cpt++]=startidx;
					startidx++;
				}
			fieldbuffer=&buffer[i+1];
		}
		else
		if (buffer[i]==',' || buffer[i]==0 || buffer[i]==0x0D || buffer[i]==0x0A)
		{
			buffer[i]=0;
			field[cpt]=atoi(fieldbuffer);
			fieldbuffer=&buffer[i+1];
			cpt++;
		}
		i++;
	}
	/* end of field list */
	field[cpt]=-1;
	
	return field;
}
#define ParameterFreeFieldList(fieldlist) MemFree(fieldlist)

/***
	CalcArrayMax
	input: 
	- array of integer
	- size of the array. If null then run until it gets -1 value
	output
	- maximum value in the array (always positive for the array terminated by -1)
*/
int CalcArrayMax(int *myarray, int asize)
{
	#undef FUNC
	#define FUNC "CalcArrayMax"

	int i,maxarray;
	
	if (asize==0)
	{
		maxarray=myarray[0];
		i=1;
		while (myarray[i]!=-1)
		{
			if (maxarray<myarray[i])
				maxarray=myarray[i];
			i++;
		}
	}
	else
	{
		maxarray=myarray[0];
		for (i=1;i<asize;i++)
			if (maxarray<myarray[i])
				maxarray=myarray[i];
	}
	return maxarray;
}

/***
	CalcArrayElem
	input: 
	- array of integer terminated by -1
	output
	- number of elem of the array
*/
int CalcArrayElem(int *myarray)
{
	#undef FUNC
	#define FUNC "CalcArrayElem"

	int cpt=0;
	
	while (myarray[cpt]!=-1)
		cpt++;
	return cpt;
}



/***
	CSV fields parsing functions

	get number of fields in a line
	to split read line into field
*/

int CSVGetFieldNumber(char *buffer, char separator)
{
	#undef FUNC
	#define FUNC "CSVGetFieldNumber"
	
	int i,l,numfield=1;
	
	l=strnlen(buffer,MAX_LINE_BUFFER);
	i=0;
	
	while (i<l && buffer[i]!=0x0D && buffer[i]!=0x0A)
	{
		while (buffer[i]!=separator && i<l && buffer[i]!=0x0D && buffer[i]!=0x0A) i++;
		if (buffer[i]==separator)
			numfield++;
		i++;
	}
	return numfield;
}
/***
	CSVGetAllFields
	
	return an array of strings after parsing a buffer with separator
	
	input: buffer (not modified by the function)
	       separator
	       
*/
char **CSVGetAllFields(char *backbuffer, char separator)
{
	#undef FUNC
	#define FUNC "CSVGetAllFields"

	char **curcolumn;
	char *colbuffer;
	char *buffer;
	int i,l,cl,ncol,icol,sep;
	
	buffer=TxtStrDup(backbuffer);
	
	l=strnlen(buffer,MAX_LINE_BUFFER);
	i=0;
	
	ncol=CSVGetFieldNumber(buffer,separator);
	if (!ncol)
	{
		logwarn("The line you read was empty! This may cause some trouble...");
		return NULL;
	}
	curcolumn=MemCalloc((ncol+1)*sizeof(char*));
	
	icol=0;
	while (i<l && buffer[i]!=0x0D && buffer[i]!=0x0A)
	{
		colbuffer=&buffer[i];
		sep=0;
		while (buffer[i]!=separator && i<l && buffer[i]!=0x0D && buffer[i]!=0x0A) i++;
		if (buffer[i]==separator)
			sep=1;
		buffer[i]=0;
		/*
		cl=strlen(colbuffer);
		curcolumn[icol]=MemCalloc(cl+1);
		strcpy(curcolumn[icol],colbuffer);
		*/
		curcolumn[icol++]=TxtStrDup(colbuffer);
		i++;
	}
	/* separator on the last character, we must add an empty field */
	if (sep)
		curcolumn[icol++]=TxtStrDup("");	
	curcolumn[icol]=NULL;
	MemFree(buffer);
	return curcolumn;
}

/***
	CSVGetFieldArrayNumber
	
	count array elements
*/
int CSVGetFieldArrayNumber(char **myfield)
{
	#undef FUNC
	#define FUNC "CSVGetFieldArrayNumber"
	
	int n=0;
	
	if (myfield==NULL)
		return 0;
	
	while (myfield[n]!=NULL) n++;
	
	return n;

}
/***
	Add a value to a list of strings
	Reallocate memory on the fly
	
	input: pointer to an array of string
	       string value
	       
	output: the pointer to the array is modified because the
	function reallocate memory to store the new string at
	the end of the array.
*/
void _internal_FieldArrayAddDynamicValue(char ***zearray, char *zevalue, int curline, char *curfunc, char *cursource)
{
	#undef FUNC
	#define FUNC "FieldArrayAddDynamicValue"
	int nbfield;
	if ((*zearray)==NULL)
		(*zearray)=MemCalloc(sizeof(char *));
	nbfield=CSVGetFieldArrayNumber((*zearray))+2;
	/* using direct calls because it is more interresting to know which is the caller */
	(*zearray)=_MemRealloc((*zearray),nbfield*sizeof(char *),cursource,curline,curfunc);
	(*zearray)[nbfield-2]=_internal_strdup(zevalue,cursource,curline,curfunc);
	(*zearray)[nbfield-1]=NULL;
}
#define FieldArrayAddDynamicValue(zearray,zevalue) _internal_FieldArrayAddDynamicValue(zearray,zevalue,__LINE__,FUNC,__FILENAME__)

/***
	CSVFreeFields

	free allocated memory for fields
*/
void CSVFreeFields(char **fields)
{
	#undef FUNC
	#define FUNC "CSVFreeFields"
	
	int i=0;
	if (fields!=NULL)
	{
		/*loginfo("%8X",fields);
		if (fields[i]==NULL)
			loginfo("NULL i=%d",i); */
		
		while (fields[i]!=NULL)
		{
			MemFree(fields[i]);
			i++;
		}
		MemFree(fields);
	}
}
void FreeArrayDynamicValue(char ***zearray)
{
	CSVFreeFields(*zearray);
	*zearray=NULL;
}
#define FreeFields(fields) CSVFreeFields(fields)


/***
	GetTxtDate
	
	input: time offset in second
	output: date in DD.MM.YYYY;HHhMMmSSs format
*/
char *GetTxtDate(long dek)
{
	#undef FUNC
	#define FUNC "GetTxtDate"
	
	char *txtdate;
	long l_time;
	struct tm *timestruct;
	
	txtdate=MemMalloc(21);
  	l_time = time(&dek);
	timestruct = localtime( &l_time);
  
	sprintf(txtdate,"%02d.%02d.%d;%02dh%02dm%02ds"       ,
                                     timestruct->tm_mday       ,
                                     timestruct->tm_mon+1      ,
                                     timestruct->tm_year+1900  ,
                                     timestruct->tm_hour       ,
                                     timestruct->tm_min        ,
                                     timestruct->tm_sec);
	return txtdate;
}

/***
	GetTxtCurrentDate
	
	output: current date in DD.MM.YYYY;HHhMMmSSs format
*/
char *GetTxtCurrentDate()
{
	#undef FUNC
	#define FUNC "GetTxtCurrentDate"
	
	return GetTxtDate(0);
}

/***
	CheckDateFormat

	check that a date is well formated	
	input: a date, formated DD.MM.YYYY;HHhMMmSSs
	output: 0 if NOK, 1 if OK
*/

#define IS_LEAP_YEAR(year) (!(year % 4) && ((year % 100) || !(year % 400)))
#define A_LEAP_YEAR (366*24*3600)
#define A_YEAR (365*24*3600)

int CheckDateFormat(char *curdate)
{
	#undef FUNC
	#define FUNC "CheckDateFormat"
	
	char *wdate;
	int ddiff,bi;
	int day,month,year,hour,minute,second;
	
	/* check for digit */
	if (strlen(curdate)!=20) {logerr("wrong date format (must be 20 characters) [%s]",curdate);return 0;}
	
	if (!isdigit(curdate[0]) || !isdigit(curdate[1])
		|| !isdigit(curdate[3]) || !isdigit(curdate[4])
		|| !isdigit(curdate[6]) || !isdigit(curdate[7]) || !isdigit(curdate[8]) || !isdigit(curdate[9])
		|| !isdigit(curdate[11]) || !isdigit(curdate[12])
		|| !isdigit(curdate[14]) || !isdigit(curdate[15])
		|| !isdigit(curdate[17]) || !isdigit(curdate[18]))
		{logerr("wrong date format (missing digits) [%s]",curdate);return 0;}

	if (isdigit(curdate[2]) || isdigit(curdate[5]) || isdigit(curdate[10]) || isdigit(curdate[13]) || isdigit(curdate[16]) || isdigit(curdate[19]))
		{logerr("wrong date format (avoid digits out of the date mask DD.MM.YYYY hh.mm.ss.) [%s]",curdate);return 0;}
	
	wdate=TxtStrDup(curdate);
	wdate[2]=wdate[5]=wdate[10]=wdate[13]=wdate[16]=wdate[19]=0;
	day=atoi(&wdate[0]);
	month=atoi(&wdate[3]);
	year=atoi(&wdate[6]);
	hour=atoi(&wdate[11]);
	minute=atoi(&wdate[14]);
	second=atoi(&wdate[17]);
	MemFree(wdate);
	
	/* check day & month */
	switch (month)
	{
		case 1:
		case 3:
		case 5:
		case 7:
		case 8:
		case 10:
		case 12:
			if (day>31) {logerr("wrong date format (31 days max) [%s]",curdate);return 0;}
			break;
		case 2:
			if IS_LEAP_YEAR(year)
			{
				if (day>29) {logerr("wrong date format (29 days max this year) [%s]",curdate);return 0;}
			}
			else
			{
				if (day>28) {logerr("wrong date format (28 days max this year) [%s]",curdate);return 0;}
			}
			break;
		case 4:
		case 6:
		case 9:
		case 11:
			if (day>30) {logerr("wrong date format (30 days max) [%s]",curdate);return 0;}
			break;
		default:
			logerr("wrong date format (month must be between 1 and 12, included) [%s]",curdate);
			return 0;
	}
	
	/* check time */
	if (hour>24 || minute>60 || second>60) {logerr("wrong date format (time is incorrect) [%s]",curdate);return 0;}
	
	return 1;
}

/***
	ConvertDate
	
	input: A date in an unknown format
	       the mask of this date
	output: same date, with the output mask
*/
char *ConvertDate(char *rawdate,char *input_mask, char *output_mask)
{
	#undef FUNC
	#define FUNC "ConvertDate"

	char newdate[256];
	char tmptext[16];
	char tmpdate[21];
	int day,month,year,hour,minute,second;
	int i,l;
	
	/* may be blank */
	day=month=year=0;
	hour=minute=second=0;
	
	l=strlen(input_mask);
	i=0;
	while (i<l)
	{
		/* DD MM YYYY hh mm ss */
		/* YY DD MM */
		switch (input_mask[i])
		{
			case 'D':day=day*10+rawdate[i]-'0';break;
			case 'M':month=month*10+rawdate[i]-'0';break;
			case 'Y':year=year*10+rawdate[i]-'0';break;
			case 'h':hour=hour*10+rawdate[i]-'0';break;
			case 'm':minute=minute*10+rawdate[i]-'0';break;
			case 's':second=second*10+rawdate[i]-'0';break;
			default:break;
		}
		i++;		
	}
	if (year<100) year+=2000;
	
	sprintf(tmpdate,"%02d %02d %04d %02d %02d %02d ",day,month,year,hour,minute,second);
	CheckDateFormat(tmpdate);

	l=strlen(output_mask);
	strcpy(newdate,"");
	i=0;
	while (i<l)
	{
		if (output_mask[i]=='%')
		{
			switch (output_mask[++i])
			{
				case 'D':sprintf(tmptext,"%02d",day);break;
				case 'M':sprintf(tmptext,"%02d",month);break;
				case 'Y':sprintf(tmptext,"%04d",year);break;
				case 'y':sprintf(tmptext,"%02d",year-2000);break;
				case 'h':sprintf(tmptext,"%02d",hour);break;
				case 'm':sprintf(tmptext,"%02d",minute);break;
				case 's':sprintf(tmptext,"%02d",second);break;
				case '%':sprintf(tmptext,"%%");break;
				default:logerr("unsupported date output format: you can use %%D %%M %%Y %%y %%h %%m %%s %%%%");
					exit(ABORT_ERROR);
			}
		}
		else
			sprintf(tmptext,"%c",output_mask[i]);
		
		strcat(newdate,tmptext);
		i++;
	}
	return TxtStrDup(newdate);
}


/***
	GetDiffDate
	
	input: two dates, formated DD.MM.YYYY;HHhMMmSSs
	output: difference in seconds
*/
int GetDiffDate(char *curdate, char *zedate)
{
	#undef FUNC
	#define FUNC "GetDiffDate"
	
	static int monthcount[12]={0,31,59,90,120,151,181,212,243,273,304,334};
	unsigned long stime1,stime2;
	char *wdate;
	int i;
	int day,month,year,hour,minute,second;
	int day2,month2,year2,hour2,minute2,second2;
	
	
	/* check correct format of the date */
	if (!CheckDateFormat(curdate) || !CheckDateFormat(zedate))
		exit(ABORT_ERROR);

	wdate=TxtStrDup(curdate);
	wdate[2]=wdate[5]=wdate[10]=wdate[13]=wdate[16]=0;
	day=atoi(&wdate[0]);
	month=atoi(&wdate[3]);
	year=atoi(&wdate[6]);
	hour=atoi(&wdate[11]);
	minute=atoi(&wdate[14]);
	second=atoi(&wdate[17]);
	MemFree(wdate);
		
	wdate=TxtStrDup(zedate);
	wdate[2]=wdate[5]=wdate[10]=wdate[13]=wdate[16]=0;
	day2=atoi(&wdate[0]);
	month2=atoi(&wdate[3]);
	year2=atoi(&wdate[6]);
	hour2=atoi(&wdate[11]);
	minute2=atoi(&wdate[14]);
	second2=atoi(&wdate[17]);
	MemFree(wdate);

	/* year offsets refer to 2000, no incidence on leap year calculation 
	   this give us a compatiblity until 2068 when compiled on 32 bits architecture...
	*/
	year-=2000;
	year2-=2000;

	stime1=second+minute*60+hour*3600+(day+monthcount[month-1])*3600*24;
	stime2=second2+minute2*60+hour2*3600+(day2+monthcount[month2-1])*3600*24;
	
	/* leap year adjustment over the years */
	i=0;
	while (i<year)
	{
		if IS_LEAP_YEAR(i) stime1+=366*24*3600; else stime1+=365*24*3600;
		i++;
	}
	if IS_LEAP_YEAR(year) if (month>2) stime1+=3600*24;
	i=0;
	while (i<year2)
	{
		if IS_LEAP_YEAR(i) stime2+=366*24*3600; else stime2+=365*24*3600;
		i++;
	}
	if IS_LEAP_YEAR(year2) if (month2>2) stime2+=3600*24;
	
	return stime1-stime2;
}



/***
	OpenDirectory
	
	input: directory to read
	       recursive mode
	input options:
	       minimal size
	       maximal size
	       older than	       
	       
	output: filename
*/


/***
	s_dirid
	structure used by ReadDir to manage multiple directories at a time
*/
struct s_dirid
{
	DIR *dir_id;
	char *dirname;
	int cpt;
	struct s_dirid *next;
};

static struct s_dirid *diridROOT=NULL;

/***
	DirOpen function
	
	open a directory for reading
	check if the dir is already open
*/
DIR *DirOpen(char *dirname)
{
	#undef FUNC
	#define FUNC "DirOpen"
	struct s_dirid *curdir;
	
	/* check parameters coherency */
	if (!dirname)
	{
		logerr("directory name must not be NULL");
		exit(ABORT_ERROR);
	}
	if (strnlen(dirname,PATH_MAX)==PATH_MAX)
	{
		logerr("cannot open directory because the argument size is bigger than PATH_MAX (%d)\n",PATH_MAX);
		exit(ABORT_ERROR);
	}
	
	/* try to find the directory in the list */
	curdir=diridROOT;
	while (curdir!=NULL)
	{
		if (!strcmp(curdir->dirname,dirname))
			break;
		else
			curdir=curdir->next;
	}
	/* if curdir is NULL then the directory is not opened yet */
	if (!curdir)
	{
		_static_library_nbdir_opened++;
		if (_static_library_nbdir_opened>_static_library_nbdir_opened_max)
			_static_library_nbdir_opened_max=_static_library_nbdir_opened;
		curdir=MemCalloc(sizeof(struct s_dirid));
		curdir->dirname=MemMalloc(strlen(dirname)+1);
		strcpy(curdir->dirname,dirname);
		curdir->next=diridROOT;
		curdir->cpt=0;
		diridROOT=curdir;
	}
	else
		return curdir->dir_id;
	
	if ((curdir->dir_id=opendir(dirname))==NULL)
	{
		int cpt=0;

		switch (errno)
		{
			case EACCES:logerr("Read permission is denied for [%s].",dirname);break;
			case EMFILE:logerr("The process has too many files open. Cannot open directory [%s]",dirname);break;
			case ENFILE:logerr("The entire system, or perhaps the file system which contains the directory [%s], cannot support any additional open files at the moment.",dirname);break;
			default:logerr("Unknown error %d during opendir [%s]: %s",errno,dirname,strerror(errno));
		}
		exit(ABORT_ERROR);
	}
	else
	{
		logdebug("opening directory [%s]",dirname);
	}
	
	return curdir->dir_id;
}

/***
	DirGetStructFromID
	
	retrieve the dir structure from the tree, with his ID
*/
struct s_dirid *DirGetStructFromID(DIR *dir_id)
{
	#undef FUNC
	#define FUNC "DirGetStructFromID"
	struct s_dirid *curdir;
	
	curdir=diridROOT;
	while (curdir!=NULL)
	{
		if (curdir->dir_id==dir_id)
			break;
		else
			curdir=curdir->next;
	}
	if (!curdir)
	{
		logerr("ID requested for an unknown directory! (was supposed to be opened)");
		exit(INTERNAL_ERROR);
	}
	
	return curdir;
}


/***
	DirClose function
	
	check for closing return code
	free the memory dir structure
*/
void DirClose(DIR *dir_id)
{
	#undef FUNC
	#define FUNC "DirClose"
	struct s_dirid *curdir;
	struct s_dirid *unlinkcell;
	
	curdir=DirGetStructFromID(dir_id);
	if (closedir(curdir->dir_id))
	{
		logerr("error while closing directory [%s]",curdir->dirname);
	}	

	logdebug("closing directory [%s] %d entr%s read",curdir->dirname,curdir->cpt,(curdir->cpt>1)?"ies":"y");
	/* unlink the cell from ROOT */
	if (curdir==diridROOT)
	{
		diridROOT=curdir->next;
	}
	else
	{
		unlinkcell=diridROOT;
		while (unlinkcell->next!=curdir)
			unlinkcell=unlinkcell->next;
		unlinkcell->next=curdir->next;
	}
	MemFree(curdir->dirname);
	MemFree(curdir);
	_static_library_nbdir_opened--;
}

/***
	DirCloseName function
	
	close a dir, from his name
	check for closing return code
	free the memory dir structure
*/
void DirCloseName(char *dirname)
{
	#undef FUNC
	#define FUNC "DirCloseName"
	DIR *curdir;
	
	curdir=DirOpen(dirname);
	DirClose(curdir);
}

/***
	DirIncCPT
	
	increase line counter when reading a directory
*/
void DirIncCPT(DIR *dir_id)
{
	#undef FUNC
	#define FUNC "DirIncCPT"
	struct s_dirid *curdir;
	
	curdir=DirGetStructFromID(dir_id);
	curdir->cpt++;
}

/***
	DirGetCPT
	
	Get line counter information
*/
int DirGetCPT(DIR *dir_id)
{
	#undef FUNC
	#define FUNC "DirGetCPT"
	struct s_dirid *curdir;
	
	curdir=DirGetStructFromID(dir_id);
	return curdir->cpt;
}


/***
	DirReadEntry
	Get the first/next entry in the directory dirname

	DirMatchEntry
	Get the entries matching the glob name
*/
char *_internal_DirReadGlob(char *dirname, int globflag)
{
	#undef FUNC
	#define FUNC "_internal_DirReadGlob"
	DIR *zedir=NULL;	
	static struct dirent *dit;
	char *dupfilename;
	char *last_slash;
	char *globname;

	dupfilename=TxtStrDup(dirname);
	if (globflag)
	{
		last_slash=strrchr(dupfilename,'/');
		if (!last_slash)
			globname="*";
		else
		{
			*last_slash=0;
			globname=last_slash+1;
		}
	}
	zedir=DirOpen(dupfilename);
	
	/* jump over . and .. then glob filename */
	if (!globflag)
		while ((dit=readdir(zedir))!=NULL && (!strcmp(dit->d_name,".") || !strcmp(dit->d_name,"..")));
	else
		while ((dit=readdir(zedir))!=NULL)
		{
			if (!strcmp(dit->d_name,".") || !strcmp(dit->d_name,".."))
				continue;
			if (!TxtGlobMatch(globname,dit->d_name))
				break;
		}
		
		
	MemFree(dupfilename);
	
	if (dit!=NULL)
	{
		DirIncCPT(zedir);
		return dit->d_name;
	}
	else
	{
		/* End of directory, we close the handle */
		DirClose(zedir);
		return NULL;
	}
}
#define DirReadEntry(dirname) _internal_DirReadGlob(dirname,0)
#define DirMatchEntry(dirname) _internal_DirReadGlob(dirname,1)

/***
	LookForFile
	
	input: filename or dirname
	output: 1 if success
	        0 if failed
	        
	lookforfile is useful when you cannot open a file and just wanted to test it
*/
int LookForFile(char *filename, char *dirname)
{
	#undef FUNC
	#define FUNC "LookForFile"
	
	char *last_slash;
	char *directory;
	char *entryname;
	char *dupfilename;

	/* checked by DirReadEntry but it is more convenient to have an error inside this function */
	if (!filename)
	{
		logerr("You must specify at least a filename with the path, or a filename and a dirname. Filename cannot be NULL!");
		exit(INTERNAL_ERROR);
	}
	if (strnlen(filename,PATH_MAX)==PATH_MAX)
	{
		logerr("cannot look for this file or directory because the argument size is bigger than PATH_MAX (%d)",PATH_MAX);
		exit(ABORT_ERROR);
	}
	dupfilename=TxtStrDup(filename);
	/* retrieve dirname from filename if necessary */	
	if (dirname==NULL)
	{
		/* split directory name and filename */
		last_slash=strrchr(dupfilename,'/');
		if (!last_slash)
		{
			/* no directory found, get it as a local file */
			dupfilename=MemRealloc(dupfilename,strlen(dupfilename)+3);
			memmove(dupfilename+2,dupfilename,strlen(dupfilename)+1);
			dupfilename[0]='.';
			dupfilename[1]='/';
			last_slash=dupfilename+1;
		}
		*last_slash=0;
		directory=dupfilename;
		filename=last_slash+1;
	}
	else
		directory=dirname;
	
	while ((entryname=DirReadEntry(directory))!=NULL)
		if (!strcmp(filename,entryname)) break;

	/* if the file is found, then we close the directory handle to free ressources */
	if (entryname!=NULL)
	{
		DirCloseName(directory);
		MemFree(dupfilename);
		return 1;
	}

	/* end of list reached, the handle is already closed by DirReadEntry */
	MemFree(dupfilename);
	return 0;
}

/***
	_do_remove
	
	remove a file or a directory
	with error management
*/
int _internal_do_remove(char *zename, char *zetype)
{
	#undef FUNC
	#define FUNC "_do_remove"

	if (zename==NULL)
	{
		logerr("the argument cannot be NULL");
		exit(INTERNAL_ERROR);
	}
	if (strnlen(zename,PATH_MAX)==PATH_MAX)
	{
		logerr("cannot remove this file or directory because the argument size is bigger than PATH_MAX (%d)",PATH_MAX);
		exit(ABORT_ERROR);
	}
	
	if (remove(zename))
	{
		switch (errno)
		{
			case EACCES:logerr("Write permission is denied for the directory from which the %s [%s] is to be removed, or the directory has the sticky bit set and you do not own the file.",zetype,zename);break;
			case EBUSY:logerr("This error indicates that the %s [%s] is being used by the system in such a way that it can't be unlinked. For example, you might see this error if the file name specifies the root directory or a mount point for a file system.",zetype,zename);break;
			case ENOENT:logerr("The %s named [%s] to be deleted doesn't exist.",zetype,zename);break;
			case EPERM:logerr("On some systems unlink cannot be used to delete the name of a directory [%s], or at least can only be used this way by a privileged user. To avoid such problems, use rmdir to delete directories.",zename);break;
			case EROFS:logerr("The directory containing the %s named [%s] to be deleted is on a read-only file system and can't be modified.",zetype,zename);break;
			case ENOTEMPTY:logerr("The directory [%s] to be deleted is not empty.",zename);break;
			default:logerr("Unknown error %d during remove [%s]: %s",errno,zename,strerror(errno));
		}
		exit(ABORT_ERROR);
	}
	logdebug("%s deleted",zename);
	return 0;	
}

#define FileRemove(filename) _internal_do_remove(filename,"file")

/***
	DirRemove
	
	delete a non-empty directory
	this function is recursive
	
	at this time, we do not want to delete socket or special files
*/
void DirRemove(char *zerep)
{
  #undef FUNC
  #define FUNC "DirRemove"
  char curname[MAX_LINE_BUFFER];
  char *curfile;
  
  if (zerep==NULL)
  {
  	logerr("the directory name cannot be NULL");
  	exit(INTERNAL_ERROR);
  }
  if (strnlen(zerep,PATH_MAX)==PATH_MAX)
  {
	logerr("cannot remove directory because the argument size is bigger than PATH_MAX (%d)",PATH_MAX);
  	exit(ABORT_ERROR);
  }
  
  while ((curfile=DirReadEntry(zerep))!= NULL )
  {
    	sprintf(curname,"%s/%s",zerep,curfile);
    	if (FileIsRegular(curname))
    		FileRemove(curname);
    	else
    	if (FileIsDir(curname))
    		DirRemove(curname);
    	else
    	{
    		logerr("The file %s cannot be deleted cause it is not a regular file",curname);
    		exit(ABORT_ERROR);
    	}    	
  }  
  _internal_do_remove(zerep,"directory");
}

/***
	FileRemoveIfExistst
	
	remove a file if it exists but do not quit in error
	if the file is not present
*/
void FileRemoveIfExists(char *filename)
{
	#undef FUNC
	#define FUNC "FileRemoveIfExists"
	if (LookForFile(filename,NULL))
		FileRemove(filename);
}
/***
	DirRemoveIfExistst
	
	remove a dir if it exists but do not quit in error
	if the dir is not present
*/
void DirRemoveIfExists(char *dirname)
{
	#undef FUNC
	#define FUNC "FileRemoveIfExists"
	if (LookForFile(dirname,NULL))
		DirRemove(dirname);
}

/***
	ChangeDir
	
	change working directory
	input argument is controlled
	it cannot be NULL and cannot be bigger than PATH_MAX (1024 bytes on AIX system)
*/
void ChangeDir(char *zedir)
{
	#undef FUNC
	#define FUNC "ChangeDir"
	
	if (!zedir)
	{
		logerr("cannot change directory as the provided parameter is NULL");
		exit(INTERNAL_ERROR);
	}
	if (strnlen(zedir,PATH_MAX)==PATH_MAX)
	{
		logerr("cannot change directory because the argument size is bigger than PATH_MAX (%d)",PATH_MAX);
		exit(ABORT_ERROR);
	}
	if (chdir(zedir)==-1)
	{
		logerr("cannot change directory to %s",zedir);
		switch (errno)
		{
			case EINVAL:logerr("The size argument is zero and buffer is not a null pointer.");break;
			case ERANGE:logerr("The size argument is less than the length of the working directory name. You need to allocate a bigger array and try again.");break;
			case EACCES:logerr("Permission to read or search a component of the directory name was denied.");break;
			default:logerr("Unknown error %d during chdir: %s",errno,strerror(errno));
		}
		exit(ABORT_ERROR);
	}
}
/***
	GetDir
	
	return the current directory path in an allocated pointer
	do not forget to free your memory after use!
	note: under AIX system, getcwd cannot be called with zero size buffer and NULL pointer...
*/
char *GetDir()
{
	#undef FUNC
	#define FUNC "GetDir"
	
	char *zesysdir;
	
	zesysdir=MemMalloc(PATH_MAX+1);
	if (getcwd(zesysdir,PATH_MAX)==NULL)
	{	
		logerr("cannot get working directory");
		switch (errno)
		{
			case EINVAL:logerr("The size argument is zero and buffer is not a null pointer.");break;
			case ERANGE:logerr("The size argument is less than the length of the working directory name. You need to allocate a bigger array and try again.");break;
			case EACCES:logerr("Permission to read or search a component of the directory name was denied.");break;
			default:logerr("Unknown error %d during getcwd: %s",errno,strerror(errno));
		}
		exit(INTERNAL_ERROR);
	}
	/* realloc to the real size with a secure len control */
	return MemRealloc(zesysdir,strnlen(zesysdir,PATH_MAX)+1);
}
#define PWD GetDir


/***
	ExecuteSysCmdGetLimit
	
	compute maximum command line size for use with system command
	The value is calculated by substracting environnement size to ARG_MAX
	
	For example, the limit will be approx 19k under shell session but
	only 16k under rex pech process. It is crap to define a fixed value
	cause system configuration may change in the future
	
	the official documentation dont give a precise answer but it seems
	to fit well by simply adding length of strings, including terminators
	
	http://publib.boulder.ibm.com/infocenter/aix/v6r1/index.jsp?topic=/com.ibm.aix.basetechref/doc/basetrf1/posix_spawn.htm
	The number of bytes available for the child process' combined argument and environment lists is {ARG_MAX}.
	The implementation specifies in the system documentation whether any list overhead, such as length words,
	null terminators, pointers, or alignment bytes, is included in this total.
*/
int ExecuteSysCmdGetLimit()
{
	#undef FUNC
	#define FUNC "ExecuteSysCmdGetLimit"
	
	int env_size;
	int env_idx;
	int idx;
	
	logdebug("computing environnement size");
	env_idx=env_size=0;
	while (environ[env_idx]!=NULL)
		env_size+=strlen(environ[env_idx++])+1;
	/* dont ask... */
	env_size+=256;
	/* check that environnement is not too big for a usable usage */
	if (env_size>ARG_MAX)
	{
		logerr("Your environnement is full, you should check you system configuration to use system command properly");
		exit(INTERNAL_ERROR);
	}	
	logdebug("environnement size=%d",env_size);
	env_size=ARG_MAX-env_size;
	logdebug("maximum argument size is %d",env_size);	
	return env_size;
}
/* compatibility mode */
#define MAX_COMMAND_LINE_BUFFER ExecuteSysCmdGetLimit()

/***
	_internal_ExecuteSysCmd
	
	execute a system command line
	the first parameter defines the behaviour in error case
	you must not call this function, use the two functions macro above
	- ExecuteSysCmd(...) will quit on error
	- ExecuteSysCmdBypassError(...) wont quit on error
	
	input: string format, ... (like printf)
*/
#define EXECUTE_SYS_CMD_QUIT_ON_ERROR 0
#define EXECUTE_SYS_CMD_BYPASS_ERROR  1

int _internal_ExecuteSysCmd(int zemode,char *format,...)
{
	#undef FUNC
	#define FUNC "_internal_ExecuteSysCmd"
	
	/* system buffer is bigger than expected, to check the limits */
	char zecommande[ARG_MAX];
	va_list argptr;
	int rc,zelen;

	static int arg_limit=-1;
	int arg_size;
	
	/* we compute the size of the environnement only once per program execution */
	if (arg_limit==-1)
		arg_limit=ExecuteSysCmdGetLimit();

	if (format==NULL)
	{
		logerr("the argument of this function cannot be NULL");
		exit(INTERNAL_ERROR);
	}	
	
	va_start(argptr,format);
	zelen=vsnprintf(NULL,0,format,argptr)+1;
	va_end(argptr);
	if (zelen>=arg_limit)
	{
		logerr("Your command line is too long (%d for %d)",zelen,arg_limit);
		logerr("You must be aware that this value is dynamic and is environnement dependent");
		logerr("You could use ExecuteSysCmdGetLimit() to get the current value");
		exit(ABORT_ERROR);
	}
	
	va_start(argptr,format);
	vsprintf(zecommande,format,argptr);	
	va_end(argptr);
	logdebug("system command: %s",zecommande);
	
	rc=system(zecommande);
	if ((WIFSIGNALED(rc) &&	(WTERMSIG(rc)==SIGINT || WTERMSIG(rc)==SIGQUIT)))
	{
		logerr("command aborted by user or system: %-200.200s...",zecommande);
		exit(ABORT_ERROR);
	}
	if(rc!=0)
	{
		switch (zemode)
		{
			default:
			case EXECUTE_SYS_CMD_QUIT_ON_ERROR:
				logerr("system command failed");
				logerr("command=%s",zecommande);
				logerr("ret=%d",rc);
				logerr("errno=%d",errno);
				logerr("strerror(errno)=%s",strerror(errno));
				exit(ABORT_ERROR);
			case EXECUTE_SYS_CMD_BYPASS_ERROR:
				logdebug("system command failed");
				logdebug("command=%s",zecommande);
				logdebug("ret=%d",rc);
				logdebug("errno=%d",errno);
				logdebug("strerror(errno)=%s",strerror(errno));
				break;
		}
	}
	return(rc);
}
#define ExecuteSysCmd(...) _internal_ExecuteSysCmd(EXECUTE_SYS_CMD_QUIT_ON_ERROR,__VA_ARGS__)
#define ExecuteSysCmdBypassError(...) _internal_ExecuteSysCmd(EXECUTE_SYS_CMD_BYPASS_ERROR,__VA_ARGS__)


/***
	GetSHORTINT
	GetINT
	GetLONGLONG

	get values from memory, regardless of endianness
	useful to read some exotics file formats
*/

/* prototype needed */
int GetEndianMode();

/***
	GetAutoEndian
	
	if the endian mode is not specified then we get the architecture endian mode
*/

int GetAutoEndian(int zemode)
{
	#undef FUNC
	#define FUNC "GetAutoEndian"
	
	if (zemode==GET_AUTO_ENDIAN)
	{
		if (_static_library_endian_mode==GET_ENDIAN_UNDEFINED)
			GetEndianMode();
		zemode=_static_library_endian_mode;
	}
	return zemode;
}
unsigned short int GetEndianSHORTINT(unsigned char *ptr, int zemode)
{
	#undef FUNC
	#define FUNC "GetEndianSHORTINT"

	unsigned short int zevalue=0;

	zemode=GetAutoEndian(zemode);
	
	switch (zemode)
	{
		case GET_LITTLE_ENDIAN:zevalue=(*ptr)+256*(*(ptr+1));break;
		case GET_BIG_ENDIAN:   zevalue=(*ptr)*256+(*(ptr+1));break;
		default:logerr("You must use GET_LITTLE_ENDIAN or GET_BIG_ENDIAN");exit(INTERNAL_ERROR);
	}
	return zevalue;
}
unsigned int GetEndianINT(unsigned char *ptr,int zemode)
{
	#undef FUNC
	#define FUNC "GetEndianINT"

	unsigned int zevalue=0;

	zemode=GetAutoEndian(zemode);
	
	switch (zemode)
	{
		case GET_LITTLE_ENDIAN:zevalue=(*ptr)+256*(*(ptr+1))+65536*(*(ptr+2))+16777216*(*(ptr+3));break;
		case GET_BIG_ENDIAN:   zevalue=16777216*(*ptr)+65536*(*(ptr+1))+256*(*(ptr+2))+(*(ptr+3));break;
		default:logerr("You must use GET_LITTLE_ENDIAN or GET_BIG_ENDIAN");exit(INTERNAL_ERROR);
	}
	return zevalue;
}
unsigned long long GetEndianLONGLONG(unsigned char *ptr,int zemode)
{
	#undef FUNC
	#define FUNC "GetEndianLONGLONG"

	/* the long long type is 64 bits in both 32 bits and 64 bits architecture */
	unsigned long long zevalue=0;

	zemode=GetAutoEndian(zemode);

	/* tiny code for 32 bits architecture (how sucks, obviously) */
	switch (zemode)
	{
		case GET_LITTLE_ENDIAN:
			zevalue=4294967296ULL*(unsigned long long)GetEndianINT(ptr+4,zemode)+(unsigned long long)GetEndianINT(ptr,zemode);
			break;
		case GET_BIG_ENDIAN:
			zevalue=4294967296ULL*(unsigned long long)GetEndianINT(ptr,zemode)+(unsigned long long)GetEndianINT(ptr+4,zemode);
			break;
		default:logerr("You must use GET_LITTLE_ENDIAN or GET_BIG_ENDIAN");exit(INTERNAL_ERROR);
	}
	
	return zevalue;
}

int GetEndianMode()
{
	#undef FUNC
	#define FUNC "GetEndianMode"
	
	unsigned short int *endian_test_value;
	unsigned char endiantest[2];
	
	endiantest[0]=0x12;
	endiantest[1]=0x34;
	endian_test_value=(unsigned short int*)endiantest;
	
	if (*endian_test_value==0x1234)
	{
		logdebug("big-endian architecture");
		_static_library_endian_mode=GET_BIG_ENDIAN;
	}
	else
	{
		logdebug("little-endian architecture");
		_static_library_endian_mode=GET_LITTLE_ENDIAN;
	}
	return _static_library_endian_mode;
}

/* shortcuts to get values with the current architecture */
#define GetShortInt(ptr) GetEndianSHORTINT(ptr,GET_AUTO_ENDIAN)
#define GetInt(ptr)           GetEndianINT(ptr,GET_AUTO_ENDIAN)
#define GetLongLong(ptr) GetEndianLONGLONG(ptr,GET_AUTO_ENDIAN)

void PutEndianSHORTINT(short int zevalue,unsigned char *ptr, int zemode)
{
	#undef FUNC
	#define FUNC "PutEndianSHORTINT"

	zemode=GetAutoEndian(zemode);
	
	switch (zemode)
	{
		case PUT_LITTLE_ENDIAN:
			*ptr=zevalue&0xFF;ptr++;
			*ptr=(zevalue&0xFF00)>>8;
			break;
		case PUT_BIG_ENDIAN:
			*ptr=(zevalue&0xFF00)>>8;ptr++;
			*ptr=zevalue&0xFF;
			break;
		default:logerr("You must use PUT_LITTLE_ENDIAN or PUT_BIG_ENDIAN");exit(INTERNAL_ERROR);
	}
}
void PutEndianINT(int zevalue, unsigned char *ptr,int zemode)
{
	#undef FUNC
	#define FUNC "PutEndianINT"

	zemode=GetAutoEndian(zemode);
	
	switch (zemode)
	{
		case PUT_LITTLE_ENDIAN:
			*ptr=zevalue&0xFF;ptr++;
			*ptr=(zevalue&0xFF00)>>8;ptr++;
			*ptr=(zevalue&0xFF0000)>>16;ptr++;
			*ptr=(zevalue&0xFF000000)>>24;
			break;
		case PUT_BIG_ENDIAN:
			*ptr=(zevalue&0xFF000000)>>24;ptr++;
			*ptr=(zevalue&0xFF0000)>>16;ptr++;
			*ptr=(zevalue&0xFF00)>>8;ptr++;
			*ptr=zevalue&0xFF;
			break;
		default:logerr("You must use GET_LITTLE_ENDIAN or GET_BIG_ENDIAN");exit(INTERNAL_ERROR);
	}
}
void PutEndianLONGLONG(unsigned long long zevalue, unsigned char *ptr,int zemode)
{
	#undef FUNC
	#define FUNC "PutEndianLONGLONG"
	/* the long long type is 64 bits in both 32 bits and 64 bits architecture */

	zemode=GetAutoEndian(zemode);

	/* tiny code for 32 bits architecture (how sucks, obviously) */
	switch (zemode)
	{
		case GET_LITTLE_ENDIAN:
			PutEndianINT((zevalue&0x00000000FFFFFFFFULL),ptr,zemode);
			PutEndianINT((zevalue&0xFFFFFFFF00000000ULL)>>32,ptr+4,zemode);
			break;
		case GET_BIG_ENDIAN:
			PutEndianINT((zevalue&0xFFFFFFFF00000000ULL)>>32,ptr,zemode);
			PutEndianINT((zevalue&0x00000000FFFFFFFFULL),ptr+4,zemode);
			break;
		default:logerr("You must use GET_LITTLE_ENDIAN or GET_BIG_ENDIAN");exit(INTERNAL_ERROR);
	}
}
/* shortcuts to put values with the current architecture */
#define PutShortInt(zeval,ptr) PutEndianSHORTINT(zeval,ptr,GET_AUTO_ENDIAN)
#define PutInt(zeval,ptr)           PutEndianINT(zeval,ptr,GET_AUTO_ENDIAN)
#define PutLongLong(zeval,ptr) PutEndianLONGLONG(zeval,ptr,GET_AUTO_ENDIAN)


/***
	SystemInfo
	
	display usage statistics in debug mode (memory, files, dirs)
	
	perform some system checking

	- endian mode
	- 32bits or 64bits mode
*/
void SystemInfo()
{
	#undef FUNC
	#define FUNC "SystemInfo"
	
	char infofile[PATH_MAX];
	pid_t process_id;
	char *buffer;
	char *info_machine;
	char *info_archi;
	char *info_os;
	char *info_version;
	char *info_release;
		
	process_id=getpid();
	
	loginfo("using %s on",__FILENAME__);

#ifdef OS_AIX
	sprintf(infofile,"/tmp/%d_systeminfo.txt",process_id);
	ExecuteSysCmd("uname -M >  %s",infofile);
	ExecuteSysCmd("uname -p >> %s",infofile);
	ExecuteSysCmd("uname -s >> %s",infofile);
	ExecuteSysCmd("uname -v >> %s",infofile);
	ExecuteSysCmd("uname -r >> %s",infofile);
	info_machine=TxtStrDup(TxtTrim(FileReadLine(infofile)));
	info_archi  =TxtStrDup(TxtTrim(FileReadLine(infofile)));
	info_os     =TxtStrDup(TxtTrim(FileReadLine(infofile)));
	info_version=TxtStrDup(TxtTrim(FileReadLine(infofile)));
	info_release=TxtStrDup(TxtTrim(FileReadLine(infofile)));
	FileReadLine(infofile);
	FileRemove(infofile);
	
	loginfo("%s %s",info_machine,info_archi);
	loginfo("running %s %s.%s",info_os,info_version,info_release);
	MemFree(info_machine);MemFree(info_archi);MemFree(info_os);MemFree(info_version);MemFree(info_release);
#endif
	
	if (sizeof(long)==8)
		loginfo("64bits mode");
	else
		loginfo("32bits mode");
		
	if (GetEndianMode()==GET_BIG_ENDIAN)
		loginfo("big-endian architecture");
	else
		loginfo("little-endian architecture");
	
}
/***
	CloseLibrary
	
	display usage statistics in debug mode (memory, files, dirs)
	
	perform a check for opened resources
	
	warn on monitor who are not closed
	warn on file who are not closed
	warn on dir who are not closed
	warn on memory blocks who are not freed
*/
void CloseLibrary()
{
	#undef FUNC
	#define FUNC "CloseLibrary"
	
	struct s_memory_trace *mem_trace;
	struct s_fileid *curfile;
	struct s_dirid *curdir;
	int i;

	logdebug("statistiques:");
	logdebug("maximum de memoire utilisee: %dko",_static_library_memory_used_max/1024);
	logdebug("maximum de fichiers ouverts: %d",_static_library_nbfile_opened_max);
	logdebug("maximum de repertoires ouverts: %d",_static_library_nbdir_opened_max);
	logdebug("");

	mem_trace=memory_trace_ROOT;
	while (mem_trace!=NULL)
	{
		logwarn("the monitor of %X:%d is still active",mem_trace->ptr,mem_trace->size);
		mem_trace=mem_trace->next;
	}	
	
	curfile=fileidROOT;
	while (curfile!=NULL)
	{
		logwarn("the file [%s](%s) was not closed",curfile->filename,curfile->opening_type);
		curfile=curfile->next;
	}

	curdir=diridROOT;
	while (curdir!=NULL)
	{
		logwarn("the directory [%s] was not closed",curdir->dirname);
		curdir=curdir->next;
	}

	for (i=0;i<_static_library_memory_monitor_top;i++)
	{
		if (mem_monitor[i].ptr!=NULL)
		{
			logwarn("memory block of %d byte%s wasn't freed %s - %s L%d operation %d",
				mem_monitor[i].size,
				(mem_monitor[i].size>1)?"s":"",
				mem_monitor[i].cursource,
				mem_monitor[i].curfunc,
				mem_monitor[i].curline,
				mem_monitor[i].mem_op);
		}		
	}
	
}

#undef __FILENAME__

/************************************
	end of mini-library
********************************EOF*/
