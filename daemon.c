#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include "lib/daemonize.h"
#include "lib/inih/include/ini.h"

typedef struct
{
	int log_level;
	const char* sleep_interval;
} configuration;

static int handler(void* user, const char* section, const char* name, 
					const char* value)
{
    configuration* pconfig = (configuration*)user;

    #define MATCH(s, n) strncasecmp(section, s) == 0 && strncasecmp(name, n) == 0
    if (MATCH("shepherd", "log_level")) {
        pconfig->log_level = atoi(value);
    } else if (MATCH("shepherd", "sleep_interval")) {
        pconfig->sleep_interval = strdup(value);
    } else {
        return 0;  /* unknown section/name, error */
    }
    return 1;
}

int main(int argc, char *argv[]) {
    configuration config;

	// Our process ID and Session ID
	pid_t pid, sid;

	// Fork off the parent process
	pid = fork();
	// If we got a bad PID, then we can exit abort the daemon.
	if (pid < 0) {
		exit(EXIT_FAILURE);
	}
	// If we got a good PID, then we can exit the parent process.
	if (pid > 0) {
		// A child can continue to run even after the parent has
		// finished executing, so we exit the parent
		exit(EXIT_SUCCESS);
	}

	// Change the file mode mask
	umask(0);

	// Open any logs here
	setlogmask(LOG_UPTO(LOG_DEBUG));
	openlog("shepherd", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_USER);
	syslog(LOG_DEBUG, "Logging mechanism initialized");

	// Create a new SID for the child process
	sid = setsid();
	if (sid < 0) {
		// Log the failure
		exit(EXIT_FAILURE);
	}

	// Change the current working directory
	if ((chdir("/")) < 0) {
		// Log the failure
		exit(EXIT_FAILURE);
	}

	// Close out the standard file descriptors
	// Because daemons generally dont interact directly with users,
	// there is no need of keeping these open
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	// Daemon-specific initialization goes here
	daemon_init();

	// Parse the log file
	if (ini_parse("shepherd.conf", handler, &config) < 0) {
		syslog(LOG_ERR, "Can not load shepherd.conf");
		closelog();
		exit(EXIT_FAILURE);
    }
    syslog(LOG_DEBUG, "Config loaded from 'test.ini'. log_level=%i, sleep_interval=%s",
        config.log_level, config.sleep_interval);

	// An infinite loop
	while (1) {
		// Verify the accessibility of the local redis instance
		// if can:
		//     set proxy destination to localhost:16739
		// if cannot:
		//	   set SLAVEOF to none
		//     set proxy destination to NEW_SLAVE:16739
		//
		// send request to 
		sleep(30); // wait 30 seconds
	}
	closelog();
	exit(EXIT_SUCCESS);
}
