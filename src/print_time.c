// vim:ts=4:sw=4:expandtab
#include <config.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <locale.h>
#include <yajl/yajl_gen.h>
#include <yajl/yajl_version.h>

#include "i3status.h"

#define STRING_SIZE 50

static bool local_timezone_init = false;
static const char *local_timezone = NULL;
static const char *current_timezone = NULL;

static const char *volle_stunde[] = { "Zwölf", "Ein", "Zwei", "Drei", "Vier", "Fünf", "Sechs", "Sieben", "Acht", "Neun", "Zehn", "Elf" };
static const char *stunde[] = { "zwölf", "eins", "zwei", "drei", "vier", "fünf", "sechs", "sieben", "acht", "neun", "zehn", "elf" };
static const char *minute[] = { "Fünf nach", "Zehn nach", "Viertel nach", "Zwanzig nach", "Fünf vor halb", "Halb", "Fünf nach halb", "Zehn nach halb", "Viertel vor", "Zehn vor", "Fünf vor" };

void set_timezone(const char *tz) {
    if (!local_timezone_init) {
        /* First call, initialize. */
        local_timezone = getenv("TZ");
        local_timezone_init = true;
    }
    if (tz == NULL || tz[0] == '\0') {
        /* User wants localtime. */
        tz = local_timezone;
    }
    if (tz != current_timezone) {
        if (tz) {
            setenv("TZ", tz, 1);
        } else {
            unsetenv("TZ");
        }
        current_timezone = tz;
    }
    tzset();
}

void print_time(yajl_gen json_gen, char *buffer, const char *title, const char *format, const char *tz, const char *locale, const char *format_time, bool hide_if_equals_localtime, time_t t) {
    char *outwalk = buffer;
    struct tm local_tm, tm;

    if (title != NULL)
        INSTANCE(title);

    set_timezone(NULL);
    localtime_r(&t, &local_tm);

    set_timezone(tz);
    localtime_r(&t, &tm);

    // When hide_if_equals_localtime is true, compare local and target time to display only if different
    time_t local_t = mktime(&local_tm);
    double diff = difftime(local_t, t);
    if (hide_if_equals_localtime && diff == 0.0) {
        goto out;
    }

    if (locale != NULL) {
        setlocale(LC_ALL, locale);
    }

    char string_time[STRING_SIZE];
    char colloq_time[STRING_SIZE];

    if (format_time == NULL) {
        outwalk += strftime(buffer, 4096, format, &tm);
    } else {
        strftime(string_time, sizeof(string_time), format_time, &tm); 

        const time_t t2 = t + 150;
        struct tm tm2;
        localtime_r(&t2, &tm2);

        if (tm2.tm_min < 5) {
            snprintf(colloq_time, sizeof(colloq_time), "%s Uhr", volle_stunde[tm2.tm_hour % 12]);
        } else {
            snprintf(colloq_time, sizeof(colloq_time), "%s %s", minute[tm2.tm_min / 5 - 1], stunde[(tm2.tm_hour + (tm2.tm_min >= 25)) % 12]);
        }

        placeholder_t placeholders[] = {
            {.name = "%time", .value = string_time},
            {.name = "%text", .value = colloq_time}};

        const size_t num = sizeof(placeholders) / sizeof(placeholder_t);
        buffer = format_placeholders(format_time, &placeholders[0], num);
    }

    if (locale != NULL) {
        setlocale(LC_ALL, "");
    }

out:
    *outwalk = '\0';
    OUTPUT_FULL_TEXT(buffer);
    if (format_time != NULL) {
        free(buffer);
    }
}
