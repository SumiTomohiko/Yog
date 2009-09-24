#line 1 "..\\..\\..\\src\\array.c"
#line 1 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\string.h"















#pragma once
#line 18 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\string.h"






#line 25 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\string.h"



extern "C" {
#line 30 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\string.h"







#line 38 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\string.h"
#line 39 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\string.h"








#line 48 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\string.h"
#line 49 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\string.h"





#line 55 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\string.h"






typedef __w64 unsigned int   size_t;
#line 63 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\string.h"

#line 65 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\string.h"



typedef unsigned short wchar_t;

#line 71 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\string.h"




#line 76 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\string.h"








#line 85 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\string.h"
#line 86 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\string.h"














        void *  __cdecl memcpy(void *, const void *, size_t);
        int     __cdecl memcmp(const void *, const void *, size_t);
        void *  __cdecl memset(void *, int, size_t);
        char *  __cdecl _strset(char *, int);
        char *  __cdecl strcpy(char *, const char *);
        char *  __cdecl strcat(char *, const char *);
        int     __cdecl strcmp(const char *, const char *);
        size_t  __cdecl strlen(const char *);
#line 109 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\string.h"
 void *  __cdecl _memccpy(void *, const void *, int, size_t);
 void *  __cdecl memchr(const void *, int, size_t);
 int     __cdecl _memicmp(const void *, const void *, size_t);



#line 116 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\string.h"
 void *  __cdecl memmove(void *, const void *, size_t);
#line 118 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\string.h"


 char *  __cdecl strchr(const char *, int);
 int     __cdecl _strcmpi(const char *, const char *);
 int     __cdecl _stricmp(const char *, const char *);
 int     __cdecl strcoll(const char *, const char *);
 int     __cdecl _stricoll(const char *, const char *);
 int     __cdecl _strncoll(const char *, const char *, size_t);
 int     __cdecl _strnicoll(const char *, const char *, size_t);
 size_t  __cdecl strcspn(const char *, const char *);
 char *  __cdecl _strdup(const char *);
 char *  __cdecl _strerror(const char *);
 char *  __cdecl strerror(int);
 char *  __cdecl _strlwr(char *);
 char *  __cdecl strncat(char *, const char *, size_t);
 int     __cdecl strncmp(const char *, const char *, size_t);
 int     __cdecl _strnicmp(const char *, const char *, size_t);
 char *  __cdecl strncpy(char *, const char *, size_t);
 char *  __cdecl _strnset(char *, int, size_t);
 char *  __cdecl strpbrk(const char *, const char *);
 char *  __cdecl strrchr(const char *, int);
 char *  __cdecl _strrev(char *);
 size_t  __cdecl strspn(const char *, const char *);
 char *  __cdecl strstr(const char *, const char *);
 char *  __cdecl strtok(char *, const char *);
 char *  __cdecl _strupr(char *);
 size_t  __cdecl strxfrm (char *, const char *, size_t);





 void * __cdecl memccpy(void *, const void *, int, size_t);
 int __cdecl memicmp(const void *, const void *, size_t);
 int __cdecl strcmpi(const char *, const char *);
 int __cdecl stricmp(const char *, const char *);
 char * __cdecl strdup(const char *);
 char * __cdecl strlwr(char *);
 int __cdecl strnicmp(const char *, const char *, size_t);
 char * __cdecl strnset(char *, int, size_t);
 char * __cdecl strrev(char *);
        char * __cdecl strset(char *, int);
 char * __cdecl strupr(char *);

#line 163 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\string.h"






 wchar_t * __cdecl wcscat(wchar_t *, const wchar_t *);
 wchar_t * __cdecl wcschr(const wchar_t *, wchar_t);
 int __cdecl wcscmp(const wchar_t *, const wchar_t *);
 wchar_t * __cdecl wcscpy(wchar_t *, const wchar_t *);
 size_t __cdecl wcscspn(const wchar_t *, const wchar_t *);
 size_t __cdecl wcslen(const wchar_t *);
 wchar_t * __cdecl wcsncat(wchar_t *, const wchar_t *, size_t);
 int __cdecl wcsncmp(const wchar_t *, const wchar_t *, size_t);
 wchar_t * __cdecl wcsncpy(wchar_t *, const wchar_t *, size_t);
 wchar_t * __cdecl wcspbrk(const wchar_t *, const wchar_t *);
 wchar_t * __cdecl wcsrchr(const wchar_t *, wchar_t);
 size_t __cdecl wcsspn(const wchar_t *, const wchar_t *);
 wchar_t * __cdecl wcsstr(const wchar_t *, const wchar_t *);
 wchar_t * __cdecl wcstok(wchar_t *, const wchar_t *);
 wchar_t * __cdecl _wcserror(int);
 wchar_t * __cdecl __wcserror(const wchar_t *);

 wchar_t * __cdecl _wcsdup(const wchar_t *);
 int __cdecl _wcsicmp(const wchar_t *, const wchar_t *);
 int __cdecl _wcsnicmp(const wchar_t *, const wchar_t *, size_t);
 wchar_t * __cdecl _wcsnset(wchar_t *, wchar_t, size_t);
 wchar_t * __cdecl _wcsrev(wchar_t *);
 wchar_t * __cdecl _wcsset(wchar_t *, wchar_t);

 wchar_t * __cdecl _wcslwr(wchar_t *);
 wchar_t * __cdecl _wcsupr(wchar_t *);
 size_t __cdecl wcsxfrm(wchar_t *, const wchar_t *, size_t);
 int __cdecl wcscoll(const wchar_t *, const wchar_t *);
 int __cdecl _wcsicoll(const wchar_t *, const wchar_t *);
 int __cdecl _wcsncoll(const wchar_t *, const wchar_t *, size_t);
 int __cdecl _wcsnicoll(const wchar_t *, const wchar_t *, size_t);







 wchar_t * __cdecl wcsdup(const wchar_t *);
 int __cdecl wcsicmp(const wchar_t *, const wchar_t *);
 int __cdecl wcsnicmp(const wchar_t *, const wchar_t *, size_t);
 wchar_t * __cdecl wcsnset(wchar_t *, wchar_t, size_t);
 wchar_t * __cdecl wcsrev(wchar_t *);
 wchar_t * __cdecl wcsset(wchar_t *, wchar_t);
 wchar_t * __cdecl wcslwr(wchar_t *);
 wchar_t * __cdecl wcsupr(wchar_t *);
 int __cdecl wcsicoll(const wchar_t *, const wchar_t *);

#line 218 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\string.h"


#line 221 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\string.h"



}
#line 226 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\string.h"

#line 228 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\string.h"
#line 2 "..\\..\\..\\src\\array.c"
#line 1 "..\\..\\..\\include\\yog/array.h"



#line 1 "..\\..\\..\\include\\yog/object.h"



#line 1 "..\\..\\..\\include\\yog/yog.h"



#line 1 ".\\config.h"










































































































































































































#line 5 "..\\..\\..\\include\\yog/yog.h"

#line 1 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\limits.h"















#pragma once
#line 18 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\limits.h"






#line 25 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\limits.h"













#line 39 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\limits.h"

















#line 57 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\limits.h"





#line 63 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\limits.h"





#line 69 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\limits.h"





#line 75 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\limits.h"








#line 84 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\limits.h"








#line 93 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\limits.h"
































#line 126 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\limits.h"
#line 7 "..\\..\\..\\include\\yog/yog.h"
#line 8 "..\\..\\..\\include\\yog/yog.h"
#line 1 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdio.h"















#pragma once
#line 18 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdio.h"






#line 25 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdio.h"







#pragma pack(push,8)
#line 34 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdio.h"


extern "C" {
#line 38 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdio.h"








#line 47 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdio.h"
















#line 64 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdio.h"



















typedef unsigned short wint_t;
typedef unsigned short wctype_t;

#line 87 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdio.h"









typedef char *  va_list;
#line 98 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdio.h"

#line 100 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdio.h"

























struct _iobuf {
        char *_ptr;
        int   _cnt;
        char *_base;
        int   _flag;
        int   _file;
        int   _charbuf;
        int   _bufsiz;
        char *_tmpfname;
        };
typedef struct _iobuf FILE;

#line 138 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdio.h"










#line 149 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdio.h"










































 extern FILE _iob[];
#line 193 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdio.h"










#line 204 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdio.h"


typedef __int64 fpos_t;







#line 215 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdio.h"
#line 216 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdio.h"


#line 219 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdio.h"




























 int __cdecl _filbuf(FILE *);
 int __cdecl _flsbuf(int, FILE *);




 FILE * __cdecl _fsopen(const char *, const char *, int);
#line 255 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdio.h"

 void __cdecl clearerr(FILE *);
 int __cdecl fclose(FILE *);
 int __cdecl _fcloseall(void);




 FILE * __cdecl _fdopen(int, const char *);
#line 265 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdio.h"

 int __cdecl feof(FILE *);
 int __cdecl ferror(FILE *);
 int __cdecl fflush(FILE *);
 int __cdecl fgetc(FILE *);
 int __cdecl _fgetchar(void);
 int __cdecl fgetpos(FILE *, fpos_t *);
 char * __cdecl fgets(char *, int, FILE *);




 int __cdecl _fileno(FILE *);
#line 279 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdio.h"

 int __cdecl _flushall(void);
 FILE * __cdecl fopen(const char *, const char *);
 int __cdecl fprintf(FILE *, const char *, ...);
 int __cdecl fputc(int, FILE *);
 int __cdecl _fputchar(int);
 int __cdecl fputs(const char *, FILE *);
 size_t __cdecl fread(void *, size_t, size_t, FILE *);
 FILE * __cdecl freopen(const char *, const char *, FILE *);
 int __cdecl fscanf(FILE *, const char *, ...);
 int __cdecl fsetpos(FILE *, const fpos_t *);
 int __cdecl fseek(FILE *, long, int);
 long __cdecl ftell(FILE *);
 size_t __cdecl fwrite(const void *, size_t, size_t, FILE *);
 int __cdecl getc(FILE *);
 int __cdecl getchar(void);
 int __cdecl _getmaxstdio(void);
 char * __cdecl gets(char *);
 int __cdecl _getw(FILE *);
 void __cdecl perror(const char *);
 int __cdecl _pclose(FILE *);
 FILE * __cdecl _popen(const char *, const char *);
 int __cdecl printf(const char *, ...);
 int __cdecl putc(int, FILE *);
 int __cdecl putchar(int);
 int __cdecl puts(const char *);
 int __cdecl _putw(int, FILE *);
 int __cdecl remove(const char *);
 int __cdecl rename(const char *, const char *);
 void __cdecl rewind(FILE *);
 int __cdecl _rmtmp(void);
 int __cdecl scanf(const char *, ...);
 void __cdecl setbuf(FILE *, char *);
 int __cdecl _setmaxstdio(int);
 int __cdecl setvbuf(FILE *, char *, int, size_t);
 int __cdecl _snprintf(char *, size_t, const char *, ...);
 int __cdecl sprintf(char *, const char *, ...);
 int __cdecl _scprintf(const char *, ...);
 int __cdecl sscanf(const char *, const char *, ...);
 int __cdecl _snscanf(const char *, size_t, const char *, ...);
 char * __cdecl _tempnam(const char *, const char *);
 FILE * __cdecl tmpfile(void);
 char * __cdecl tmpnam(char *);
 int __cdecl ungetc(int, FILE *);
 int __cdecl _unlink(const char *);
 int __cdecl vfprintf(FILE *, const char *, va_list);
 int __cdecl vprintf(const char *, va_list);
 int __cdecl _vsnprintf(char *, size_t, const char *, va_list);
 int __cdecl vsprintf(char *, const char *, va_list);
 int __cdecl _vscprintf(const char *, va_list);







#line 337 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdio.h"




 FILE * __cdecl _wfsopen(const wchar_t *, const wchar_t *, int);
#line 343 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdio.h"

 wint_t __cdecl fgetwc(FILE *);
 wint_t __cdecl _fgetwchar(void);
 wint_t __cdecl fputwc(wchar_t, FILE *);
 wint_t __cdecl _fputwchar(wchar_t);
 wint_t __cdecl getwc(FILE *);
 wint_t __cdecl getwchar(void);
 wint_t __cdecl putwc(wchar_t, FILE *);
 wint_t __cdecl putwchar(wchar_t);
 wint_t __cdecl ungetwc(wint_t, FILE *);

 wchar_t * __cdecl fgetws(wchar_t *, int, FILE *);
 int __cdecl fputws(const wchar_t *, FILE *);
 wchar_t * __cdecl _getws(wchar_t *);
 int __cdecl _putws(const wchar_t *);

 int __cdecl fwprintf(FILE *, const wchar_t *, ...);
 int __cdecl wprintf(const wchar_t *, ...);
 int __cdecl _snwprintf(wchar_t *, size_t, const wchar_t *, ...);

 int __cdecl swprintf(wchar_t *, const wchar_t *, ...);


extern "C++"  int __cdecl swprintf(wchar_t *, size_t, const wchar_t *, ...);
#line 368 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdio.h"
 int __cdecl _scwprintf(const wchar_t *, ...);
 int __cdecl vfwprintf(FILE *, const wchar_t *, va_list);
 int __cdecl vwprintf(const wchar_t *, va_list);
 int __cdecl _vsnwprintf(wchar_t *, size_t, const wchar_t *, va_list);

 int __cdecl vswprintf(wchar_t *, const wchar_t *, va_list);


extern "C++"  int __cdecl vswprintf(wchar_t *, size_t, const wchar_t *, va_list);
#line 378 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdio.h"
 int __cdecl _vscwprintf(const wchar_t *, va_list);
 int __cdecl fwscanf(FILE *, const wchar_t *, ...);
 int __cdecl swscanf(const wchar_t *, const wchar_t *, ...);
 int __cdecl _snwscanf(const wchar_t *, size_t, const wchar_t *, ...);
 int __cdecl wscanf(const wchar_t *, ...);






 FILE * __cdecl _wfdopen(int, const wchar_t *);
 FILE * __cdecl _wfopen(const wchar_t *, const wchar_t *);
 FILE * __cdecl _wfreopen(const wchar_t *, const wchar_t *, FILE *);
 void __cdecl _wperror(const wchar_t *);
 FILE * __cdecl _wpopen(const wchar_t *, const wchar_t *);
 int __cdecl _wremove(const wchar_t *);
 wchar_t * __cdecl _wtempnam(const wchar_t *, const wchar_t *);
 wchar_t * __cdecl _wtmpnam(wchar_t *);



#line 401 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdio.h"


#line 404 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdio.h"
































 int __cdecl fcloseall(void);
 FILE * __cdecl fdopen(int, const char *);
 int __cdecl fgetchar(void);
 int __cdecl fileno(FILE *);
 int __cdecl flushall(void);
 int __cdecl fputchar(int);
 int __cdecl getw(FILE *);
 int __cdecl putw(int, FILE *);
 int __cdecl rmtmp(void);
 char * __cdecl tempnam(const char *, const char *);
 int __cdecl unlink(const char *);

#line 449 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdio.h"


}
#line 453 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdio.h"


