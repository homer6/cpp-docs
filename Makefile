# Install the cpp-docs Claude Code skills for use in any project.
#
#   make install      link the user-level skills into ~/.claude/skills
#   make uninstall    remove those links
#
# The links point back at this checkout, so `git pull` keeps them current.
# Override the target with SKILLS_DIR=/path/to/skills.

SKILLS_DIR ?= $(HOME)/.claude/skills
REPO       := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
SKILLS     := modern-cpp cpp-guidelines

.PHONY: install uninstall

install:
	@mkdir -p "$(SKILLS_DIR)"
	@for s in $(SKILLS); do \
	  dst="$(SKILLS_DIR)/$$s"; src="$(REPO)/.claude/skills/$$s"; \
	  if [ -L "$$dst" ]; then ln -sfn "$$src" "$$dst"; echo "updated  $$dst -> $$src"; \
	  elif [ -e "$$dst" ]; then echo "skipped  $$dst exists and isn't a link; move it aside first" >&2; exit 1; \
	  else ln -s "$$src" "$$dst"; echo "linked   $$dst -> $$src"; fi; \
	done

uninstall:
	@for s in $(SKILLS); do \
	  dst="$(SKILLS_DIR)/$$s"; \
	  if [ -L "$$dst" ]; then rm "$$dst"; echo "removed  $$dst"; \
	  elif [ -e "$$dst" ]; then echo "skipped  $$dst isn't a link" >&2; \
	  fi; \
	done
