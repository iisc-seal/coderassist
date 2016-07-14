# problem is the name of the root folder containing all the student submissions
problem=$1
# inputfile is the file containing the list of all submissions for which finddp
# succeeded
inputfile="$problem/successfiles.txt"
# outputfile is the file to which the features will be output
outputfile="$problem/features.csv"

if [ -e $outputfile ]
then
    rm -f $outputfile
fi

while read line
do
    id=$(sed "s/\(.*\).\(ac\|wa\).\(c\|cpp\)$/\1/" <<< $line)
    idsingle=$(sed "s/.*\/\([0-9][0-9]*\).\(ac\|wa\).\(c\|cpp\)$/\1/" <<< $line)

    if [ ! -e $id.features ]
    then
	continue
    fi
    outputLine="$line"
    while read fLine
    do
	value=`echo "$fLine" | cut -d: -f2`
	outputLine="$outputLine, $value"
    done < $id.features
    echo "$outputLine" >> $outputfile
done < $inputfile
