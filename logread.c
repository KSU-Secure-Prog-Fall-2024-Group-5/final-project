#include <stdlib.h> // -> EXIT_*
#include <stdio.h>  // -> printf

#include "common.h"
#include "logutils.h"

// Macro for printing out correct program usage
#define logread_print_usage() \
	printf( \
		"logread usage:\n logread -K <token> -S <log>\n logread -K <token> " \
		"-R (-E <name> | -G <name>) <log>\n")

// Stores configuration for the program passed to it through arguments
typedef struct {
	char *token;
	char *logname;
	int mode; // 0 for -S mode, 1 for -R mode
	LogPerson person;
} arguments;

char *person_role_str(LogPersonRole role) {
	return role == LOG_ROLE_EMPLOYEE ? "employee"
		: role == LOG_ROLE_GUEST     ? "guest"
									 : "invalid";
}

char *event_type_str(LogEventType event) {
	return event == LOG_EVENT_ARRIVAL  ? "arrives"
		: event == LOG_EVENT_DEPARTURE ? "departs"
									   : "invalid";
}

// Prints out all entries in a log nicely
// Side effects: prints to screen
void printLog(LogEntryList *logEntries, int n_entries) {
	printf("\nLOG CONTAINS:\n\n");

	for (int i = 0; i < n_entries; i++) {
		LogEntry *current = &logEntries->entry[i];

		char *role = person_role_str(current->person.role);
		char *event = event_type_str(current->event);

		if (current->room_id != UINT32_MAX) {
			printf("[%i] At %i in room %i, %s %s %s\n", i, current->timestamp,
				current->room_id, role, current->person.name, event);
		} else {
			printf("[%i] At %i, %s %s %s the gallery\n", i, current->timestamp,
				role, current->person.name, event);
		}
	}
}

// Finds and prints all records in a log associated with a given LogPerson
// Side effects: prints to screen
void findPerson(LogEntryList *logEntries, LogPerson person, int n_entries) {
	char *role = person_role_str(person.role);
	int name_len = strlen(person.name);

	printf("\nLOG ENTRIES WITH %s '%s':\n\n", role, person.name);

	for (int i = 0; i < n_entries; i++) {
		LogEntry *current = &logEntries->entry[i];
		LogPerson *cPerson = &current->person;

		char *event = event_type_str(current->event);

		if (strncmp(person.name, cPerson->name, name_len) == 0 &&
			cPerson->role == person.role) {
			printf("[%i] %i, %i, %s %s %s\n", i, current->timestamp,
				current->room_id, role, current->person.name, event);
		}
	}
}

// Parse arguments provided to program
// Returns 0 on success, 1 on failure
// Side effects: modifies the arguments struct passed to it
int logread_parse_args(int argv, char *argc[], arguments *args) {
	if (strncmp(argc[1], "-K", 2) != 0) return 1; // Ensures are 1 == "-K"
	if (strncmp(argc[2], "", 1) <= 0)
		return 1; // Checks if arg 2 is empty or not

	args->token = duplicate_string(argc[2]);

	args->person.name = NULL;

	// -S is exclusive, if it is found, we return
	if (strncmp(argc[3], "-S", 2) == 0) {
		args->mode = 0;

		if (strncmp(argc[4], "", 1) <= 0) return 1;
		args->logname = duplicate_string(argc[4]);

		return 0;
	}

	args->mode = 1;

	// In -R mode, we must have 6 options
	if (argv != 7) return 1;

	// If arg 3 isn't -R or -S, something is wrong
	if (strncmp(argc[3], "-R", 2) != 0) return 1;

	if (strncmp(argc[4], "-E", 2) == 0) {
		args->person.role = LOG_ROLE_EMPLOYEE;
	} else if (strncmp(argc[4], "-G", 2) == 0) {
		args->person.role = LOG_ROLE_GUEST;
	} else
		return 1;

	if (strncmp(argc[5], "", 1) <= 0) return 1;
	args->person.name = duplicate_string(argc[5]);

	if (strncmp(argc[6], "", 1) <= 0) return 1;
	args->logname = duplicate_string(argc[6]);

	return 0;
}

int main(int argv, char *argc[]) {
	if (argv <= 4) {
		printf("Not enough arguments.\n");
		logread_print_usage();
		return EXIT_FAILURE;
	}

	arguments args;
	if (logread_parse_args(argv, argc, &args) != 0) {
		printf("Error while parsing input\n");
		logread_print_usage();
		return EXIT_FAILURE;
	}

	printf("Parsed args:\n Token: %s\n Logname: %s\n Person: %s\n", args.token,
		args.logname, args.person.name);

	if (!init_libgcrypt()) {
		printf("Unable to initialize libgcrypt\n");
		return EXIT_FAILURE;
	};

	LogFile *log = logfile_read(args.logname, args.token);
	if (log == NULL) {
		printf("Error reading log file\n");
		return EXIT_FAILURE;
	}

	if (args.mode == 0) {
		printLog(&log->entries, log->entries.length);
	} else {
		findPerson(&log->entries, args.person, log->entries.length);
		free(args.person.name);
	}

	logfile_free(log);

	free(args.token);
	free(args.logname);

	return EXIT_SUCCESS;
}
