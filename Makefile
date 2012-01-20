PROG=	usbiff
SRCS=	mbox.c usbiff.c usbnotifier.c

CC=		clang
CFLAGS+=	-Wall -Wextra -pedantic -Werror
LDFLAGS+=	-lusb

NO_MAN=

.include <bsd.prog.mk>
