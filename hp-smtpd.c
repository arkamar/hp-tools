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
const char * hostname = "hp-smtpd";

static
const char * maildir = NULL;

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
setup() {
	const char * env_hostname = getenv("HOSTNAME");

	if (env_hostname)
		hostname = env_hostname;

	maildir = getenv("MAILDIR");
}

static
void
smtp_greet(char * code) {
	out(code);
	fputs(hostname, stdout);
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
	const time_t now = time(NULL);
	char tmp_name[256];
	char new_name[256];
	char hostname[64];
	FILE * file;
	int state = 1;
	char ch;

	gethostname(hostname, sizeof hostname);
	sprintf(tmp_name, "tmp/%lu.%u.%s", now, pid, hostname);
	sprintf(new_name, "new/%lu.%u.%s", now, pid, hostname);
	file = fopen(tmp_name, "w");

	if (file == NULL) {
		fprintf(stderr, "Cannot open '%s': %s\n", new_name, strerror(errno));
		fprintf(stderr, "Using stderr instead of file\n");
		file = stderr;
	}

	for (;;) {
		ch = getchar();

		switch (state) {
		case 0:
			if (ch == '\n') { if (file != stderr) fclose(file); straynewline(); }
			if (ch == '\r') { state = 4; continue; }
			break;
		case 1: /* \r\n */
			if (ch == '\n') { if (file != stderr) fclose(file); straynewline(); }
			if (ch == '.') { state = 2; continue; }
			if (ch == '\r') { state = 4; continue; }
			state = 0;
			break;
		case 2: /* \r\n + . */
			if (ch == '\n') { if (file != stderr) fclose(file); straynewline(); }
			if (ch == '\r') { state = 3; continue; }
			state = 0;
			break;
		case 3: /* \r\n + .\r */
			if (ch == '\n') {
				if (file != stderr) {
					fclose(file);
					if (rename(tmp_name, new_name)) {
						fprintf(stderr, "Cannot move file %s to %s: %s\n", tmp_name, new_name, strerror(errno));
					}
				}
				return;
			}
			putc('.', file);
			putc('\r', file);
			if (ch == '\r') { state = 4; continue; }
			state = 0;
			break;
		case 4: /* + \r */
			if (ch == '\n') { state = 1; break; }
			if (ch != '\r') { putc('\r', file); state = 0; }
		}
		putc(ch, file);
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
die_control() {
	out("421 unable to read controls (#4.3.0)\r\n");
	flush();
	_exit(1);
}


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

	setup();

	if (chdir(maildir) == -1) {
		fprintf(stderr, "Cannot change directory to maildir: %s\n", maildir);
		die_control();
	}

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
	return 0;
}
