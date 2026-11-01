# Install the cpp-docs Claude Code skills for use in any project.
#
#   make install      link the user-level skills into ~/.claude/skills
#   make update       pull the latest release, then re-link (upgrade)
#   make uninstall    remove those links
#
# The links point back at this checkout, so pulling it upgrades the skills in
# place. `make install` also upgrades older installs: a link to another
# checkout is re-pointed here, and an older copy (a plain folder holding our
# skill) is moved to $(SKILLS_BACKUP)/<name>-<timestamp> and replaced by a link.
# Override the target with SKILLS_DIR=/path/to/skills.

SKILLS_DIR ?= $(HOME)/.claude/skills
# Older copies go here, outside SKILLS_DIR, so Claude Code doesn't load them twice.
SKILLS_BACKUP ?= $(SKILLS_DIR)-backup
REPO       := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
SKILLS     := modern-cpp cpp-guidelines

.PHONY: install update uninstall

install:
	@mkdir -p "$(SKILLS_DIR)"
	@for s in $(SKILLS); do \
	  dst="$(SKILLS_DIR)/$$s"; src="$(REPO)/.claude/skills/$$s"; \
	  if [ -L "$$dst" ]; then \
	    old="$$(readlink "$$dst")"; \
	    if [ "$$old" = "$$src" ]; then echo "current  $$dst"; \
	    else ln -sfn "$$src" "$$dst"; echo "upgraded $$dst -> $$src (was a link to $$old)"; fi; \
	  elif [ -d "$$dst" ] && grep -q "^name: $$s$$" "$$dst/SKILL.md" 2>/dev/null; then \
	    mkdir -p "$(SKILLS_BACKUP)"; bak="$(SKILLS_BACKUP)/$$s-$$(date +%Y%m%d%H%M%S)"; \
	    mv "$$dst" "$$bak"; ln -s "$$src" "$$dst"; \
	    echo "upgraded $$dst -> $$src (older copy moved to $$bak)"; \
	  elif [ -e "$$dst" ]; then echo "skipped  $$dst exists and isn't a cpp-docs '$$s' skill; move it aside first" >&2; exit 1; \
	  else ln -s "$$src" "$$dst"; echo "linked   $$dst -> $$src"; fi; \
	done

update:
	@branch="$$(git -C "$(REPO)" rev-parse --abbrev-ref HEAD)"; \
	if [ "$$branch" != main ]; then echo "note: $(REPO) is on '$$branch'; releases are on main"; fi; \
	git -C "$(REPO)" pull --ff-only
	@$(MAKE) --no-print-directory install

uninstall:
	@for s in $(SKILLS); do \
	  dst="$(SKILLS_DIR)/$$s"; \
	  if [ -L "$$dst" ]; then rm "$$dst"; echo "removed  $$dst"; \
	  elif [ -e "$$dst" ]; then echo "skipped  $$dst isn't a link" >&2; \
	  fi; \
	done
