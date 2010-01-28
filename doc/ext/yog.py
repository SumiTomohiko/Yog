# -*- coding: utf-8 -*-

from re import compile
from sphinx.addnodes import desc_name, desc_parameter, desc_parameterlist
from sphinx.directives.desc import DescDirective
from sphinx.util.compat import directive_dwim

yog_sig_re = compile(r"(.*)\((.*)\)\Z")

class MethodDirective(DescDirective):

    def parse_signature(self, sig, signode):
        m = yog_sig_re.match(sig)
        if m is None:
            raise ValueError
        name, args = m.groups()

        signode += desc_name(name, name)

        params = desc_parameterlist()
        for token in args.split(","):
            token = token.strip()
            params += desc_parameter(token, token)
        signode += params

        return name

def setup(app):
    app.add_directive("method", directive_dwim(MethodDirective))

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
