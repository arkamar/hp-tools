#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct commands {
	char * cmd;
	void (*fun)(char * arg);
	void (*flush)();
};

void
flush() {
	fflush(stdout);
}

void
out(char * str) {
	fputs(str, stdout);
}

void
smtp_greet(char * code) {
	out(code);
	fputs("shit", stdout);
}

void
smtp_helo(char * arg) {
	smtp_greet("250 ");
	out("\r\n");
}

void
smtp_ehlo(char * arg) {
	smtp_greet("250-");
	out("\r\n250-PIPELINING\r\n250 8BITMIME\r\n");
}

void err_unimpl(char * arg) { out("502 unimplemented (#5.5.1)\r\n"); }
void err_noop  (char * arg) { out("250 ok\r\n"); }
void err_vrfy  (char * arg) { out("252 send some mail, i'll try my best\r\n"); }

void
smtp_rcpt(char * arg) {
	out("250 ok\r\n");
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

struct commands smtp_commands[] = {
	{ "rcpt", smtp_rcpt, 0 },
	{ "ehlo", smtp_ehlo, flush },
	{ "quit", smtp_quit, flush },
	{ "helo", smtp_helo, flush },
	{ "rset", smtp_rset, 0 },
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

	smtp_greet("200 ");
	out(" ESMTP\r\n");
	while ((len = getline(&line, &cap, stdin)) > 0) {
		for (i = 0; smtp_commands[i].cmd; i++) {
			if (!strncasecmp(line, smtp_commands[i].cmd, 4)) {
				smtp_commands[i].fun(line);
				if (smtp_commands[i].flush)
					smtp_commands[i].flush();
			}
		}
	}
	free(line);
}
