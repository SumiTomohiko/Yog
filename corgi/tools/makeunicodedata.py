#
# (re)generate unicode property and type databases
#
# this script converts a unicode 3.2 database file to
# Modules/unicodedata_db.h, Modules/unicodename_db.h,
# and Objects/unicodetype_db.h
#
# history:
# 2000-09-24 fl   created (based on bits and pieces from unidb)
# 2000-09-25 fl   merged tim's splitbin fixes, separate decomposition table
# 2000-09-25 fl   added character type table
# 2000-09-26 fl   added LINEBREAK, DECIMAL, and DIGIT flags/fields (2.0)
# 2000-11-03 fl   expand first/last ranges
# 2001-01-19 fl   added character name tables (2.1)
# 2001-01-21 fl   added decomp compression; dynamic phrasebook threshold
# 2002-09-11 wd   use string methods
# 2002-10-18 mvl  update to Unicode 3.2
# 2002-10-22 mvl  generate NFC tables
# 2002-11-24 mvl  expand all ranges, sort names version-independently
# 2002-11-25 mvl  add UNIDATA_VERSION
# 2004-05-29 perky add east asian width information
# 2006-03-10 mvl  update to Unicode 4.1; add UCD 3.2 delta
# 2008-06-11 gb   add PRINTABLE_MASK for Atsuo Ishimoto's ascii() patch
#
# written by Fredrik Lundh (fredrik@pythonware.com)
#

from os.path import join
import sys

SCRIPT = sys.argv[0]
try:
    src_dir = sys.argv[1]
except IndexError:
    src_dir = "src"
try:
    dest_dir = sys.argv[2]
except IndexError:
    dest_dir = src_dir

# The Unicode Database
UNICODE_DATA = join(src_dir, "UnicodeData.txt")
UNIHAN = join(src_dir, "Unihan.txt")
DERIVED_CORE_PROPERTIES = join(src_dir, "DerivedCoreProperties.txt")
LINE_BREAK = join(src_dir, "LineBreak.txt")

MANDATORY_LINE_BREAKS = ["BK", "CR", "LF", "NL"]

def parse_hex(s):
    return int(s, 16)

with open(join(src_dir, "unicode.c")) as fp:
    for line in [line.strip() for line in fp]:
        if not line.startswith("#define"):
            continue
        _, name, val = line.split()
        if name == "ALPHA_MASK":
            ALPHA_MASK = parse_hex(val)
        elif name == "DECIMAL_MASK":
            DECIMAL_MASK = parse_hex(val)
        elif name == "DIGIT_MASK":
            DIGIT_MASK = parse_hex(val)
        elif name == "NODELTA_MASK":
            NODELTA_MASK = parse_hex(val)
        elif name == "NUMERIC_MASK":
            NUMERIC_MASK = parse_hex(val)

def make_tables():
    print("--- Reading", UNICODE_DATA, "...")
    unicode = UnicodeData(UNICODE_DATA,
                          UNIHAN,
                          DERIVED_CORE_PROPERTIES,
                          LINE_BREAK)
    print(len(list(filter(None, unicode.table))), "characters")
    make_unicodetype(unicode)

def make_cases(filename, chars):
    with open(join(dest_dir, filename), "w") as fp:
        for codepoint in sorted(chars):
            print("case 0x%04X:" % (codepoint, ), file=fp)

