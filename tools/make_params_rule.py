# -*- coding: utf-8 -*-

def make_symbol(name, id):
    return "%(name)s(%(id)s)" % { "name": name, "id": id }

if __name__ == "__main__":
    terms = ["params_without_default", "params_with_default", "block_param", "var_param", "kw_param"]
    a = []
    for term in terms:
        a.append([term, ""])
    s = []
    for a0 in a[0]:
        for a1 in a[1]:
            for a2 in a[2]:
                for a3 in a[3]:
                    for a4 in a[4]:
                        rule = []
                        symbols = "BCDEF"
                        symbol_index = 0
                        if a0 != "":
                            symbol = symbols[symbol_index]
                            rule.append(make_symbol(a0, symbol))
                            symbol_index += 1
                        if a1 != "":
                            symbol = symbols[symbol_index]
                            rule.append(make_symbol(a1, symbol))
                            symbol_index += 1
                        if a2 != "":
                            symbol = symbols[symbol_index]
                            rule.append(make_symbol(a2, symbol))
                            symbol_index += 1
                        if a3 != "":
                            symbol = symbols[symbol_index]
                            rule.append(make_symbol(a3, symbol))
                            symbol_index += 1
                        if a4 != "":
                            symbol = symbols[symbol_index]
                            rule.append(make_symbol(a4, symbol))
                            symbol_index += 1
                        rules = " COMMA ".join(rule)
                        if rules == "":
                            rules = "/* empty */"
                        s.append("""
params(A) ::= %(rules)s. {""" % { "rules": rules })
                        if a0 == "":
                            s.append("""
    YogVal params_without_default = YNIL;""")
                        if a1 == "":
                            s.append("""
    YogVal params_with_default = YNIL;""")
                        if a2 == "":
                            s.append("""
    YogVal block_param = YNIL;""")
                        if a3 == "":
                            s.append("""
    YogVal var_param = YNIL;""")
                        if a4 == "":
                            s.append("""
    YogVal kw_param = YNIL;""")

                        params = ["A"]
                        symbol_index = 0
                        if a0 != "":
                            symbol = symbols[symbol_index]
                            symbol_index += 1
                            params.append(symbol)
                        else:
                            params.append("params_without_default")
                        if a1 != "":
                            symbol = symbols[symbol_index]
                            symbol_index += 1
                            params.append(symbol)
                        else:
                            params.append("params_with_default")
                        if a2 != "":
                            symbol = symbols[symbol_index]
                            symbol_index += 1
                            params.append(symbol)
                        else:
                            params.append("block_param")
                        if a3 != "":
                            symbol = symbols[symbol_index]
                            symbol_index += 1
                            params.append(symbol)
                        else:
                            params.append("var_param")
                        if a4 != "":
                            symbol = symbols[symbol_index]
                            symbol_index += 1
                            params.append(symbol)
                        else:
                            params.append("kw_param")
                        s.append("""
    PARAMS_NEW(%(params)s);
}""" % { "params": ", ".join(params) })

    print "".join(s)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
