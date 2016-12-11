me=${1}
while read -r line
do
   words=${line}
   for word in $words
   do 
	if [[ $word =~ $re1 ]]; then
	    echo ' matching word: '$word
	    count=$((count+1))
	fi
   done 
done < ${filename}

echo "matching count: $count"


