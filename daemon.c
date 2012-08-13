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
#include "lib/rabbitmq/librabbitmq/amqp.h"
#include "lib/rabbitmq/librabbitmq/amqp_framing.h"

// These are temporary and should be removed!
#include <assert.h>
#include "lib/rabbitmq/examples/utils.h"

char * get_amqp_body(void const *buffer, size_t len)
{
	unsigned char *buf = (unsigned char *) buffer;
	long count = 0;
	char body[len]; 
	size_t i;

	for (i = 0; i < len; i++) {
		char ch = (char)buf[i];

		if (isprint(ch)) {		
			body[count] = ch;
			count++;
		}
	}

	return body;
}

static void run(amqp_connection_state_t conn)
{
	uint64_t start_time = now_microseconds();
	int received = 0;
	int previous_received = 0;
	uint64_t previous_report_time = start_time;
	uint64_t next_summary_time = start_time + 1000000;
	amqp_frame_t frame;
	int result;
	size_t body_received;
	size_t body_target;
	uint64_t now;

	while (1) {
		amqp_maybe_release_buffers(conn);
		result = amqp_simple_wait_frame(conn, &frame);
		if (result < 0)
			return;

		if (frame.frame_type != AMQP_FRAME_METHOD)
			continue;

		if (frame.payload.method.id != AMQP_BASIC_DELIVER_METHOD)
			continue;

		result = amqp_simple_wait_frame(conn, &frame);
		if (result < 0)
			return;

		if (frame.frame_type != AMQP_FRAME_HEADER) {
			fprintf(stderr, "Expected header!");
			abort();
		}

		body_target = frame.payload.properties.body_size;
		body_received = 0;

		while (body_received < body_target) {
			result = amqp_simple_wait_frame(conn, &frame);
			if (result < 0)
				return;

		if (frame.frame_type != AMQP_FRAME_BODY) {
			fprintf(stderr, "Expected body!");
			abort();
		}

		  body_received += frame.payload.body_fragment.len;
		}

		char * body = get_amqp_body(frame.payload.body_fragment.bytes,
									frame.payload.body_fragment.len);
		syslog (LOG_DEBUG, "Message recieved: %s\n", (char*)body);
		/*printf("Message recieved: %s\n", (char*)get_amqp_body(
			frame.payload.body_fragment.bytes,
			frame.payload.body_fragment.len
		));*/
	}
}

static amqp_connection_state_t rabbitmq_connect(char const *hostname,
												int port,
												char const *exchange,
												char const *bindingkey)
{
	int sockfd;
	amqp_connection_state_t conn;

	amqp_bytes_t queuename;

	conn = amqp_new_connection();

	die_on_error(sockfd = amqp_open_socket(hostname, port),
				"Opening socket");
	amqp_set_sockfd(conn, sockfd);
	die_on_amqp_error(amqp_login(conn, "/", 0, 131072, 0, 
						AMQP_SASL_METHOD_PLAIN, "guest", "guest"), 
						"Logging in");
	amqp_channel_open(conn, 1);
	die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");

	{
		amqp_queue_declare_ok_t *r = amqp_queue_declare(conn, 1,
												amqp_empty_bytes, 0, 0,
												0, 1, amqp_empty_table);
		die_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring queue");
		queuename = amqp_bytes_malloc_dup(r->queue);
		if (queuename.bytes == NULL) {
			fprintf(stderr, "Out of memory while copying queue name");
			return NULL;
		}
	}

	amqp_queue_bind(conn, 1, queuename, amqp_cstring_bytes(exchange),
					amqp_cstring_bytes(bindingkey), amqp_empty_table);
	die_on_amqp_error(amqp_get_rpc_reply(conn), "Binding queue");

	amqp_basic_consume(conn, 1, queuename, amqp_empty_bytes, 0, 1, 0,
						amqp_empty_table);
	die_on_amqp_error(amqp_get_rpc_reply(conn), "Consuming");

	return conn;
}

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
    
	// Open any logs here
	setlogmask(LOG_UPTO(LOG_DEBUG));
	openlog ("shepherd", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_USER);
	syslog (LOG_DEBUG, "Logging mechanism initialized");

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
	fflush(stdout);
	amqp_connection_state_t conn = rabbitmq_connect("localhost", 5672,
													"amq.direct", "test");

	// An infinite loop
	run(conn);

	die_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS),
						"Closing channel");
	die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS), 
						"Closing connection");
	die_on_error(amqp_destroy_connection(conn), "Ending connection");

	closelog ();
	exit(EXIT_SUCCESS);
}
