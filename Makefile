PROG=	usbiff
SRCS=	conf_lexer.l conf_parser.y usbiff.c usbiff_config.c usbiff_mbox.c usbiff_signal.c usbnotifier.c

CC=		clang
CFLAGS+=	-I. -I${.CURDIR}
CFLAGS+=	-Wall -Wextra -pedantic -Werror
LDFLAGS+=	-lusb

MAN1=		usbiff.1
MAN5=		usbiffrc.5

CTAGS=		ctags

.if defined(DEBUG)
CFLAGS+=	-ggdb -O0
.endif

.include <bsd.prog.mk>
