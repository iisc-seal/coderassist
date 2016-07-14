program=$1
problem=$2
parallel=$3

if [ "$parallel" = "" ]
then
    parallel=0
fi

pathpresent=`echo "$program" | grep -c "\/"`
isref=`echo "$program" | grep -c "^ref\.[0-9][0-9]*\.c"`
if [ $pathpresent -eq 0 ]
then
    inputfuncfile="../inputFunc"
    outputfuncfile="../outputFunc"
else
    if [ "$problem" = "-" ]
    then
	echo "There is no problem given. Can't find inputFunc and outputFunc without that"
	exit
    fi
    inputfuncfile="$problem/inputFunc"
    outputfuncfile="$problem/outputFunc"
fi

if [ -e $inputfuncfile ]
then
    if [ $pathpresent -eq 0 ]
    then
	if [ $isref -eq 0 ]
	then
	    id=$(sed "s/\([0-9][0-9]*\)\.\(wa\|ac\).*$/\1/g" <<< $program)
	    dir=""
	    if [ "$id" = "$program" ]
	    then
		id=$(sed "s/.*\(ref\.[0-9][0-9]*\)\.cpp.*$/\1/g" <<< $program)
	    fi
	else
	    id=$(sed "s/\(ref\.[0-9][0-9]*\)\.c$/\1/g" <<< $program)
	    dir=""
	    if [ "$id" = "$program" ]
	    then
		id=$(sed "s/.*\(ref\.[0-9][0-9]*\)\.cpp.*$/\1/g" <<< $program)
	    fi
	fi
    else
	if [ $isref -eq 0 ]
	then
	    id=$(sed "s/.*\/\([0-9][0-9]*\)\.\(wa\|ac\).*$/\1/g" <<< $program)
	    dir=$(sed "s/\(.*\)\/[0-9][0-9]*\.\(wa\|ac\).*$/\1/g" <<< $program)
	else
	    id=$(sed "s/.*\/\(ref\.[0-9][0-9]*\)\.c$/\1/g" <<< $program)
	    dir=$(sed "s/\(.*\)\/ref\.[0-9][0-9]*\.c$/\1/g" <<< $program)
	fi
    fi
else
    echo "ERROR: Cannot find inputFunc file"
    exit

fi

exists=`grep -c "^$id:" $inputfuncfile`
if [ $exists -ne 0 ]
then
    inputFunc=`grep "$id" $inputfuncfile | cut -d: -f2`
else
    inputFunc=""
fi
exists=`grep -c "^$id:" $outputfuncfile`
if [ $exists -ne 0 ]
then
    outputFunc=`grep "$id" $outputfuncfile | cut -d: -f2`
else
    outputFunc=""
fi

if [ "$dir" = "" ]
then
    outputfile="$id.output"
    prefix="$id"
else
    outputfile="$dir/$id.output"
    prefix="$dir/$id"
fi
str="/usr/bin/time -f \"Time: %e\" findFeatures $program -- $prefix "
arguments=""
if [ "$inputFunc" != "" ]
then
    numOfInputFuncs=`echo "$inputFunc" | awk -F',' '{print NF}'`
    i=1
    while [ $i -le $numOfInputFuncs ]
    do
	inputFuncName=`echo "$inputFunc" | cut -d',' -f$i`
	str="$str -inp $inputFuncName"
	arguments="$arguments -inp $inputFuncName"
	i=`expr $i + 1`
    done
fi
if [ "$outputFunc" != "" ]
then
    numOfOutputFuncs=`echo "$outputFunc" | awk -F',' '{print NF}'`
    i=1
    while [ $i -le $numOfOutputFuncs ]
    do
	outputFuncName=`echo "$outputFunc" | cut -d',' -f$i`
	str="$str -out $outputFuncName"
	arguments="$arguments -out $outputFuncName"
	i=`expr $i + 1`
    done
fi
if [ "$problem" = "-" ]
then
    presentpath=`pwd`
    sum=`echo "$presentpath" | grep -c "SUMTRIAN"`
    if [ $sum -ne 0 ]
    then
	echo "problem SUMTRIAN"
	arguments="$arguments -problem SUMTRIAN"
	str="$str -problem SUMTRIAN"
    fi
    sum=`echo "$presentpath" | grep -c "PPTEST"`
    if [ $sum -ne 0 ]
    then
	echo "problem PPTEST"
	arguments="$arguments -problem PPTEST"
	str="$str -problem PPTEST"
    fi
    sum=`echo "$presentpath" | grep -c "MGCRNK"`
    if [ $sum -ne 0 ]
    then
	echo "problem MGCRNK"
	arguments="$arguments -problem MGCRNK"
	str="$str -problem MGCRNK"
    fi
    sum=`echo "$presentpath" | grep -c "MARCHA1"`
    if [ $sum -ne 0 ]
    then
	echo "problem MARCHA1"
	arguments="$arguments -problem MARCHA1"
	str="$str -problem MARCHA1"
    fi
else
    arguments="$arguments -problem $problem"
    str="$str -problem $problem"
fi
# Clean up the directory
rm -f $prefix.feedback.* $prefix.typemap $prefix.features $prefix.*.summary
str="$str &> $outputfile"
echo "$str"
if [ $parallel -eq 1 ]
then
    /usr/bin/time -f "Time: %e" findFeatures $program -- $prefix $arguments &> $outputfile &
    exit
else
    eval "$str"
fi

time=`grep "Time: " $outputfile | sed "s/^Time: \([0-9][0-9\.]*\)$/\1/"`
if [ "$time" = "" ]
then
    time=0
fi
echo "Time: $time"

err=`grep "ERROR:" $outputfile | wc -l`
if [ $err -gt 0 ]
then
    echo "Error!!"
else
    term=`grep "Command terminated " $outputfile | wc -l`
    if [ $term -gt 0 ]
    then
	echo "Probably seg fault!!"
    else
	echo "Success!!"
    fi
fi
