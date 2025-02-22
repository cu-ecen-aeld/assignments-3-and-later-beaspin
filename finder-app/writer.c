#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <file> <text>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *filename = argv[1];
    char *text = argv[2];

    openlog("writer", LOG_PID | LOG_CONS, LOG_USER);

    syslog(LOG_DEBUG, "Writing %s to %s", text, filename);

    FILE *fp = fopen(filename, "W");
    if (!fp) {
        syslog(LOG_ERR, "Error opening file %s: %s", filename, strerror(errno));
        closelog();
        exit(EXIT_FAILURE);
    }

    if (fputs(text, fp) == EOF) {
        syslog(LOG_ERR, "Error writing to file %s: %s", filename, strerror(errno));
        fclose(fp);
        closelog();
        exit(EXIT_FAILURE);
    }

    if (fclose(fp) == EOF) {
        syslog(LOG_ERR, "Error closing file %s: %s", filename, strerror(errno));
        closelog();
        exit(EXIT_FAILURE);
    }

    closelog();
    return EXIT_SUCCESS;
}
