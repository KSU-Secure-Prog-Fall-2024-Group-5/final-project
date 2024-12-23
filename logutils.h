#pragma once

#include <stddef.h>  // -> size_t, ptrdiff_t
#include <stdint.h>  // -> uint*_t
#include <stdlib.h>  // -> malloc, free
#include <stdbool.h> // -> bool

typedef enum {
	LOG_ROLE_EMPLOYEE = '&',
	LOG_ROLE_GUEST = '%',
} LogPersonRole;

typedef enum {
	LOG_EVENT_ARRIVAL = '>',
	LOG_EVENT_DEPARTURE = '<',
} LogEventType;

typedef struct {
	char *name;
	LogPersonRole role;
} LogPerson;

typedef struct {
	uint32_t timestamp; // within 0-3f..f
	uint32_t room_id;   // within 0-3f..f, or UINT32_MAX for entire building
	LogPerson person;
	LogEventType event;
} LogEntry;

typedef struct {
	size_t length;
	LogEntry *entry;
} LogEntryList;

typedef struct {
	char *token_to_save;
	LogEntryList entries;
} LogFile;

// STARTLOG and ENDLOG markers are not in LogEntries vec

LogFile *logfile_read(char *filename, char *given_token);
// ENDLOG is only checked for, not added to entries. if there's too many, don't
// care if not found, error, return NULL, panic

void logfile_write(char *, LogFile *);
// appends ENDLOG transparently

void logfile_free(LogFile *);

const char *validate_token(char *);
const char *validate_name(char *);

const char *logentry_validate(LogEntry *);

void logentry_push(LogEntryList *, LogEntry);
LogEntry logentry_pop(LogEntryList *);
void logentry_free(LogEntryList *);
// it's vaguely vec-like

/*
token is a password, we also have
SL_PUBLIC and SL_PRIVATE env vars
which will be generated and when a new log is created, it will use those keys.
*/
