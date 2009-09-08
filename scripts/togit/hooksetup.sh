#!/bin/sh
# This sets up the post-commit hooks to send mail and notify cia,
# for all d-i repos. It is designed to be run again when adding a new repo.

set -e

cd /git/d-i

for repo in *.git; do
	proj=$(echo $repo | sed 's/\.git$//')

	# TODO search for [l10n] and SILENT_COMMIT in commit message; send
	# former to bubulle and drop latter.
	cat >hooks/post-receive <<END
#!/bin/sh
# Do not edit this file; it is regenerated by scripts/togit/hooksetup.sh
exec /usr/local/bin/git-commit-notice
END
	EOF
	chmod 755 hooks/post-receive

	git config --add hooks.mailinglist "debian-installer_cvs@packages.qa.debian.org"
	git config --add hooks.replyto "debian-boot@lists.debian.org"
	# This makes the repo show up as a subproject under
	# debian-installer in CIA.
	git config --add hooks.cia-project "debian-installer/$proj"
done
