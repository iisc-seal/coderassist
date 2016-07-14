problem=$1
inputfile=$2

prefix="last"
lang="c"

curpath=`pwd`
x="$curpath/$problem"

arbitraryfile="arbitrary"

errorfiles=$x/$prefix.$lang.errorfiles.txt
errordetails=$x/$prefix.$lang.errordetails.txt
backedgefiles=$x/$prefix.$lang.backedgefiles.txt
heterofiles=$x/$prefix.$lang.heterofiles.txt
successfiles=$x/$prefix.$lang.successfiles.txt
emptyfiles=$x/$prefix.$lang.emptyfiles.txt
segfaultfiles=$x/$prefix.$lang.segfaultfiles.txt
skipfiles=$x/$prefix.$lang.skipfiles.txt
partialloopfiles=$x/$prefix.$lang.partialloopfiles.txt
pointerfiles=$x/$prefix.$lang.pointerfiles.txt
recordfiles=$x/$prefix.$lang.recordfiles.txt
filteredfiles=$x/$prefix.$lang.filteredfiles.txt
filteredinspectfiles=$x/$prefix.$lang.filtered.inspectfiles.txt
inscopefiles=$x/$prefix.$lang.inscopefiles.txt
residualfiles=$x/$prefix.$lang.residualfiles.txt
cycleincdgfiles=$x/$prefix.$lang.cycleincdgfiles.txt
multiplestmtfiles=$x/$prefix.$lang.multiplestmtfiles.txt
singlestmtfiles=$x/$prefix.$lang.singlestmtfiles.txt

totaltime=0
mintime=0
mintimesub=""
maxtime=0
maxtimesub=""
totalloc=0
residue=0

rm -rf $multiplestmtfiles $singlestmtfiles

if [ -e $residualfiles ]
then
    rm $residualfiles
fi

if [ -e $inscopefiles ]
then
    rm $inscopefiles
fi

if [ -e $errorfiles ]
then
	rm $errorfiles
fi

if [ -e $errordetails ]
then
	rm $errordetails
fi

if [ -e $backedgefiles ]
then
	rm $backedgefiles
fi

if [ -e $heterofiles ]
then
	rm $heterofiles
fi

if [ -e $successfiles ]
then
	rm $successfiles
fi

if [ -e $emptyfiles ]
then
	rm $emptyfiles
fi

if [ -e $segfaultfiles ]
then
	rm $segfaultfiles
fi

if [ -e $skipfiles ]
then
	rm $skipfiles
fi

if [ -e $partialloopfiles ]
then
	rm $partialloopfiles
fi

if [ -e $pointerfiles ]
then
	rm $pointerfiles
fi

if [ -e $recordfiles ]
then
	rm $recordfiles
fi

if [ -e $filteredfiles ]
then
    rm $filteredfiles
fi

if [ -e $filteredinspectfiles ]
then
    rm $filteredinspectfiles
fi

if [ -e $cycleincdgfiles ]
then
    rm $cycleincdgfiles
fi

errornum=0
errornumwa=0
errornumac=0
backedge=0
backedgewa=0
backedgeac=0
success=0
successwa=0
successac=0
heterogeneous=0
heterogeneouswa=0
heterogeneousac=0
empty=0
emptywa=0
emptyac=0
segfault=0
segfaultwa=0
segfaultac=0
skip=0
skipwa=0
skipac=0
partialloop=0
partialloopwa=0
partialloopac=0
pointer=0
pointerwa=0
pointerac=0
record=0
recordwa=0
recordac=0
cycle=0
cyclewa=0
cycleac=0
multi=0
multiwa=0
multiac=0
total=0
totalwa=0
totalac=0

