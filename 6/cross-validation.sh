#!/bin/bash
[ $# -ne 1 ] && { echo "expected executable to test" >&2; exit 1; }

FILES=10
SUM_CORRECT=0
SUM_ALL=0
for i in $(seq -w 0 1 $((FILES-1))); do
	CORRECT=$("$1" <(find data ! -name "$i" -type f | xargs cat) <(cut -d ',' -f2- data/$i) |
			paste - <(cut -d ',' -f1 data/$i) | awk '$1==$2 { diff++ } END { print diff }')
	ALL=$(wc -l < data/$i)
	awk -v c=$CORRECT -v a=$ALL 'BEGIN { printf "%d/%d = %5.1f%%\n", c, a, 100*c/a }'
	((SUM_CORRECT+=CORRECT))
	((SUM_ALL+=ALL))
done
awk -v c=$SUM_CORRECT -v a=$SUM_ALL 'BEGIN { printf "Average:\n%d/%d = %5.1f%%\n", c, a, 100*c/a }'
exit 0
