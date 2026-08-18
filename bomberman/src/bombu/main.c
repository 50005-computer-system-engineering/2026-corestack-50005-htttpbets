#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <libgen.h>

// Trims a trailing \n left over from fgets
static void trim_newline(char* s)
{
    size_t len = strlen(s);
    if (len > 0 && s[len - 1] == '\n') {
        s[len - 1] = '\0';
    }
}

// bombapp is a raylib window, so it can't cleanly prompt for a server IP on
// stdin before the window opens. bombu is that terminal front door: it
// collects the IP here, then hands off to bombapp for the rest (bombapp
// makes its own fresh network connection once launched, since exec() drops
// this process's sockets and background thread anyway)
int main(void)
{
    char server_ip[INET_ADDRSTRLEN + 2] = {0};
    while (1) {
        printf("Enter bombd server IP (default 127.0.0.1): ");
        fflush(stdout);

        if (fgets(server_ip, sizeof(server_ip), stdin) == NULL) {
            printf("[bombu] Input error.\n");
            return -1;
        }

        // Oversized input leaves the rest of the line in stdin, throw it away
        if (strchr(server_ip, '\n') == NULL) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {
                // Discard
            }
        }

        trim_newline(server_ip);
        if (server_ip[0] == '\0') {
            strcpy(server_ip, "127.0.0.1");
            break;
        }

        struct in_addr addr;
        if (inet_pton(AF_INET, server_ip, &addr) == 1) {
            break;
        }
        printf("[bombu] Invalid IP address '%s'. Please enter a valid IPv4 address.\n", server_ip);
    }

    printf("[bombu] Launching bombapp, connecting to %s...\n", server_ip);
    fflush(stdout);

    // bombapp lives next to this binary in build/bin, so resolve it relative
    // to our own executable rather than assuming the current working directory
    char self_path[4096];
    ssize_t len = readlink("/proc/self/exe", self_path, sizeof(self_path) - 1);
    if (len < 0) {
        perror("[bombu] readlink");
        return -1;
    }
    self_path[len] = '\0';

    char dir_buf[4096];
    strncpy(dir_buf, self_path, sizeof(dir_buf) - 1);
    dir_buf[sizeof(dir_buf) - 1] = '\0';
    char* dir = dirname(dir_buf);

    char bombapp_path[4160];
    snprintf(bombapp_path, sizeof(bombapp_path), "%s/bombapp", dir);

    char* exec_argv[] = {bombapp_path, server_ip, NULL};
    execv(bombapp_path, exec_argv);

    // Only reached if execv itself failed
    perror("[bombu] execv");
    fprintf(stderr, "[bombu] Could not launch bombapp at %s. Build it first with `make`.\n", bombapp_path);
    return -1;
}