#pragma pack(pop)
#line 457 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdio.h"

#line 459 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdio.h"
#line 9 "..\\..\\..\\include\\yog/yog.h"





#line 15 "..\\..\\..\\include\\yog/yog.h"


#line 18 "..\\..\\..\\include\\yog/yog.h"

















#line 36 "..\\..\\..\\include\\yog/yog.h"


typedef unsigned int YogVal;
typedef unsigned int ID;

typedef unsigned int pc_t;





















#line 64 "..\\..\\..\\include\\yog/yog.h"

typedef unsigned int uint_t;
typedef int int_t;































typedef struct YogEnv YogEnv;

typedef void* (*ObjectKeeper)(YogEnv*, void*, void*);
typedef void (*ChildrenKeeper)(YogEnv*, void*, ObjectKeeper, void*);
typedef void (*Finalizer)(YogEnv*, void*);

typedef uint_t flags_t;








YogVal YogVal_from_int(YogEnv*, int_t);
YogVal YogVal_get_attr(YogEnv*, YogVal, ID);
YogVal YogVal_get_class(YogEnv*, YogVal);
YogVal YogVal_get_descr(YogEnv*, YogVal, YogVal, YogVal);
int_t YogVal_is_subclass_of(YogEnv*, YogVal, YogVal);
void YogVal_print(YogEnv*, YogVal);
void YogVal_set_attr(YogEnv*, YogVal, ID, YogVal);










#line 131 "..\\..\\..\\include\\yog/yog.h"
#line 1 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdarg.h"















#pragma once
#line 18 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdarg.h"






#line 25 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdarg.h"







#pragma pack(push,8)
#line 34 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdarg.h"


extern "C" {
#line 38 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdarg.h"








#line 47 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdarg.h"





typedef __w64 unsigned int   uintptr_t;
#line 54 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdarg.h"

#line 56 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdarg.h"

















#line 74 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdarg.h"











#line 86 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdarg.h"


#line 89 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdarg.h"













#line 103 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdarg.h"

































































































#line 201 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdarg.h"


}
#line 205 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdarg.h"


#pragma pack(pop)
#line 209 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdarg.h"

#line 211 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\stdarg.h"
#line 132 "..\\..\\..\\include\\yog/yog.h"

static void
TRACE(const char* fmt, ...)
{
    va_list ap;
    ( ap = (va_list)( &reinterpret_cast<const char &>(fmt) ) + ( (sizeof(fmt) + sizeof(int) - 1) & ~(sizeof(int) - 1) ) );
    vprintf(fmt, ap);
    ( ap = (va_list)0 );
}
#line 142 "..\\..\\..\\include\\yog/yog.h"





typedef unsigned char uint8_t;
#line 149 "..\\..\\..\\include\\yog/yog.h"

#line 151 "..\\..\\..\\include\\yog/yog.h"



#line 5 "..\\..\\..\\include\\yog/object.h"

struct YogBasicObj {
    uint_t id_upper;
    uint_t id_lower;
    flags_t flags;
    YogVal klass;
};







typedef struct YogBasicObj YogBasicObj;

struct YogObj {
    struct YogBasicObj base;
    YogVal attrs;
};




typedef struct YogObj YogObj;

typedef YogVal (*Allocator)(struct YogEnv*, YogVal);











void YogBasicObj_init(YogEnv*, YogVal, uint_t, YogVal);
void YogBasicObj_keep_children(YogEnv*, void*, ObjectKeeper, void*);
YogVal YogObj_allocate(YogEnv*, YogVal);
void YogObj_class_init(YogEnv*, YogVal);
YogVal YogObj_get_attr(YogEnv*, YogVal, ID);
void YogObj_init(YogEnv*, YogVal, uint_t, YogVal);
void YogObj_keep_children(YogEnv*, void*, ObjectKeeper, void*);
YogVal YogObj_new(YogEnv*, YogVal);
void YogObj_set_attr(YogEnv*, YogVal, const char*, YogVal);
void YogObj_set_attr_id(YogEnv*, YogVal, ID, YogVal);
void YogObject_boot(YogEnv*, YogVal);
void YogObject_eval_builtin_script(YogEnv*, YogVal);



#line 59 "..\\..\\..\\include\\yog/object.h"



#line 5 "..\\..\\..\\include\\yog/array.h"
#line 1 "..\\..\\..\\include\\yog/yog.h"





















































































































































#line 151 "..\\..\\..\\include\\yog/yog.h"



#line 6 "..\\..\\..\\include\\yog/array.h"

struct YogValArray {
    uint_t size;
    YogVal items[0];
};

typedef struct YogValArray YogValArray;

struct YogArray {
    struct YogBasicObj base;
    uint_t size;
    YogVal body;
};

typedef struct YogArray YogArray;








YogVal YogArray_add(YogEnv*, YogVal, YogVal);
YogVal YogArray_at(YogEnv*, YogVal, uint_t);
YogVal YogArray_define_class(YogEnv*);
void YogArray_eval_builtin_script(YogEnv*, YogVal);
void YogArray_extend(YogEnv*, YogVal, YogVal);
YogVal YogArray_new(YogEnv*);
YogVal YogArray_of_size(YogEnv*, uint_t);
void YogArray_push(YogEnv*, YogVal, YogVal);
YogVal YogArray_shift(YogEnv*, YogVal);
uint_t YogArray_size(YogEnv*, YogVal);
YogVal YogValArray_at(YogEnv*, YogVal, uint_t);
YogVal YogValArray_new(YogEnv*, uint_t);
uint_t YogValArray_size(YogEnv*, YogVal);



#line 46 "..\\..\\..\\include\\yog/array.h"



#line 3 "..\\..\\..\\src\\array.c"
#line 1 "..\\..\\..\\include\\yog/error.h"



#line 1 "..\\..\\..\\include\\yog/yog.h"





















































































































































#line 151 "..\\..\\..\\include\\yog/yog.h"



#line 5 "..\\..\\..\\include\\yog/error.h"








void YogError_bug(YogEnv*, const char*, uint_t, const char*, ...);
void YogError_out_of_memory(YogEnv*);
void YogError_print_stacktrace(YogEnv*);
void YogError_raise(YogEnv*, YogVal);
void YogError_raise_ArgumentError(YogEnv*, const char*, ...);
void YogError_raise_AttributeError(YogEnv*, const char*, ...);
void YogError_raise_EOFError(YogEnv*, const char*, ...);
void YogError_raise_ImportError(YogEnv*, const char*, ...);
void YogError_raise_IndexError(YogEnv*, const char*, ...);
void YogError_raise_KeyError(YogEnv*, const char*, ...);
void YogError_raise_LocalJumpError(YogEnv*, const char*, ...);
void YogError_raise_NameError(YogEnv*, const char*, ...);
void YogError_raise_SyntaxError(YogEnv*, const char*, ...);
void YogError_raise_TypeError(YogEnv*, const char*, ...);
void YogError_raise_ValueError(YogEnv*, const char*, ...);
void YogError_raise_ZeroDivisionError(YogEnv*, const char*, ...);
void YogError_raise_binop_type_error(YogEnv*, YogVal, YogVal, const char*);
void YogError_raise_comparison_type_error(YogEnv*, YogVal, YogVal);
void YogError_warn(YogEnv*, const char*, uint_t, const char*, ...);















#line 48 "..\\..\\..\\include\\yog/error.h"


static void
YOG_WARN(YogEnv* env, const char* fmt, ...)
{
    
}

static void
YOG_BUG(YogEnv* env, const char* fmt, ...)
{
    
}

static void
YOG_ASSERT(YogEnv* env, int_t test, const char* fmt, ...)
{
    
}
#line 68 "..\\..\\..\\include\\yog/error.h"

#line 70 "..\\..\\..\\include\\yog/error.h"



#line 4 "..\\..\\..\\src\\array.c"
#line 1 "..\\..\\..\\include\\yog/eval.h"



#line 1 "..\\..\\..\\include\\yog/yog.h"





















































































































































#line 151 "..\\..\\..\\include\\yog/yog.h"



#line 5 "..\\..\\..\\include\\yog/eval.h"








YogVal YogEval_call_method(YogEnv*, YogVal, const char*, uint_t, YogVal*);
YogVal YogEval_call_method0(YogEnv*, YogVal, const char*);
YogVal YogEval_call_method1(YogEnv*, YogVal, const char*, YogVal);
YogVal YogEval_call_method2(YogEnv*, YogVal, const char*, uint_t, YogVal*, YogVal);
YogVal YogEval_call_method_id(YogEnv*, YogVal, ID, uint_t, YogVal*);
YogVal YogEval_call_method_id2(YogEnv*, YogVal, ID, uint_t, YogVal*, YogVal);
YogVal YogEval_eval_file(YogEnv*, FILE*, const char*, const char*);
void YogEval_eval_package(YogEnv*, YogVal, YogVal);
YogVal YogEval_mainloop(YogEnv*);
void YogEval_push_finish_frame(YogEnv*);



#line 27 "..\\..\\..\\include\\yog/eval.h"



#line 5 "..\\..\\..\\src\\array.c"
#line 1 "..\\..\\..\\include\\yog/frame.h"



#line 1 "..\\..\\..\\include\\yog/yog.h"





















































































































































#line 151 "..\\..\\..\\include\\yog/yog.h"



#line 5 "..\\..\\..\\include\\yog/frame.h"

enum YogFrameType {
    FRAME_C,
    FRAME_METHOD,
    FRAME_NAME,
    FRAME_FINISH,
};

typedef enum YogFrameType YogFrameType;

struct YogFrame {
    YogVal prev;
    enum YogFrameType type;
};

typedef struct YogFrame YogFrame;

struct YogCFrame {
    struct YogFrame base;
    YogVal self;
    YogVal args;
    YogVal f;
};

typedef struct YogCFrame YogCFrame;

struct YogOuterVars {
    uint_t size;
    YogVal items[0];
};

typedef struct YogOuterVars YogOuterVars;

struct YogScriptFrame {
    struct YogFrame base;
    pc_t pc;
    YogVal code;
    uint_t stack_size;
    YogVal stack;
    YogVal globals;
    YogVal outer_vars;
    YogVal frame_to_long_return;
    YogVal frame_to_long_break;
};

typedef struct YogScriptFrame YogScriptFrame;

struct YogFinishFrame {
    struct YogFrame base;
    pc_t pc;
    YogVal code;
    uint_t stack_size;
    YogVal stack;
};

typedef struct YogFinishFrame YogFinishFrame;



struct YogNameFrame {
    struct YogScriptFrame base;
    YogVal self;
    YogVal vars;
};

typedef struct YogNameFrame YogNameFrame;






struct YogMethodFrame {
    struct YogScriptFrame base;
    YogVal vars;
};

typedef struct YogMethodFrame YogMethodFrame;













YogVal YogCFrame_new(YogEnv*);
YogVal YogFinishFrame_new(YogEnv*);
YogVal YogMethodFrame_new(YogEnv*);
YogVal YogNameFrame_new(YogEnv*);
YogVal YogOuterVars_new(YogEnv*, uint_t);
YogVal YogScriptFrame_pop_stack(YogEnv*, YogScriptFrame*);
void YogScriptFrame_push_stack(YogEnv*, YogScriptFrame*, YogVal);








#line 112 "..\\..\\..\\include\\yog/frame.h"



#line 6 "..\\..\\..\\src\\array.c"
#line 1 "..\\..\\..\\include\\yog/function.h"



#line 1 ".\\config.h"










































































































































































































#line 5 "..\\..\\..\\include\\yog/function.h"


#line 8 "..\\..\\..\\include\\yog/function.h"
#line 1 "..\\..\\..\\include\\yog/object.h"

























































#line 59 "..\\..\\..\\include\\yog/object.h"



#line 9 "..\\..\\..\\include\\yog/function.h"
#line 1 "..\\..\\..\\include\\yog/yog.h"





















































































































































#line 151 "..\\..\\..\\include\\yog/yog.h"



#line 10 "..\\..\\..\\include\\yog/function.h"

struct YogNativeFunction {
    struct YogBasicObj base;

    ID func_name;
    ID class_name;
    void* f;
};

typedef struct YogNativeFunction YogNativeFunction;

struct YogFunction {
    struct YogBasicObj base;

    YogVal code;
    YogVal globals;
    YogVal outer_vars;
    YogVal frame_to_long_return;
    YogVal frame_to_long_break;
};

typedef struct YogFunction YogFunction;

struct YogInstanceMethod {
    struct YogBasicObj base;

    YogVal self;
    YogVal f;
};

typedef struct YogInstanceMethod YogInstanceMethod;








YogVal YogCallable_call(YogEnv*, YogVal, uint_t, YogVal*);
YogVal YogCallable_call1(YogEnv*, YogVal, YogVal);
YogVal YogCallable_call2(YogEnv*, YogVal, uint_t, YogVal*, YogVal);
YogVal YogFunction_define_class(YogEnv*);
YogVal YogFunction_new(YogEnv*);
YogVal YogInstanceMethod_define_class(YogEnv*);
YogVal YogInstanceMethod_new(YogEnv*);
YogVal YogNativeFunction_define_class(YogEnv*);
YogVal YogNativeFunction_new(YogEnv*, ID, const char*, void*);
YogVal YogNativeInstanceMethod_define_class(YogEnv*);
YogVal YogNativeInstanceMethod_new(YogEnv*);



#line 64 "..\\..\\..\\include\\yog/function.h"



