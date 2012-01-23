" usbiff syntax file
" Filename:     usbiff.vim
" Language:     usbiff configuration file
" Maintainer:   Romain Tartiere <romain@blogreen.org>
" URL:		http://romain.blogreen.org/files/usbiff.vim
" Last Change:	Mon Jan  9 14:02:54 CET 2012
" Filenames:	usbiffrc
" Version:	1.0
"

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

syn keyword usbiffKeyword	flash-delay mailboxes set mbox-hook signal-hook unset
syn keyword usbiffVariable	color priority
syn keyword usbiffBoolVariable	flash noflash ignore noignore
syn keyword usbiffSignal	SIGUSR1 SIGUSR2
syn keyword usbiffColor		red green blue cyan magenta yellow white none contained

syn match   usbiffString        "[[:alnum:]/_\.-]\+"
syn match   usbiffString        "`[^`]*`"
syn region  usbiffString        start=+"+ skip=+\\\\\|\\"+ end=+"+
syn match   usbiffNumber        "[-+]\?[0-9]\+"
syn match   usbiffComment	"\s*#.*$" contains=usbiffTodo
syn keyword usbiffTodo		TODO NOTE FIXME XXX BUG HACK contained

syn match   usbiffSetStrAssignment /=\s*[0-9A-Za-z_]\+/hs=s+1 contains=usbiffColor
syn match   usbiffSetNumberAssignment /=\s*[-+]\?[0-9]\+/hs=s+1
syn match   usbiffSetBoolAssignment /=\s*\%(yes\|no\)/hs=s+1

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_usbiff_syn_inits")
  if version < 508
    let did_usbiff_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink usbiffKeyword	Statement

  HiLink usbiffVariable Identifier
  HiLink usbiffBoolVariable Identifier
  HiLink usbiffSignal Identifier

  HiLink usbiffSetNumberAssignment Number
  HiLink usbiffSetBoolAssignment	Boolean
  HiLink usbiffSetStrAssignment	String

  HiLink usbiffNumber	Number

  HiLink usbiffString	String
  HiLink usbiffColor	Special

  HiLink usbiffComment	Comment

  HiLink usbiffTodo	Todo

  delcommand HiLink
endif

let b:current_syntax = "usbiff"
set iskeyword+=-

