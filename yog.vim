
sy region Comment start="(:" end=":)"
sy region Comment start="#" end="$"

sy keyword Boolean true false
sy keyword Constant nil
sy match Constant "'[A-Za-z_][A-Za-z_0-9]*\>"
sy region String start="\"" skip="\\\"" end="\""
sy match Number "\<[0-9][0-9_]*\>"
sy match Number "\<0[Xx][A-Fa-f0-9][A-Fa-f0-9_]*\>"
sy match Number "\<0[Bb][0-1][0-1_]*\>"
sy match Number "\<0[Oo][0-7][0-7_]*\>"
sy match Float "[0-9][0-9]*\.[0-9][0-9]*"

sy keyword Exception try except finally
sy keyword Keyword end from import as def do class return
sy keyword Conditional if elif else
sy keyword Statement nonlocal
sy keyword Repeat while

" vim: tabstop=2 shiftwidth=2 expandtab softtabstop=2
