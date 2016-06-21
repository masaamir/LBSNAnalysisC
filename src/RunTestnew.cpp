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
	string file = "foursquare";
//	string file = "Gowalla";
	string pathSeperator = "\\";
//string file="BrightKi";
	int window = 100; //hour
	int bucket = 8;
	int numberOfSeed = 100;

	double tau;
	bool withFriend = false;
	bool isforward = true;
	bool monitor = false;
	bool weigthed = true;
	bool isRelative = false;
	double alpha = 0.9;
	double alphaArray[] = { 0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1 };
	int seedArray[] = { 10, 20, 30, 40, 50 };
	string cmd = "seed";
	string forward = "";
	string seedfile = "";
	if (argc > 0) {

		window = stoi(argv[2]); //hours
		file = argv[1];
		numberOfSeed = 100;

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
				} else if (extra.compare("-m=false") == 0) {
					monitor = false;
				}
			} else if (extra[1] == 'w') {
				if (extra.compare("-w=true") == 0) {
					weigthed = true;
				} else if (extra.compare("-f=false") == 0) {
					weigthed = false;
				}
			} else if (extra[1] == 'r') {
				if (extra.compare("-r=true") == 0) {
					isRelative = true;
				} else if (extra.compare("-f=false") == 0) {
					isRelative = false;
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
				} else if (extra.compare("-c=7") == 0) {
					cmd = "backward";
				} else if (extra.compare("-c=8") == 0) {
					cmd = "exact";
				} else if (extra.compare("-c=9") == 0) {
					cmd = "memoryExact";
				} else if (extra.compare("-c=10") == 0) {
					cmd = "memoryApprox";
				} else if (extra.compare("-c=11") == 0) {
					cmd = "spreadWeighted";
				}
			} else if (extra[1] == 'p') {
				seedfile = extra.substr(3, extra.length());
			} else if (extra[1] == 'k') {
				numberOfSeed = atoi(extra.substr(3, extra.length()).c_str());
			} else if (extra[1] == 't') {
				tau = atof(extra.substr(3, extra.length()).c_str());
			} else if (extra[1] == 'a') {
				alpha = atof(extra.substr(3, extra.length()).c_str());
			}

		}

	}
	//cout << cmd << endl;
//	cout << runCommand("systeminfo | find \"Virtual Memory: In Use:\"") << endl;
//	string folder = "D:\\dataset\\new\\";
	string folder = "D:\\dataset\\new\\" + file + "\\LBSNData\\";
