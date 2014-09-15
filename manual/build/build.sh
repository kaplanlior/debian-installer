#!/bin/bash

set -e

[ -r ./po_functions ] || exit 1
. ./po_functions

manual_release=${manual_release:=jessie}

if [ -z "$languages" ]; then
    # Buildlist of languages
    # Based on list of languages used in official builds
    languages="$(cd ../debian; ./getfromlist langlist | tr $'\n' ' ')"
fi

if [ -z "$architectures" ]; then
    # Note: this list is no longer being maintained; see debian/archlist instead
    architectures="$(cd ../debian; ./getfromlist archlist | tr $'\n' ' ')"
fi

if [ -z "$destination" ]; then
    destination="/tmp/manual"
fi

if [ -z "$formats" ]; then
    #formats="html pdf ps txt"
    formats="html pdf txt"
fi

for arch in $architectures; do
    eval arch_destination=$destination
    [ -e "$arch_destination" ] || mkdir -p "$arch_destination"
done

if [ "$official_build" ]; then
    # Propagate this to children.
    export official_build
fi

# Delete any old generated XML files
clear_xml

# We need to merge the XML files for English and update the POT files
export PO_USEBUILD="1"
update_templates

for lang in $languages; do
    echo "Language: $lang";

    # Update PO files and create XML files
    if [ ! -d ../$lang ] && uses_po; then
        generate_xml
    fi
done
    
make languages="$languages" architectures="$architectures" destination="${destination//\$/\$\$}" formats="$formats"

PRESEED="../en/appendix/preseed.xml"
if [ -f $PRESEED ] && [ -f preseed.pl ] ; then
    for arch in $architectures; do
	eval arch_destination=$destination
	./preseed.pl -r $manual_release $PRESEED >$arch_destination/example-preseed.txt
    done
fi

# Delete temporary PO files and generated XML files
clear_po
clear_xml
