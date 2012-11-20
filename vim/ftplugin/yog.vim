
syntax region Comment start="(:" end=":)"
syntax region Comment start="#" end="$"

syntax keyword Boolean true false
syntax keyword Constant nil
syntax match Constant "'\h\w*"
syntax region String start="\"" skip="\(\\\"\|\\\\\)" end="\""
syntax match Number "\<\d\(\d\|_\)*"
syntax match Number "\<0[Xx]\x\(\x\|_\)*"
syntax match Number "\<0[Bb][0-1][0-1_]*"
syntax match Number "\<0[Oo]\o\(\o\|_\)*"
syntax match Float "\<\d\d*\.\d\d*"

syntax keyword Exception try raise except finally
syntax keyword Keyword class def do end module return
syntax keyword Include from import as
syntax keyword Conditional if elif else
syntax keyword Statement nonlocal
syntax keyword Repeat while next break
syntax keyword Special __FILE__ __LINE__ self super

syntax match Type "\<\u\w*"
"syntax match Identifier "\<\l\w*[!?]\="
syntax match Function "\(def\s\s*\)\@<=\h\w*[!?]\="

" vim: tabstop=2 shiftwidth=2 expandtab softtabstop=2
