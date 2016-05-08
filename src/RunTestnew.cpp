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
#include <map>
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
	bool monitor = false;
	bool weigthed = false;
	string cmd = "";
	string forward = "";
	string seedfile = "";
	if (argc > 0) {

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
			} else if (extra[1] == 'm') {
				if (extra.compare("-m=true") == 0) {
					monitor = true;
				} else if (extra.compare("-f=false") == 0) {
					monitor = false;
				}
			} else if (extra[1] == 'w') {
				if (extra.compare("-w=true") == 0) {
					weigthed = true;
				} else if (extra.compare("-f=false") == 0) {
					weigthed = false;
				}
			} else if (extra[1] == 'c') {
				if (extra.compare("-c=1") == 0) {
					cmd = "influnceset";
				} else if (extra.compare("-c=2") == 0) {
					cmd = "seed";
				} else if (extra.compare("-c=3") == 0) {
					cmd = "unitCompare";
				} else if (extra.compare("-c=4") == 0) {
					cmd = "accuracy";
				} else if (extra.compare("-c=5") == 0) {
					cmd = "spread";
				} else if (extra.compare("-c=6") == 0) {
					cmd = "naive";
				}
			} else if (extra[1] == 'p') {
				seedfile = extra.substr(3, extra.length());
			} else if (extra[1] == 'k') {
				k = stoi(extra.substr(3, extra.length()));
			}

		}

	}

	//string folder = "D:\\dataset\\Synthetic Data\\" + file + "\\";
	string folder = "D:\\dataset\\new\\" + file + "\\LBSNData\\";
	string ifile = folder + "checkins.csv";
	string ofile = folder + file;
	string friendfile = folder + "friends.txt";
	LocationInfluence li(bucket, ifile, ofile, folder);
	if (cmd.compare("naive") == 0) {

		li.findtopKusingNaive(ofile, k);
	}
	// To test accuracy
	if (cmd.compare("accuracy") == 0) {
		li.FindInflunceApproxUnitFreq(window, k, false, true);
		li.FindInflunceExactUnitFreq(window, isforward, true);
	}
	if (cmd.compare("unitCompare") == 0) {
		li.FindInflunceApproxUnitFreq(window, k, true, false);
		li.FindInflunceApproxUnitFreqBackward(window, true);
	}

	if (cmd.compare("influnceset") == 0) {
		std::cout << "finding influnce set " << file << " window: " << window
				<< " frequency: " << minFreq << " weighted:" << weigthed
				<< " with friend:" << withFriend << std::endl;
		if (weigthed) {
			if (withFriend) {
				li.generateFriendshipData(friendfile);
			}
			li.FindInflunceWeigthed(window, isforward, withFriend, monitor);

			li.queryWeighted(minFreq);
		} else {
			li.FindInflunceApprox(window, isforward, monitor);
			li.queryAll(minFreq);
		}
	}
	if (cmd.compare("seed") == 0) {
		std::cout << "finding " << k << " seed for " << file << " window: "
				<< window << " frequency: " << minFreq << " weighted:"
				<< weigthed << " with friend:" << withFriend << std::endl;
		if (weigthed) {
			if (withFriend) {
				li.generateFriendshipData(friendfile);
			}
			li.FindInflunceWeigthed(window, isforward, withFriend, monitor);

			li.findWeigthedSeed("", minFreq, k);
		} else {
			if (k > 1) {
				li.FindInflunceApprox(window, isforward, monitor);
				li.findseed(minFreq, k);
			} else {
				li.FindInflunceApproxUnitFreq(window, k, false, true);
			}
		}
	}
	if (cmd.compare("spread") == 0) {
		std::cout << "finding spread of " << k << " seed for " << seedfile
				<< " window: " << window << " frequency: " << minFreq
				<< std::endl;
		li.FindInflunceExact(window, true);
		li.queryExact(minFreq);
		li.queryInflunceSet(folder + "\\" + seedfile + ".keys", k,
				folder + "\\" + seedfile + ".spread");
	}
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