while read line
do
    y="$line"
    echo $line
    idsingle=$(sed "s/.*\/\([0-9][0-9]*\).\(ac\|wa\).\(c\|cpp\)$/\1/" <<< $y)
    if [ -e $x/$arbitraryfile ]
    then
	c=`grep -c -w "$idsingle" $x/$arbitraryfile`
	if [ $c -ne 0 ]
	then
	    continue
	fi
    fi

    id=$(sed "s/\(.*\).\(ac\|wa\).\(c\|cpp\)$/\1/" <<< $y)
    status=$(sed "s/\(.*\).\(ac\|wa\).\(c\|cpp\)$/\2/" <<< $y)

    total=`expr $total + 1`
    if [ "$status" = "ac" ]
    then
	totalac=`expr $totalac + 1`
    else
	totalwa=`expr $totalwa + 1`
    fi

    if [ ! -e $id.findDP.output ]
    then
	empty=`expr $empty + 1`
	if [ "$status" = "ac" ]
	then
	    emptyac=`expr $emptyac + 1`
	    echo "$y" >> $inscopefiles
	else
	    emptywa=`expr $emptywa + 1`
	fi
	echo "$y" >> $emptyfiles
	echo "$y" >> $filteredfiles
	echo "$y" >> $filteredinspectfiles
	continue
    fi

    n=`cat $id.findDP.output | wc -l`
    if [ $n -eq 0 ]
    then
	empty=`expr $empty + 1`
	if [ "$status" = "ac" ]
	then
	    emptyac=`expr $emptyac + 1`
	    echo "$y" >> $inscopefiles
	else
	    emptywa=`expr $emptywa + 1`
	fi
	echo "$y" >> $emptyfiles
	echo "$y" >> $filteredfiles
	echo "$y" >> $filteredinspectfiles
	continue
    fi
    s=`grep -c "!!SKIPPING!!" $id.findDP.output`
    if [ $s -gt 0 ]
    then
	echo "$y" >> $skipfiles
	grep "!!SKIPPING!!" $id.findDP.output >> $skipfiles
	skip=`expr $skip + 1`
	if [ "$status" = "ac" ]
	then
	    skipac=`expr $skipac + 1`
	else
	    skipwa=`expr $skipwa + 1`
	fi
	continue
    fi
    n=`grep "ERROR" $id.findDP.output | wc -l`
    if [ $n -gt 0 ]
    then
	h=`grep "ERROR: Found heterogeneous block" $id.findDP.output | wc -l`
	if [ $h -gt 0 ]
	then
	    subtime=`grep "Time: " $id.findDP.output | sed "s/^Time: \([0-9][0-9\.]*\)$/\1/"`
	    if [ "$subtime" = "" ]
	    then
		subtime=0
	    fi
#	    mflag=`echo "$subtime > $maxtime" | bc`
#	    if [ $mflag -eq 1 ]
#	    then
#		maxtime=$subtime
#		maxtimesub="$line"
#	    fi
#	    mflag=`echo "$subtime < $mintime" | bc`
#	    if [ $mflag -eq 1 ]
#	    then
#		mintime=$subtime
#		mintimesub="$line"
#	    fi
#	    totaltime=`echo "$totaltime + $subtime" | bc`
#	    sublines=`cat $line | wc -l`
#	    totalloc=`echo "$totalloc + $sublines" | bc`
	    echo "$y" >> $heterofiles
	    echo "$y" >> $filteredfiles
	    echo "$y" >> $filteredinspectfiles
	    heterogeneous=`expr $heterogeneous + 1`
	    if [ "$status" = "ac" ]
	    then
		heterogeneousac=`expr $heterogeneousac + 1`
		echo "$y" >> $inscopefiles
	    else
		heterogeneouswa=`expr $heterogeneouswa + 1`
	    fi
	else
	    b=`grep "ERROR:" $id.findDP.output | grep "back edge" | wc -l`
	    if [ $b -gt 0 ]
	    then
		subtime=`grep "Time: " $id.findDP.output | sed "s/^Time: \([0-9][0-9\.]*\)$/\1/"`
		if [ "$subtime" = "" ]
		then
		    subtime=0
		fi
#		mflag=`echo "$subtime > $maxtime" | bc`
#		if [ $mflag -eq 1 ]
#		then
#		    maxtime=$subtime
#		    maxtimesub="$line"
#		fi
#		mflag=`echo "$subtime < $mintime" | bc`
#		if [ $mflag -eq 1 ]
#		then
#		    mintime=$subtime
#		    mintimesub="$line"
#		fi
#		totaltime=`echo "$totaltime + $subtime" | bc`
#		sublines=`cat $line | wc -l`
#		totalloc=`echo "$totalloc + $sublines" | bc`
		echo "$y" >> $backedgefiles
		echo "$y" >> $filteredfiles
		backedge=`expr $backedge + 1`
	    	if [ "$status" = "ac" ]
	    	then
		    backedgeac=`expr $backedgeac + 1`
		    echo "$y" >> $inscopefiles
	    	else
		    backedgewa=`expr $backedgewa + 1`
	    	fi
	    else
		p=`grep "ERROR: Loop headers are partially equal" $id.findDP.output | wc -l`
		if [ $p -gt 0 ]
		then
		    subtime=`grep "Time: " $id.findDP.output | sed "s/^Time: \([0-9][0-9\.]*\)$/\1/"`
		    if [ "$subtime" = "" ]
		    then
			subtime=0
		    fi
