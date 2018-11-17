#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

struct commands {
	char * cmd;
	void (*fun)(char * arg);
	void (*flush)();
};

static
unsigned int pid;

static
void
flush() {
	fflush(stdout);
}

static
void
out(char * str) {
	fputs(str, stdout);
}

static
void
put(char * str) {
	fwrite(str, 1, 1, stderr);
}

static
void
smtp_greet(char * code) {
	out(code);
	fputs("test", stdout);
}

static
void
smtp_helo(char * arg) {
	smtp_greet("250 ");
	out("\r\n");
}

static
void
straynewline() {
	out("451 See http://pobox.com/~djb/docs/smtplf.html.\r\n");
	flush();
	_exit(1);
}

static
void
blast() {
	char ch;
	int state = 1;

	for (;;) {
		ch = getchar();
		put(&ch);

		switch (state) {
		case 0:
			if (ch == '\n') straynewline();
			if (ch == '\r') { state = 4; continue; }
			break;
		case 1: /* \r\n */
			if (ch == '\n') straynewline();
			if (ch == '.') { state = 2; continue; }
			if (ch == '\r') { state = 4; continue; }
			state = 0;
			break;
		case 2: /* \r\n + . */
			if (ch == '\n') straynewline();
			if (ch == '\r') { state = 3; continue; }
			state = 0;
			break;
		case 3: /* \r\n + .\r */
			if (ch == '\n') return;
			put(".");
			put("\r");
			if (ch == '\r') { state = 4; continue; }
			state = 0;
			break;
		case 4: /* + \r */
			if (ch == '\n') { state = 1; break; }
			if (ch != '\r') { put("\r"); state = 0; }
		}
	}
}

static
void
smtp_ehlo(char * arg) {
	smtp_greet("250-");
	out("\r\n250-PIPELINING\r\n250 8BITMIME\r\n");
}

static void err_unimpl(char * arg) { out("502 unimplemented (#5.5.1)\r\n"); }
static void err_noop  (char * arg) { out("250 ok\r\n"); }
static void err_vrfy  (char * arg) { out("252 send some mail, i'll try my best\r\n"); }

static
void
smtp_rcpt(char * arg) {
	out("250 ok\r\n");
}

static
void
smtp_mail(char * arg) {
	out("250 ok\r\n");
}

static
void
smtp_data(char * arg) {
	out("354 go ahead\r\n");
	flush();
	blast();
	pid += rand() % 6 + 1;
	printf("250 ok %lu qp %u\r\n", time(0), pid);
}

static
void
smtp_rset(char * arg) {
	out("250 flushed\r\n");
}

static
void
smtp_help(char * arg) {
	out("214 netqmail home page: http://qmail.org/netqmail\r\n");
}

static
void
smtp_quit(char * arg) {
	smtp_greet("221 ");
	out("\r\n");
	flush();
	exit(0);
}

static
struct commands smtp_commands[] = {
	{ "rcpt", smtp_rcpt, flush },
	{ "mail", smtp_mail, flush },
	{ "data", smtp_data, flush },
	{ "quit", smtp_quit, flush },
	{ "helo", smtp_helo, flush },
	{ "ehlo", smtp_ehlo, flush },
	{ "rset", smtp_rset, flush },
	{ "help", smtp_help, flush },
	{ "noop", err_noop, flush },
	{ "vrfy", err_vrfy, flush },
	{ NULL, err_unimpl, flush }
};


int
main(int argc, char * argv[]) {
	char * line = NULL;
	size_t cap = 0;
	ssize_t len;
	int i;

	struct timeval tv = { 10, 0 };

	srand(time(0));
	pid = getpid();

	setsockopt(1, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
	setsockopt(1, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv);

	smtp_greet("200 ");
	out(" ESMTP\r\n");
	while ((len = getline(&line, &cap, stdin)) > 0) {

		fputs(line, stderr);

		for (i = 0; smtp_commands[i].cmd; i++)
			if (!strncasecmp(line, smtp_commands[i].cmd, 4))
				break;

		smtp_commands[i].fun(line);
		if (smtp_commands[i].flush)
			smtp_commands[i].flush();
	}

	if (errno) {
		fprintf(stderr, "Error: %s\n", strerror(errno));
		out("451 timeout (#4.4.2)\r\n");
		flush();
	}

	free(line);
}
