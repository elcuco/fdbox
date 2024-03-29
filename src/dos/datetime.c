/* This file is part of fdbox
For license - read license.txt
*/

/*
 * TODO
 * - support for reading AM/PM time format
 * - support for different date separators (depending on locale)
 * - support for different date locale (MM/DD/YY vs DD/MM/YY etc).
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "dos/datetime.h"
#include "fdbox.h"
#include "lib/args.h"
#include "lib/strextra.h"

#ifdef _POSIX_C_SOURCE
#include <stdbool.h>
#include <sys/time.h>
#endif

#ifdef __WIN32__
#include <stdbool.h>
#include <sys/time.h>
#endif

#ifdef __TURBOC__
#include "lib/tc202/stdbool.h"
#include <dos.h>
#endif

struct date_time_config {
        bool interactive;
        char *new_date_time;

        struct command_config global;
};

static void date_time_config_init(struct date_time_config *config);
static bool date_time_config_parse(int argc, char *argv[], struct date_time_config *config,
                                   char interactive_char);
static void date_time_config_print(struct date_time_config *config);
static int date_set_new_date(char *new_date);
static void date_print_date();
static void date_print_extended_help();
static int date_set_new_time(char *new_time);
static void date_print_time();
static void time_print_extended_help();

int command_time(int argc, char *argv[]) {
        struct date_time_config config;
        date_time_config_init(&config);
        if (!date_time_config_parse(argc, argv, &config, 't')) {
                time_print_extended_help();
                return EXIT_FAILURE;
        }
        if (config.global.show_help) {
                time_print_extended_help();
                return EXIT_SUCCESS;
        }
        if (config.new_date_time != NULL) {
                return date_set_new_time(config.new_date_time);
        }
        date_print_time();
        if (config.interactive) {
                char new_date[128];
                printf("Enter new time (hh:mm:ss [am/pm]): ");
                fgets(new_date, 128, stdin);
                return date_set_new_time(new_date);
        }
        return EXIT_SUCCESS;
}

int command_date(int argc, char *argv[]) {
        struct date_time_config config;
        date_time_config_init(&config);
        if (!date_time_config_parse(argc, argv, &config, 'd')) {
                date_print_extended_help();
                return EXIT_FAILURE;
        }
        /* date_config_print(&config); */

        if (config.global.show_help) {
                date_print_extended_help();
                return EXIT_SUCCESS;
        }
        if (config.new_date_time != NULL) {
                return date_set_new_date(config.new_date_time);
        }
        date_print_date();
        if (config.interactive) {
                char new_date[128];
                printf("Enter new date (mm-dd-[yy]yy): ");
                fgets(new_date, 128, stdin);
                return date_set_new_date(new_date);
        }
        return EXIT_SUCCESS;
}

const char *help_date() { return "Display or modify system time"; }
const char *help_time() { return "Display or modify system date"; }

static void date_time_config_init(struct date_time_config *config) {
        config->interactive = true;
        config->new_date_time = NULL;
        command_config_init(&config->global);
}

static bool date_time_config_parse(int argc, char *argv[], struct date_time_config *config,
                                   char interactive_char) {
        int c;
        do {
                c = command_config_parse(argc, argv, &config->global);
                /* cannot switch/case here - as we have a variable ... */
                if (tolower(c) == interactive_char) {
                        config->interactive = false;
                } else if (c == ARG_PROCESSED) {
                        continue;
                } else if (c != ARG_DONE) {
                        return false;
                }
        } while (c >= 0);

        /* yes, this looks ugly, but the memory can be modified... */
        config->new_date_time = (char *)config->global.files.file[0];
        return true;
}

static void date_time_config_print(struct date_time_config *config) {
        printf("\tInteractive = %s\n", str_bool(config->interactive));
        printf("\tnew date = %s\n", config->new_date_time ? config->new_date_time : "NULL");
        printf("\tShow help = %s\n", str_bool(config->global.show_help));
        command_config_print(&config->global);
}