#		    mflag=`echo "$subtime > $maxtime" | bc`
#		    if [ $mflag -eq 1 ]
#		    then
#			maxtime=$subtime
#			maxtimesub="$line"
#		    fi
#		    mflag=`echo "$subtime < $mintime" | bc`
#		    if [ $mflag -eq 1 ]
#		    then
#			mintime=$subtime
#			mintimesub="$line"
#		    fi
#		    totaltime=`echo "$totaltime + $subtime" | bc`
#		    sublines=`cat $line | wc -l`
#		    totalloc=`echo "$totalloc + $sublines" | bc`
		    echo "$y" >> $partialloopfiles
		    echo "$y" >> $filteredfiles
		    partialloop=`expr $partialloop + 1`
		    if [ "$status" = "ac" ]
		    then
			partialloopac=`expr $partialloopac + 1`
			echo "$y" >> $inscopefiles
		    else
			partialloopwa=`expr $partialloopwa + 1`
		    fi
		else
		    p=`grep "ERROR: Var is of Pointer type. Not handling" $id.findDP.output | wc -l`
		    if [ $p -gt 0 ]
		    then
			echo "$y" >> $pointerfiles
			pointer=`expr $pointer + 1`
			if [ "$status" = "ac" ]
			then
			    pointerac=`expr $pointerac + 1`
			else
			    pointerwa=`expr $pointerwa + 1`
			fi
		    else
			r=`grep "ERROR: Var is of Record type. Not handling" $id.findDP.output | wc -l`
			if [ $r -gt 0 ]
			then
			    echo  "$y" >> $recordfiles
			    record=`expr $record + 1`
			    if [ "$status" = "ac" ]
			    then
				recordac=`expr $recordac + 1`
			    else
				recordwa=`expr $recordwa + 1`
			    fi
			else
			    c=`grep "ERROR: " $id.findDP.output | grep "cycle in the CDG" | wc -l`
			    if [ $c -gt 0 ]
			    then
				echo "$y" >> $cycleincdgfiles
				cycle=`expr $cycle + 1`
				if [ "$status" = "ac" ]
				then
				    cycleac=`expr $cycleac + 1`
				else
				    cyclewa=`expr $cyclewa + 1`
				fi
			    else
				echo "$y" >> $errorfiles
				echo "$y" >> $errordetails
				echo "$y" >> $filteredfiles
				echo "$y" >> $filteredinspectfiles
				grep "ERROR: " $id.findDP.output >> $errordetails
				errornum=`expr $errornum + 1`
				if [ "$status" = "ac" ]
				then
				    errornumac=`expr $errornumac + 1`
				    echo "$y" >> $inscopefiles
				else
				    errornumwa=`expr $errornumwa + 1`
				fi
			    fi
	    	    	fi
	    	    fi
		fi
	    fi
    	fi
    else
	subtime=`grep "Time: " $id.findDP.output | sed "s/^Time: \([0-9][0-9\.]*\)$/\1/"`
	if [ "$subtime" = "" ]
	then
	    subtime=0
	fi
