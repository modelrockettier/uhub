#!/bin/sh
#
# This is an optional git pre-commit hook to automatically update the
# exodus test.c files when committing changes to the tests directory.
#
# It comes in handy when you're working with the tests/ directory to
# ensure your changes to the .tcc files stay in sync with the exodus
# test.c files.
#
# To use this, copy it to the repository's .git/hooks/ directory and
# name it "pre-commit".  It must also be executable. E.g.
#   cp tests/git-pre-commit.sh .git/hooks/pre-commit
#   chmod +x .git/hooks/pre-commit

quit() {
	if [ -n "$*" ]; then
		echo "$*" >&2
	fi
	exit 0
}

# Ignore if there are no cached changes to tests/
cached_tests=$(git diff --name-only --cached tests) || quit "git-diff failed: $?"
test -n "$cached_tests" || quit

# The test.c files are always regenerated so md5sum them before and after
# running the update script to see if they change.
pre_md5=$(md5sum tests/*/test.c)

./tests/update.sh >/dev/null || quit "Failed to update test scripts: $?"

post_md5=$(md5sum tests/*/test.c)

changed_files=$(printf '%s\n' "$pre_md5" "$post_md5" \
		| sort \
		| uniq -u \
		| cut -d' ' -f3- \
		| sort -u)

if [ -n "$changed_files" ]; then
	printf "%s\n" \
		"NOTE: The following files have been changed:" \
		"$changed_files" \
		"" \
		"Consider running: git add tests/*/test.c" \
		>&2
	exit 1
fi

quit
