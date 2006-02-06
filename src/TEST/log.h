extern void logInfo(const char *fmt, ...) ;
extern void logPass(const char *fmt, ...) ;
extern void logFail(const char *fmt, ...) ;
extern void logStart(void) ;
extern void logSummarize(void) ;

#define logTest(actual, expected) {\
    int a = actual ;\
    int e = expected ;\
    if (a == e) {\
	logPass("\"(" #actual ") == " #expected "\", expected = %d", e) ;\
    } else {\
	logFail("\"(" #actual ") != " #expected "\", actual %d, expected %d",\
	    a, e) ;\
    }\
}
