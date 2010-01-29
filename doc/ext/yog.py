# -*- coding: utf-8 -*-

from re import compile
from docutils.nodes import Text, emphasis, field_list, list_item, paragraph
from sphinx.addnodes import desc, desc_content, desc_name, desc_parameter,\
                            desc_parameterlist
from sphinx.directives.desc import DescDirective
from sphinx.util.compat import Directive, directive_dwim

yog_sig_re = compile(r"(.*)\((.*)\)")

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

class ClassDirective(Directive):

    fields = ["base", "include"]
    has_content = True

    def handle_doc_fields(self, node):
        for child in node.children:
            if not isinstance(child, field_list):
                continue

            new_list = field_list()
            includings = []
            new_list += includings

            for name, body in child:
                print name
                if name == "base":
                    para = paragraph()
                    para += emphasis("base class ", "base class")
                    para += Text(" -- ", " -- ")
                    para += body.children
                    print body
                    print body.children
                    new_list += para
                elif name == "include":
                    para = paragraph()
                    para += emphasis("including", "including")
                    para += Text(" -- ", " -- ")
                    para += body.children

                    item = list_item()
                    item += para
                    includings.append(item)

            child.replace_self(new_list)

    def run(self):
        node = desc()
        node["desctype"] = self.name

        contentnode = desc_content()
        self.state.nested_parse(self.content, self.content_offset, contentnode)
        self.handle_doc_fields(contentnode)
        node.append(contentnode)

        return [node]

def setup(app):
    app.add_directive("class", directive_dwim(ClassDirective))
    app.add_directive("method", directive_dwim(MethodDirective))

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
