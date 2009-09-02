# Ditz bug plugin
#
# This plugin adds the "bugs" command to ditz. This command shows a list of
# bugs.
#
# How to use
#
# 1. sudo cp tools/bugs.rb /usr/lib/ruby/gems/1.8/gems/ditz-0.5/lib/plugins
# 2. add the "- bugs" entry to your project's .ditz-plugins.

module Ditz
  class Operator
    operation :bugs, "List bugs"
    def bugs project, config
      issues = project.issues.select do |i|
        next i.bug? && i.open?
      end

      print_todo_list_by_release_for project, issues
    end
  end
end

# vim: tabstop=2 shiftwidth=2 expandtab softtabstop=2
