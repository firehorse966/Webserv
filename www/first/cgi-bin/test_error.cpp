#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/*
 * test_error.cgi
 * CGI test script to provoke server error conditions:
 * - If 'test=timeout', simulates a timeout by returning 408.
 * - If 'test=segfault', returns 500 and exits (error status test).
 * - Otherwise, returns normal 200 response.
 */
int main(void) {
    char *query = getenv("QUERY_STRING");
    char test_param[32] = {0};

    /* Default status */
    const char *status_line = "Status: 200 OK";
    const char *content_type = "Content-Type: text/plain";

    /* Parse 'test' parameter */
    if (query) {
        char *p = strstr(query, "test=");
        if (p) sscanf(p, "test=%31[^&]", test_param);
    }

    /* Choose status based on test type */
    if (strcmp(test_param, "segfault") == 0) {
        status_line = "Status: 500 Internal Server Error";
    } else if (strcmp(test_param, "timeout") == 0) {
        status_line = "Status: 408 Request Timeout";
    }

    /* Output CGI headers */
    printf("%s\r\n", content_type);
    printf("%s\r\n\r\n", status_line);

    /* Output message and exit */
    if (strcmp(test_param, "segfault") == 0) {
        printf("Simulating server error (500) and exiting.\n");
        fflush(stdout);
        return EXIT_FAILURE;
    }
    if (strcmp(test_param, "timeout") == 0) {
        printf("Simulating timeout error (408) and exiting.\n");
        fflush(stdout);
        return EXIT_FAILURE;
    }

    /* Normal response */
    printf("Hello from test_error.cgi!\n");
    return EXIT_SUCCESS;
}
