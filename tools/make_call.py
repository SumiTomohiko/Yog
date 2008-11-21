# -*- coding: utf-8 -*-

if __name__ == "__main__":
    s = ["""
    switch (kwargc) {"""]
    for kwargc in xrange(2):
        s.append("""
        case %(kwargc)d: 
            {
                switch (varargc) {""" % { "kwargc": kwargc })

        for varargc in xrange(2):
            s.append("""
                    case %(varargc)d: 
                        {
                            switch (argc) {""" % { "varargc": varargc })
            for argc in xrange(16):
                s.append("""
                                case %(argc)d:
                                    {
                                        return (*f->f)(env, self""" % { "argc": argc })
                for i in xrange(argc):
                    s.append(", args[%(index)d]" % { "index": i })
                if 0 < varargc:
                    s.append(", (YogArray*)YOGVAL_OBJ(args[%(index)d])" % { "index": argc })
                if 0 < kwargc:
                    s.append(", YogVal_undef()" % { "index": argc + varargc })
                s.append(""");
                                        break;
                                    }""")
            s.append("""
                                default:
                                    YOG_ASSERT(env, FALSE, "Too many arguments.");
                                    break;
                            }
                        }""")

        s.append("""
                    default:
                        YOG_ASSERT(env, FALSE, "Too many varargc.");
                        break;
                }
            }""")

    s.append("""
        default:
            YOG_ASSERT(env, FALSE, "Too many kwargc.");
            break;
    }
    
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4 filetype=c
 */""")

    print "".join(s)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
