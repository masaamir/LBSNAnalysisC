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


	int window = 100; //hour
	int bucket = 8;
	int k = 100;
	int minFreq = 1;
	bool withFriend = false;
	bool isforward = true;
	string forward = "";
	if (argc > 0) {
		//std::cout << argv[0] << argv[1]	<<argv[2]	<< std::endl;
		window = stoi(argv[2]); //hours
		file = argv[1];
		k = 100;
		minFreq = 1;
		string extra;
		std::cout << file << " @ " << argv[2] << std::endl;
		for (int i = 3; i < argc; i++) {
			extra = argv[i];
			if (extra[1] == 'd') {
				if (extra.compare("-d=true") == 0) {
					isforward = true;
				} else if (extra.compare("-d=false") == 0) {
					isforward = false;
				}
			} else if (extra[1] == 'f') {
				if (extra.compare("-f=true") == 0) {
					withFriend = true;
				} else if (extra.compare("-f=false") == 0) {
					withFriend = false;
				}
			}

		}

	}

	//string folder = "D:\\dataset\\Synthetic Data\\" + file + "\\";
	string folder = "D:\\dataset\\new\\" + file + "\\LBSNData\\";
	string ifile = folder + "checkins.csv";
	string ofile = folder + file;
	string friendfile = folder + "friends.txt";
	LocationInfluence li(bucket, ifile, ofile);

	// To test accuracy

	//li.FindInflunceExactUnitFreq(window, isforward, true);

	//li.FindInflunceApproxUnitFreq(window, isforward, true, false);
//****************************************//
	//li.topKwithoutFrequency(window, k);
	//li.FindInflunceApproxUnitFreqBackward(window, k, true);
	if (withFriend) {
		li.generateFriendshipData(friendfile);
	}
	li.FindInflunceApprox(window, isforward);
	li.queryAll(minFreq);
//	li.FindInflunceWeigthed(window, isforward, withFriend);

//	li.queryWeighted(minFreq);

	//li.FindInflunceExact(window, isforward);
	//std::this_thread::sleep_for(std::chrono::seconds(100));
	//li.topK(minFreq, k);
	/*
	 li.FindInflunceApprox(window, isforward);
	 li.clean();
	 std::this_thread::sleep_for(std::chrono::seconds(100));
	 li.queryAll();
	 */
	//mem = exec("systeminfo | find \"Virtual Memory: In Use:\"");
	//std::cout << mem << std::endl;
	return 0;
}
