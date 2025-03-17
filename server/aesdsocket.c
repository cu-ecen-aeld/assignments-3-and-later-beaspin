#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define PORT 9000
#define BUFFER_SIZE 1024
#define FILE_PATH "/var/tmp/aesdsocketdata"

int server_socket = -1, client_socket = -1;

void handle_signal(int signo) {
	syslog(LOG_INFO, "Caught signal %d, shutting down", signo);
	if (server_socket != -1) {
	    shutdown(server_socket, SHUT_RDWR);
	    close(server_socket);
	    server_socket = -1;
	}

	if (client_socket != -1) {
	    shutdown(client_socket, SHUT_RDWR);
	    close(client_socket);
	    client_socket = -1;
	}

	remove(FILE_PATH);
	syslog(LOG_INFO, "File %s deleted, exiting.", FILE_PATH);
	closelog();

	exit(EXIT_SUCCESS);
}

void daemonize() {
	pid_t pid = fork();

	if (pid < 0) {
	    syslog(LOG_ERR, "Fork failed: %s", strerror(errno));
	    exit(EXIT_FAILURE);
	}

	if (pid > 0) {
	    exit(EXIT_SUCCESS);
	}

	umask(0);

	if (chdir("/") < 0) {
	    syslog(LOG_ERR, "chdir failed: %s", strerror(errno));
	    exit(EXIT_FAILURE);
	}

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	openlog("aesdsocket", LOG_PID | LOG_CONS, LOG_USER);
}

int main(int argc, char *argv[]) {
	struct sockaddr_in server_addr, client_addr;
	socklen_t client_len = sizeof(client_addr);
	char buffer[BUFFER_SIZE];
	ssize_t bytes_received;
	int daemon_mode = 0;

	if (argc > 1 && strcmp(argv[1], "-d") == 0) {
	    daemon_mode = 1;
	}

	openlog("aesdsocket", LOG_PID | LOG_CONS, LOG_USER);
	signal(SIGINT, handle_signal);
	signal(SIGTERM, handle_signal);

	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == -1) {
	    syslog(LOG_ERR, "Failed to create socket: %s", strerror(errno));
	    return (EXIT_FAILURE);
	}

	int opt =1;
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
	    syslog(LOG_ERR, "setsockopt(SO_REUSEADDR) failed: %s", strerror(errno));
	    exit(EXIT_FAILURE);
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);

	if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
	    syslog(LOG_ERR, "Bind failed: %s", strerror(errno));
	    return -1;
	}

	if (daemon_mode) {
	    daemonize();
	}

	if (listen(server_socket, 10) == -1) {
	    syslog(LOG_ERR, "Listen failed: %s", strerror(errno));
	    return -1;
	}

	while (1) {
	    client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
	    if (client_socket == -1) {
		syslog(LOG_ERR, "Accept failed: %s", strerror(errno));
		close(client_socket);
		continue;
	    }

	    syslog(LOG_INFO, "Accepted connection from %s", inet_ntoa(client_addr.sin_addr));
	    int file_fd = open(FILE_PATH, O_CREAT | O_APPEND | O_WRONLY, 0644);
	    if (file_fd == -1) {
		syslog(LOG_ERR, "Failed to open file: %s", strerror(errno));
		close(client_socket);
		continue;
	    }

	    while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
		buffer[bytes_received] = '\0';

		if (bytes_received > 0 && buffer[bytes_received -1] != '\n') {
		    if (bytes_received < BUFFER_SIZE - 1) {
		        buffer[bytes_received] = '\n';
		        bytes_received++;
			buffer[bytes_received] = '\0';
		    }
		}

		write(file_fd, buffer, bytes_received);

		if (strchr(buffer, '\n')) break;
	    }
	    close(file_fd);

	    file_fd = open(FILE_PATH, O_RDONLY);
	    if (file_fd != -1) {
		while ((bytes_received = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
		    send(client_socket, buffer, bytes_received, 0);
		}
		close(file_fd);
	    }

	    syslog(LOG_INFO, "Closed connection from %s", inet_ntoa(client_addr.sin_addr));
	    close(client_socket);
	}

	return 0;
}
