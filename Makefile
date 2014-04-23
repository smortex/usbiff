PROG=	usbiff
SRCS=	conf_lexer.l conf_parser.y usbiff.c usbiff_config.c usbiff_mbox.c usbiff_signal.c usbnotifier.c

CC=		clang
CFLAGS+=	-I. -I${.CURDIR}
CFLAGS+=	-Wall -Wextra -pedantic -Werror
CFLAGS+=	-Dlint
LDFLAGS+=	-lusb
YFLAGS+=	-v

MAN=		usbiff.1 usbiffrc.5

CTAGS=		ctags

.if defined(DEBUG)
CFLAGS+=	-ggdb -O0
.endif

CLEANFILES=	conf_parser.output

.include <bsd.prog.mk>