#line 7 "..\\..\\..\\src\\array.c"
#line 1 "..\\..\\..\\include\\yog/gc.h"




















#line 22 "..\\..\\..\\include\\yog/gc.h"




#line 1 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\sys/types.h"















#pragma once
#line 18 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\sys/types.h"






#line 25 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\sys/types.h"






typedef long    time_t;         
#line 33 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\sys/types.h"

typedef __int64 __time64_t;
#line 36 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\sys/types.h"

#line 38 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\sys/types.h"




typedef unsigned short _ino_t;          



typedef unsigned short ino_t;
#line 48 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\sys/types.h"


#line 51 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\sys/types.h"




typedef unsigned int _dev_t;            



typedef unsigned int dev_t;
#line 61 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\sys/types.h"


#line 64 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\sys/types.h"




typedef long _off_t;                    



typedef long off_t;
#line 74 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\sys/types.h"


#line 77 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\sys/types.h"

#line 79 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\sys/types.h"
#line 27 "..\\..\\..\\include\\yog/gc.h"
#line 1 "..\\..\\..\\include\\yog/yog.h"





















































































































































#line 151 "..\\..\\..\\include\\yog/yog.h"



#line 28 "..\\..\\..\\include\\yog/gc.h"








YogVal YogGC_allocate(YogEnv*, ChildrenKeeper, Finalizer, size_t);
void YogGC_bind_to_gc(YogEnv*);
void YogGC_delete(YogEnv*);
void YogGC_free_from_gc(YogEnv*);
void YogGC_keep(YogEnv*, YogVal*, ObjectKeeper, void*);
void YogGC_perform(YogEnv*);
void YogGC_perform_major(YogEnv*);
void YogGC_perform_minor(YogEnv*);
void YogGC_suspend(YogEnv*);



#line 49 "..\\..\\..\\include\\yog/gc.h"



#line 8 "..\\..\\..\\src\\array.c"
#line 1 "..\\..\\..\\include\\yog/class.h"



#line 1 ".\\config.h"










































































































































































































#line 5 "..\\..\\..\\include\\yog/class.h"


#line 8 "..\\..\\..\\include\\yog/class.h"
#line 1 "..\\..\\..\\include\\yog/object.h"

























































#line 59 "..\\..\\..\\include\\yog/object.h"



#line 9 "..\\..\\..\\include\\yog/class.h"
#line 1 "..\\..\\..\\include\\yog/yog.h"





















































































































































#line 151 "..\\..\\..\\include\\yog/yog.h"



#line 10 "..\\..\\..\\include\\yog/class.h"

typedef void (*GetAttrExecutor)(YogEnv*, YogVal, ID);
typedef YogVal (*GetAttrCaller)(YogEnv*, YogVal, ID);
typedef void (*Executor)(YogEnv*, YogVal, uint8_t, YogVal*, YogVal, uint8_t, YogVal*, YogVal, YogVal);
typedef YogVal (*Caller)(YogEnv*, YogVal, uint8_t, YogVal*, YogVal, uint8_t, YogVal*, YogVal, YogVal);

struct YogClass {
    struct YogObj base;
    YogVal super;
    Allocator allocator;
    ID name;
    GetAttrExecutor exec_get_attr;
    GetAttrCaller call_get_attr;
    void (*exec_get_descr)(YogEnv*, YogVal, YogVal, YogVal);
    YogVal (*call_get_descr)(YogEnv*, YogVal, YogVal, YogVal);
    void (*exec_set_descr)(YogEnv*, YogVal, YogVal, YogVal);
    Executor exec;
    Caller call;
};

typedef struct YogClass YogClass;








YogVal YogClass_allocate(YogEnv*, YogVal);
void YogClass_boot(YogEnv*, YogVal);
void YogClass_class_init(YogEnv*, YogVal);
void YogClass_define_allocator(YogEnv*, YogVal, Allocator);
void YogClass_define_caller(YogEnv*, YogVal, Caller);
void YogClass_define_class_method(YogEnv*, YogVal, const char*, void*);
void YogClass_define_descr_get_caller(YogEnv*, YogVal, YogVal (*)(YogEnv*, YogVal, YogVal, YogVal));
void YogClass_define_descr_get_executor(YogEnv*, YogVal, void (*)(YogEnv*, YogVal, YogVal, YogVal));
void YogClass_define_descr_set_executor(YogEnv*, YogVal, void (*)(YogEnv*, YogVal, YogVal, YogVal));
void YogClass_define_executor(YogEnv*, YogVal, Executor);
void YogClass_define_get_attr_caller(YogEnv*, YogVal, GetAttrCaller);
void YogClass_define_get_attr_executor(YogEnv*, YogVal, GetAttrExecutor);
void YogClass_define_method(YogEnv*, YogVal, const char*, void*);
void YogClass_define_property(YogEnv*, YogVal, const char*, void*, void*);
YogVal YogClass_get_attr(YogEnv*, YogVal, ID);
void YogClass_include_module(YogEnv*, YogVal, YogVal);
YogVal YogClass_new(YogEnv*, const char*, YogVal);



#line 60 "..\\..\\..\\include\\yog/class.h"



#line 9 "..\\..\\..\\src\\array.c"
#line 1 "..\\..\\..\\include\\yog/misc.h"












#line 14 "..\\..\\..\\include\\yog/misc.h"















#line 30 "..\\..\\..\\include\\yog/misc.h"


#line 33 "..\\..\\..\\include\\yog/misc.h"








void YogMisc_eval_source(YogEnv*, YogVal, const char*);



#line 46 "..\\..\\..\\include\\yog/misc.h"



#line 10 "..\\..\\..\\src\\array.c"
#line 1 "..\\..\\..\\include\\yog/object.h"

























































#line 59 "..\\..\\..\\include\\yog/object.h"



#line 11 "..\\..\\..\\src\\array.c"
#line 1 "..\\..\\..\\include\\yog/string.h"



#line 1 "..\\..\\..\\include\\yog/object.h"

























































#line 59 "..\\..\\..\\include\\yog/object.h"



#line 5 "..\\..\\..\\include\\yog/string.h"
#line 1 "..\\..\\..\\include\\yog/yog.h"





















































































































































#line 151 "..\\..\\..\\include\\yog/yog.h"



#line 6 "..\\..\\..\\include\\yog/string.h"

struct YogCharArray {
    uint_t size;
    char items[0];
};

typedef struct YogCharArray YogCharArray;

struct YogString {
    struct YogBasicObj base;
    YogVal encoding;
    uint_t size;
    YogVal body;
};

typedef struct YogString YogString;





#line 1 "..\\..\\..\\include\\yog/encoding.h"



#line 1 "..\\..\\..\\onig\\oniguruma.h"
































extern "C" {
#line 35 "..\\..\\..\\onig\\oniguruma.h"









#line 45 "..\\..\\..\\onig\\oniguruma.h"


#line 48 "..\\..\\..\\onig\\oniguruma.h"
#line 49 "..\\..\\..\\onig\\oniguruma.h"






#line 56 "..\\..\\..\\onig\\oniguruma.h"












#line 69 "..\\..\\..\\onig\\oniguruma.h"
#line 70 "..\\..\\..\\onig\\oniguruma.h"






#line 77 "..\\..\\..\\onig\\oniguruma.h"
#line 78 "..\\..\\..\\onig\\oniguruma.h"



















#line 98 "..\\..\\..\\onig\\oniguruma.h"

typedef unsigned char  OnigUChar;
typedef unsigned long  OnigCodePoint;
typedef unsigned int   OnigCtype;
typedef unsigned int   OnigDistance;



typedef unsigned int OnigCaseFoldType; 

1 OnigCaseFoldType OnigDefaultCaseFoldFlag;



















typedef struct {
  int byte_len;  
  int code_len;  
  OnigCodePoint code[3];
} OnigCaseFoldCodeItem;

typedef struct {
  OnigCodePoint esc;
  OnigCodePoint anychar;
  OnigCodePoint anytime;
  OnigCodePoint zero_or_one_time;
  OnigCodePoint one_or_more_time;
  OnigCodePoint anychar_anytime;
} OnigMetaCharTableType;
  
typedef int (*OnigApplyAllCaseFoldFunc)(OnigCodePoint from, OnigCodePoint* to, int to_len, void* arg);

typedef struct OnigEncodingTypeST {
  int    (*mbc_enc_len)(const OnigUChar* p);
  const char*   name;
  int           max_enc_len;
  int           min_enc_len;
  int    (*is_mbc_newline)(const OnigUChar* p, const OnigUChar* end);
  OnigCodePoint (*mbc_to_code)(const OnigUChar* p, const OnigUChar* end);
  int    (*code_to_mbclen)(OnigCodePoint code);
  int    (*code_to_mbc)(OnigCodePoint code, OnigUChar *buf);
  int    (*mbc_case_fold)(OnigCaseFoldType flag, const OnigUChar** pp, const OnigUChar* end, OnigUChar* to);
  int    (*apply_all_case_fold)(OnigCaseFoldType flag, OnigApplyAllCaseFoldFunc f, void* arg);
  int    (*get_case_fold_codes_by_str)(OnigCaseFoldType flag, const OnigUChar* p, const OnigUChar* end, OnigCaseFoldCodeItem acs[]);
  int    (*property_name_to_ctype)(struct OnigEncodingTypeST* enc, OnigUChar* p, OnigUChar* end);
  int    (*is_code_ctype)(OnigCodePoint code, OnigCtype ctype);
  int    (*get_ctype_code_range)(OnigCtype ctype, OnigCodePoint* sb_out, const OnigCodePoint* ranges[]);
  OnigUChar* (*left_adjust_char_head)(const OnigUChar* start, const OnigUChar* p);
  int    (*is_allowed_reverse_match)(const OnigUChar* p, const OnigUChar* end);
} OnigEncodingType;

typedef OnigEncodingType* OnigEncoding;

1 OnigEncodingType OnigEncodingASCII;
1 OnigEncodingType OnigEncodingISO_8859_1;
1 OnigEncodingType OnigEncodingISO_8859_2;
1 OnigEncodingType OnigEncodingISO_8859_3;
1 OnigEncodingType OnigEncodingISO_8859_4;
1 OnigEncodingType OnigEncodingISO_8859_5;
1 OnigEncodingType OnigEncodingISO_8859_6;
1 OnigEncodingType OnigEncodingISO_8859_7;
1 OnigEncodingType OnigEncodingISO_8859_8;
1 OnigEncodingType OnigEncodingISO_8859_9;
1 OnigEncodingType OnigEncodingISO_8859_10;
1 OnigEncodingType OnigEncodingISO_8859_11;
1 OnigEncodingType OnigEncodingISO_8859_13;
1 OnigEncodingType OnigEncodingISO_8859_14;
1 OnigEncodingType OnigEncodingISO_8859_15;
1 OnigEncodingType OnigEncodingISO_8859_16;
1 OnigEncodingType OnigEncodingUTF8;
1 OnigEncodingType OnigEncodingUTF16_BE;
1 OnigEncodingType OnigEncodingUTF16_LE;
1 OnigEncodingType OnigEncodingUTF32_BE;
1 OnigEncodingType OnigEncodingUTF32_LE;
1 OnigEncodingType OnigEncodingEUC_JP;
1 OnigEncodingType OnigEncodingEUC_TW;
1 OnigEncodingType OnigEncodingEUC_KR;
1 OnigEncodingType OnigEncodingEUC_CN;
1 OnigEncodingType OnigEncodingSJIS;
1 OnigEncodingType OnigEncodingKOI8;
1 OnigEncodingType OnigEncodingKOI8_R;
1 OnigEncodingType OnigEncodingCP1251;
1 OnigEncodingType OnigEncodingBIG5;
1 OnigEncodingType OnigEncodingGB18030;



































































































































1
OnigUChar* onigenc_step_back (OnigEncoding enc, const OnigUChar* start, const OnigUChar* s, int n);



1
int onigenc_init (void);
1
int onigenc_set_default_encoding (OnigEncoding enc);
1
OnigEncoding onigenc_get_default_encoding (void);
1
void  onigenc_set_default_caseconv_table (const OnigUChar* table);
1
OnigUChar* onigenc_get_right_adjust_char_head_with_prev (OnigEncoding enc, const OnigUChar* start, const OnigUChar* s, const OnigUChar** prev);
1
OnigUChar* onigenc_get_prev_char_head (OnigEncoding enc, const OnigUChar* start, const OnigUChar* s);
1
OnigUChar* onigenc_get_left_adjust_char_head (OnigEncoding enc, const OnigUChar* start, const OnigUChar* s);
1
OnigUChar* onigenc_get_right_adjust_char_head (OnigEncoding enc, const OnigUChar* start, const OnigUChar* s);
1
int onigenc_strlen (OnigEncoding enc, const OnigUChar* p, const OnigUChar* end);
1
int onigenc_strlen_null (OnigEncoding enc, const OnigUChar* p);
1
int onigenc_str_bytelen_null (OnigEncoding enc, const OnigUChar* p);













typedef unsigned int        OnigOptionType;

























typedef struct {
  unsigned int   op;
  unsigned int   op2;
  unsigned int   behavior;
  OnigOptionType options;   
  OnigMetaCharTableType meta_char_table;
} OnigSyntaxType;

1 OnigSyntaxType OnigSyntaxASIS;
1 OnigSyntaxType OnigSyntaxPosixBasic;
1 OnigSyntaxType OnigSyntaxPosixExtended;
1 OnigSyntaxType OnigSyntaxEmacs;
1 OnigSyntaxType OnigSyntaxGrep;
1 OnigSyntaxType OnigSyntaxGnuRegex;
1 OnigSyntaxType OnigSyntaxJava;
1 OnigSyntaxType OnigSyntaxPerl;
1 OnigSyntaxType OnigSyntaxPerl_NG;
1 OnigSyntaxType OnigSyntaxRuby;














1 OnigSyntaxType*   OnigDefaultSyntax;








































































































































































typedef struct OnigCaptureTreeNodeStruct {
  int group;   
  int beg;
  int end;
  int allocated;
  int num_childs;
  struct OnigCaptureTreeNodeStruct** childs;
} OnigCaptureTreeNode;


struct re_registers {
  int  allocated;
  int  num_regs;
  int* beg;
  int* end;
  
  OnigCaptureTreeNode* history_root;  
};










typedef struct re_registers   OnigRegion;

typedef struct {
  OnigEncoding enc;
  OnigUChar* par;
  OnigUChar* par_end;
} OnigErrorInfo;

typedef struct {
  int lower;
  int upper;
} OnigRepeatRange;

typedef void (*OnigWarnFunc) (const char* s);
extern void onig_null_warn (const char* s);













typedef struct re_pattern_buffer {
  
  unsigned char* p;         
  unsigned int used;        
  unsigned int alloc;       

  int state;                     
  int num_mem;                   
  int num_repeat;                
  int num_null_check;            
  int num_comb_exp_check;        
  int num_call;                  
  unsigned int capture_history;  
  unsigned int bt_mem_start;     
  unsigned int bt_mem_end;       
  int stack_pop_level;
  int repeat_range_alloc;
  OnigRepeatRange* repeat_range;

  OnigEncoding      enc;
  OnigOptionType    options;
  OnigSyntaxType*   syntax;
  OnigCaseFoldType  case_fold_flag;
  void*             name_table;

  
  int            optimize;          
  int            threshold_len;     
  int            anchor;            
  OnigDistance   anchor_dmin;       
  OnigDistance   anchor_dmax;       
  int            sub_anchor;        
  unsigned char *exact;
  unsigned char *exact_end;
  unsigned char  map[256]; 
  int           *int_map;                   
  int           *int_map_backward;          
  OnigDistance   dmin;                      
  OnigDistance   dmax;                      

  
  struct re_pattern_buffer* chain;  
} OnigRegexType;

typedef OnigRegexType*  OnigRegex;


  typedef OnigRegexType  regex_t;
#line 700 "..\\..\\..\\onig\\oniguruma.h"


typedef struct {
  int             num_of_elements;
  OnigEncoding    pattern_enc;
  OnigEncoding    target_enc;
  OnigSyntaxType* syntax;
  OnigOptionType  option;
  OnigCaseFoldType   case_fold_flag;
} OnigCompileInfo;


1
int onig_init (void);
1
int onig_error_code_to_str (OnigUChar* s, int err_code, ...);
1
void onig_set_warn_func (OnigWarnFunc f);
1
void onig_set_verb_warn_func (OnigWarnFunc f);
1
int onig_new (OnigRegex*, const OnigUChar* pattern, const OnigUChar* pattern_end, OnigOptionType option, OnigEncoding enc, OnigSyntaxType* syntax, OnigErrorInfo* einfo);
1
int onig_new_deluxe (OnigRegex* reg, const OnigUChar* pattern, const OnigUChar* pattern_end, OnigCompileInfo* ci, OnigErrorInfo* einfo);
1
void onig_free (OnigRegex);
1
int onig_recompile (OnigRegex, const OnigUChar* pattern, const OnigUChar* pattern_end, OnigOptionType option, OnigEncoding enc, OnigSyntaxType* syntax, OnigErrorInfo* einfo);
1
int onig_recompile_deluxe (OnigRegex reg, const OnigUChar* pattern, const OnigUChar* pattern_end, OnigCompileInfo* ci, OnigErrorInfo* einfo);
1
int onig_search (OnigRegex, const OnigUChar* str, const OnigUChar* end, const OnigUChar* start, const OnigUChar* range, OnigRegion* region, OnigOptionType option);
1
int onig_match (OnigRegex, const OnigUChar* str, const OnigUChar* end, const OnigUChar* at, OnigRegion* region, OnigOptionType option);
1
OnigRegion* onig_region_new (void);
1
void onig_region_init (OnigRegion* region);
1
void onig_region_free (OnigRegion* region, int free_self);
1
void onig_region_copy (OnigRegion* to, OnigRegion* from);
1
void onig_region_clear (OnigRegion* region);
1
int onig_region_resize (OnigRegion* region, int n);
1
int onig_region_set (OnigRegion* region, int at, int beg, int end);
1
int onig_name_to_group_numbers (OnigRegex reg, const OnigUChar* name, const OnigUChar* name_end, int** nums);
1
int onig_name_to_backref_number (OnigRegex reg, const OnigUChar* name, const OnigUChar* name_end, OnigRegion *region);
1
int onig_foreach_name (OnigRegex reg, int (*func)(const OnigUChar*, const OnigUChar*,int,int*,OnigRegex,void*), void* arg);
1
int onig_number_of_names (OnigRegex reg);
1
int onig_number_of_captures (OnigRegex reg);
1
int onig_number_of_capture_histories (OnigRegex reg);
1
OnigCaptureTreeNode* onig_get_capture_tree (OnigRegion* region);
1
int onig_capture_tree_traverse (OnigRegion* region, int at, int(*callback_func)(int,int,int,int,int,void*), void* arg);
1
int onig_noname_group_capture_is_active (OnigRegex reg);
1
OnigEncoding onig_get_encoding (OnigRegex reg);
1
OnigOptionType onig_get_options (OnigRegex reg);
1
OnigCaseFoldType onig_get_case_fold_flag (OnigRegex reg);
1
OnigSyntaxType* onig_get_syntax (OnigRegex reg);
1
int onig_set_default_syntax (OnigSyntaxType* syntax);
1
void onig_copy_syntax (OnigSyntaxType* to, OnigSyntaxType* from);
1
unsigned int onig_get_syntax_op (OnigSyntaxType* syntax);
1
unsigned int onig_get_syntax_op2 (OnigSyntaxType* syntax);
1
unsigned int onig_get_syntax_behavior (OnigSyntaxType* syntax);
1
OnigOptionType onig_get_syntax_options (OnigSyntaxType* syntax);
1
void onig_set_syntax_op (OnigSyntaxType* syntax, unsigned int op);
1
void onig_set_syntax_op2 (OnigSyntaxType* syntax, unsigned int op2);
1
void onig_set_syntax_behavior (OnigSyntaxType* syntax, unsigned int behavior);
1
void onig_set_syntax_options (OnigSyntaxType* syntax, OnigOptionType options);
1
int onig_set_meta_char (OnigSyntaxType* syntax, unsigned int what, OnigCodePoint code);
1
void onig_copy_encoding (OnigEncoding to, OnigEncoding from);
1
OnigCaseFoldType onig_get_default_case_fold_flag (void);
1
int onig_set_default_case_fold_flag (OnigCaseFoldType case_fold_flag);
1
unsigned int onig_get_match_stack_limit_size (void);
1
int onig_set_match_stack_limit_size (unsigned int size);
1
int onig_end (void);
1
const char* onig_version (void);
1
const char* onig_copyright (void);


}
#line 816 "..\\..\\..\\onig\\oniguruma.h"

