#!/bin/bash
#
#Brandon Chin
#Assignment 6

FILE=$1

echo -n "Number of lines: "
wc -l -c $FILE | awk '{print $1}'

echo -n "Number of words: "
wc -w $FILE | awk '{print $1}'

echo "Most repetitive word: "

cat $FILE | tr '[:punct:] [:space:]' ' ' | tr 'A-Z' 'a-z' | tr -s ' ' | tr ' ' '\n' | sort | uniq -c | sort -rn | head -1

echo "Least repetitive word: "

cat $FILE | tr '[:punct:] [:space:]' ' ' | tr 'A-Z' 'a-z' | tr -s ' ' | tr ' ' '\n' | sort | uniq -c | sort -n | head -1

echo "Number of Words that Start and End with \"D\" or \"d\""
egrep -i '\<d' $FILE | egrep -i 'd\>' | wc -w

echo "Number of Words that Start with \"A\" or \"a\""
egrep -i '\<a' $FILE | wc -w

echo "Number of numeric words"
# egrep '/<[0-9]' $FILE | wc -w
egrep -o '\b[0-9]+\b' $FILE | wc -w

echo "Number of alphanumerical words"
egrep -o -I '[a-z]+\d+|\d+[a-z]+' $FILE | wc -w
