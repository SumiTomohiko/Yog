# -*- coding: utf-8 -*-

from pygments.lexer import RegexLexer
from pygments.token import Comment, Generic, Keyword, Literal, Name, Number, \
        String, Text

__all__ = ["YogLexer"]

class YogLexer(RegexLexer):
    name = "Yog"
    tokens = {
        "root": [
            (r"\($", Comment.Multiline, "comment"),
            ("\"", String, "string"),
            ("/.*/", String.Regex),
            (r"#.*$", Comment),
            (">>>", Generic.Prompt),
            (r"\.\.\.", Generic.Prompt),
            (r"as\b", Keyword),
            (r"break\b", Keyword),
            (r"class\b", Keyword),
            (r"def\b", Keyword),
            (r"do\b", Keyword),
            (r"elif\b", Keyword),
            (r"else\b", Keyword),
            (r"end\b", Keyword),
            (r"except\b", Keyword),
            (r"finally\b", Keyword),
            (r"if\b", Keyword),
            (r"import\b", Keyword),
            (r"module\b", Keyword),
            (r"next\b", Keyword),
            (r"nonlocal\b", Keyword),
            (r"raise\b", Keyword),
            (r"return\b", Keyword),
            (r"try\b", Keyword),
            (r"while\b", Keyword),
            (r"__LINE__\b", Literal),
            (r"false\b", Literal),
            (r"nil\b", Literal),
            (r"true\b", Literal),
            (r"self\b", Literal),
            (r"0[bB][0-1_]+", Number.Integer),
            (r"0[oO][0-8_]+", Number.Oct),
            (r"0[xX][0-9a-fA-F_]+", Number.Hex),
            (r"[0-9_]+\.[0-9_]+", Number.Float),
            (r"[0-9_]+", Number.Integer),
            (r"\.", Text, "name"),
            (r"^\s*@[0-9a-zA-Z_]+", Name.Decorator),
            (".", Text)],
        "comment": [
            (r"\($", Comment.Multiline, "#push"),
            (r"$\)", Comment.Multiline, "#pop"),
            (".", Comment.Multiline)],
        "string": [
            (r"\\\\", String.Escape),
            (r"\\n", String.Escape),
            (r"\\\"", String.Escape),
            (r"\"", String, "#pop"),
            (".", String)],
        "name": [
            ("[a-zA-Z]+", Text, "#pop")]}

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
