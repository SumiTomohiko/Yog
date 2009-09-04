# -*- coding: utf-8 -*-

from pygments.lexer import RegexLexer
from pygments.token import Keyword, Literal

__all__ = ["YogLexer"]

class YogLexer(RegexLexer):
    name = "Yog"
    tokens = { "root": [
        ("as", Keyword),
        ("break", Keyword),
        ("class", Keyword),
        ("def", Keyword),
        ("do", Keyword),
        ("elif", Keyword),
        ("else", Keyword),
        ("end", Keyword),
        ("except", Keyword),
        ("finally", Keyword),
        ("if", Keyword),
        ("import", Keyword),
        ("module", Keyword),
        ("next", Keyword),
        ("nonlocal", Keyword),
        ("raise", Keyword),
        ("return", Keyword),
        ("try", Keyword),
        ("while", Keyword),
        ("__LINE__", Literal),
        ("false", Literal),
        ("nil", Literal),
        ("true", Literal),
        (".", Literal)]}

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
