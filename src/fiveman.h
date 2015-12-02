#ifndef FIVEMAN_H
#define FIVEMAN_H

#include <string.h>

#define DEFAULT_FILENAME "Procfile"
#define DEFAULT_FILENAME_LENGTH (strlen(DEFAULT_FILENAME))
#define DIR_SEPARATOR "/"
#define DIR_SEPARATOR_LENGTH (strlen(DIR_SEPARATOR))
#define DEFAULT_COMMAND "start"
#define ACTIVE_STATUS_TITLE "[ACTIVE]"
#define INACTIVE_STATUS_TITLE "[INACTIVE]"
#define STOPPED_STATUS_TITLE "[STOPPED]"
#define BOTTOM_BAR_TEXT "[Up Arrow / Down Arrow] Choose Process | [l] Launch Process | [s] Stop Process | [o] View stdout | [e] View stderr | [q] Quit"
#define MICROSECONDS_PER_SECOND 1000000
#define SECONDS_PER_YEAR 31536000
#define SECONDS_PER_MONTH 2592000
#define SECONDS_PER_DAY 86400
#define SECONDS_PER_HOUR 3600
#define SECONDS_PER_MINUTE 60
#define BYTES_PER_KILOBYTE 1024L
#define BYTES_PER_MEGABYTE (BYTES_PER_KILOBYTE * 1024L)
#define BYTES_PER_GIGABYTE (BYTES_PER_MEGABYTE * 1024L)
#define BYTES_PER_TERABYTE (BYTES_PER_GIGABYTE * 1024L)
#define BYTES_PER_PETABYTE (BYTES_PER_TERABYTE * 1024L)
#define BYTES_PER_EXABYTE (BYTES_PER_TERABYTE * 1024L)

#endif
