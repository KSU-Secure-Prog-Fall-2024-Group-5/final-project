#include <stddef.h> // -> size_t, ptrdiff_t
#include <stdio.h>
#include <ctype.h> // -> isalnum
#include <string.h>

#include "common.h"
#include "logutils.h"

const char *validate_token(char *given_token) {
	if (given_token == NULL) return "token is required";
	char curr;
	while ((curr = *(given_token++)) != '\0') {
		// (A-Z)(a-z)(0-9) no spaces case sensitive
		if (!isalnum(curr))
			return "tokens must only have alphanumeric characters";
	}
	return NULL;
}

const char *validate_name(char *name) {
	if (name == NULL) return "name is required";
	char curr;
	while ((curr = *(name++)) != '\0') {
		// (A-Z)(a-z) no spaces case sensitive
		if (curr == ' ') return "spaces are not allowed in name";
		if (isdigit(curr)) return "numbers are not allowed in name";
		if (!isalpha(curr))
			return "names must only have alphabetical characters and no spaces";
	}
	return NULL;
}

const char *logentry_validate(LogEntry *entry) {
	const char *msg;

	if (entry->timestamp & 0xC0000000)
		return "timestamp (-T <timestamp>) is required";
	if ((entry->room_id & 0xC0000000) && (entry->room_id != UINT32_MAX))
		return "room id (-R <room-id>) is in invalid range";
	if ((entry->person.name == NULL) || (entry->person.role == '\0'))
		return "employee name (-E <employee-name>) or guest name "
			   "(-G <guest-name>) but not both is required";
	if (entry->event == '\0')
		return "arrival event (-A) or departure event (-L) required";

	if ((msg = validate_name(entry->person.name)) != NULL) return msg;

	return NULL;
}

LogFile *logfile_read(char *filename, char *given_token) {
	FILE *file = fopen(filename, "r");
	if (file == NULL) {
		printf(CONSOLE_VIS_ERROR
			"ERROR: Unable to open file '%s'" CONSOLE_VIS_RESET "\n",
			filename);
		return NULL;
	}

    char header[8];
    int read = fread(header, 1, 8, file);
    if (read != 8 || strncmp("STARTLOG", header, 8) != 0) {
        printf(CONSOLE_VIS_ERROR "ERROR: '%s' is not a valid log\n" CONSOLE_VIS_RESET, filename);
        return NULL;
    }

    int tokenSize = strlen(given_token); 
    char* buf = malloc(tokenSize);
    read = fread(buf, 1, tokenSize, file);
    if (read != tokenSize || strncmp(given_token, buf, tokenSize) != 0) {
        printf("Error: tokens do not match");
        return NULL;
    }
    free(buf);

    char *f_buf; // File buffer
    char *e_buf; // Entry buffer
    char *p_buf; // Previous entry buffer

    f_buf = malloc(1024);
    e_buf = malloc(1024);
    p_buf = malloc(1024);
    
    fgets(f_buf, 1024, file);

    LogFile *parsed = malloc(sizeof(LogFile));
    //for (size_t i = 0; i < sizeofarr(parsed.token); i++) parsed.token[i] = 0;
    parsed->entries.entry = NULL;
    parsed->entries.length = 0;

    e_buf = strtok(f_buf, "#");
    while(e_buf[0] != '\0' && strncmp(e_buf, "ENDLOG", 6) != 0 && strncmp(p_buf, "ENDLOG", 6) != 0) {
        // Read timestamp
        uint32_t timestamp = strtoul(e_buf, NULL, 2);
        printf("Adjusted timestamp: %i\n", timestamp);

        // Read employee-name | guest-name | room-id
        p_buf = e_buf;
        e_buf = strtok(NULL, "#");
        printf("2: %s\n", e_buf);
        
        // Read arrival-time | departure-time
        p_buf = e_buf;
        e_buf = strtok(NULL, "#");
        printf("3: %s\n", e_buf);
        
        // Optional : room-id
        p_buf = e_buf;
        e_buf = strtok(NULL, "#");
        if (e_buf[0] != '\n') {
            printf("4: %s\n", e_buf);
        }
        
        // Load next entry or end of log
        p_buf = e_buf;
        fgets(f_buf, 1024, file);
        e_buf = strtok(f_buf, "#");
    }
    p_buf = e_buf;

    if (strncmp(p_buf, "ENDLOG", 6) != 0) {
        printf(CONSOLE_VIS_ERROR "ERROR: '%s' is not a valid log! Read %s\n" CONSOLE_VIS_RESET, filename, p_buf);
        return NULL;
    }

    printf("Log '%s' seems good!\n", filename);

    free(f_buf);
    free(e_buf);

	fclose(file);
	file = NULL;

	return parsed;
}

void logentry_push(LogEntryList* list, LogEntry entry) {
    LogEntry *newEntries = malloc((list->length + 1) * sizeof(LogEntry));

    for (size_t i = 0; i < list->length; i++) {
        newEntries[i] = list->entry[i];
    }

    newEntries[list->length] = entry;
    free(list->entry);
    list->length += 1;
    list->entry = newEntries;
}

LogEntry* logentry_pop(LogEntryList* list) {
    LogEntry *newEntries = malloc((list->length - 1) * sizeof(LogEntry));

    for (size_t i = 0; i < list->length; i++) {
        newEntries[i] = list->entry[i];
    }

    LogEntry *popped = &list->entry[list->length];
    list->length -= 1;
    list->entry = newEntries;

    return popped;
}
