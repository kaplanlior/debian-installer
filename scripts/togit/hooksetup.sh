#!/bin/sh
# This sets up the post-commit hooks to send mail and notify cia,
# for all d-i repos. It is designed to be run again when adding a new repo.

set -e

cd /git/d-i

for repo in *.git attic/*.git; do
	cd /git/d-i/$repo

	proj=$(echo $repo | sed 's/\.git$//')

	# TODO search for [l10n] and SILENT_COMMIT in commit message; send
	# former to bubulle and drop latter.
	cat >hooks/post-receive <<EOF
#!/bin/sh
# Do not edit this file; it is regenerated by scripts/togit/hooksetup.sh
exec /usr/local/bin/git-commit-notice
EOF
	chmod 755 hooks/post-receive

	git config --replace-all hooks.mailinglist "debian-installer_cvs@packages.qa.debian.org"
	git config --replace-all hooks.replyto "debian-boot@lists.debian.org"
	# This makes the repo show up as a subproject under
	# debian-installer in CIA.
	git config --add hooks.cia-project "debian-installer/$proj"

	echo "d-i $proj repository" > description
done
