
source <sfile>:p:h/yog.vim

sy match Identifier "<[A-Za-z_][A-Za-z0-9_]*>"
sy match Define "%[^ ][^ ]*"
setlocal iskeyword+={,},;,:
sy keyword Special : { } ;

" vim: tabstop=2 shiftwidth=2 expandtab softtabstop=2
