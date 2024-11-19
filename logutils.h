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
	uint32_t timestamp; // should be within 0-3fffffff
	uint32_t room_id;   // should be within 0-3fffffff or UINT32_MAX for entire building
	LogPerson person;
	LogEventType event;
} LogEntry;

typedef struct {
	uint8_t header[128 / 8];
	LogEntry *entries;
	size_t entries_len;
} LogFile;

typedef struct {
	size_t length;
	LogEntry *entry;
} LogEntries;

LogFile read_logfile(const char* filename);

void logentry_push(LogEntries *);
LogEntry logentry_pop(LogEntries *);