static int date_set_new_date(char *new_date) {
        int year = -1, month = -1, day = -1;
        char *token;

        token = strtok(new_date, "-");
        month = token != NULL ? strtol(token, NULL, 10) : -1;
        token = strtok(NULL, "-");
        day = token != NULL ? strtol(token, NULL, 10) : -1;
        token = strtok(NULL, "-");
        year = token != NULL ? strtol(token, NULL, 10) : -1;

        if (year <= 0 || month <= 0 || day <= 0) {
                return EXIT_FAILURE;
        }
        /*        printf("TODO: set the date to %02d-%02d-%4d\n", day, month, year); */

#if defined(_POSIX_C_SOURCE)
        {
                time_t new_time;
                struct tm tm;
                struct timeval tv;
                new_time = time(NULL);
                tm = *localtime(&new_time);
                tm.tm_year = year - 1900;
                tm.tm_mon = month - 1;
                tm.tm_mday = day;
                new_time = mktime(&tm);
                tv.tv_sec = new_time;
                tv.tv_usec = 0;
                return settimeofday(&tv, NULL);
        }
#elif defined(__MSDOS__)
        {
                struct date the_date;
                the_date.da_year = year;
                the_date.da_mon = month;
                the_date.da_day = day;
                setdate(&the_date);
        }
#elif defined(__WIN32__)
        // TODO: We do not support this on windows yet
        return EXIT_FAILURE;
#endif
        return EXIT_SUCCESS;
}

static void date_print_date() {
        time_t t = time(NULL);
        struct tm *tm = localtime(&t);
        printf("Current date is %02d-%02d-%4d\n", tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900);
}

static int date_set_new_time(char *new_time) {
        int hour = -1, minute = -1, second = -1;
        char *token;

        token = strtok(new_time, ":");
        hour = token != NULL ? strtol(token, NULL, 10) : -1;
        token = strtok(NULL, ":");
        minute = token != NULL ? strtol(token, NULL, 10) : -1;
        token = strtok(NULL, ":");
        second = token != NULL ? strtol(token, NULL, 10) : 0;
        token = strtok(NULL, ":");

#if 0
        TODO: we cannot support PM/AM yet
        if (token != NULL && token[0] != '\0') {
                if (token[2] != '\0') {
                        return EXIT_FAILURE;
                }
                if (toupper(token[1]) != 'M') {
                        return EXIT_FAILURE;
                }
                if (toupper(token[0]) == 'A') {
                        if (hour > 12) {
                                return EXIT_FAILURE;
                        }
                }
                if (toupper(token[0]) == 'P') {
                        if (hour > 12) {
                                return EXIT_FAILURE;
                        }
                        hour += 12;
                }
        }
#endif
        if (hour < 0 || minute < 0 || second < 0) {
                return EXIT_FAILURE;
        }

#if defined(_POSIX_C_SOURCE)
        {
                time_t new_time;
                struct tm tm;
                struct timeval tv;
                new_time = time(NULL);
                tm = *localtime(&new_time);
                tm.tm_hour = hour;
                tm.tm_min = minute;
                tm.tm_sec = second;
                new_time = mktime(&tm);
                tv.tv_sec = new_time;
                tv.tv_usec = 0;
                return settimeofday(&tv, NULL);
        }
#elif defined(__MSDOS__)
        {
                struct time the_time;
                printf("TODO: set time to %02d:%02d:%02d\n", hour, minute, second);

                the_time.ti_hour = hour;
                the_time.ti_min = minute;
                the_time.ti_sec = second;
                the_time.ti_hund = 0;
                settime(&the_time);
        }
#elif defined(__WIN32__)
        return EXIT_FAILURE;
#endif

        return EXIT_SUCCESS;
}

static void date_print_time() {
        time_t t = time(NULL);
        struct tm *tm = localtime(&t);
        printf("Current time is %02d:%02d:%02d\n", tm->tm_hour, tm->tm_min, tm->tm_sec);
}

static void date_print_extended_help() {
        printf("%s\n", help_date());
        printf("        date /d [new date]\n");
        printf("        /d displays date and exits\n\n");
        printf("If you pass a date to this command, it will just set it\n");
        printf("and not be interactive. If you don't - the command will\n");
        printf("prompt you for a new date\n");
}

static void time_print_extended_help() {
        printf("%s\n", help_time());
        printf("        time /t [new time]\n");
        printf("        /t displays time and exits\n\n");
        printf("If you pass a time to this command, it will just set it\n");
        printf("and not be interactive. If you don't - the command will\n");
        printf("prompt you for a new time\n");
}