# --------------------------------------------------------------------
# unicode character type tables
def make_unicodetype(unicode):
    # extract unicode types
    table = []
    cache = {}
    index = [0] * len(unicode.chars)
    numeric = {}
    spaces = []
    linebreaks = []

    for char in unicode.chars:
        record = unicode.table[char]
        if record is None:
            continue
        # extract database properties
        category = record.category
        bidirectional = record.bidi
        properties = record.properties
        flags = 0
        delta = True
        if category in ["Lm", "Lt", "Lu", "Ll", "Lo"]:
            flags |= ALPHA_MASK
        if ('Line_Break' in properties) or (bidirectional == "B"):
            linebreaks.append(char)
        if (category == "Zs") or (bidirectional in ("WS", "B", "S")):
            spaces.append(char)
        # use delta predictor for lower if it fits
        lower = parse_hex(record.lowercase) if record.lowercase != "" else char
        lower_d = lower - char
        if -32768 <= lower_d <= 32767:
            # use deltas
            lower = lower_d & 0xffff # To be in range of 16bits
        else:
            flags |= NODELTA_MASK
        # decimal digit, integer digit
        if record.decimal != "":
            flags |= DECIMAL_MASK
        if record.digit != "":
            flags |= DIGIT_MASK
        if record.numeric != "":
            flags |= NUMERIC_MASK
        item = (lower, flags)
        # add entry to index and item tables
        i = cache.get(item)
        if i is None:
            cache[item] = i = len(table)
            table.append(item)
        index[char] = i

    print(len(table), "unique character type entries")
    print(sum(map(len, numeric.values())), "numeric code points")
    print(len(spaces), "whitespace code points")
    print(len(linebreaks), "linebreak code points")

    with open(join(dest_dir, "entries.inc"), "w") as fp:
        for item in table:
            print("{ %d, %d }," % item, file=fp)

    # split decomposition index table
    index1, index2, shift = splitbins(index)
    with open(join(dest_dir, "indexes.inc"), "w") as fp:
        print("#define SHIFT", shift, file=fp)
        Array("index1", index1).dump(fp)
        Array("index2", index2).dump(fp)

    make_cases("whitespaces.inc", spaces)
    make_cases("linebreaks.inc", linebreaks)

def iterate_records(filename, sep=";", encoding="utf-8"):
    with open(filename, encoding=encoding) as fp:
        for line in fp:
            data = line.split("#")[0].strip()
            if data == "":
                continue
            yield [s.strip() for s in data.split(sep)]

CHARS_NUM = 0x110000

class Char:
    def __init__(self, name, category, combining, bidi, decimal, digit, numeric, lowercase):
        self.name = name
        self.category = category
        self.combining = combining
        self.bidi = bidi
        self.decimal = decimal
        self.digit = digit
        self.numeric = numeric
        self.lowercase = lowercase
        self.properties = set()

    def __repr__(self):
        attrs = [["name", self.name], ["category", self.category], ["combining", self.combining], ["bidi", self.bidi], ["decimal", self.decimal], ["digit", self.digit], ["numeric", self.numeric], ["lowercase", self.lowercase], ["properties", ", ".join(self.properties)]]
        return "<Char %s>" % (", ".join(["%s=\"%s\"" % (name, val) for name, val in attrs]))

def iterate_codepoint():
    for c in range(0x110000):
        yield c

def extract_range(s):
    if '..' not in s:
        first = parse_hex(s)
        return first, first + 1
    first, last = [parse_hex(c) for c in s.split('..')]
    return first, last + 1

# --------------------------------------------------------------------
# the following support code is taken from the unidb utilities
# Copyright (c) 1999-2000 by Secret Labs AB
# load a unicode-data file from disk
class UnicodeData:
    # Record structure:
    # [ID, name, category, combining, bidi, decomp,  (6)
    #  decimal, digit, numeric, bidi-mirrored, Unicode-1-name, (11)
    #  ISO-comment, uppercase, lowercase, titlecase, ea-width, (16)
    #  derived-props] (17)
    def __init__(self, data, unihan, derivedprops, linebreakprops):
        table = [None] * CHARS_NUM
        for rec in iterate_records(data):
            codepoint = parse_hex(rec[0])
            try:
                c = Char(rec[1], rec[2], rec[3], rec[4], rec[6], rec[7], rec[8], rec[13])
            except IndexError:
                continue
            table[codepoint] = c

        # expand first-last ranges
        field = None
        for c in iterate_codepoint():
            rec = table[c]
            if rec is not None:
                if rec.name.endswith("First>"):
                    rec.name = ""
                    field = rec
                elif rec.name.endswith("Last>"):
                    rec.name = ""
                    field = None
            elif field is not None:
                table[c] = field

        # public attributes
        self.table = table
        self.chars = list(range(CHARS_NUM)) # unicode 3.2

        for r, p in iterate_records(derivedprops):
            if ".." in r:
                first, last = [parse_hex(c) for c in r.split('..')]
                chars = list(range(first, last + 1))
            else:
                chars = [parse_hex(r)]
            for char in chars:
                if table[char] is None:
                    continue
                # Some properties (e.g. Default_Ignorable_Code_Point)
                # apply to unassigned code points; ignore them
                table[char].properties.add(p)

        for rec in iterate_records(linebreakprops):
            if (len(rec) < 2) or (rec[1] not in MANDATORY_LINE_BREAKS):
                continue
            first, last = extract_range(rec[0])
            for char in range(first, last):
                table[char].properties.add("Line_Break")

        for rec in iterate_records(unihan, "\t", "utf-8"):
            if not rec[0].startswith("U+"):
                continue
            try:
                code, tag, value = line.split(None, 3)[:3]
            except ValueError:
                continue
            if tag not in ('kAccountingNumeric', 'kPrimaryNumeric', 'kOtherNumeric'):
                continue
            c = parse_hex(code[len("U+"):])
            # Patch the numeric field
            if table[c] is None:
                continue
            table[c].numeric = value.strip().replace(',', '')

