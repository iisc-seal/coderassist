# problem is the name of the root folder containing all the student submissions
problem=$1
# inputfile is the file that lists all submissions (each line in this file is a
# full path to each submission)
inputfile=$2

timelimit=1 # in minutes
runPreprocess=1 # 1 to run preprocess, 0 to disable
maxProc=10

# Runs findFeatures on each submission in the input file
time bash -x scripts/runGetFeaturesPart.sh $problem $maxProc $timelimit $inputfile $runPreprocess &> $problem/part1.output
# Identify the submissions on which findFeatures was successful
time bash scripts/geterrors.sh $problem last.c.$problemlower.txt last c &> $problem/stats.output
# Cluster submissions and get reference implementations for each cluster
bash -x cluster-scripts/getClusters.sh $problem &> $problem/clusters.output
