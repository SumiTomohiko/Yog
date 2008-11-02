# -*- coding: utf-8 -*-

if __name__ == "__main__":
    terms = ["params_without_default", "params_with_default", "block_param", "var_param", "kw_param"]
    a = []
    for term in terms:
        a.append([term, ""])
    n = 0
    s = []
    for a0 in a[0]:
        for a1 in a[1]:
            for a2 in a[2]:
                for a3 in a[3]:
                    for a4 in a[4]:
                        rule = []
                        if a0 != "":
                            rule.append(a0)
                        if a1 != "":
                            rule.append(a1)
                        if a2 != "":
                            rule.append(a2)
                        if a3 != "":
                            rule.append(a3)
                        if a4 != "":
                            rule.append(a4)
                        if 0 < len(rule):
                            if n == 0:
                                s.append("""
params  : """)
                            else:
                                s.append("""
        | """)
                            s.append(" COMMA ".join(rule))
                            s.append("""
        {
        }""")
                            n =+ 1

    print "".join(s)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
