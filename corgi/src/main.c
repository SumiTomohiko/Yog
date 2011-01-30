#include "corgi/config.h"
#if defined(CORGI_HAVE_ALLOCA_H)
#   include <alloca.h>
#endif
#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "corgi.h"

typedef CorgiUInt Bool;
#define TRUE    (42 == 42)
#define FALSE   !TRUE

#define TRACE(...) do { \
    FILE* fp = stdout; \
    fprintf(fp, "%s:%u ", __FILE__, __LINE__); \
    fprintf(fp, __VA_ARGS__); \
    fprintf(fp, "\n"); \
    fflush(fp); \
} while (0)

struct Options {
    Bool debug;
    CorgiUInt group_id;
    const char* group_name;
    Bool ignore_case;
};

typedef struct Options Options;

static void
usage()
{
    puts("corgi OPTIONS COMMAND ...");
    puts("");
    puts("OPTIONS:");
    puts("  --debug, -d: Enable debugging");
    puts("  --group-id, -g: Group number to show");
    puts("  --help, -h: Show this message");
    puts("  --version, -v: Show version information and exit");
    puts("");
    puts("COMMAND:");
    puts("  disassemble <regexp>");
    puts("  dump <regexp>");
    puts("  match <regexp> <string>");
    puts("  search <regexp> <string>");
}

static int
get_char_bytes(const char* s)
{
#define RETURN_IF_MATCH(x, v) do { \
    if ((s[0] & (x)) == (x)) { \
        return (v); \
    } \
} while (0)
    RETURN_IF_MATCH(0xfc, 6);
    RETURN_IF_MATCH(0xf8, 5);
    RETURN_IF_MATCH(0xf0, 4);
    RETURN_IF_MATCH(0xe0, 3);
    RETURN_IF_MATCH(0xc0, 2);
#undef RETURN_IF_MATCH
    return 1;
}

static CorgiChar
conv_utf8_char_to_utf32(const char* s)
{
#define GET_1_BIT(x)    ((x) & 0x01)
#define GET_2_BITS(x)   ((x) & 0x03)
#define GET_3_BITS(x)   ((x) & 0x07)
#define GET_4_BITS(x)   ((x) & 0x0f)
#define GET_5_BITS(x)   ((x) & 0x1f)
#define GET_6_BITS(x)   ((x) & 0x3f)
    switch (get_char_bytes(s)) {
    case 1:
        return s[0];
    case 2:
        return (GET_5_BITS(s[0]) << 6) + GET_6_BITS(s[1]);
    case 3:
        return (GET_4_BITS(s[0]) << 12) + (GET_6_BITS(s[1]) << 6) + GET_6_BITS(s[2]);
    case 4:
        return (GET_3_BITS(s[0]) << 18) + (GET_6_BITS(s[1]) << 12) + (GET_6_BITS(s[2]) << 12) + GET_6_BITS(s[3]);
    case 5:
        return (GET_2_BITS(s[0]) << 24) + (GET_6_BITS(s[1]) << 18) + (GET_6_BITS(s[2]) << 12) + (GET_6_BITS(s[3]) << 6) + GET_6_BITS(s[4]);
    case 6:
        return (GET_1_BIT(s[0]) << 30) + (GET_2_BITS(s[1]) << 24) + (GET_6_BITS(s[2]) << 18) + (GET_6_BITS(s[3]) << 12) + (GET_6_BITS(s[4]) << 6) + GET_6_BITS(s[5]);
    default:
        return '?';
    }
#undef GET_6_BITS
#undef GET_5_BITS
#undef GET_4_BITS
#undef GET_3_BITS
#undef GET_2_BITS
#undef GET_1_BIT
}

static void
conv_utf8_to_utf32(CorgiChar* dest, const char* src)
{
    const char* pc;
    CorgiChar* q = dest;
    for (pc = src; *pc != '\0'; pc += get_char_bytes(pc)) {
        *q = conv_utf8_char_to_utf32(pc);
        q++;
    }
}

static int
count_chars(const char* s)
{
    int n = 0;
    const char* pc;
    for (pc = s; *pc != '\0'; pc += get_char_bytes(pc)) {
        n++;
    }
    return n;
}

