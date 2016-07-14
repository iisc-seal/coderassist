problem=$1
maxProc=$2
timelimit=$3
inputfile=$4
runPreprocess=$5

inputFuncfile="inputFunc"
curpath=`pwd`
countProc=0

x="$curpath/$problem"
while read line
do
	y="$line"
	id=$(sed "s/\(.*\).\(ac\|wa\).\(c\|cpp\)$/\1/" <<< $y)
	c=$(sed "s/\(.*\).\(ac\|wa\).\(c\|cpp\)$/\3/" <<< $y)
	echo $id

	idsingle=$(sed "s/.*\/\([0-9][0-9]*\).\(ac\|wa\).\(c\|cpp\)$/\1/" <<< $y)
		
	if [ -e $x/$inputFuncfile ]
	then
	    inputFunc=`grep "$idsingle" $x/$inputFuncfile | cut -d: -f2`
	else
	    inputFunc=""
	fi

	copyfilename=$(sed "s/\(.*\)\.\(ac\|wa\)\.\(c\|cpp\)$/\1.\2.copy.\3/" <<< $y)

	if [ $runPreprocess -ne 1 ] && [ ! -e $copyfilename ]
	then
	    echo "ERROR: No $copyfilename"
	    exit
        fi
	bash -x /mnt/hdd1/shalini/fseDataSet/scripts/runTransform.sh $y $problem $runPreprocess 1 1

	countProc=`expr $countProc + 1`
	if [ $countProc -gt $maxProc ]
	then
	    p=`pidof findDP`
	    echo $p
	    for x1 in $p
	    do
		echo $x1
		timeElapsed=`ps -p $x1 -o etime= | sed "s/^ *\([^ ]*\)$/\1/" | cut -d: -f1`
		echo $timeElapsed
		if [ "$timeElapsed" = "" ]
		then
		    timeElapsed=0
		fi
		if [ $timeElapsed -ge $timelimit ]
		then
		    kill -9 $x1
		fi
	    done
	    p=`pidof findDP | wc -w`
	    while [ $p -ge $maxProc ]
	    do
		sleep $p
		p=`pidof findDP`
		for x1 in $p
		do
		    echo $x1
		    timeElapsed=`ps -p $x1 -o etime= | sed "s/^ *\([^ ]*\)$/\1/" | cut -d: -f1`
		    echo $timeElapsed
		    if [ "$timeElapsed" = "" ]
		    then
			timeElapsed=0
		    fi
		    if [ $timeElapsed -ge $timelimit ]
		    then
			kill -9 $x1
		    fi
		done
		p=`pidof findDP | wc -w`
	    done
	    countProc=`pidof findDP | wc -w`
	fi
done < $inputfile

p=`pidof findDP`
echo $p
for x in $p
do
    echo $x
    timeElapsed=`ps -p $x -o etime= | sed "s/^ *\([^ ]*\)$/\1/" | cut -d: -f1`
    echo $timeElapsed
    if [ "$timeElapsed" = "" ]
    then
	timeElapsed=0
    fi
    if [ $timeElapsed -ge $timelimit ]
    then
	kill -9 $x
    fi
done

p=`pidof findDP | wc -w`
while [ $p -ne 0 ]
do
    sleep $p
    p=`pidof findDP`
    for x in $p
    do
	echo $x
	timeElapsed=`ps -p $x -o etime= | sed "s/^ *\([^ ]*\)$/\1/" | cut -d: -f1`
	echo $timeElapsed
	if [ "$timeElapsed" = "" ]
	then
	    timeElapsed=0
	fi
	if [ $timeElapsed -ge $timelimit ]
	then
	    kill -9 $x
	fi
    done
    p=`pidof findDP | wc -w`
done
