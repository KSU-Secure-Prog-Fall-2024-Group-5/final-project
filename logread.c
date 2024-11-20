#include <stdlib.h> // -> EXIT_*
#include <stdio.h>  // -> printf

#include "common.h"
#include "logutils.h"

#define logread_print_usage() printf("logread usage:\n logread -K <token> -S <log>\n logread -K <token> -R (-E <name> | -G <name>) <log>\n")

typedef struct {
    char* token;
    char* logname;
    int mode; // 0 for -S mode, 1 for -R mode
    LogPerson person; 
} arguments;

int logread_parse_args(int argv, char *argc[], arguments *args) {

    if (strncmp(argc[1], "-K", 2) != 0) return 1; // Ensures are 1 == "-K"
    if (strncmp(argc[2], "", 1) <= 0) return 1; // Checks if arg 2 is empty or not
                                                
    int length = strlen(argc[2]);
    args->token = malloc(length);
    strncpy(args->token, argc[2], length); 

    // -S is exclusive, if it is found, we return
    if (strncmp(argc[3], "-S", 2) == 0) { 
        args->mode = 0;

        if (strncmp(argc[4], "", 1) <= 0) return 1; 
        length = strlen(argc[4]);

        args->logname = malloc(length);
        strncpy(args->logname, argc[4], length);

        return 0;
    }

    // If arg 3 isn't -R or -S, something is wrong
    if (strncmp(argc[3], "-R", 2) != 0) return 1;

    if (strncmp(argc[4], "-E", 2) == 0) {
       args->person.role = LOG_ROLE_EMPLOYEE; 
    } else if (strncmp(argc[4], "-R", 2) == 0) {
        args->person.role = LOG_ROLE_GUEST;
    } else return 1;

    length = strlen(argc[5]);
    args->person.name = malloc(length);
    strncpy(args->person.name, argc[5], length);

    length = strlen(argc[6]);
    args->logname = malloc(length);
    strncpy(args->logname, argc[6], length);

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

    printf("Parsed args:\n Token: %s\n Logname: %s\n Person: %s\n", args.token, args.logname, args.person.name);

	if (!init_libgcrypt()) {
        printf("Unable to initialize libgcrypt\n");
        return EXIT_FAILURE;
    };

    free(args.token);
    free(args.logname);

	return EXIT_SUCCESS;
}
