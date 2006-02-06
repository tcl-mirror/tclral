#include <stdio.h>
#include <stdarg.h>
#include "log.h"

int nInfo ;
int nPass ;
int nFail ;

static void
vlogMsg(
    const char *label,
    const char *fmt,
    va_list ap)
{
    static char result[BUFSIZ] ;

    vsnprintf(result, sizeof(result), fmt, ap) ;
    printf("%s: %s\n", label, result) ;
}

void
logMsg(
    const char *label,
    const char *fmt,
    ...)
{
    va_list ap ;

    va_start(ap, fmt) ;
    vlogMsg(label, fmt, ap) ;
    va_end(ap) ;
}

void
logInfo(
    const char *fmt,
    ...)
{
    va_list ap ;

    va_start(ap, fmt) ;
    vlogMsg("INFO", fmt, ap) ;
    va_end(ap) ;
    ++nInfo ;
}

void
logPass(
    const char *fmt,
    ...)
{
    va_list ap ;

    va_start(ap, fmt) ;
    vlogMsg("PASS", fmt, ap) ;
    va_end(ap) ;
    ++nPass ;
}

void
logFail(
    const char *fmt,
    ...)
{
    va_list ap ;

    va_start(ap, fmt) ;
    vlogMsg("FAIL", fmt, ap) ;
    va_end(ap) ;
    ++nFail ;
}

void
logStart(void)
{
    nInfo = nPass = nFail = 0 ;
}

void
logSummarize(void)
{
    logMsg("SUMMARY", "%d passed", nPass) ;
    logMsg("SUMMARY", "%d failed", nFail) ;
    logMsg("SUMMARY", "%d informational", nInfo) ;
}
