# hp-tools - Honeypot tools

**hp-tools** is meant to be a set of tools built on top of [```ucspi-tcp```](https://cr.yp.to/ucspi-tcp.html) or [```s6-networking```](http://skarnet.org/software/s6-networking/) or similar, for creation of simple honeypots.
I started this pet project with utility ```hp-smtpd``` that pretends behaviour of [```qmail-smtpd```](http://www.qmail.org/man/man8/qmail-smtpd.html) and logs every incomming byte to the standard error output.
It could be executed with command:

> tcpserver 0 25 ./hp-smtpd

where ```tcpserver``` is from [```ucspi-tcp```](https://cr.yp.to/ucspi-tcp.html) package.

To compile it run:

> make

## References

1. https://cr.yp.to/ucspi-tcp.html
1. http://skarnet.org/software/s6-networking/
1. http://www.qmail.org/man/man8/qmail-smtpd.html
