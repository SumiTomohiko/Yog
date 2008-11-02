
    switch (kwargc) {
        case 0: 
            {
                switch (varargc) {
                    case 0: 
                        {
                            switch (argc) {
                                case 0:
                                    {
                                        return (*f->f)(env, self);
                                        break;
                                    }
                                case 1:
                                    {
                                        return (*f->f)(env, self, args[0]);
                                        break;
                                    }
                                case 2:
                                    {
                                        return (*f->f)(env, self, args[0], args[1]);
                                        break;
                                    }
                                case 3:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2]);
                                        break;
                                    }
                                case 4:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3]);
                                        break;
                                    }
                                case 5:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4]);
                                        break;
                                    }
                                case 6:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5]);
                                        break;
                                    }
                                case 7:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
                                        break;
                                    }
                                case 8:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
                                        break;
                                    }
                                case 9:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
                                        break;
                                    }
                                case 10:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9]);
                                        break;
                                    }
                                case 11:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10]);
                                        break;
                                    }
                                case 12:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11]);
                                        break;
                                    }
                                case 13:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12]);
                                        break;
                                    }
                                case 14:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13]);
                                        break;
                                    }
                                case 15:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13], args[14]);
                                        break;
                                    }
                                default:
                                    Yog_assert(env, FALSE, "Too many arguments.");
                                    break;
                            }
                        }
                    case 1: 
                        {
                            switch (argc) {
                                case 0:
                                    {
                                        return (*f->f)(env, self, (YogArray*)YOGVAL_OBJ(args[0]));
                                        break;
                                    }
                                case 1:
                                    {
                                        return (*f->f)(env, self, args[0], (YogArray*)YOGVAL_OBJ(args[1]));
                                        break;
                                    }
                                case 2:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], (YogArray*)YOGVAL_OBJ(args[2]));
                                        break;
                                    }
                                case 3:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], (YogArray*)YOGVAL_OBJ(args[3]));
                                        break;
                                    }
                                case 4:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], (YogArray*)YOGVAL_OBJ(args[4]));
                                        break;
                                    }
                                case 5:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], (YogArray*)YOGVAL_OBJ(args[5]));
                                        break;
                                    }
                                case 6:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], (YogArray*)YOGVAL_OBJ(args[6]));
                                        break;
                                    }
                                case 7:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], (YogArray*)YOGVAL_OBJ(args[7]));
                                        break;
                                    }
                                case 8:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], (YogArray*)YOGVAL_OBJ(args[8]));
                                        break;
                                    }
                                case 9:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], (YogArray*)YOGVAL_OBJ(args[9]));
                                        break;
                                    }
                                case 10:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], (YogArray*)YOGVAL_OBJ(args[10]));
                                        break;
                                    }
                                case 11:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], (YogArray*)YOGVAL_OBJ(args[11]));
                                        break;
                                    }
                                case 12:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], (YogArray*)YOGVAL_OBJ(args[12]));
                                        break;
                                    }
                                case 13:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], (YogArray*)YOGVAL_OBJ(args[13]));
                                        break;
                                    }
                                case 14:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13], (YogArray*)YOGVAL_OBJ(args[14]));
                                        break;
                                    }
                                case 15:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13], args[14], (YogArray*)YOGVAL_OBJ(args[15]));
                                        break;
                                    }
                                default:
                                    Yog_assert(env, FALSE, "Too many arguments.");
                                    break;
                            }
                        }
                    default:
                        Yog_assert(env, FALSE, "Too many varargc.");
                        break;
                }
            }
        case 1: 
            {
                switch (varargc) {
                    case 0: 
                        {
                            switch (argc) {
                                case 0:
                                    {
                                        return (*f->f)(env, self, YogVal_undef());
                                        break;
                                    }
                                case 1:
                                    {
                                        return (*f->f)(env, self, args[0], YogVal_undef());
                                        break;
                                    }
                                case 2:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], YogVal_undef());
                                        break;
                                    }
                                case 3:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], YogVal_undef());
                                        break;
                                    }
                                case 4:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], YogVal_undef());
                                        break;
                                    }
                                case 5:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], YogVal_undef());
                                        break;
                                    }
                                case 6:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], YogVal_undef());
                                        break;
                                    }
                                case 7:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], YogVal_undef());
                                        break;
                                    }
                                case 8:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], YogVal_undef());
                                        break;
                                    }
                                case 9:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], YogVal_undef());
                                        break;
                                    }
                                case 10:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], YogVal_undef());
                                        break;
                                    }
                                case 11:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], YogVal_undef());
                                        break;
                                    }
                                case 12:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], YogVal_undef());
                                        break;
                                    }
                                case 13:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], YogVal_undef());
                                        break;
                                    }
                                case 14:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13], YogVal_undef());
                                        break;
                                    }
                                case 15:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13], args[14], YogVal_undef());
                                        break;
                                    }
                                default:
                                    Yog_assert(env, FALSE, "Too many arguments.");
                                    break;
                            }
                        }
                    case 1: 
                        {
                            switch (argc) {
                                case 0:
                                    {
                                        return (*f->f)(env, self, (YogArray*)YOGVAL_OBJ(args[0]), YogVal_undef());
                                        break;
                                    }
                                case 1:
                                    {
                                        return (*f->f)(env, self, args[0], (YogArray*)YOGVAL_OBJ(args[1]), YogVal_undef());
                                        break;
                                    }
                                case 2:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], (YogArray*)YOGVAL_OBJ(args[2]), YogVal_undef());
                                        break;
                                    }
                                case 3:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], (YogArray*)YOGVAL_OBJ(args[3]), YogVal_undef());
                                        break;
                                    }
                                case 4:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], (YogArray*)YOGVAL_OBJ(args[4]), YogVal_undef());
                                        break;
                                    }
                                case 5:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], (YogArray*)YOGVAL_OBJ(args[5]), YogVal_undef());
                                        break;
                                    }
                                case 6:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], (YogArray*)YOGVAL_OBJ(args[6]), YogVal_undef());
                                        break;
                                    }
                                case 7:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], (YogArray*)YOGVAL_OBJ(args[7]), YogVal_undef());
                                        break;
                                    }
                                case 8:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], (YogArray*)YOGVAL_OBJ(args[8]), YogVal_undef());
                                        break;
                                    }
                                case 9:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], (YogArray*)YOGVAL_OBJ(args[9]), YogVal_undef());
                                        break;
                                    }
                                case 10:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], (YogArray*)YOGVAL_OBJ(args[10]), YogVal_undef());
                                        break;
                                    }
                                case 11:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], (YogArray*)YOGVAL_OBJ(args[11]), YogVal_undef());
                                        break;
                                    }
                                case 12:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], (YogArray*)YOGVAL_OBJ(args[12]), YogVal_undef());
                                        break;
                                    }
                                case 13:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], (YogArray*)YOGVAL_OBJ(args[13]), YogVal_undef());
                                        break;
                                    }
                                case 14:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13], (YogArray*)YOGVAL_OBJ(args[14]), YogVal_undef());
                                        break;
                                    }
                                case 15:
                                    {
                                        return (*f->f)(env, self, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13], args[14], (YogArray*)YOGVAL_OBJ(args[15]), YogVal_undef());
                                        break;
                                    }
                                default:
                                    Yog_assert(env, FALSE, "Too many arguments.");
                                    break;
                            }
                        }
                    default:
                        Yog_assert(env, FALSE, "Too many varargc.");
                        break;
                }
            }
        default:
            Yog_assert(env, FALSE, "Too many kwargc.");
            break;
    }
    
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4 filetype=c
 */
