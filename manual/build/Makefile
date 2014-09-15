TARGETS=$(foreach language,$(languages),$(addprefix $(language).,$(architectures)))

.PHONY: $(TARGETS)

all: $(TARGETS)

$(TARGETS): %:
	target=$@ ; \
	lang=$${target%.*} ; \
	lang_id="$$(echo $$lang | tr A-Z a-z | sed "s/_/-/")" ; \
	arch=$${target#*.} ; \
	eval arch_destination=$$destination ; \
	echo "Architecture: $$arch" ; \
	if [ -n "$$noarchdir" ]; then \
		destsuffix="$$lang" ; \
	else \
		if [ -n "$$web" ] ; then \
			destsuffix="$$arch" ; \
		else \
			destsuffix="$$lang.$$arch" ; \
		fi ; \
	fi ; \
	export destdir=build.out.$$lang.$$arch ; \
	export tempdir=build.tmp.$$lang.$$arch ; \
	./buildone.sh "$$arch" "$$lang" "$$formats" ; \
	mkdir -p "$$arch_destination/$$destsuffix" ; \
	for format in $$formats; do \
		if [ "$$format" = html ]; then \
			mv ./$$destdir/html/* "$$arch_destination/$$destsuffix" ; \
		else \
			# Do not fail because of missing PDF support for some languages \
			if [ -n "$$web" ] ; then \
				mv ./$$destdir/install.$$lang.$$format "$$arch_destination/$$destsuffix/install.$$format.$$lang_id" 2>/dev/null || true ; \
			else \
				mv ./$$destdir/install.$$lang.$$format "$$arch_destination/$$destsuffix" 2>/dev/null || true ; \
			fi ; \
		fi ; \
	done ; \
	./clear.sh
