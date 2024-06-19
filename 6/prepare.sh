#!/bin/bash
[ $# -ne 1 ] && { echo "Usage: $0 <filename>" >&2; exit 1; }

mkdir -p data
split -a1 -d -n l/10 "$1" data/
