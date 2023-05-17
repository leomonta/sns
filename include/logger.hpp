#pragma once

#define LOG_ALL     0
#define LOG_DEBUG   1
#define LOG_INFO    2
#define LOG_WARNING 3
#define LOG_ERROR   4
#define LOG_FATAL   5

static const char *sslErrStr[]{

    "SSL_ERROR_NONE",
    "SSL_ERROR_SSL",
    "SSL_ERROR_WANT_READ",
    "SSL_ERROR_WANT_WRITE",
    "SSL_ERROR_WANT_X509_LOOKUP",
    "SSL_ERROR_SYSCALL",
    "SSL_ERROR_ZERO_RETURN",
    "SSL_ERROR_WANT_CONNECT",
    "SSL_ERROR_WANT_ACCEPT",
    "SSL_ERROR_WANT_ASYNC",
    "SSL_ERROR_WANT_ASYNC_JOB",
    "SSL_ERROR_WANT_CLIENT_HELLO_CB",
    "SSL_ERROR_WANT_RETRY_VERIFY",
};

void log(const char logLevel, const char *mex, ...);

void setLogLevel(const char minLogLevel);