static CorgiUInt
conv_utf32_char_to_utf8(CorgiChar c , char* dest)
{
    if (isascii(c)) {
        dest[0] = c;
        return 1;
    }
    if (c < 0x800) {
        dest[0] = 0xc0 | (c >> 6);
        dest[1] = 0x80 | (c & 0x3f);
        return 2;
    }
    if (c < 0x10000) {
        dest[0] = 0xe0 | (c >> 12);
        dest[1] = 0x80 | ((c >> 6) & 0x3f);
        dest[2] = 0x80 | (c & 0x3f);
        return 3;
    }
    if (c < 0x200000) {
        dest[0] = 0xf0 | (c >> 18);
        dest[1] = 0x80 | ((c >> 12) & 0x3f);
        dest[2] = 0x80 | ((c >> 6) & 0x3f);
        dest[3] = 0x80 | (c & 0x3f);
        return 4;
    }
    if (c < 0x4000000) {
        dest[0] = 0xf8 | (c >> 24);
        dest[1] = 0x80 | ((c >> 18) & 0x3f);
        dest[2] = 0x80 | ((c >> 12) & 0x3f);
        dest[3] = 0x80 | ((c >> 6) & 0x3f);
        dest[4] = 0x80 | (c & 0x3f);
        return 5;
    }
    dest[0] = 0xfc | (c >> 30);
    dest[1] = 0x80 | ((c >> 24) & 0x3f);
    dest[2] = 0x80 | ((c >> 18) & 0x3f);
    dest[3] = 0x80 | ((c >> 12) & 0x3f);
    dest[4] = 0x80 | ((c >> 6) & 0x3f);
    dest[5] = 0x80 | (c & 0x3f);
    return 6;
}

static void
conv_utf32_to_utf8(char* s, CorgiChar* begin, CorgiChar* end)
{
    char* dest = s;
    CorgiChar* src;
    for (src = begin; src < end; src++) {
        dest += conv_utf32_char_to_utf8(*src, dest);
    }
    *dest = '\0';
}

static void
print_error(const char* msg, CorgiStatus status)
{
    printf("%s: %s (%u)\n", msg, corgi_strerror(status), status);
}

static CorgiStatus
get_group_id(CorgiRegexp* regexp, Options* opts, CorgiUInt* group_id)
{
    if (opts->group_name == NULL) {
        *group_id = opts->group_id;
        return CORGI_OK;
    }
    const char* name = opts->group_name;
    int size = count_chars(name);
    CorgiChar* begin = (CorgiChar*)alloca(sizeof(CorgiChar) * size);
    conv_utf8_to_utf32(begin, name);
    CorgiChar* end = begin + size;
    return corgi_group_name2id(regexp, begin, end, group_id);
}

typedef CorgiStatus (*Worker)(CorgiMatch*, CorgiRegexp*, CorgiChar*, CorgiChar*, CorgiChar*, CorgiOptions);

static CorgiStatus
work_with_match(CorgiRegexp* regexp, CorgiMatch* match, Options* opts, const char* s, const char* t, Worker f)
{
    int target_size = count_chars(t);
    CorgiChar* target = (CorgiChar*)alloca(sizeof(CorgiChar) * target_size);
    conv_utf8_to_utf32(target, t);
    CorgiChar* end = target + target_size;
    CorgiOptions corgi_opts = 0;
    if (opts->debug) {
        corgi_opts |= CORGI_OPT_DEBUG;
    }
    CorgiStatus status = f(match, regexp, target, end, target, corgi_opts);
    if (status == CORGI_MISMATCH) {
        return 1;
    }
    if (status != CORGI_OK) {
        print_error("Match failed", status);
        return 1;
    }
    CorgiUInt group_id;
    status = get_group_id(regexp, opts, &group_id);
    if (status != CORGI_OK) {
        print_error("Can't get group id", status);
        return 1;
    }
    CorgiInt matched_begin;
    CorgiInt matched_end;
    if (group_id == 0) {
        matched_begin = match->begin;
        matched_end = match->end;
    }
    else {
        corgi_get_group_range(match, group_id - 1, &matched_begin, &matched_end);
    }
    if (matched_begin < 0) {
        return 0;
    }
    CorgiUInt matched_size = matched_end - matched_begin;
    char* u = (char*)alloca(6 * matched_size + 1);
    conv_utf32_to_utf8(u, target + matched_begin, target + matched_end);
    printf("%s", u);
    return 0;
}

static int
work_with_regexp(CorgiRegexp* regexp, Options* opts, const char* s, const char* t, Worker f)
{
    int size = count_chars(s);
    CorgiChar* begin = (CorgiChar*)alloca(sizeof(CorgiChar) * size);
    conv_utf8_to_utf32(begin, s);
    CorgiChar* end = begin + size;
    CorgiOptions corgi_opts = 0;
    if (opts->ignore_case) {
        corgi_opts |= CORGI_OPT_IGNORE_CASE;
    }
    CorgiStatus status = corgi_compile(regexp, begin, end, corgi_opts);
    if (status != CORGI_OK) {
        print_error("Compile failed", status);
        return 1;
    }

    CorgiMatch match;
    corgi_init_match(&match);
    status = work_with_match(regexp, &match, opts, s, t, f);
    corgi_fini_match(&match);
    return status == CORGI_OK ? 0 : 1;
}