# stuff to deal with arrays of unsigned integers
class Array:
    def __init__(self, name, data):
        self.name = name
        self.data = data

    def dump(self, file):
        # write data to file, as a C array
        size = get_size(self.data)
        print(self.name + ":", size * len(self.data), "bytes", file=sys.stderr)
        file.write("static ")
        if size == 1:
            file.write("unsigned char")
        elif size == 2:
            file.write("unsigned short")
        else:
            file.write("unsigned int")
        file.write(" " + self.name + "[] = {\n")
        if self.data:
            s = "    "
            for item in self.data:
                i = str(item) + ", "
                if len(s) + len(i) > 78:
                    file.write(s + "\n")
                    s = "    " + i
                else:
                    s = s + i
            if s.strip():
                file.write(s + "\n")
        file.write("};\n\n")

def get_size(data):
    # return smallest possible integer size for the given array
    maxdata = max(data)
    if maxdata < 256:
        return 1
    if maxdata < 65536:
        return 2
    return 4

def splitbins(t):
    """t -> (t1, t2, shift).  Split a table to save space.

    t is a sequence of ints.  This function can be useful to save space if
    many of the ints are the same.  t1 and t2 are lists of ints, and shift
    is an int, chosen to minimize the combined size of t1 and t2 (in C
    code), and where for each i in range(len(t)),
        t[i] == t2[(t1[i >> shift] << shift) + (i & mask)]
    where mask is a bitmask isolating the last "shift" bits.
    """

    def dump(t1, t2, shift, bytes):
        print("%d + %d bins at shift %d; %d bytes" % (len(t1), len(t2), shift, bytes), file=sys.stderr)
    print("Size of original table:", len(t) * get_size(t), "bytes", file=sys.stderr)
    n = len(t)-1    # last valid index
    maxshift = 0    # the most we can shift n and still have something left
    if n > 0:
        while n >> 1:
            n >>= 1
            maxshift += 1
    del n
    bytes = sys.maxsize  # smallest total size so far
    t = tuple(t)    # so slices can be dict keys
    for shift in range(maxshift + 1):
        t1 = []
        t2 = []
        size = 2**shift
        bincache = {}
        for i in range(0, len(t), size):
            bin = t[i:i+size]
            index = bincache.get(bin)
            if index is None:
                index = len(t2)
                bincache[bin] = index
                t2.extend(bin)
            t1.append(index >> shift)
        # determine memory size
        b = len(t1) * get_size(t1) + len(t2) * get_size(t2)
        dump(t1, t2, shift, b)
        if b < bytes:
            best = t1, t2, shift
            bytes = b
    t1, t2, shift = best
    print("Best:", end=' ', file=sys.stderr)
    dump(t1, t2, shift, bytes)
    if __debug__:
        # exhaustively verify that the decomposition is correct
        mask = ~((~0) << shift) # i.e., low-bit mask of shift bits
        for i in range(len(t)):
            assert t[i] == t2[(t1[i >> shift] << shift) + (i & mask)]
    return best

if __name__ == "__main__":
    make_tables()

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
