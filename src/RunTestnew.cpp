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
#include "util.h"
using namespace util;
int main(int argc, char *argv[]) {
//string file="foursquare";
	string file = "Gowalla";
//string file="BrightKi";
	string mem = exec("systeminfo | find \"Virtual Memory: In Use:\"");
	std::cout << mem << std::endl;
	int window = 100; //hour
	int bucket = 8;
	int k = 100;
	int minFreq = 10;

	bool isforward = true;

	if (argc > 0) {
		//std::cout << argv[0] << argv[1]	<<argv[2]	<< std::endl;
		window = stoi(argv[2]); //hours
		file = argv[1];
		k = 100;
		minFreq = 10;
	}
	std::cout << file << " @ " << argv[2] << std::endl;
	//string folder = "D:\\dataset\\Synthetic Data\\" + file + "\\";
	string folder = "D:\\dataset\\new\\" + file + "\\LBSNData\\";
	string ifile = folder + "checkins.csv";
	string ofile = folder + file;
	LocationInfluence li(bucket, ifile, ofile);
	// To test accuracy

	//li.FindInflunceExactUnitFreq(window, isforward, true);

	//li.FindInflunceApproxUnitFreq(window, isforward, true, false);
//****************************************//
	//li.topKwithoutFrequency(window, k);
	//li.FindInflunceApproxUnitFreqBackward(window, k, true);
	li.FindInflunceApprox(window, isforward);
	li.queryAll(minFreq);
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
	mem = exec("systeminfo | find \"Virtual Memory: In Use:\"");
	std::cout << mem << std::endl;
	return 0;
}
