
" Write the following line in ~/.vim/filetype.vim
" au BufNewFile,BufRead *.yogram setf yogram

source <sfile>:p:h/yog.vim

sy match Identifier "<[a-z_][a-z_]*>"
sy match Define "%[^ ][^ ]*"
setlocal iskeyword+=-,>,{,},;
sy keyword Special -> { } ;

" vim: tabstop=2 shiftwidth=2 expandtab softtabstop=2
