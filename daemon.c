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
#include "lib/iniparser/src/iniparser.h"

int main(int argc, char *argv[]) {

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

	int sleep_interval;
	dictionary * config;
	int log_level;

	config = iniparser_load("shepherd.ini");
    if (config == NULL) {
		exit(EXIT_FAILURE);
    }

	sleep_interval = iniparser_getint(config, "shepherd:sleep_interval", 15);
	log_level = iniparser_getint(config, "shepherd:log_level", LOG_INFO);

	// Open any logs here
	setlogmask(LOG_UPTO(LOG_DEBUG));
	openlog("shepherd", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_USER);
	syslog(LOG_DEBUG, "Logging mechanism initialized");
	syslog(LOG_DEBUG, "sheperd:sleep_interval = %i", sleep_interval);
	syslog(LOG_DEBUG, "sheperd:log_level = %i", log_level);

	// Create a new SID for the child process
	sid = setsid();
	if (sid < 0) {
		// Log the failure
		syslog(LOG_ERR, "Unable to create SID for child process");
		exit(EXIT_FAILURE);
	}

	// TODO: Kris - What is this for?
	// Change the current working directory
	//if ((chdir("/")) < 0) {
		//// Log the failure
		//syslog(LOG_ERR, "Unable to change current directory");
		//exit(EXIT_FAILURE);
	//}

	// Close out the standard file descriptors
	// Because daemons generally dont interact directly with users,
	// there is no need of keeping these open
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	// Daemon-specific initialization goes here
	daemon_init();

	// An infinite loop
	while (1) {
		syslog(LOG_DEBUG, "Ping!");

		// Verify the accessibility of the local redis instance
		// if can:
		//     set proxy destination to localhost:16739
		// if cannot:
		//	   set SLAVEOF to none
		//     set proxy destination to NEW_SLAVE:16739
		//
		// send request to 
		sleep(sleep_interval); // wait 30 seconds
	}
	closelog();
	exit(EXIT_SUCCESS);
}
