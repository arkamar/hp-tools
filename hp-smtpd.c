#include <stdio.h>
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
err_unimpl(char * arg) {
	out("502 unimplemented (#5.5.1)\r\n");
}

void
smtp_rcpt(char * arg) {
	out("250 ok\r\n");
}

struct commands smtp_commands[] = {
	{ "rcpt", smtp_rcpt, 0 },
	{ NULL, err_unimpl, flush }
};

int
main(int argc, char * argv[]) {
	smtp_greet("200 ");
	out(" ESMTP\r\n");
}
