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
//	CountDistinctSketch cds(0.01, 0.99, 7);
//	vector<set<string> > exact;
//	const char *ar_str[] = { "hello", "some", "one", "hello", "alice", "one",
//			"lady", "let", "us", "lady", "alice", "in", "wonderland", "us",
//			"lady", "lady", "some", "hello", "none", "pie" };
//	string temp = "a";

//	for (int i = 0; i < 100; i++) {
////		set<string> s;
//		for (int j = 0; j < 10; j++) {

//			temp = ar_str[rand() % 20];

//			s.insert(temp);
//			cds.update(i, temp);
//		}
//		exact.push_back(s);
//	}
//	for (int i = 0; i < 100; i++) {
//		std::cout << exact[i].size() << " " << cds.estimate(i) << std::endl;
//	}

	string folder = "D:\\dataset\\Gowalla\\Modified\\";
	string ifile = folder + "Gowalla_totalCheckins.csv";
	string ofile = folder + "Gowalla";

	int bucket = 8;
	int window = 5;
	int k = 100;
	int minFreq = 100;
	LocationInfluence li(bucket, ifile, ofile);
	bool isforward = true;
	//li.topKwithoutFrequency(window, k);
	li.ParseApprox(window, isforward);
	//li.ParseExact(window, isforward);
	std::this_thread::sleep_for(std::chrono::seconds(100));
	li.topK(minFreq, k);
	/*
	 li.ParseApprox(window, isforward);
	 li.clean();
	 std::this_thread::sleep_for(std::chrono::seconds(100));
	 li.queryAll();
	 */
	return 0;
}