#line 818 "..\\..\\..\\onig\\oniguruma.h"
#line 5 "..\\..\\..\\include\\yog/encoding.h"
#line 1 "..\\..\\..\\include\\yog/yog.h"





















































































































































#line 151 "..\\..\\..\\include\\yog/yog.h"



#line 6 "..\\..\\..\\include\\yog/encoding.h"

struct YogEncoding {
    OnigEncoding onig_enc;
};

typedef struct YogEncoding YogEncoding;

#line 1 "..\\..\\..\\include\\yog/string.h"


























































#line 60 "..\\..\\..\\include\\yog/string.h"



#line 14 "..\\..\\..\\include\\yog/encoding.h"








YogVal YogEncoding_get_default(YogEnv*);
char* YogEncoding_left_adjust_char_head(YogEnv*, YogVal, const char*, const char*);
int_t YogEncoding_mbc_size(YogEnv*, YogVal, const char*);
YogVal YogEncoding_new(YogEnv*, OnigEncoding);
YogVal YogEncoding_normalize_name(YogEnv*, YogVal);



#line 31 "..\\..\\..\\include\\yog/encoding.h"



#line 28 "..\\..\\..\\include\\yog/string.h"








YogVal YogCharArray_new_str(YogEnv*, const char*);
void YogString_add(YogEnv*, YogVal, YogVal);
void YogString_add_cstr(YogEnv*, YogVal, const char*);
char YogString_at(YogEnv*, YogVal, uint_t);
void YogString_clear(YogEnv*, YogVal);
YogVal YogString_clone(YogEnv*, YogVal);
YogVal YogString_define_class(YogEnv*);
char* YogString_dup(YogEnv*, const char*);
void YogString_eval_builtin_script(YogEnv*, YogVal);
int_t YogString_hash(YogEnv*, YogVal);
ID YogString_intern(YogEnv*, YogVal);
YogVal YogString_multiply(YogEnv*, YogVal, int_t);
YogVal YogString_new(YogEnv*);
YogVal YogString_new_format(YogEnv*, const char*, ...);
YogVal YogString_new_range(YogEnv*, YogVal, const char*, const char*);
YogVal YogString_new_size(YogEnv*, uint_t);
YogVal YogString_new_str(YogEnv*, const char*);
void YogString_push(YogEnv*, YogVal, char);
uint_t YogString_size(YogEnv*, YogVal);
YogVal YogString_to_i(YogEnv*, YogVal);



#line 60 "..\\..\\..\\include\\yog/string.h"



#line 12 "..\\..\\..\\src\\array.c"
#line 1 "..\\..\\..\\include\\yog/thread.h"



#line 1 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\setjmp.h"
















#pragma once
#line 19 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\setjmp.h"






#line 26 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\setjmp.h"







#pragma pack(push,8)
#line 35 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\setjmp.h"


extern "C" {
#line 39 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\setjmp.h"

















#line 57 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\setjmp.h"














#line 72 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\setjmp.h"







typedef struct __JUMP_BUFFER {
    unsigned long Ebp;
    unsigned long Ebx;
    unsigned long Edi;
    unsigned long Esi;
    unsigned long Esp;
    unsigned long Eip;
    unsigned long Registration;
    unsigned long TryLevel;
    unsigned long Cookie;
    unsigned long UnwindFunc;
    unsigned long UnwindData[6];
} _JUMP_BUFFER;




































































































































































































































































































#line 385 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\setjmp.h"





typedef int jmp_buf[16];

#line 393 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\setjmp.h"




int __cdecl _setjmp(jmp_buf);


 __declspec(noreturn) void __cdecl longjmp(jmp_buf, int);


#line 404 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\setjmp.h"


}
#line 408 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\setjmp.h"


#pragma pack(pop)
#line 412 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\setjmp.h"

#line 414 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\setjmp.h"
#line 5 "..\\..\\..\\include\\yog/thread.h"

#line 1 "..\\..\\..\\include\\yog/gc/copying.h"





#line 7 "..\\..\\..\\include\\yog/gc/copying.h"
#line 1 "..\\..\\..\\include\\yog/yog.h"





















































































































































#line 151 "..\\..\\..\\include\\yog/yog.h"



#line 8 "..\\..\\..\\include\\yog/gc/copying.h"

struct YogCopyingHeader {
    ChildrenKeeper keeper;
    Finalizer finalizer;
    void* forwarding_addr;
    size_t size;




#line 19 "..\\..\\..\\include\\yog/gc/copying.h"
};

typedef struct YogCopyingHeader YogCopyingHeader;





struct YogCopyingHeap {
    size_t size;
    unsigned char* free;
    unsigned char items[0];
};

typedef struct YogCopyingHeap YogCopyingHeap;

struct YogCopying {
    struct YogCopying* prev;
    struct YogCopying* next;
    int_t refered;

    unsigned int err;
    size_t heap_size;
    struct YogCopyingHeap* active_heap;
    struct YogCopyingHeap* inactive_heap;
    unsigned char* scanned;
    unsigned char* unscanned;
};

typedef struct YogCopying YogCopying;








void* YogCopying_alloc(YogEnv*, YogCopying*, ChildrenKeeper, Finalizer, size_t);
void YogCopying_allocate_heap(YogEnv*, YogCopying*);
void YogCopying_cheney_scan(YogEnv*, YogCopying*);
void* YogCopying_copy(YogEnv*, YogCopying*, void*);
void YogCopying_delete_garbage(YogEnv*, YogCopying*);
void YogCopying_finalize(YogEnv*, YogCopying*);
void YogCopying_init(YogEnv*, YogCopying*, size_t);
int_t YogCopying_is_empty(YogEnv*, YogCopying*);
int_t YogCopying_is_in_active_heap(YogEnv*, YogCopying*, void*);
int_t YogCopying_is_in_inactive_heap(YogEnv*, YogCopying*, void*);
void YogCopying_iterate_objects(YogEnv*, YogCopying*, void (*)(YogEnv*, YogCopyingHeader*));
void YogCopying_keep_vm(YogEnv*, YogCopying*);
void YogCopying_post_gc(YogEnv*, YogCopying*);
void YogCopying_prepare(YogEnv*, YogCopying*);
void YogCopying_scan(YogEnv*, YogCopying*, ObjectKeeper, void*);



#line 76 "..\\..\\..\\include\\yog/gc/copying.h"



#line 7 "..\\..\\..\\include\\yog/thread.h"








#line 16 "..\\..\\..\\include\\yog/thread.h"
#line 1 "..\\..\\..\\include\\yog/object.h"

























































#line 59 "..\\..\\..\\include\\yog/object.h"



#line 17 "..\\..\\..\\include\\yog/thread.h"
#line 1 "..\\..\\..\\include\\yog/yog.h"





















































































































































#line 151 "..\\..\\..\\include\\yog/yog.h"



#line 18 "..\\..\\..\\include\\yog/thread.h"

enum YogJmpStatus {
    JMP_RAISE = 1,
    JMP_RETURN = 2,
    JMP_BREAK = 3,
};

typedef enum YogJmpStatus YogJmpStatus;

struct YogJmpBuf {
    jmp_buf buf;
    struct YogJmpBuf* prev;
};

typedef struct YogJmpBuf YogJmpBuf;

struct YogLocals {
    struct YogLocals* next;
    uint_t num_vals;
    uint_t size;
    YogVal* vals[4];
};

typedef struct YogLocals YogLocals;

#line 1 "..\\..\\..\\include\\yog/env.h"



#line 1 "..\\..\\..\\include\\yog/vm.h"



#line 1 "..\\..\\..\\pthreads-w32\\pthread.h"










































































#line 76 "..\\..\\..\\pthreads-w32\\pthread.h"



#line 80 "..\\..\\..\\pthreads-w32\\pthread.h"