#	mflag=`echo "$subtime > $maxtime" | bc`
#	if [ $mflag -eq 1 ]
#	then
#	    maxtime=$subtime
#	    maxtimesub="$line"
#	fi
#	mflag=`echo "$subtime < $mintime" | bc`
#	if [ $mflag -eq 1 ]
#	then
#	    mintime=$subtime
#	    mintimesub="$line"
#	fi
#	totaltime=`echo "$totaltime + $subtime" | bc`
#	sublines=`cat $line | wc -l`
#	totalloc=`echo "$totalloc + $sublines" | bc`
	t=`grep "Command terminated by" $id.findDP.output | wc -l`
	if [ $t -gt 0 ]
	then
	    echo "$y" >> $segfaultfiles
	    echo "$y" >> $filteredfiles
	    echo "$y" >> $filteredinspectfiles
	    segfault=`expr $segfault + 1`
	    if [ "$status" = "ac" ]
	    then
		segfaultac=`expr $segfaultac + 1`
		echo "$y" >> $inscopefiles
	    else
		segfaultwa=`expr $segfaultwa + 1`
	    fi
	else
	    echo "$y" >> $successfiles
	    echo "$y" >> $filteredfiles
	    mflag=`echo "$subtime > $maxtime" | bc`
	    if [ $mflag -eq 1 ]
	    then
		maxtime=$subtime
		maxtimesub="$line"
	    fi
	    mflag=`echo "$subtime < $mintime" | bc`
	    if [ $mflag -eq 1 ]
	    then
		mintime=$subtime
		mintimesub="$line"
	    fi
	    totaltime=`echo "$totaltime + $subtime" | bc`
	    sublines=`cat $line | wc -l`
	    totalloc=`echo "$totalloc + $sublines" | bc`
	    r=`cat $id.residual | wc -l`
	    if [ $r -gt 1 ]
	    then
		residue=`expr $residue + 1`
		echo "$y" >> $residualfiles
	    fi
	    success=`expr $success + 1`
	    if [ "$status" = "ac" ]
	    then
		successac=`expr $successac + 1`
		echo "$y" >> $inscopefiles
	    else
		successwa=`expr $successwa + 1`
	    fi

	    # Multiple stmts
	    r=`grep "STAT: Multiple Stmts" $id.findDP.output | wc -l`
	    if [ $r -gt 0 ]
	    then
		echo "$y" >> $multiplestmtfiles
		if [ "$status" = "ac" ]
		then
		    multiac=`expr $multiac + 1`
		else
		    multiwa=`expr $multiwa + 1`
		fi
		multi=`expr $multi + 1`
	    else
		echo "$y" >> $singlestmtfiles
	    fi
	fi
    fi

done < $inputfile

manualskipped=`cat $x/arbitrary | wc -l`
echo "Skipped programs are due to Recursion/Class or Struct/Template func/No array"
echo "Manually skipped: $manualskipped"
echo "Total: $total wa: $totalwa ac: $totalac"
echo "Skipped: $skip wa: $skipwa ac: $skipac"
echo "Empty: $empty wa: $emptywa ac: $emptyac"
echo "Error: `expr $errornum + $heterogeneous + $backedge`"
echo "Heterogeneous: $heterogeneous wa: $heterogeneouswa ac: $heterogeneousac"
echo "Partial Loop: $partialloop wa: $partialloopwa ac: $partialloopac"
echo "Pointer: $pointer wa: $pointerwa ac: $pointerac"
echo "Record: $record wa: $recordwa ac: $recordac"
echo "Back Edge: $backedge wa: $backedgewa ac: $backedgeac"
echo "Cycle in CDG: $cycle wa: $cyclewa ac: $cycleac"
echo "Seg fault: $segfault wa: $segfaultwa ac: $segfaultac"
echo "Other Errors: $errornum wa: $errornumwa ac: $errornumac"
echo "Success: $success wa: $successwa ac: $successac"

echo ""

echo "Total: $total wa: $totalwa ac: $totalac"
unsupported=`expr $skip + $pointer + $record`
unsupportedwa=`expr $skipwa + $pointerwa + $recordwa`
unsupportedac=`expr $skipac + $pointerac + $recordac`
echo "Unsupported: `expr $unsupported` wa: `expr $unsupportedwa` ac: `expr $unsupportedac` (skip + pointer + record)"
limitation=`expr $partialloop + $backedge + $heterogeneous + $cycle`
limitationwa=`expr $partialloopwa + $backedgewa + $heterogeneouswa + $cyclewa`
limitationac=`expr $partialloopac + $backedgeac + $heterogeneousac + $cycleac`
echo "Limitation: $limitation wa: $limitationwa ac: $limitationac (partial loop + back edge + heterogeneous + cycle in CDG)"
filteredtotal=`expr $total - $unsupported - $limitation`
filteredwa=`expr $totalwa - $unsupportedwa - $limitationwa`
filteredac=`expr $totalac - $unsupportedac - $limitationac`
echo "After filtering, total: `expr $filteredtotal` wa: `expr $filteredwa` ac: `expr $filteredac`"
echo "Success: $success wa: $successwa ac: $successac"
echo "Multiple Stmts: $multi wa: $multiwa ac: $multiac"

totalavgtime=`echo "$totaltime / $filteredtotal" | bc`
echo "Total time: $totaltime"
echo "Avg time: $totalavgtime"
echo "Min time: $mintime - $mintimesub"
echo "Max time: $maxtime - $maxtimesub"
avgloc=`echo "$totalloc / $filteredtotal" | bc`
echo "Avg LOC: $avgloc"
echo "Residual for: $residue"