//	string folder = "D:\\dataset\\new\\NYC\\" + file + "\\";
//	string folder = "/home/aamir/Study/rohit/new/" + file + "/LBSNData/";
	//string folder = "/home/aamir/Study/rohit/new/synthetic/" + file + "/";
	string ifile = folder + "checkins.csv";
	string ofile = folder + file;
	string friendfile = folder + "friends.txt";
	LocationInfluence li(bucket, ifile, ofile, folder);
	if (cmd.compare("naive") == 0) {

		li.findtopKusingNaive(ofile, numberOfSeed);
	} else if (cmd.compare("backward") == 0) {

		li.FindInflunceApproxUnitFreqBackwardNew(window, true);
	} else if (cmd.compare("exact") == 0) {
		if (withFriend) {
			li.generateExactFriendshipData(friendfile);
		}
		li.FindInflunceExact(window, withFriend);
	} else if (cmd.compare("accuracy") == 0) {
		//	li.FindInflunceApproxUnitFreq(window, numberOfSeed, false, true);
		//li.FindInflunceExactUnitFreq(window, isforward, true);
		cout << "accuracy for " << file << " @ " << window << " with tau= "
				<< tau << " and withFriend=" << withFriend << " and isRelative="
				<< isRelative << endl;
		if (withFriend) {
			li.generateExactFriendshipData(friendfile);
			li.generateFriendshipData(friendfile);
		}
		li.FindInflunceExact(window, withFriend);
		li.queryAllExact(tau, isRelative);

		li.FindInflunceWeigthed(window, withFriend, false);
		li.queryWeighted(tau, isRelative);

	} else if (cmd.compare("memoryExact") == 0) {
		//	li.FindInflunceApproxUnitFreq(window, numberOfSeed, false, true);
		//li.FindInflunceExactUnitFreq(window, isforward, true);
		cout << "Memory Exact for " << file << " @ " << window
				<< " and withFriend=" << withFriend << endl;

		if (withFriend) {
			li.generateExactFriendshipData(friendfile);

		}
		li.FindInflunceExact(window, withFriend);
		cout << "memory used : " << getValue() / 1024 << endl;
	} else if (cmd.compare("memoryApprox") == 0) {
		//	li.FindInflunceApproxUnitFreq(window, numberOfSeed, false, true);
		//li.FindInflunceExactUnitFreq(window, isforward, true);
		cout << "Memory Approx for " << file << " @ " << window
				<< " and withFriend=" << withFriend << endl;
		if (withFriend) {

			li.generateFriendshipData(friendfile);
		}

		li.FindInflunceWeigthed(window, withFriend, false);
		cout << "memory used : " << getValue() / 1024 << endl;
	} else if (cmd.compare("unitCompare") == 0) {
		li.FindInflunceApproxUnitFreq(window, numberOfSeed, false, false);

	} else if (cmd.compare("influnceset") == 0) {

		std::cout << "finding influnce set " << file << " window: " << window
				<< " frequency: " << tau << " weighted:" << weigthed
				<< " with friend:" << withFriend << std::endl;
		if (weigthed) {
			if (withFriend) {
				li.generateFriendshipData(friendfile);
			}
			li.FindInflunceWeigthed(window, withFriend, monitor);

			li.queryWeighted(tau, isRelative);
		} else {
			li.FindInflunceApprox(window, isforward, monitor);
			li.queryAll(tau, false);
		}
	} else if (cmd.compare("seed") == 0) {
		std::cout << " resultFor: " << numberOfSeed << " seed for " << file
				<< " window: " << window << " frequency: " << tau
				<< " weighted:" << weigthed << " with friend:" << withFriend
				<< std::endl;
		int spread;
		if (weigthed) {

			if (withFriend) {
				li.generateFriendshipData(friendfile);
			}
			li.FindInflunceWeigthed(window, withFriend, monitor);
			for (double a : alphaArray) {
				alpha = a;
				string keyfile = "";
				if (withFriend) {
					keyfile = ofile + "WeightedFriend_w" + to_string(window)
							+ "_f" + to_string(tau);
					li.findWeigthedSeed(keyfile, tau, numberOfSeed, alpha,
							isRelative);
				} else {
					keyfile = ofile + "Weighted_w" + to_string(window) + "_f"
							+ to_string(tau);
					li.findWeigthedSeed(keyfile, tau, numberOfSeed, alpha,
							isRelative);
				}

				for (int seedc : seedArray) {
					stringstream seedFile;
					seedFile << keyfile << "_s" << numberOfSeed << "_a" << alpha
							<< ".keys";
				//	cout<<seedFile.str()<<endl;
					spread = li.getWeightedSpread(tau, isRelative,
							seedFile.str(), seedc);
					cout << "resultFor: expected spread " << spread
							<< " for alpha: " << alpha << " seed count "
							<< seedc << endl;
				}
			}

		} else {
			if (tau > 1) {
				li.FindInflunceApprox(window, isforward, monitor);
				li.findseed(tau, numberOfSeed);
			} else {
				li.FindInflunceApproxUnitFreq(window, numberOfSeed, false,
						true);
			}
		}
	} else if (cmd.compare("spread") == 0) {
		std::cout << "finding spread of " << numberOfSeed << " seed for "
				<< seedfile << " window: " << window << " frequency: " << tau
				<< std::endl;
		li.FindInflunceExact(window, withFriend);
		li.queryExact(tau, false);
		li.queryInflunceSet(folder + pathSeperator + seedfile + ".keys",
				numberOfSeed, folder + pathSeperator + seedfile + "_f5.spread");
	} else if (cmd.compare("spreadWeighted") == 0) {
		std::cout << "finding spread of " << numberOfSeed << " seed for "
				<< seedfile << " window: " << window << " frequency: " << tau
				<< std::endl;
		li.FindInflunceExact(window, withFriend);
		li.queryExact(tau, false);
		li.queryInflunceSet(folder + pathSeperator + seedfile + ".keys",
				numberOfSeed, folder + pathSeperator + seedfile + "_f5.spread");
	} else {
		std::cout << "no command found" << std::endl;
	}
//	cout << runCommand("systeminfo | find \"Virtual Memory: In Use:\"") << endl;
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