#line 92 "..\\..\\..\\pthreads-w32\\pthread.h"





#line 98 "..\\..\\..\\pthreads-w32\\pthread.h"





#line 104 "..\\..\\..\\pthreads-w32\\pthread.h"






#line 111 "..\\..\\..\\pthreads-w32\\pthread.h"




















































































#line 196 "..\\..\\..\\pthreads-w32\\pthread.h"










#line 207 "..\\..\\..\\pthreads-w32\\pthread.h"








#line 216 "..\\..\\..\\pthreads-w32\\pthread.h"


#line 1 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\time.h"
















#pragma once
#line 19 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\time.h"






#line 26 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\time.h"







#pragma pack(push,8)
#line 35 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\time.h"


extern "C" {
#line 39 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\time.h"








#line 48 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\time.h"
















#line 65 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\time.h"



















typedef __int64 __time64_t;     
#line 86 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\time.h"

#line 88 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\time.h"


typedef long clock_t;

#line 93 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\time.h"























struct tm {
        int tm_sec;     
        int tm_min;     
        int tm_hour;    
        int tm_mday;    
        int tm_mon;     
        int tm_year;    
        int tm_wday;    
        int tm_yday;    
        int tm_isdst;   
        };

#line 129 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\time.h"












 extern int _daylight;


 extern long _dstbias;


 extern long _timezone;


 extern char * _tzname[2];




 char * __cdecl asctime(const struct tm *);
 char * __cdecl ctime(const time_t *);
 clock_t __cdecl clock(void);
 double __cdecl difftime(time_t, time_t);
 struct tm * __cdecl gmtime(const time_t *);
 struct tm * __cdecl localtime(const time_t *);
 time_t __cdecl mktime(struct tm *);
 size_t __cdecl strftime(char *, size_t, const char *,
        const struct tm *);
 char * __cdecl _strdate(char *);
 char * __cdecl _strtime(char *);
 time_t __cdecl time(time_t *);




 void __cdecl _tzset(void);
#line 173 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\time.h"


 char * __cdecl _ctime64(const __time64_t *);
 struct tm * __cdecl _gmtime64(const __time64_t *);
 struct tm * __cdecl _localtime64(const __time64_t *);
 __time64_t __cdecl _mktime64(struct tm *);
 __time64_t __cdecl _time64(__time64_t *);
#line 181 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\time.h"



unsigned __cdecl _getsystime(struct tm *);
unsigned __cdecl _setsystime(struct tm *, unsigned);











 
 wchar_t * __cdecl _wasctime(const struct tm *);
 wchar_t * __cdecl _wctime(const time_t *);
 size_t __cdecl wcsftime(wchar_t *, size_t, const wchar_t *,
        const struct tm *);
 wchar_t * __cdecl _wstrdate(wchar_t *);
 wchar_t * __cdecl _wstrtime(wchar_t *);


 wchar_t * __cdecl _wctime64(const __time64_t *);
#line 208 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\time.h"


#line 211 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\time.h"








 extern int daylight;
 extern long timezone;
 extern char * tzname[2];

 void __cdecl tzset(void);

#line 226 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\time.h"



}
#line 231 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\time.h"


#pragma pack(pop)
#line 235 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\time.h"

#line 237 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\time.h"
#line 219 "..\\..\\..\\pthreads-w32\\pthread.h"


#line 222 "..\\..\\..\\pthreads-w32\\pthread.h"



#line 226 "..\\..\\..\\pthreads-w32\\pthread.h"







enum {
  PTW32_FALSE = 0,
  PTW32_TRUE = (! PTW32_FALSE)
};










#line 248 "..\\..\\..\\pthreads-w32\\pthread.h"


#line 251 "..\\..\\..\\pthreads-w32\\pthread.h"
#line 252 "..\\..\\..\\pthreads-w32\\pthread.h"









#line 1 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\errno.h"
















#pragma once
#line 19 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\errno.h"






#line 26 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\errno.h"



extern "C" {
#line 31 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\errno.h"


















#line 50 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\errno.h"






#line 57 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\errno.h"
 extern int errno;
#line 59 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\errno.h"
















































}
#line 109 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\errno.h"

#line 111 "E:\\Program Files\\Microsoft Visual Studio .NET 2003\\VC7\\INCLUDE\\errno.h"
#line 262 "..\\..\\..\\pthreads-w32\\pthread.h"
#line 263 "..\\..\\..\\pthreads-w32\\pthread.h"
#line 264 "..\\..\\..\\pthreads-w32\\pthread.h"






#line 271 "..\\..\\..\\pthreads-w32\\pthread.h"



#line 275 "..\\..\\..\\pthreads-w32\\pthread.h"













#line 1 "..\\..\\..\\pthreads-w32\\sched.h"














































#line 48 "..\\..\\..\\pthreads-w32\\sched.h"





#line 54 "..\\..\\..\\pthreads-w32\\sched.h"





#line 60 "..\\..\\..\\pthreads-w32\\sched.h"






#line 67 "..\\..\\..\\pthreads-w32\\sched.h"




#line 72 "..\\..\\..\\pthreads-w32\\sched.h"















#line 88 "..\\..\\..\\pthreads-w32\\sched.h"










#line 99 "..\\..\\..\\pthreads-w32\\sched.h"


#line 102 "..\\..\\..\\pthreads-w32\\sched.h"
#line 103 "..\\..\\..\\pthreads-w32\\sched.h"










#line 114 "..\\..\\..\\pthreads-w32\\sched.h"
#line 115 "..\\..\\..\\pthreads-w32\\sched.h"








#line 124 "..\\..\\..\\pthreads-w32\\sched.h"
typedef int pid_t;
#line 126 "..\\..\\..\\pthreads-w32\\sched.h"



enum {
  SCHED_OTHER = 0,
  SCHED_FIFO,
  SCHED_RR,
  SCHED_MIN   = SCHED_OTHER,
  SCHED_MAX   = SCHED_RR
};

struct sched_param {
  int sched_priority;
};


extern "C"
{
#line 145 "..\\..\\..\\pthreads-w32\\sched.h"

 int __cdecl sched_yield (void);

 int __cdecl sched_get_priority_min (int policy);

 int __cdecl sched_get_priority_max (int policy);

 int __cdecl sched_setscheduler (pid_t pid, int policy);

 int __cdecl sched_getscheduler (pid_t pid);
















}                               
#line 173 "..\\..\\..\\pthreads-w32\\sched.h"




#line 178 "..\\..\\..\\pthreads-w32\\sched.h"

#line 289 "..\\..\\..\\pthreads-w32\\pthread.h"









#line 299 "..\\..\\..\\pthreads-w32\\pthread.h"



#line 303 "..\\..\\..\\pthreads-w32\\pthread.h"
#line 304 "..\\..\\..\\pthreads-w32\\pthread.h"



struct timespec {
        long tv_sec;
        long tv_nsec;
};
#line 312 "..\\..\\..\\pthreads-w32\\pthread.h"



#line 316 "..\\..\\..\\pthreads-w32\\pthread.h"



#line 320 "..\\..\\..\\pthreads-w32\\pthread.h"



#line 324 "..\\..\\..\\pthreads-w32\\pthread.h"


