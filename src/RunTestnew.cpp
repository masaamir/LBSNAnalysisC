/*
 * RunTest.cpp
 *
 *  Created on: Jan 1, 2016
 *      Author: Rohit
 */
#include <iostream>
#include <vector>
#include <string>
#include "hyperloglog.hpp"
#include "CountDistinctSketch.hpp"
# include <map>
# include <cstdlib>
# include <cmath>
#include "LocationInflunce.hpp"
#include <set>
#include <chrono>
#include <thread>
using namespace std;
using namespace cds;
using namespace hll;
using namespace lif;

int main() {
//string file="foursquare";
	string file = "Gowalla";
//string file="BrightKi";
	string folder = "D:\\dataset\\new\\" + file + "\\LBSNData\\";
	string ifile = folder + "checkins.csv";
	string ofile = folder + file;

	int window = 100; //hours
	int bucket = 8;
	int k = 100;
	int minFreq = 10;
	LocationInfluence li(bucket, ifile, ofile);
	bool isforward = true;

	// To test accuracy

//	li.FindInflunceExactUnitFreq(window, isforward, true);
	//li.FindInflunceApproxUnitFreq(window, isforward, false, false);
//****************************************//
	//li.topKwithoutFrequency(window, k);
	li.FindInflunceApproxUnitFreqBackward(window,k,true);
	//li.FindInflunceApprox(window, isforward);
	//li.FindInflunceWeigthed(window,isforward);
	//li.FindInflunceExact(window, isforward);
	//std::this_thread::sleep_for(std::chrono::seconds(100));
	//li.topK(minFreq, k);
	/*
	 li.FindInflunceApprox(window, isforward);
	 li.clean();
	 std::this_thread::sleep_for(std::chrono::seconds(100));
	 li.queryAll();
	 */
	return 0;
}

