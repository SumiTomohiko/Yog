
module Ditz
  class Operator
    operation :bugs, "List bugs"
    def bugs project, config
      issues = project.issues.select do |i|
        next i.type == :bugfix
      end

      print_todo_list_by_release_for project, issues
    end
  end
end

# vim: tabstop=2 shiftwidth=2 expandtab softtabstop=2
