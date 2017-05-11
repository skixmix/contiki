#!/bin/bash -e

base_addr=$1
prefix=${base_addr%::*}

[ -z "$base_addr" ] && echo "usage: $0 <ipv6 addr>" && exit 0

level=$2

children=$(
	curl -sg "http://[$base_addr]/" \
	| sed '1,/Routes/ d; /<\/pre>/,$ d' \
	| sed 's/<[^>]*>//g' \
	| sed 's/.*via fe80:://i; s/).*//' \
	| sort | uniq
)

printf "%*c %s\n" $[ level * 5 ] "" "$base_addr"
for child in $children; do
	bash $0 $prefix::$child $[ level + 1 ]
done