extern "C"
{
#line 329 "..\\..\\..\\pthreads-w32\\pthread.h"



















































































































































































  






  












#line 529 "..\\..\\..\\pthreads-w32\\pthread.h"















#line 545 "..\\..\\..\\pthreads-w32\\pthread.h"















#line 561 "..\\..\\..\\pthreads-w32\\pthread.h"





typedef struct {
    void * p;                   
    unsigned int x;             
} ptw32_handle_t;

typedef ptw32_handle_t pthread_t;
typedef struct pthread_attr_t_ * pthread_attr_t;
typedef struct pthread_once_t_ pthread_once_t;
typedef struct pthread_key_t_ * pthread_key_t;
typedef struct pthread_mutex_t_ * pthread_mutex_t;
typedef struct pthread_mutexattr_t_ * pthread_mutexattr_t;
typedef struct pthread_cond_t_ * pthread_cond_t;
typedef struct pthread_condattr_t_ * pthread_condattr_t;
#line 580 "..\\..\\..\\pthreads-w32\\pthread.h"
typedef struct pthread_rwlock_t_ * pthread_rwlock_t;
typedef struct pthread_rwlockattr_t_ * pthread_rwlockattr_t;
typedef struct pthread_spinlock_t_ * pthread_spinlock_t;
typedef struct pthread_barrier_t_ * pthread_barrier_t;
typedef struct pthread_barrierattr_t_ * pthread_barrierattr_t;









enum {



  PTHREAD_CREATE_JOINABLE       = 0,  
  PTHREAD_CREATE_DETACHED       = 1,




  PTHREAD_INHERIT_SCHED         = 0,
  PTHREAD_EXPLICIT_SCHED        = 1,  




  PTHREAD_SCOPE_PROCESS         = 0,
  PTHREAD_SCOPE_SYSTEM          = 1,  




  PTHREAD_CANCEL_ENABLE         = 0,  
  PTHREAD_CANCEL_DISABLE        = 1,




  PTHREAD_CANCEL_ASYNCHRONOUS   = 0,
  PTHREAD_CANCEL_DEFERRED       = 1,  





  PTHREAD_PROCESS_PRIVATE       = 0,
  PTHREAD_PROCESS_SHARED        = 1,




  PTHREAD_BARRIER_SERIAL_THREAD = -1
};




















struct pthread_once_t_
{
  int          done;        
  void *       lock;
  int          reserved1;
  int          reserved2;
};





























enum
{
  
  PTHREAD_MUTEX_FAST_NP,
  PTHREAD_MUTEX_RECURSIVE_NP,
  PTHREAD_MUTEX_ERRORCHECK_NP,
  PTHREAD_MUTEX_TIMED_NP = PTHREAD_MUTEX_FAST_NP,
  PTHREAD_MUTEX_ADAPTIVE_NP = PTHREAD_MUTEX_FAST_NP,
  
  PTHREAD_MUTEX_NORMAL = PTHREAD_MUTEX_FAST_NP,
  PTHREAD_MUTEX_RECURSIVE = PTHREAD_MUTEX_RECURSIVE_NP,
  PTHREAD_MUTEX_ERRORCHECK = PTHREAD_MUTEX_ERRORCHECK_NP,
  PTHREAD_MUTEX_DEFAULT = PTHREAD_MUTEX_NORMAL
};


typedef struct ptw32_cleanup_t ptw32_cleanup_t;



#pragma warning( disable : 4229 )
#line 715 "..\\..\\..\\pthreads-w32\\pthread.h"

typedef void (* __cdecl ptw32_cleanup_callback_t)(void *);


#pragma warning( default : 4229 )
#line 721 "..\\..\\..\\pthreads-w32\\pthread.h"

struct ptw32_cleanup_t
{
  ptw32_cleanup_callback_t routine;
  void *arg;
  struct ptw32_cleanup_t *prev;
};






























        

































































































#line 857 "..\\..\\..\\pthreads-w32\\pthread.h"

#line 859 "..\\..\\..\\pthreads-w32\\pthread.h"












 int __cdecl pthread_attr_init (pthread_attr_t * attr);

 int __cdecl pthread_attr_destroy (pthread_attr_t * attr);

 int __cdecl pthread_attr_getdetachstate (const pthread_attr_t * attr,
                                         int *detachstate);

 int __cdecl pthread_attr_getstackaddr (const pthread_attr_t * attr,
                                       void **stackaddr);

 int __cdecl pthread_attr_getstacksize (const pthread_attr_t * attr,
                                       size_t * stacksize);

 int __cdecl pthread_attr_setdetachstate (pthread_attr_t * attr,
                                         int detachstate);

 int __cdecl pthread_attr_setstackaddr (pthread_attr_t * attr,
                                       void *stackaddr);

 int __cdecl pthread_attr_setstacksize (pthread_attr_t * attr,
                                       size_t stacksize);

 int __cdecl pthread_attr_getschedparam (const pthread_attr_t *attr,
                                        struct sched_param *param);

 int __cdecl pthread_attr_setschedparam (pthread_attr_t *attr,
                                        const struct sched_param *param);

 int __cdecl pthread_attr_setschedpolicy (pthread_attr_t *,
                                         int);

 int __cdecl pthread_attr_getschedpolicy (pthread_attr_t *,
                                         int *);

 int __cdecl pthread_attr_setinheritsched(pthread_attr_t * attr,
                                         int inheritsched);

 int __cdecl pthread_attr_getinheritsched(pthread_attr_t * attr,
                                         int * inheritsched);

 int __cdecl pthread_attr_setscope (pthread_attr_t *,
                                   int);

 int __cdecl pthread_attr_getscope (const pthread_attr_t *,
                                   int *);




 int __cdecl pthread_create (pthread_t * tid,
                            const pthread_attr_t * attr,
                            void *(*start) (void *),
                            void *arg);

 int __cdecl pthread_detach (pthread_t tid);

 int __cdecl pthread_equal (pthread_t t1,
                           pthread_t t2);

 void __cdecl pthread_exit (void *value_ptr);

 int __cdecl pthread_join (pthread_t thread,
                          void **value_ptr);

 pthread_t __cdecl pthread_self (void);

 int __cdecl pthread_cancel (pthread_t thread);

 int __cdecl pthread_setcancelstate (int state,
                                    int *oldstate);

 int __cdecl pthread_setcanceltype (int type,
                                   int *oldtype);

 void __cdecl pthread_testcancel (void);

 int __cdecl pthread_once (pthread_once_t * once_control,
                          void (*init_routine) (void));


 ptw32_cleanup_t * __cdecl ptw32_pop_cleanup (int execute);

 void __cdecl ptw32_push_cleanup (ptw32_cleanup_t * cleanup,
                                 void (*routine) (void *),
                                 void *arg);
#line 957 "..\\..\\..\\pthreads-w32\\pthread.h"




 int __cdecl pthread_key_create (pthread_key_t * key,
                                void (*destructor) (void *));

 int __cdecl pthread_key_delete (pthread_key_t key);

 int __cdecl pthread_setspecific (pthread_key_t key,
                                 const void *value);

 void * __cdecl pthread_getspecific (pthread_key_t key);





 int __cdecl pthread_mutexattr_init (pthread_mutexattr_t * attr);

 int __cdecl pthread_mutexattr_destroy (pthread_mutexattr_t * attr);

 int __cdecl pthread_mutexattr_getpshared (const pthread_mutexattr_t
                                          * attr,
                                          int *pshared);

 int __cdecl pthread_mutexattr_setpshared (pthread_mutexattr_t * attr,
                                          int pshared);

 int __cdecl pthread_mutexattr_settype (pthread_mutexattr_t * attr, int kind);
 int __cdecl pthread_mutexattr_gettype (pthread_mutexattr_t * attr, int *kind);




 int __cdecl pthread_barrierattr_init (pthread_barrierattr_t * attr);

 int __cdecl pthread_barrierattr_destroy (pthread_barrierattr_t * attr);

 int __cdecl pthread_barrierattr_getpshared (const pthread_barrierattr_t
                                            * attr,
                                            int *pshared);

 int __cdecl pthread_barrierattr_setpshared (pthread_barrierattr_t * attr,
                                            int pshared);




 int __cdecl pthread_mutex_init (pthread_mutex_t * mutex,
                                const pthread_mutexattr_t * attr);

 int __cdecl pthread_mutex_destroy (pthread_mutex_t * mutex);

 int __cdecl pthread_mutex_lock (pthread_mutex_t * mutex);

 int __cdecl pthread_mutex_timedlock(pthread_mutex_t *mutex,
                                    const struct timespec *abstime);

 int __cdecl pthread_mutex_trylock (pthread_mutex_t * mutex);

 int __cdecl pthread_mutex_unlock (pthread_mutex_t * mutex);




 int __cdecl pthread_spin_init (pthread_spinlock_t * lock, int pshared);

 int __cdecl pthread_spin_destroy (pthread_spinlock_t * lock);

 int __cdecl pthread_spin_lock (pthread_spinlock_t * lock);

 int __cdecl pthread_spin_trylock (pthread_spinlock_t * lock);

 int __cdecl pthread_spin_unlock (pthread_spinlock_t * lock);




 int __cdecl pthread_barrier_init (pthread_barrier_t * barrier,
                                  const pthread_barrierattr_t * attr,
                                  unsigned int count);

 int __cdecl pthread_barrier_destroy (pthread_barrier_t * barrier);

 int __cdecl pthread_barrier_wait (pthread_barrier_t * barrier);




 int __cdecl pthread_condattr_init (pthread_condattr_t * attr);

 int __cdecl pthread_condattr_destroy (pthread_condattr_t * attr);

 int __cdecl pthread_condattr_getpshared (const pthread_condattr_t * attr,
                                         int *pshared);

 int __cdecl pthread_condattr_setpshared (pthread_condattr_t * attr,
                                         int pshared);




 int __cdecl pthread_cond_init (pthread_cond_t * cond,
                               const pthread_condattr_t * attr);

 int __cdecl pthread_cond_destroy (pthread_cond_t * cond);

 int __cdecl pthread_cond_wait (pthread_cond_t * cond,
                               pthread_mutex_t * mutex);

 int __cdecl pthread_cond_timedwait (pthread_cond_t * cond,
                                    pthread_mutex_t * mutex,
                                    const struct timespec *abstime);

 int __cdecl pthread_cond_signal (pthread_cond_t * cond);

 int __cdecl pthread_cond_broadcast (pthread_cond_t * cond);




 int __cdecl pthread_setschedparam (pthread_t thread,
                                   int policy,
                                   const struct sched_param *param);

 int __cdecl pthread_getschedparam (pthread_t thread,
                                   int *policy,
                                   struct sched_param *param);

 int __cdecl pthread_setconcurrency (int);
 
 int __cdecl pthread_getconcurrency (void);




 int __cdecl pthread_rwlock_init(pthread_rwlock_t *lock,
                                const pthread_rwlockattr_t *attr);

 int __cdecl pthread_rwlock_destroy(pthread_rwlock_t *lock);

 int __cdecl pthread_rwlock_tryrdlock(pthread_rwlock_t *);

 int __cdecl pthread_rwlock_trywrlock(pthread_rwlock_t *);

 int __cdecl pthread_rwlock_rdlock(pthread_rwlock_t *lock);

 int __cdecl pthread_rwlock_timedrdlock(pthread_rwlock_t *lock,
                                       const struct timespec *abstime);

 int __cdecl pthread_rwlock_wrlock(pthread_rwlock_t *lock);

 int __cdecl pthread_rwlock_timedwrlock(pthread_rwlock_t *lock,
                                       const struct timespec *abstime);

 int __cdecl pthread_rwlock_unlock(pthread_rwlock_t *lock);

 int __cdecl pthread_rwlockattr_init (pthread_rwlockattr_t * attr);

 int __cdecl pthread_rwlockattr_destroy (pthread_rwlockattr_t * attr);

 int __cdecl pthread_rwlockattr_getpshared (const pthread_rwlockattr_t * attr,
                                           int *pshared);

 int __cdecl pthread_rwlockattr_setpshared (pthread_rwlockattr_t * attr,
                                           int pshared);







 int __cdecl pthread_kill(pthread_t thread, int sig);








 int __cdecl pthread_mutexattr_setkind_np(pthread_mutexattr_t * attr,
                                         int kind);
 int __cdecl pthread_mutexattr_getkind_np(pthread_mutexattr_t * attr,
                                         int *kind);




 int __cdecl pthread_delay_np (struct timespec * interval);
 int __cdecl pthread_num_processors_np(void);





 int __cdecl pthread_win32_process_attach_np(void);
 int __cdecl pthread_win32_process_detach_np(void);
 int __cdecl pthread_win32_thread_attach_np(void);
 int __cdecl pthread_win32_thread_detach_np(void);




 int __cdecl pthread_win32_test_features_np(int);
enum ptw32_features {
  PTW32_SYSTEM_INTERLOCKED_COMPARE_EXCHANGE = 0x0001, 
  PTW32_ALERTABLE_ASYNC_CANCEL              = 0x0002  
};









 void * __cdecl pthread_timechange_handler_np(void *);

#line 1180 "..\\..\\..\\pthreads-w32\\pthread.h"






 void * __cdecl pthread_getw32threadhandle_np(pthread_t thread);

















 int __cdecl pthreadCancelableWait (void * waitHandle);
 int __cdecl pthreadCancelableTimedWait (void * waitHandle,
                                        unsigned long timeout);

#line 1209 "..\\..\\..\\pthreads-w32\\pthread.h"







#line 1217 "..\\..\\..\\pthreads-w32\\pthread.h"




#line 1222 "..\\..\\..\\pthreads-w32\\pthread.h"
#line 1223 "..\\..\\..\\pthreads-w32\\pthread.h"
#line 1224 "..\\..\\..\\pthreads-w32\\pthread.h"
#line 1225 "..\\..\\..\\pthreads-w32\\pthread.h"













#line 1239 "..\\..\\..\\pthreads-w32\\pthread.h"



























#line 1267 "..\\..\\..\\pthreads-w32\\pthread.h"






class ptw32_exception {};
class ptw32_exception_cancel : public ptw32_exception {};
class ptw32_exception_exit   : public ptw32_exception {};

#line 1278 "..\\..\\..\\pthreads-w32\\pthread.h"







 unsigned long __cdecl ptw32_get_exception_services_code(void);

#line 1288 "..\\..\\..\\pthreads-w32\\pthread.h"






























































#line 1351 "..\\..\\..\\pthreads-w32\\pthread.h"


}                               
#line 1355 "..\\..\\..\\pthreads-w32\\pthread.h"



#line 1359 "..\\..\\..\\pthreads-w32\\pthread.h"


#line 1362 "..\\..\\..\\pthreads-w32\\pthread.h"




#line 1367 "..\\..\\..\\pthreads-w32\\pthread.h"

#line 1369 "..\\..\\..\\pthreads-w32\\pthread.h"
#line 5 "..\\..\\..\\include\\yog/vm.h"


#line 8 "..\\..\\..\\include\\yog/vm.h"
#line 1 "..\\..\\..\\include\\yog/gc.h"















































#line 49 "..\\..\\..\\include\\yog/gc.h"



#line 9 "..\\..\\..\\include\\yog/vm.h"

#line 1 "..\\..\\..\\include\\yog/gc/copying.h"










































































#line 76 "..\\..\\..\\include\\yog/gc/copying.h"



#line 11 "..\\..\\..\\include\\yog/vm.h"








#line 20 "..\\..\\..\\include\\yog/vm.h"
#line 1 "..\\..\\..\\include\\yog/yog.h"





















































































































































#line 151 "..\\..\\..\\include\\yog/yog.h"



#line 21 "..\\..\\..\\include\\yog/vm.h"



struct YogVM {
    int_t gc_stress;

    ID next_id;
    YogVal id2name;
    YogVal name2id;
    pthread_rwlock_t sym_lock;

    YogVal cArray;
    YogVal cBignum;
    YogVal cBool;
    YogVal cClassMethod;
    YogVal cCode;
    YogVal cDict;
    YogVal cFile;
    YogVal cFixnum;
    YogVal cFloat;
    YogVal cFunction;
    YogVal cInstanceMethod;
    YogVal cClass;
    YogVal cMatch;
    YogVal cModule;
    YogVal cNativeFunction;
    YogVal cNativeInstanceMethod;
    YogVal cNil;
    YogVal cObject;
    YogVal cPackage;
    YogVal cPackageBlock;
    YogVal cProperty;
    YogVal cRegexp;
    YogVal cSet;
    YogVal cString;
    YogVal cSymbol;
    YogVal cThread;

    YogVal eArgumentError;
    YogVal eAttributeError;
    YogVal eEOFError;
    YogVal eException;
    YogVal eImportError;
    YogVal eIndexError;
    YogVal eKeyError;
    YogVal eLocalJumpError;
    YogVal eNameError;
    YogVal eSyntaxError;
    YogVal eTypeError;
    YogVal eValueError;
    YogVal eZeroDivisionError;

    YogVal mComparable;

    YogVal pkgs;
    pthread_rwlock_t pkgs_lock;
    YogVal search_path;

    YogVal encodings;

    YogVal finish_code;

    YogVal main_thread;
    YogVal running_threads;
    uint_t next_thread_id;
    pthread_mutex_t next_thread_id_lock;

    pthread_mutex_t global_interp_lock;
    int_t running_gc;
    int_t waiting_suspend;
    uint_t suspend_counter;
    pthread_cond_t threads_suspend_cond;
    pthread_cond_t gc_finish_cond;
    void* heaps;
    void* last_heap;
    pthread_cond_t vm_finish_cond;
    uint_t gc_id;



#line 102 "..\\..\\..\\include\\yog/vm.h"
};

typedef struct YogVM YogVM;








void YogVM_acquire_global_interp_lock(YogEnv*, YogVM*);
void YogVM_add_heap(YogEnv*, YogVM*, void*);
void YogVM_add_thread(YogEnv*, YogVM*, YogVal);
void YogVM_boot(YogEnv*, YogVM*, uint_t, char**);
void YogVM_configure_search_path(YogEnv*, YogVM*, const char*);
void YogVM_delete(YogEnv*, YogVM*);
YogVal YogVM_id2name(YogEnv*, YogVM*, ID);
YogVal YogVM_import_package(YogEnv*, YogVM*, ID);
void YogVM_init(YogVM*);
ID YogVM_intern(YogEnv*, YogVM*, const char*);
uint_t YogVM_issue_thread_id(YogEnv*, YogVM*);
void YogVM_keep_children(YogEnv*, void*, ObjectKeeper, void*);
void YogVM_register_package(YogEnv*, YogVM*, const char*, YogVal);
void YogVM_release_global_interp_lock(YogEnv*, YogVM*);
void YogVM_remove_thread(YogEnv*, YogVM*, YogVal);
void YogVM_set_main_thread(YogEnv*, YogVM*, YogVal);
void YogVM_wait_finish(YogEnv*, YogVM*);



#line 134 "..\\..\\..\\include\\yog/vm.h"



#line 5 "..\\..\\..\\include\\yog/env.h"

struct YogEnv {
    struct YogVM* vm;
    YogVal thread;
};

#line 12 "..\\..\\..\\include\\yog/env.h"



#line 44 "..\\..\\..\\include\\yog/thread.h"
















#line 61 "..\\..\\..\\include\\yog/thread.h"





#line 67 "..\\..\\..\\include\\yog/thread.h"



















































































struct YogThread {
    struct YogBasicObj base;

    YogVal prev;
    YogVal next;

    uint_t thread_id;
    uint_t next_obj_id;

    void* heap;
    struct YogLocals* locals;
    int_t gc_bound;

    YogVal cur_frame;

    struct YogJmpBuf* jmp_buf_list;
    YogVal jmp_val;
    YogVal frame_to_long_jump;

    YogVal block;

    pthread_t pthread;

    YogVal recursive_stack;
};

typedef struct YogThread YogThread;












#line 190 "..\\..\\..\\include\\yog/thread.h"
































void YogThread_config_bdw(YogEnv*, YogVal);
void YogThread_config_copying(YogEnv*, YogVal, size_t);
void YogThread_config_generational(YogEnv*, YogVal, size_t, size_t, size_t, uint_t);
void YogThread_config_mark_sweep(YogEnv*, YogVal, size_t);
void YogThread_config_mark_sweep_compact(YogEnv*, YogVal, size_t, size_t);
YogVal YogThread_define_class(YogEnv*);
void YogThread_init(YogEnv*, YogVal, YogVal);
void YogThread_issue_object_id(YogEnv*, YogVal, YogVal);
YogVal YogThread_new(YogEnv*);



#line 235 "..\\..\\..\\include\\yog/thread.h"



#line 13 "..\\..\\..\\src\\array.c"
#line 1 "..\\..\\..\\include\\yog/yog.h"





















































































































































#line 151 "..\\..\\..\\include\\yog/yog.h"



#line 14 "..\\..\\..\\src\\array.c"

uint_t
YogValArray_size(YogEnv* env, YogVal array)
{
    return ((YogValArray*)((void*)(array)))->size;
}

YogVal
YogValArray_at(YogEnv* env, YogVal array, uint_t n)
{
    uint_t size = ((YogValArray*)((void*)(array)))->size;
    YOG_ASSERT(env, n < size, "Index exceed array body size.");
    return ((YogValArray*)((void*)(array)))->items[n];
}

