#ifndef _DBGOUT_H_
#include <windows.h>
static void DEBUG_OUT(const char *format, ... )
{
  static char buf[1024];
  va_list argl;

  va_start(argl,format);
  _vsnprintf(buf,1024,format,argl);
  OutputDebugString(buf);
  va_end(argl);
}

static void DEBUG_OUT_FILE(const char *format, ... )
{
  static FILE *fp=NULL;
  va_list argl;
  static char buf[1024];

  if(!fp) {
    fp=fopen("D:\\sndlog.log","a");
  }
  va_start(argl,format);
  _vsnprintf(buf,1024,format,argl);
  fprintf(fp,"%s",buf);
  va_end(argl);
}
#endif