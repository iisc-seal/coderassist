program=$1
problem=$2
preprocess=$3
parallel=$4
debug=$5

if [ "$parallel" = "" ]
then
    parallel=0
fi

if [ "$debug" = "" ]
then
    debug=0
fi

isref=`echo "$program" | grep -c "^ref\.[0-9][0-9]*\.c"`
pathpresent=`echo "$program" | grep -c "\/"`
if [ $pathpresent -ne 0 ]
then
    if [ $isref -eq 0 ]
    then
	id=$(sed "s/.*\/\([0-9][0-9]*\)\.\(wa\|ac\).*$/\1/g" <<< $program)
	st=$(sed "s/.*\/\([0-9][0-9]*\)\.\(wa\|ac\).*$/\2/g" <<< $program)
	prefix=$(sed "s/\(.*\)\/[0-9][0-9]*\.\(wa\|ac\).c/\1/g" <<< $program)
    else
	id=$(sed "s/.*\/\(ref\.[0-9][0-9]*\)\.c$/\1/g" <<< $program)
	st=""
	prefix=$(sed "s/\(.*\)\/\(ref\.[0-9][0-9]*\)\.c$/\1/g" <<< $program)
    fi
else
    if [ $isref -eq 0 ]
    then
	id=$(sed "s/\([0-9][0-9]*\)\.\(wa\|ac\).*$/\1/g" <<< $program)
	st=$(sed "s/\([0-9][0-9]*\)\.\(wa\|ac\).*$/\2/g" <<< $program)
	prefix=""
    else
	id=$(sed "s/\(ref\.[0-9][0-9]*\)\.c$/\1/g" <<< $program)
	st=""
	prefix=""
    fi
fi

if [ "$prefix" = "" ]
then
    if [ $isref -eq 0 ]
    then
	origFile="$id.$st.c"
	copyFile="$id.$st.copy.c"
    else
	origFile="$id.c"
	copyFile="$id.c"
    fi
else
    if [ $isref -eq 0 ]
    then
	origFile="$prefix/$id.$st.c"
	copyFile="$prefix/$id.$st.copy.c"
    else
	origFile="$prefix/$id.c"
	copyFile="$prefix/$id.c"
    fi
fi

if [ "$preprocess" = "" ]
then
    preprocess=0
fi


if [ ! -e $origFile ]
then
    echo "$origFile does not exist!"
    exit
fi

if [ $preprocess -eq 1 ]
then
    cp $origFile $copyFile
    cp scripts/.clang-format .
    str="clang-format -i $copyFile"
    eval $str

    judge=`grep -c "#define *ONLINE_JUDGE" $copyFile`
    if [ $judge -eq 0 ]
    then
	sed -i '1i #define ONLINE_JUDGE' $copyFile
    fi
    st=`grep -c "#include *<stdio.h>" $copyFile`
    if [ $st -eq 0 ]
    then
	sed -i '1i #include<stdio.h>' $copyFile
    fi
    st=`grep -c "#include *<stdlib.h>" $copyFile`
    if [ $st -eq 0 ]
    then
	sed -i '1i #include<stdlib.h>' $copyFile
    else 
	st=`grep -c "\/\/ *#include *<stdlib.h>" $copyFile`
	if [ $st -ne 0 ]
	then
	    sed -i '1i #include<stdlib.h>' $copyFile
	fi
    fi
    st=`grep -c "#include *<string.h>" $copyFile`
    if [ $st -eq 0 ]
    then
	sed -i '1i #include<string.h>' $copyFile
    fi

    # check if main is defined without return type
    st=`grep -c "^ *main(.*$" $copyFile`
    if [ $st -ne 0 ]
    then
	sed -i 's/^ *\(main(.*\)$/int \1/' $copyFile
    fi

    cp .clang-format .
    str="clang-format -i $copyFile"
    eval $str
    str="transform $copyFile --"
    eval $str
    str="clang-format -i $copyFile"
    eval $str
fi

if [ "$problem" = "" ]
then
    problem="-"
fi

if [ $debug -eq 1 ]
then
    bash -x scripts/runtool.sh $copyFile $problem $parallel
else
    bash scripts/runtool.sh $copyFile $problem $parallel
fi