YogVal
YogArray_at(YogEnv* env, YogVal array, uint_t n)
{
    size_t size = ((YogArray*)((void*)(array)))->size;
    YOG_ASSERT(env, n < size, "Index exceed array size.");

    return YogValArray_at(env, ((YogArray*)((void*)(array)))->body, n);
}

uint_t
YogArray_size(YogEnv* env, YogVal array)
{
    return ((YogArray*)((void*)(array)))->size;
}

static void
YogValArray_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogValArray* array = ((YogValArray*)((void*)(ptr)));

    uint_t size = array->size;
    uint_t i = 0;
    for (i = 0; i < size; i++) {
        YogGC_keep(env, &array->items[i], keeper, heap);
    }
}

YogVal
YogValArray_new(YogEnv* env, uint_t size)
{
    YogVal array = YogGC_allocate(env, YogValArray_keep_children, 0, sizeof(YogValArray) + size * sizeof(YogVal));
    ((YogValArray*)((void*)(array)))->size = size;
	uint_t i;
    for (i = 0; i < size; i++) {
        ((YogValArray*)((void*)(array)))->items[i] = 0x02;
    }

    return array;
}

static uint_t
multiple_size(uint_t min_size, uint_t cur_size, uint_t ratio)
{
    while (cur_size < min_size) {
        cur_size *= ratio;
    }
    return cur_size;
}

static uint_t
get_next_size(uint_t min_size, uint_t cur_size, uint_t ratio)
{
    if (cur_size == 0) {
        return 1;
    }

    return multiple_size(min_size, cur_size, ratio);
}

static void
ensure_body_size(YogEnv* env, YogVal array, uint_t size)
{
    YogLocals* __cur_locals__ = ((YogThread*)((void*)(((env))->thread)))->locals; YogLocals __locals_array__; __locals_array__.num_vals = 1; __locals_array__.size = 1; __locals_array__.vals[0] = &(array); __locals_array__.vals[1] = 0; __locals_array__.vals[2] = 0; __locals_array__.vals[3] = 0; do { __locals_array__.next = ((YogThread*)((void*)(((env))->thread)))->locals; ((YogThread*)((void*)(((env))->thread)))->locals = &__locals_array__; } while (0);;

    YogVal old_body = 0x02;
    YogLocals __locals_old_body__; __locals_old_body__.num_vals = 1; __locals_old_body__.size = 1; __locals_old_body__.vals[0] = &(old_body); __locals_old_body__.vals[1] = 0; __locals_old_body__.vals[2] = 0; __locals_old_body__.vals[3] = 0; do { __locals_old_body__.next = ((YogThread*)((void*)((env)->thread)))->locals; ((YogThread*)((void*)((env)->thread)))->locals = &__locals_old_body__; } while (0);;

    YogVal body = ((YogArray*)((void*)(array)))->body;
    if (((YogValArray*)((void*)(body)))->size < size) {
        old_body = body;
        size_t old_size = ((YogValArray*)((void*)(old_body)))->size;

        uint_t new_size = get_next_size(size, old_size, (2));

        YogVal new_body = YogValArray_new(env, new_size);
        size_t cur_size = ((YogArray*)((void*)(array)))->size;
        YogVal* to = ((YogValArray*)((void*)(new_body)))->items;
        YogVal* from = ((YogValArray*)((void*)(old_body)))->items;
        memcpy(to, from, sizeof(YogVal) * cur_size);

        ((YogArray*)((void*)(array)))->body = new_body;
    }

    do { ((YogThread*)((void*)((env)->thread)))->locals = __cur_locals__; return; } while (0);
}

void
YogArray_push(YogEnv* env, YogVal array, YogVal val)
{
    YogLocals* __cur_locals__ = ((YogThread*)((void*)(((env))->thread)))->locals; YogLocals __locals_array_val__; __locals_array_val__.num_vals = 2; __locals_array_val__.size = 1; __locals_array_val__.vals[0] = &(array); __locals_array_val__.vals[1] = &(val); __locals_array_val__.vals[2] = 0; __locals_array_val__.vals[3] = 0; do { __locals_array_val__.next = ((YogThread*)((void*)(((env))->thread)))->locals; ((YogThread*)((void*)(((env))->thread)))->locals = &__locals_array_val__; } while (0);;

    ensure_body_size(env, array, YogArray_size(env, array) + 1);

    YogVal body = ((YogArray*)((void*)(array)))->body;
    size_t size = ((YogArray*)((void*)(array)))->size;
    ((YogValArray*)((void*)(body)))->items[size] = val;
    ((YogArray*)((void*)(array)))->size++;

    do { ((YogThread*)((void*)((env)->thread)))->locals = __cur_locals__; return; } while (0);
}

void
YogArray_extend(YogEnv* env, YogVal array, YogVal a)
{
    YogLocals* __cur_locals__ = ((YogThread*)((void*)(((env))->thread)))->locals; YogLocals __locals_array_a__; __locals_array_a__.num_vals = 2; __locals_array_a__.size = 1; __locals_array_a__.vals[0] = &(array); __locals_array_a__.vals[1] = &(a); __locals_array_a__.vals[2] = 0; __locals_array_a__.vals[3] = 0; do { __locals_array_a__.next = ((YogThread*)((void*)(((env))->thread)))->locals; ((YogThread*)((void*)(((env))->thread)))->locals = &__locals_array_a__; } while (0);;

    uint_t old_size = YogArray_size(env, array);
    uint_t new_size = old_size + YogArray_size(env, a);
    ensure_body_size(env, array, new_size);

    YogVal to = ((YogArray*)((void*)(array)))->body;
    YogVal* p = &((YogValArray*)((void*)(to)))->items[old_size];
    YogVal from = ((YogArray*)((void*)(a)))->body;
    YogVal* q = &((YogValArray*)((void*)(from)))->items[0];
    memcpy(p, q, sizeof(YogVal) * YogArray_size(env, a));

    ((YogArray*)((void*)(array)))->size = new_size;

    do { ((YogThread*)((void*)((env)->thread)))->locals = __cur_locals__; return; } while (0);
}

static void
YogArray_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    YogArray* array = ((YogArray*)((void*)(ptr)));
    YogGC_keep(env, &array->body, keeper, heap);
}

static YogVal
allocate_object(YogEnv* env, YogVal klass, uint_t size)
{
    YogLocals* __cur_locals__ = ((YogThread*)((void*)(((env))->thread)))->locals; YogLocals __locals_klass__; __locals_klass__.num_vals = 1; __locals_klass__.size = 1; __locals_klass__.vals[0] = &(klass); __locals_klass__.vals[1] = 0; __locals_klass__.vals[2] = 0; __locals_klass__.vals[3] = 0; do { __locals_klass__.next = ((YogThread*)((void*)(((env))->thread)))->locals; ((YogThread*)((void*)(((env))->thread)))->locals = &__locals_klass__; } while (0);;

    YogVal body = 0x02;
    YogVal array = 0x02;
    YogLocals __locals_body_array__; __locals_body_array__.num_vals = 2; __locals_body_array__.size = 1; __locals_body_array__.vals[0] = &(body); __locals_body_array__.vals[1] = &(array); __locals_body_array__.vals[2] = 0; __locals_body_array__.vals[3] = 0; do { __locals_body_array__.next = ((YogThread*)((void*)((env)->thread)))->locals; ((YogThread*)((void*)((env)->thread)))->locals = &__locals_body_array__; } while (0);;

    body = YogValArray_new(env, size);
    array = YogGC_allocate(env, YogArray_keep_children, 0, sizeof(YogArray));
    YogBasicObj_init(env, array, 0, klass);
    ((YogArray*)((void*)(array)))->size = 0;
    ((YogArray*)((void*)(array)))->body = body;

    do { ((YogThread*)((void*)((env)->thread)))->locals = __cur_locals__; return array; } while (0);
}

YogVal
YogArray_of_size(YogEnv* env, uint_t size)
{
    return allocate_object(env, env->vm->cArray, size);
}

static YogVal
allocate(YogEnv* env, YogVal klass)
{
    YogLocals* __cur_locals__ = ((YogThread*)((void*)(((env))->thread)))->locals; YogLocals __locals_klass__; __locals_klass__.num_vals = 1; __locals_klass__.size = 1; __locals_klass__.vals[0] = &(klass); __locals_klass__.vals[1] = 0; __locals_klass__.vals[2] = 0; __locals_klass__.vals[3] = 0; do { __locals_klass__.next = ((YogThread*)((void*)(((env))->thread)))->locals; ((YogThread*)((void*)(((env))->thread)))->locals = &__locals_klass__; } while (0);;

    YogVal array = 0x02;
    YogLocals __locals_array__; __locals_array__.num_vals = 1; __locals_array__.size = 1; __locals_array__.vals[0] = &(array); __locals_array__.vals[1] = 0; __locals_array__.vals[2] = 0; __locals_array__.vals[3] = 0; do { __locals_array__.next = ((YogThread*)((void*)((env)->thread)))->locals; ((YogThread*)((void*)((env)->thread)))->locals = &__locals_array__; } while (0);;


    array = allocate_object(env, klass, 1);


    do { ((YogThread*)((void*)((env)->thread)))->locals = __cur_locals__; return array; } while (0);
}

YogVal
YogArray_new(YogEnv* env)
{

    return YogArray_of_size(env, (1));

}

YogVal
YogArray_add(YogEnv* env, YogVal self, YogVal array)
{
    YogLocals* __cur_locals__ = ((YogThread*)((void*)(((env))->thread)))->locals; YogLocals __locals_self_array__; __locals_self_array__.num_vals = 2; __locals_self_array__.size = 1; __locals_self_array__.vals[0] = &(self); __locals_self_array__.vals[1] = &(array); __locals_self_array__.vals[2] = 0; __locals_self_array__.vals[3] = 0; do { __locals_self_array__.next = ((YogThread*)((void*)(((env))->thread)))->locals; ((YogThread*)((void*)(((env))->thread)))->locals = &__locals_self_array__; } while (0);;
    YogVal val = 0x02;
    YogLocals __locals_val__; __locals_val__.num_vals = 1; __locals_val__.size = 1; __locals_val__.vals[0] = &(val); __locals_val__.vals[1] = 0; __locals_val__.vals[2] = 0; __locals_val__.vals[3] = 0; do { __locals_val__.next = ((YogThread*)((void*)((env)->thread)))->locals; ((YogThread*)((void*)((env)->thread)))->locals = &__locals_val__; } while (0);;

    uint_t size = YogArray_size(env, array);
    uint_t i;
    for (i = 0; i < size; i++) {
        val = YogArray_at(env, array, i);
        YogArray_push(env, self, val);
    }

    do { ((YogThread*)((void*)((env)->thread)))->locals = __cur_locals__; return self; } while (0);
}

static YogVal
add(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    YogLocals* __cur_locals__ = ((YogThread*)((void*)(((env))->thread)))->locals; YogLocals __locals_self_args_kw_block__; __locals_self_args_kw_block__.num_vals = 4; __locals_self_args_kw_block__.size = 1; __locals_self_args_kw_block__.vals[0] = &(self); __locals_self_args_kw_block__.vals[1] = &(args); __locals_self_args_kw_block__.vals[2] = &(kw); __locals_self_args_kw_block__.vals[3] = &(block); do { __locals_self_args_kw_block__.next = ((YogThread*)((void*)(((env))->thread)))->locals; ((YogThread*)((void*)(((env))->thread)))->locals = &__locals_self_args_kw_block__; } while (0);;
    YogVal array = 0x02;
    YogVal right = 0x02;
    YogVal val = 0x02;
    YogLocals __locals_array_right_val__; __locals_array_right_val__.num_vals = 3; __locals_array_right_val__.size = 1; __locals_array_right_val__.vals[0] = &(array); __locals_array_right_val__.vals[1] = &(right); __locals_array_right_val__.vals[2] = &(val); __locals_array_right_val__.vals[3] = 0; do { __locals_array_right_val__.next = ((YogThread*)((void*)((env)->thread)))->locals; ((YogThread*)((void*)((env)->thread)))->locals = &__locals_array_right_val__; } while (0);;

    right = YogArray_at(env, args, 0);
    YOG_ASSERT(env, (((right) & 0x03) == 0), "operand is not Array");
    YOG_ASSERT(env, ((((YogBasicObj*)((void*)((right))))->klass) == (env)->vm->cArray), "operand is not Array");

    array = YogArray_new(env);
    YogArray_add(env, array, self);
    YogArray_add(env, array, right);

    do { ((YogThread*)((void*)((env)->thread)))->locals = __cur_locals__; return array; } while (0);
}

static YogVal
lshift(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    YogLocals* __cur_locals__ = ((YogThread*)((void*)(((env))->thread)))->locals; YogLocals __locals_self_args_kw_block__; __locals_self_args_kw_block__.num_vals = 4; __locals_self_args_kw_block__.size = 1; __locals_self_args_kw_block__.vals[0] = &(self); __locals_self_args_kw_block__.vals[1] = &(args); __locals_self_args_kw_block__.vals[2] = &(kw); __locals_self_args_kw_block__.vals[3] = &(block); do { __locals_self_args_kw_block__.next = ((YogThread*)((void*)(((env))->thread)))->locals; ((YogThread*)((void*)(((env))->thread)))->locals = &__locals_self_args_kw_block__; } while (0);;
    YogVal elem = YogArray_at(env, args, 0);
    YogArray_push(env, self, elem);
    do { ((YogThread*)((void*)((env)->thread)))->locals = __cur_locals__; return self; } while (0);
}

