" Vim syntax file
" Language:	Yog
" Maintainer:	SumiTomohiko <SumiTomohiko@neko-daisuki.ddo.jp>
" NOTICE: write following lines in @PREFIX@/share/vim/vim71/filetype.vim
" NOTICE: "Yog
" NOTICE: au BufNewFile,BufRead *.yg 			setf yog

if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

syn keyword   yogStatement    break continue do end except finally return try 
syn keyword   yogStatement    global def class nextgroup=yogFunction skipwhite
syn keyword   yogStatement    module nextgroup=yogFunction
syn match     yogFunction	    "[a-zA-Z_][a-zA-Z0-9_]*?\?!\?" contained
syn keyword   yogRepeat       while
syn keyword   yogConditional  if elif else
syn keyword   yogOperator	    && \|\| !
syn keyword   yogPreCondit    import from as
syn match     yogComment      "#.*$" contains=yogTodo,@Spell
syn keyword   yogTodo         TODO FIXME XXX contained

" Decorators
syn match     yogDecorator	  "@" display nextgroup=yogFunction skipwhite

" strings
syn region    yogString		    matchgroup=Normal start=+'+ end=+'+ skip=+\\\\\|\\'+ contains=yogEscape,@Spell
syn region    yogString		    matchgroup=Normal start=+"+ end=+"+ skip=+\\\\\|\\"+ contains=yogEscape,@Spell
syn match     yogEscape		    +\\[abfnrtv'"\\]+ contained
syn match     yogEscape		    "\\\o\{1,3}" contained
syn match     yogEscape		    "\\x\x\{2}" contained
syn match     yogEscape		    "\(\\u\x\{4}\|\\U\x\{8}\)" contained
syn match     yogEscape		    "\\$"

" numbers (including longs and complex)
syn match     yogNumber	      "\<0x\x\+[Ll]\=\>"
syn match     yogNumber	      "\<\d\+[LljJ]\=\>"
syn match     yogNumber	      "\.\d\+\([eE][+-]\=\d\+\)\=[jJ]\=\>"
syn match     yogNumber	      "\<\d\+\.\([eE][+-]\=\d\+\)\=[jJ]\=\>"
syn match     yogNumber	      "\<\d\+\.\d\+\([eE][+-]\=\d\+\)\=[jJ]\=\>"

syn keyword   yogBuiltin	    true false nil self

"syn keyword yogException	ArithmeticError AssertionError AttributeError

" trailing whitespace
syn match     yogSpaceError   display excludenl "\S\s\+$"ms=s+1
" mixed tabs and spaces
syn match     yogSpaceError   display " \+\t"
syn match     yogSpaceError   display "\t\+ "

" This is fast but code inside triple quoted strings screws it up. It
" is impossible to fix because the only way to know if you are inside a
" triple quoted string is to start from the beginning of the file. If
" you have a fast machine you can try uncommenting the "sync minlines"
" and commenting out the rest.
syn sync match yogSync grouphere NONE "):$"
syn sync maxlines=200
"syn sync minlines=2000

if version >= 508 || !exists("did_yog_syn_inits")
  if version <= 508
    let did_yog_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  " The default methods for highlighting.  Can be overridden later
  HiLink yogStatement     Statement
  HiLink yogFunction      Function
  HiLink yogConditional   Conditional
  HiLink yogRepeat        Repeat
  HiLink yogString        String
  HiLink yogRawString     String
  HiLink yogEscape        Special
  HiLink yogOperator      Operator
  HiLink yogPreCondit     PreCondit
  HiLink yogComment       Comment
  HiLink yogTodo          Todo
  HiLink yogDecorator     Define
  HiLink yogNumber        Number
  HiLink yogBuiltin       Number
  HiLink yogException     Exception
  HiLink yogSpaceError    Error

  delcommand HiLink
endif

let b:current_syntax = "yog"

" vim: tabstop=2 shiftwidth=2 expandtab softtabstop