static int
work_main(Options* opts, int argc, char* argv[], Worker f)
{
    if (argc < 2) {
        usage();
        return 1;
    }
    CorgiRegexp regexp;
    corgi_init_regexp(&regexp);
    int ret = work_with_regexp(&regexp, opts, argv[0], argv[1], f);
    corgi_fini_regexp(&regexp);
    return ret;
}

static int
dump_main(Options* opts, int argc, char* argv[])
{
    if (argc < 1) {
        usage();
        return 1;
    }
    int size = count_chars(argv[0]);
    CorgiChar* re = alloca(sizeof(CorgiChar) * size);
    conv_utf8_to_utf32(re, argv[0]);
    CorgiOptions corgi_opts = 0;
    if (opts->ignore_case) {
        corgi_opts |= CORGI_OPT_IGNORE_CASE;
    }
    if (corgi_dump(re, re + size, corgi_opts) != CORGI_OK) {
        return 1;
    }
    return 0;
}

static int
disassemble_with_regexp(Options* opts, CorgiRegexp* regexp, const char* s)
{
    int size = count_chars(s);
    CorgiChar* begin = (CorgiChar*)alloca(sizeof(CorgiChar) * size);
    conv_utf8_to_utf32(begin, s);
    CorgiChar* end = begin + size;
    CorgiOptions corgi_opts = 0;
    if (opts->ignore_case) {
        corgi_opts |= CORGI_OPT_IGNORE_CASE;
    }
    if (corgi_compile(regexp, begin, end, corgi_opts) != CORGI_OK) {
        return 1;
    }
    if (corgi_disassemble(regexp) != CORGI_OK) {
        return 1;
    }
    return 0;
}

static int
disassemble_main(Options* opts, int argc, char* argv[])
{
    if (argc < 1) {
        usage();
        return 1;
    }
    CorgiRegexp regexp;
    corgi_init_regexp(&regexp);
    int ret = disassemble_with_regexp(opts, &regexp, argv[0]);
    corgi_fini_regexp(&regexp);
    return ret;
}

static int
corgi_main(Options* opts, int argc, char* argv[])
{
    if (argc < 1) {
        usage();
        return 1;
    }
    const char* cmd = argv[0];
    int cmd_argc = argc - 1;
    char** cmd_argv = argv + 1;
    if ((strcmp(cmd, "search") == 0) || (strcmp(cmd, "match") == 0)) {
        Worker f = strcmp(cmd, "search") == 0 ? corgi_search : corgi_match;
        return work_main(opts, cmd_argc, cmd_argv, f);
    }
    if (strcmp(cmd, "dump") == 0) {
        return dump_main(opts, cmd_argc, cmd_argv);
    }
    if (strcmp(cmd, "disassemble") == 0) {
        return disassemble_main(opts, cmd_argc, cmd_argv);
    }
    usage();
    return 1;
}

int
main(int argc, char* argv[])
{
    struct option longopts[] = {
        { "debug", no_argument, NULL, 'd' },
        { "group-id", required_argument, NULL, 'g' },
        { "group-name", required_argument, NULL, 'G' },
        { "help", no_argument, NULL, 'h' },
        { "ignore-case", no_argument, NULL, 'i' },
        { "version", no_argument, NULL, 'v' },
        { 0, 0, 0, 0 },
    };
    Options opts;
    bzero(&opts, sizeof(Options));
    int opt;
    char* s;
    while ((opt = getopt_long(argc, argv, "Gdg:hiv", longopts, NULL)) != -1) {
        switch (opt) {
        case 'G':
            s = (char*)alloca(strlen(optarg) + 1);
            strcpy(s, optarg);
            opts.group_name = s;
            break;
        case 'd':
            opts.debug = TRUE;
            break;
        case 'g':
            opts.group_id = atoi(optarg);
            break;
        case 'h':
            usage();
            return 0;
        case 'i':
            opts.ignore_case = TRUE;
            break;
        case 'v':
            printf("corgi %s\n", CORGI_PACKAGE_VERSION);
            return 0;
        case '?':
        default:
            usage();
            return 1;
        }
    }
    return corgi_main(&opts, argc - optind, argv + optind);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