static YogVal
assign_subscript(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    YogLocals* __cur_locals__ = ((YogThread*)((void*)(((env))->thread)))->locals; YogLocals __locals_self_args_kw_block__; __locals_self_args_kw_block__.num_vals = 4; __locals_self_args_kw_block__.size = 1; __locals_self_args_kw_block__.vals[0] = &(self); __locals_self_args_kw_block__.vals[1] = &(args); __locals_self_args_kw_block__.vals[2] = &(kw); __locals_self_args_kw_block__.vals[3] = &(block); do { __locals_self_args_kw_block__.next = ((YogThread*)((void*)(((env))->thread)))->locals; ((YogThread*)((void*)(((env))->thread)))->locals = &__locals_self_args_kw_block__; } while (0);;
    YogVal val = 0x02;
    YogVal body = 0x02;
    YogLocals __locals_val_body__; __locals_val_body__.num_vals = 2; __locals_val_body__.size = 1; __locals_val_body__.vals[0] = &(val); __locals_val_body__.vals[1] = &(body); __locals_val_body__.vals[2] = 0; __locals_val_body__.vals[3] = 0; do { __locals_val_body__.next = ((YogThread*)((void*)((env)->thread)))->locals; ((YogThread*)((void*)((env)->thread)))->locals = &__locals_val_body__; } while (0);;

    uint_t index = ((int)(YogArray_at(env, args, 0)) >> 1);
    uint_t size = YogArray_size(env, self);
    if (size <= index) {
        YogError_raise_IndexError(env, "array assignment index out of range");
    }

    val = YogArray_at(env, args, 1);

    body = ((YogArray*)((void*)(self)))->body;
    ((YogValArray*)((void*)(body)))->items[index] = val;

    do { ((YogThread*)((void*)((env)->thread)))->locals = __cur_locals__; return self; } while (0);
}

static YogVal
subscript(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    YogLocals* __cur_locals__ = ((YogThread*)((void*)(((env))->thread)))->locals; YogLocals __locals_self_args_kw_block__; __locals_self_args_kw_block__.num_vals = 4; __locals_self_args_kw_block__.size = 1; __locals_self_args_kw_block__.vals[0] = &(self); __locals_self_args_kw_block__.vals[1] = &(args); __locals_self_args_kw_block__.vals[2] = &(kw); __locals_self_args_kw_block__.vals[3] = &(block); do { __locals_self_args_kw_block__.next = ((YogThread*)((void*)(((env))->thread)))->locals; ((YogThread*)((void*)(((env))->thread)))->locals = &__locals_self_args_kw_block__; } while (0);;
    YogVal index = YogArray_at(env, args, 0);
    YogVal v = YogArray_at(env, self, ((int)(index) >> 1));
    do { ((YogThread*)((void*)((env)->thread)))->locals = __cur_locals__; return v; } while (0);
}

static YogVal
each(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    YogLocals* __cur_locals__ = ((YogThread*)((void*)(((env))->thread)))->locals; YogLocals __locals_self_args_kw_block__; __locals_self_args_kw_block__.num_vals = 4; __locals_self_args_kw_block__.size = 1; __locals_self_args_kw_block__.vals[0] = &(self); __locals_self_args_kw_block__.vals[1] = &(args); __locals_self_args_kw_block__.vals[2] = &(kw); __locals_self_args_kw_block__.vals[3] = &(block); do { __locals_self_args_kw_block__.next = ((YogThread*)((void*)(((env))->thread)))->locals; ((YogThread*)((void*)(((env))->thread)))->locals = &__locals_self_args_kw_block__; } while (0);;

    YogVal arg[] = { 0x02 };
    YogLocals __locals_arg__; __locals_arg__.num_vals = 1; __locals_arg__.size = ((sizeof(arg) / sizeof(arg[0]))); __locals_arg__.vals[0] = (arg); __locals_arg__.vals[1] = 0; __locals_arg__.vals[2] = 0; __locals_arg__.vals[3] = 0; do { __locals_arg__.next = ((YogThread*)((void*)((env)->thread)))->locals; ((YogThread*)((void*)((env)->thread)))->locals = &__locals_arg__; } while (0);;

    uint_t size = YogArray_size(env, self);
    uint_t i;
    for (i = 0; i < size; i++) {
        arg[0] = YogArray_at(env, self, i);
        YogCallable_call(env, block, (sizeof(arg) / sizeof(arg[0])), arg);
    }

    do { ((YogThread*)((void*)((env)->thread)))->locals = __cur_locals__; return self; } while (0);
}

static YogVal
get_size(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    YogLocals* __cur_locals__ = ((YogThread*)((void*)(((env))->thread)))->locals; YogLocals __locals_self_args_kw_block__; __locals_self_args_kw_block__.num_vals = 4; __locals_self_args_kw_block__.size = 1; __locals_self_args_kw_block__.vals[0] = &(self); __locals_self_args_kw_block__.vals[1] = &(args); __locals_self_args_kw_block__.vals[2] = &(kw); __locals_self_args_kw_block__.vals[3] = &(block); do { __locals_self_args_kw_block__.next = ((YogThread*)((void*)(((env))->thread)))->locals; ((YogThread*)((void*)(((env))->thread)))->locals = &__locals_self_args_kw_block__; } while (0);;
    YogVal retval = 0x02;
    YogLocals __locals_retval__; __locals_retval__.num_vals = 1; __locals_retval__.size = 1; __locals_retval__.vals[0] = &(retval); __locals_retval__.vals[1] = 0; __locals_retval__.vals[2] = 0; __locals_retval__.vals[3] = 0; do { __locals_retval__.next = ((YogThread*)((void*)((env)->thread)))->locals; ((YogThread*)((void*)((env)->thread)))->locals = &__locals_retval__; } while (0);;

    uint_t size = YogArray_size(env, self);
    
    retval = (((size) << 1) + 1);

    do { ((YogThread*)((void*)((env)->thread)))->locals = __cur_locals__; return retval; } while (0);
}

YogVal
YogArray_shift(YogEnv* env, YogVal self)
{
    YogLocals* __cur_locals__ = ((YogThread*)((void*)(((env))->thread)))->locals; YogLocals __locals_self__; __locals_self__.num_vals = 1; __locals_self__.size = 1; __locals_self__.vals[0] = &(self); __locals_self__.vals[1] = 0; __locals_self__.vals[2] = 0; __locals_self__.vals[3] = 0; do { __locals_self__.next = ((YogThread*)((void*)(((env))->thread)))->locals; ((YogThread*)((void*)(((env))->thread)))->locals = &__locals_self__; } while (0);;
    YogVal retval = 0x02;
    YogVal body = 0x02;
    YogVal elem = 0x02;
    YogLocals __locals_retval_body_elem__; __locals_retval_body_elem__.num_vals = 3; __locals_retval_body_elem__.size = 1; __locals_retval_body_elem__.vals[0] = &(retval); __locals_retval_body_elem__.vals[1] = &(body); __locals_retval_body_elem__.vals[2] = &(elem); __locals_retval_body_elem__.vals[3] = 0; do { __locals_retval_body_elem__.next = ((YogThread*)((void*)((env)->thread)))->locals; ((YogThread*)((void*)((env)->thread)))->locals = &__locals_retval_body_elem__; } while (0);;

    uint_t size = YogArray_size(env, self);
    YOG_ASSERT(env, 0 < size, "array is empty");

    retval = YogArray_at(env, self, 0);

    body = ((YogArray*)((void*)(self)))->body;
    uint_t i;
    for (i = 1; i < size; i++) {
        elem = YogArray_at(env, self, i);
        ((YogValArray*)((void*)(body)))->items[i - 1] = elem;
    }
    ((YogValArray*)((void*)(body)))->items[size - 1] = 0x02;

    ((YogArray*)((void*)(self)))->size--;

    do { ((YogThread*)((void*)((env)->thread)))->locals = __cur_locals__; return retval; } while (0);
}

static YogVal
YogArray_pop(YogEnv* env, YogVal self)
{
    YogLocals* __cur_locals__ = ((YogThread*)((void*)(((env))->thread)))->locals; YogLocals __locals_self__; __locals_self__.num_vals = 1; __locals_self__.size = 1; __locals_self__.vals[0] = &(self); __locals_self__.vals[1] = 0; __locals_self__.vals[2] = 0; __locals_self__.vals[3] = 0; do { __locals_self__.next = ((YogThread*)((void*)(((env))->thread)))->locals; ((YogThread*)((void*)(((env))->thread)))->locals = &__locals_self__; } while (0);;
    YogVal retval = 0x02;
    YogLocals __locals_retval__; __locals_retval__.num_vals = 1; __locals_retval__.size = 1; __locals_retval__.vals[0] = &(retval); __locals_retval__.vals[1] = 0; __locals_retval__.vals[2] = 0; __locals_retval__.vals[3] = 0; do { __locals_retval__.next = ((YogThread*)((void*)((env)->thread)))->locals; ((YogThread*)((void*)((env)->thread)))->locals = &__locals_retval__; } while (0);;

    uint_t size = YogArray_size(env, self);
    if (size < 1) {
        YogError_raise_IndexError(env, "pop from empty list");
    }

    retval = YogArray_at(env, self, size - 1);
    ((YogArray*)((void*)(self)))->size--;

    do { ((YogThread*)((void*)((env)->thread)))->locals = __cur_locals__; return retval; } while (0);
}

static YogVal
pop(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    YogLocals* __cur_locals__ = ((YogThread*)((void*)(((env))->thread)))->locals; YogLocals __locals_self_args_kw_block__; __locals_self_args_kw_block__.num_vals = 4; __locals_self_args_kw_block__.size = 1; __locals_self_args_kw_block__.vals[0] = &(self); __locals_self_args_kw_block__.vals[1] = &(args); __locals_self_args_kw_block__.vals[2] = &(kw); __locals_self_args_kw_block__.vals[3] = &(block); do { __locals_self_args_kw_block__.next = ((YogThread*)((void*)(((env))->thread)))->locals; ((YogThread*)((void*)(((env))->thread)))->locals = &__locals_self_args_kw_block__; } while (0);;
    YogVal obj = 0x02;
    YogLocals __locals_obj__; __locals_obj__.num_vals = 1; __locals_obj__.size = 1; __locals_obj__.vals[0] = &(obj); __locals_obj__.vals[1] = 0; __locals_obj__.vals[2] = 0; __locals_obj__.vals[3] = 0; do { __locals_obj__.next = ((YogThread*)((void*)((env)->thread)))->locals; ((YogThread*)((void*)((env)->thread)))->locals = &__locals_obj__; } while (0);;

    obj = YogArray_pop(env, self);

    do { ((YogThread*)((void*)((env)->thread)))->locals = __cur_locals__; return obj; } while (0);
}

static YogVal
push(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    YogLocals* __cur_locals__ = ((YogThread*)((void*)(((env))->thread)))->locals; YogLocals __locals_self_args_kw_block__; __locals_self_args_kw_block__.num_vals = 4; __locals_self_args_kw_block__.size = 1; __locals_self_args_kw_block__.vals[0] = &(self); __locals_self_args_kw_block__.vals[1] = &(args); __locals_self_args_kw_block__.vals[2] = &(kw); __locals_self_args_kw_block__.vals[3] = &(block); do { __locals_self_args_kw_block__.next = ((YogThread*)((void*)(((env))->thread)))->locals; ((YogThread*)((void*)(((env))->thread)))->locals = &__locals_self_args_kw_block__; } while (0);;
    YogVal obj = 0x02;
    YogLocals __locals_obj__; __locals_obj__.num_vals = 1; __locals_obj__.size = 1; __locals_obj__.vals[0] = &(obj); __locals_obj__.vals[1] = 0; __locals_obj__.vals[2] = 0; __locals_obj__.vals[3] = 0; do { __locals_obj__.next = ((YogThread*)((void*)((env)->thread)))->locals; ((YogThread*)((void*)((env)->thread)))->locals = &__locals_obj__; } while (0);;

    obj = YogArray_at(env, args, 0);
    YogArray_push(env, self, obj);

    do { ((YogThread*)((void*)((env)->thread)))->locals = __cur_locals__; return self; } while (0);
}

void
YogArray_eval_builtin_script(YogEnv* env, YogVal klass)
{

    const char* src =
#line 1 "f:\\projects\\yog\\src\\array.inc"
"\n"
"def join(sep)\n"
"  s = \"\"\n"
"  i = 0\n"
"  self.each() do [elem]\n"
"    nonlocal i\n"
"    if 0 < i\n"
"      s << sep\n"
"    end\n"
"    s << elem\n"
"    i += 1\n"
"  end\n"
"\n"
"  return s\n"
"end\n"
"\n"
"def include?(obj)\n"
"  self.each() do [elem]\n"
"    if (elem.class == obj.class) && (elem == obj)\n"
"      return true\n"
"    end\n"
"  end\n"
"\n"
"  return false\n"
"end\n"
"\n"
"def to_s()\n"
"  def f(obj)\n"
"    return \"[...]\"\n"
"  end\n"
"\n"
"  def g(obj)\n"
"    a = []\n"
"    obj.each() do [elem]\n"
"      a.push(elem.inspect())\n"
"    end\n"
"    return \"[\" + a.join(\", \") + \"]\"\n"
"  end\n"
"\n"
"  return __recurse__(self, f, g)\n"
"end\n"
"\n"
#line 391 "..\\..\\..\\src\\array.c"
    ;
    YogMisc_eval_source(env, klass, src);
#line 394 "..\\..\\..\\src\\array.c"
}

YogVal
YogArray_define_class(YogEnv* env)
{
    YogLocals* __cur_locals__ = ((YogThread*)((void*)((env)->thread)))->locals;

    YogVal klass = 0x02;
    YogLocals __locals_klass__; __locals_klass__.num_vals = 1; __locals_klass__.size = 1; __locals_klass__.vals[0] = &(klass); __locals_klass__.vals[1] = 0; __locals_klass__.vals[2] = 0; __locals_klass__.vals[3] = 0; do { __locals_klass__.next = ((YogThread*)((void*)((env)->thread)))->locals; ((YogThread*)((void*)((env)->thread)))->locals = &__locals_klass__; } while (0);;

    klass = YogClass_new(env, "Array", env->vm->cObject);
    YogClass_define_allocator(env, klass, allocate);

    YogClass_define_method(env, klass, "+", add);
    YogClass_define_method(env, klass, "<<", lshift);
    YogClass_define_method(env, klass, "[]", subscript);
    YogClass_define_method(env, klass, "[]=", assign_subscript);
    YogClass_define_method(env, klass, "each", each);
    YogClass_define_method(env, klass, "pop", pop);
    YogClass_define_method(env, klass, "push", push);

    YogClass_define_property(env, klass, "size", get_size, 0);

    do { ((YogThread*)((void*)((env)->thread)))->locals = __cur_locals__; return klass; } while (0);
}




