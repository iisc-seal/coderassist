#include<iostream>
#include<fstream>
#include<cstdlib>
#include<map>
using namespace std;

int main(int argc, char** argv) {
    if (argc < 5) {
	cout << "ERROR: Invalid input\n";
	cout << "Input Format: sortedFeatures.csv, filesWithClusterNums.csv, "
	     << "clustersWithRefs.csv, inputFileTofixdp.txt";
	return -1;
    }

    string sortedFeatures(argv[1]);
    string clusterNumsFile(argv[2]);
    string clusterRefsFile(argv[3]);
    string fixdpInputFile(argv[4]);

    // count of submissions (all/ac/wa) in each cluster
    map<int, int> clusterCount, clusterCountAC, clusterCountWA;
    // ref for each cluster
    map<int, string> clusterRef;
    // map cluster to vector of file names 
    map<int, map<int, string> > clusterArray, clusterArrayAC, clusterArrayWA;

    fstream sortedFS, clusterFS, clusterRefFS, fixdpFS;
    sortedFS.open(sortedFeatures.c_str(), fstream::in);
    if (!sortedFS.is_open()) {
	cout << "ERROR: Cannot open file " << sortedFeatures << "\n";
	return -1;
    }
    clusterFS.open(clusterNumsFile.c_str(), fstream::out | fstream::trunc);
    if (!clusterFS.is_open()) {
	cout << "ERROR: Cannot open file " << clusterNumsFile << "\n";
	return -1;
    }
    clusterRefFS.open(clusterRefsFile.c_str(), fstream::out | fstream::trunc);
    if (!clusterRefFS.is_open()) {
	cout << "ERROR: Cannot open file " << clusterNumsFile << "\n";
	return -1;
    }
    fixdpFS.open(fixdpInputFile.c_str(), fstream::out | fstream::trunc);
    if (!fixdpFS.is_open()) {
	cout << "ERROR: Cannot open file " << clusterNumsFile << "\n";
	return -1;
    }

    string line;
    string prevline = "", prevFeatures = "";
    int clusterNum = 0, subNum = 0, subNumAC = 0, subNumWA = 0;
    while (getline(sortedFS, line)) {
	string currline = line;
	size_t firstcomma = currline.find_first_of(',');
	string currFeatures = currline.substr(firstcomma+1, string::npos);
	cout << "currline: " << currline << "\n";
	cout << "currFeatures: " << currFeatures << "\n";
	cout << "prevline: " << prevline << "\n";
	cout << "prevFeatures: " << prevFeatures << "\n";
	if (currFeatures.compare(prevFeatures) != 0) {
	    // New cluster starting
	    if (clusterNum != 0) {
		// Not the first cluster
		if (clusterCount.find(clusterNum) == clusterCount.end()) {
		    clusterCount[clusterNum] = subNum;
		} else {
		    cout << "ERROR: clusterCount has entry for " << clusterNum 
			 << "\n";
		    return -1;
		}
		if (clusterCountAC.find(clusterNum) == clusterCountAC.end()) {
		    clusterCountAC[clusterNum] = subNumAC;
		} else {
		    cout << "ERROR: clusterCountAC has entry for " << clusterNum 
			 << "\n";
		    return -1;
		}
		if (clusterCountWA.find(clusterNum) == clusterCountWA.end()) {
		    clusterCountWA[clusterNum] = subNumWA;
		} else {
		    cout << "ERROR: clusterCountWA has entry for " << clusterNum 
			 << "\n";
		    return -1;
		}

		// Get random representative
		int random;
		if (clusterCountAC[clusterNum] != 0) {
		    //srand(time(NULL));
		    random = rand() % clusterCountAC[clusterNum] + 1;
		    if (clusterRef.find(clusterNum) == clusterRef.end()) {
			clusterRef[clusterNum] = clusterArrayAC[clusterNum][random];
		    } else {
			cout << "ERROR: clusterRef has entry for " << clusterNum 
			     << "\n";
			return -1;
		    }
		} else if (clusterCountWA[clusterNum] != 0) {
		    random = rand() % clusterCountWA[clusterNum] + 1;
		    if (clusterRef.find(clusterNum) == clusterRef.end()) {
			clusterRef[clusterNum] = clusterArrayWA[clusterNum][random];
		    } else {
			cout << "ERROR: clusterRef has entry for " << clusterNum 
			     << "\n";
			return -1;
		    }
		} else {
		    cout << "ERROR: No submissions in cluster " << clusterNum 
			 << "\n";
		    return -1;
		}
	    }
	    clusterNum++;
	    subNum = 0;
	    subNumAC = 0;
	    subNumWA = 0;
	}
	prevline = currline;
	prevFeatures = currFeatures;
	subNum++;

	size_t posOfFirstComma = currline.find_first_of(',');
	string progID = "";
	if (posOfFirstComma != string::npos) {
	    for (size_t pos = 0; pos < posOfFirstComma; pos++)
		progID.push_back(currline[pos]);
	} else {
	    cout << "ERROR: Cannot find progID in line " << currline << "\n";
	    return -1;
	}

	clusterFS << progID << "," << clusterNum << "\n";
	if (clusterArray.find(clusterNum) == clusterArray.end()) {
	    map<int, string> subs;
	    subs[subNum] = progID;
	    clusterArray[clusterNum] = subs;
	} else {
	    if (clusterArray[clusterNum].find(subNum) ==
		    clusterArray[clusterNum].end()) {
		clusterArray[clusterNum][subNum] = progID;
	    } else {
		cout << "ERROR: clusterArray[" << clusterNum 
		     << "] has entry for sub " << subNum << "\n";
		return -1;
	    }
	}

	if (progID.find(".ac.c") != string::npos) {
	    // ac program
	    subNumAC++;
	    if (clusterArrayAC.find(clusterNum) == clusterArrayAC.end()) {
		map<int, string> subs;
		subs[subNumAC] = progID;
		clusterArrayAC[clusterNum] = subs;
	    } else {
		if (clusterArrayAC[clusterNum].find(subNumAC) == 
			clusterArrayAC[clusterNum].end()) {
		    clusterArrayAC[clusterNum][subNumAC] = progID;
		} else {
		    cout << "ERROR: clusterArrayAC[" << clusterNum
			 << "] has entry for sub " << subNum << "\n";
		    return -1;
		}
	    }
	} else {
	    // wa program
	    subNumWA++;
	    if (clusterArrayWA.find(clusterNum) == clusterArrayWA.end()) {
		map<int, string> subs;
		subs[subNumWA] = progID;
		clusterArrayWA[clusterNum] = subs;
	    } else {
		if (clusterArrayWA[clusterNum].find(subNumWA) == 
			clusterArrayWA[clusterNum].end()) {
		    clusterArrayWA[clusterNum][subNumWA] = progID;
		} else {
		    cout << "ERROR: clusterArrayWA[" << clusterNum
			 << "] has entry for sub " << subNum << "\n";
		    return -1;
		}
	    }
	}
    }

    // Processing for last cluster.
    if (clusterCount.find(clusterNum) == clusterCount.end()) {
	clusterCount[clusterNum] = subNum;
    } else {
	cout << "ERROR: clusterCount has entry for " << clusterNum 
	     << "\n";
	return -1;
    }
    if (clusterCountAC.find(clusterNum) == clusterCountAC.end()) {
	clusterCountAC[clusterNum] = subNumAC;
    } else {
	cout << "ERROR: clusterCountAC has entry for " << clusterNum 
	     << "\n";
	return -1;
    }
    if (clusterCountWA.find(clusterNum) == clusterCountWA.end()) {
	clusterCountWA[clusterNum] = subNumWA;
    } else {
	cout << "ERROR: clusterCountWA has entry for " << clusterNum 
	     << "\n";
	return -1;
    }

    // Get random representative
    int random;
    if (clusterCountAC[clusterNum] != 0) {
	random = rand() % clusterCountAC[clusterNum] + 1;
	if (clusterRef.find(clusterNum) == clusterRef.end()) {
	    clusterRef[clusterNum] = clusterArrayAC[clusterNum][random];
	} else {
	    cout << "ERROR: clusterRef has entry for " << clusterNum 
		 << "\n";
	    return -1;
	}
    } else if (clusterCountWA[clusterNum] != 0) {
	random = rand() % clusterCountWA[clusterNum] + 1;
	if (clusterRef.find(clusterNum) == clusterRef.end()) {
	    clusterRef[clusterNum] = clusterArrayWA[clusterNum][random];
	} else {
	    cout << "ERROR: clusterRef has entry for " << clusterNum 
		 << "\n";
	    return -1;
	}
    } else {
	cout << "ERROR: No submissions in cluster " << clusterNum 
	     << "\n";
	return -1;
    }
    cout << "clusterCount:\n";
    for (std::map<int, int>::iterator cIt = clusterCount.begin(); cIt != 
	    clusterCount.end(); cIt++)
	cout << cIt->first << " -> " << cIt->second << "\n";
    cout << "clusterCountAC:\n";
    for (std::map<int, int>::iterator cIt = clusterCountAC.begin(); cIt != 
	    clusterCountAC.end(); cIt++)
	cout << cIt->first << " -> " << cIt->second << "\n";
    cout << "clusterCountWA:\n";
    for (std::map<int, int>::iterator cIt = clusterCountWA.begin(); cIt != 
	    clusterCountWA.end(); cIt++)
	cout << cIt->first << " -> " << cIt->second << "\n";
    cout << "clusterRef:\n";
    for (std::map<int, string>::iterator cIt = clusterRef.begin(); cIt != 
	    clusterRef.end(); cIt++) {
	cout << cIt->first << " -> " << cIt->second << "\n";
    }

    // write sub to ref map
    for (std::map<int, map<int, string> >::iterator cIt = clusterArray.begin();
	    cIt != clusterArray.end(); cIt++) {
	int clusterNum = cIt->first;
	if (clusterRef.find(clusterNum) == clusterRef.end()) {
	    cout << "ERROR: Cannot find ref for cluster " << clusterNum << "\n";
	    return -1;
	}
	string ref = clusterRef[clusterNum];
	clusterRefFS << clusterNum << "," << ref << "\n";

	for (std::map<int, string>::iterator sIt = cIt->second.begin(); sIt != 
		cIt->second.end(); sIt++) {
	    string sub = sIt->second;
	    fixdpFS << sub << "," << ref << "\n";
	}
    }

    cout << "OUTPUT:\n";
    for (std::map<int, int>::iterator cIt = clusterCount.begin(); cIt != 
	    clusterCount.end(); cIt++) {
	cout << "Cluster " << cIt->first << ": " << cIt->second;
	if (clusterCountAC.find(cIt->first) == clusterCountAC.end()) {
	    cout << "ERROR: Cannot find entry for cluster " << cIt->first 
		 << " in clusterCountAC\n";
	    return -1;
	} else {
	    cout << " ac: " << clusterCountAC[cIt->first];
	}
	if (clusterCountWA.find(cIt->first) == clusterCountWA.end()) {
	    cout << "ERROR: Cannot find entry for cluster " << cIt->first
		 << " in clusterCountWA\n";
	    return -1;
	} else {
	    cout << " wa: " << clusterCountWA[cIt->first];
	}
	if (clusterRef.find(cIt->first) == clusterRef.end()) {
	    cout << "ERROR: Cannot find entry for cluster " << cIt->first
		 << " in clusterRef\n";
	    return -1;
	} else {
	    cout << " ref: " << clusterRef[cIt->first];
	}
	cout << "\n";
    }

    sortedFS.close();
    clusterFS.close();
    clusterRefFS.close();
    fixdpFS.close();

    return 0;
}
