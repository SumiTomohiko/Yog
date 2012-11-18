
syntax region Comment start="(:" end=":)"
syntax region Comment start="#" end="$"

syntax keyword Boolean true false
syntax keyword Constant nil
syntax match Constant "'[A-Za-z_][A-Za-z_0-9]*\>"
syntax region String start="\"" skip="\\\"" end="\""
syntax match Number "\<[0-9][0-9_]*\>"
syntax match Number "\<0[Xx][A-Fa-f0-9][A-Fa-f0-9_]*\>"
syntax match Number "\<0[Bb][0-1][0-1_]*\>"
syntax match Number "\<0[Oo][0-7][0-7_]*\>"
syntax match Float "[0-9][0-9]*\.[0-9][0-9]*"

syntax keyword Exception try raise except finally
syntax keyword Keyword class def do end module return
syntax keyword Include from import as
syntax keyword Conditional if elif else
syntax keyword Statement nonlocal
syntax keyword Repeat while next break
syntax keyword Special __FILE__ __LINE__ self super

syntax match Type "\<_*[A-Z][A-Za-z_0-9]*\>"
syntax match Identifier "[A-Za-z_][A-Za-z0-9_]*[\\!\\?]?"
syntax match Function "\(def\s\s*\)\@<=\h\w*"

" vim: tabstop=2 shiftwidth=2 expandtab softtabstop=2
