PROG=	usbiff
SRCS=	config.c conf_lexer.l conf_parser.y mbox.c signal.c usbiff.c usbnotifier.c

CC=		clang
CFLAGS+=	-Wall -Wextra -pedantic -Werror
LDFLAGS+=	-lusb

MAN1=		usbiff.1
MAN5=		usbiffrc.5

.if defined(DEBUG)
CFLAGS+=	-ggdb -O0
.endif

.include <bsd.prog.mk>
