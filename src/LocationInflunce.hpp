/*
 * LocationInflunce.hpp
 *
 *  Created on: Mar 14, 2016
 *      Author: Rohit
 */

#ifndef LOCATIONINFLUNCE_HPP_
#define LOCATIONINFLUNCE_HPP_
#include <iostream>
#include <map>
#include <fstream>
#include <sstream>
#include "Timer.h"
#include "hyperloglog.hpp"
#include "Split.h"
#include <set>
using namespace hll;
using namespace std;
namespace lif {

struct edge {
	string user;
	int location;
	long checkin;
};
struct node {
	int nodeid;
	HyperLogLog nodeset;
	double estimate;
};

struct locationnode {
	int locationid;
	HyperLogLog visitor;
	map<int, HyperLogLog> influenceset;
};
bool sortByEstimate(const node &lhs, const node &rhs) {
	return lhs.estimate > rhs.estimate;
}
bool sortByInfluenceSet(const locationnode &lhs, const locationnode &rhs) {
	return lhs.influenceset.size() > rhs.influenceset.size();
}
inline bool operator<(const edge& lhs, const edge& rhs) {
	return lhs.checkin > rhs.checkin;
}

inline bool forward(const edge& lhs, const edge& rhs) {
	return rhs.checkin > lhs.checkin;
}
class LocationInfluence {
public:
	map<int, map<int, HyperLogLog>> locationSummary;
	map<int, locationnode> weigthedLocationSummary;
	map<int, map<int, set<string>>> exactLocationSummary;
	map<string, map<int, long> > userSummary;
	map<int, HyperLogLog> compactLocationSummary;
	uint8_t numberofbuckets;
	string datafile;
	string outfile;
	long window = 0;
	LocationInfluence(uint8_t b = 7, string file = "", string ofile = "")
	throw (invalid_argument) {

		numberofbuckets = b;
		datafile = file;
		outfile = ofile;
	}
	void topKwithoutFrequency(int wp, int k) {
		long window = wp * 24 * 60 * 60;

		vector<string> temp;
		vector<edge> data;
		ifstream infile(datafile.c_str());
		edge tempEdge;

		std::cout << datafile << std::endl;
		string line;
		Platform::Timer timer;
		timer.Start();
		while (infile >> line) {

			temp = Tools::Split(line, ',');

			tempEdge.user = temp[0];
			tempEdge.location = atoi(temp[1].c_str());
			tempEdge.checkin = stol(temp[2].c_str());
			data.push_back(tempEdge);

		}
		sort(data.begin(), data.end());
		int datasize = data.size();
		std::cout << "read and sorted data " << datasize << " in "
		<< timer.LiveElapsedMilliseconds() << std::endl;
		timer.Start();
		int locationid;
		long checkintime;
		string userid;
		long diff;
		int srcLoc;
		string destLoc;
		map<int, long> newuser;

		typedef std::map<int, long>::iterator it_type;
		for (int i = 0; i < datasize - 1; i++) {
			userid = data[i].user;
			locationid = data[i].location;
			checkintime = data[i].checkin;

			//	locationSummary[locationid];

			newuser.clear();

			if (userSummary.find(userid) != userSummary.end()) {

				for (it_type it = userSummary[userid].begin();
						it != userSummary[userid].end(); it++) {
					diff = it->second - checkintime;
					srcLoc = locationid;
					destLoc = to_string(it->first);

					if ((diff) < window) {
						newuser[it->first] = it->second;
						if (compactLocationSummary.find(srcLoc)
								== compactLocationSummary.end()) {
							HyperLogLog hll(numberofbuckets);

							compactLocationSummary[srcLoc] = hll;
						}

						compactLocationSummary[srcLoc].add(destLoc.c_str(),
								destLoc.size());

					}
				}
			}
			newuser[locationid] = checkintime;

			userSummary[userid] = newuser;

			if (i % 100000 == 0) {
				std::cout << i << " " << userSummary.size() << std::endl;
				cleanup(checkintime, window, false);
			}
		} //end of data for loop
		std::cout << "finished parsing " << timer.LiveElapsedSeconds()
		<< std::endl;

		//finding top k
		findseed(outfile, k);
	}
	void FindInflunceWeigthed(int wp,bool isforward) {

		window = 0;

		window = wp * 24 * 60 * 60;

		vector<string> temp;
		vector<edge> data;
		ifstream infile(datafile.c_str());
		edge tempEdge;

		std::cout << datafile << std::endl;
		string line;
		Platform::Timer timer;
		timer.Start();
		while (infile >> line) {

			temp = Tools::Split(line, ',');

			tempEdge.user = temp[0];
			tempEdge.location = atoi(temp[1].c_str());
			tempEdge.checkin = stol(temp[2].c_str());
			data.push_back(tempEdge);

		}
		if (isforward) {
			sort(data.begin(), data.end(), forward);
		} else {
			sort(data.begin(), data.end());
		}
		int datasize = data.size();
		std::cout << "read and sorted data " << datasize << " in "
		<< timer.LiveElapsedMilliseconds() << std::endl;
		std::map<int, HyperLogLog>::iterator ithll;

		timer.Start();
		int locationid;
		long checkintime;
		string userid;
		long diff;
		int srcLoc, destLoc;
		map<int, long> newuser;
		map<int, HyperLogLog> newlochll;
		typedef std::map<int, long>::iterator it_type;
		for (int i = 0; i < datasize - 1; i++) {
			userid = data[i].user;
			locationid = data[i].location;
			checkintime = data[i].checkin;

			//	locationSummary[locationid];

			newuser.clear();

			if (userSummary.find(userid) != userSummary.end()) {

				for (it_type it = userSummary[userid].begin();
						it != userSummary[userid].end(); it++) {
					if (isforward) {
						diff = checkintime - it->second;
						destLoc = locationid;
						srcLoc = it->first;
					} else {
						srcLoc = locationid;
						destLoc = it->first;
						diff = it->second - checkintime;
					}
					if ((diff) < window) {
						newuser[it->first] = it->second;

						if (weigthedLocationSummary.find(srcLoc)
								== weigthedLocationSummary.end()) {
							newlochll.clear();
							locationnode newlocation;
							newlocation.influenceset=newlochll;
							newlocation.locationid=srcLoc;
							HyperLogLog nhll(numberofbuckets);
							newlocation.visitor=nhll ;

							weigthedLocationSummary[srcLoc] = newlocation;
						}
						ithll = weigthedLocationSummary[srcLoc].influenceset.find(destLoc);
						if (ithll == weigthedLocationSummary[srcLoc].influenceset.end()) {
							HyperLogLog hll(numberofbuckets);
							hll.add(userid.c_str(), userid.size());
							weigthedLocationSummary[srcLoc].influenceset[destLoc] = hll;
						} else {
							weigthedLocationSummary[srcLoc].influenceset[destLoc].add(userid.c_str(),
									userid.size());
						}

						weigthedLocationSummary[srcLoc].visitor.add(userid.c_str(),userid.length());

					}
				} //end of for loop

			}

			newuser[locationid] = checkintime;

			userSummary[userid] = newuser;

			if (i % 100000 == 0) {
				std::cout << i << " " << userSummary.size() << std::endl;
				cleanup(checkintime, window, isforward);
			}
		} //end of data for loop
		std::cout << "finished parsing " << timer.LiveElapsedSeconds()
		<< std::endl;

	}
	void FindInflunceApprox(int wp, bool isforward) {
		window = 0;

		window = wp * 24 * 60 * 60;

		vector<string> temp;
		vector<edge> data;
		ifstream infile(datafile.c_str());
		edge tempEdge;

		std::cout << datafile << std::endl;
		string line;
		Platform::Timer timer;
		timer.Start();
		while (infile >> line) {

			temp = Tools::Split(line, ',');

			tempEdge.user = temp[0];
			tempEdge.location = atoi(temp[1].c_str());
			tempEdge.checkin = stol(temp[2].c_str());
			data.push_back(tempEdge);

		}
		if (isforward) {
			sort(data.begin(), data.end(), forward);
		} else {
			sort(data.begin(), data.end());
		}
		int datasize = data.size();
		std::cout << "read and sorted data " << datasize << " in "
		<< timer.LiveElapsedMilliseconds() << std::endl;
		std::map<int, HyperLogLog>::iterator ithll;

		timer.Start();
		int locationid;
		long checkintime;
		string userid;
		long diff;
		int srcLoc, destLoc;
		map<int, long> newuser;
		map<int, HyperLogLog> newlochll;
		typedef std::map<int, long>::iterator it_type;
		for (int i = 0; i < datasize - 1; i++) {
			userid = data[i].user;
			locationid = data[i].location;
			checkintime = data[i].checkin;

			//	locationSummary[locationid];

			newuser.clear();

			if (userSummary.find(userid) != userSummary.end()) {

				for (it_type it = userSummary[userid].begin();
						it != userSummary[userid].end(); it++) {
					if (isforward) {
						diff = checkintime - it->second;
						destLoc = locationid;
						srcLoc = it->first;
					} else {
						srcLoc = locationid;
						destLoc = it->first;
						diff = it->second - checkintime;
					}
					if ((diff) < window) {
						newuser[it->first] = it->second;
						if (locationSummary.find(srcLoc)
								== locationSummary.end()) {
							newlochll.clear();

							locationSummary[srcLoc] = newlochll;
						}
						ithll = locationSummary[srcLoc].find(destLoc);
						if (ithll == locationSummary[srcLoc].end()) {
							HyperLogLog hll(numberofbuckets);
							hll.add(userid.c_str(), userid.size());
							locationSummary[srcLoc][destLoc] = hll;
						} else {
							locationSummary[srcLoc][destLoc].add(userid.c_str(),
									userid.size());
						}

					}
				} //end of for loop

			}

			newuser[locationid] = checkintime;

			userSummary[userid] = newuser;

			if (i % 100000 == 0) {
				std::cout << i << " " << userSummary.size() << std::endl;
				cleanup(checkintime, window, isforward);
			}
		} //end of data for loop
		std::cout << "finished parsing " << timer.LiveElapsedSeconds()
		<< std::endl;
	}
	void FindInflunceExact(int wp, bool isforward) {
		window = 0;

		window = wp * 24 * 60 * 60;

		vector<string> temp;
		vector<edge> data;
		ifstream infile(datafile.c_str());
		edge tempEdge;

		std::cout << datafile << std::endl;
		string line;
		Platform::Timer timer;
		timer.Start();
		while (infile >> line) {

			temp = Tools::Split(line, ',');

			tempEdge.user = temp[0];
			tempEdge.location = atoi(temp[1].c_str());
			tempEdge.checkin = stol(temp[2].c_str());
			data.push_back(tempEdge);

		}
		if (isforward) {
			sort(data.begin(), data.end(), forward);
		} else {
			sort(data.begin(), data.end());
		}
		int datasize = data.size();
		std::cout << "read and sorted data " << datasize << " in "
		<< timer.LiveElapsedMilliseconds() << std::endl;
		std::map<int, set<string>>::iterator ithll;

		timer.Start();
		int locationid;
		long checkintime;
		string userid;
		long diff;
		int srcLoc, destLoc;
		map<int, long> newuser;
		map<int, set<string>> newlocset;
		typedef std::map<int, long>::iterator it_type;
		for (int i = 0; i < datasize - 1; i++) {
			userid = data[i].user;
			locationid = data[i].location;
			checkintime = data[i].checkin;

			//	locationSummary[locationid];

			newuser.clear();

			if (userSummary.find(userid) != userSummary.end()) {

				for (it_type it = userSummary[userid].begin();
						it != userSummary[userid].end(); it++) {
					if (isforward) {
						diff = checkintime - it->second;
						destLoc = locationid;
						srcLoc = it->first;
					} else {
						srcLoc = locationid;
						destLoc = it->first;
						diff = it->second - checkintime;
					}
					if ((diff) < window) {
						newuser[it->first] = it->second;
						if (exactLocationSummary.find(srcLoc)
								== exactLocationSummary.end()) {
							newlocset.clear();

							exactLocationSummary[srcLoc] = newlocset;
						}
						ithll = exactLocationSummary[srcLoc].find(destLoc);
						if (ithll == exactLocationSummary[srcLoc].end()) {
							set<string> result;
							result.insert(userid);
							exactLocationSummary[srcLoc][destLoc] = result;
						} else {
							exactLocationSummary[srcLoc][destLoc].insert(userid);
						}

					}
				} //end of for loop

			}

			newuser[locationid] = checkintime;

			userSummary[userid] = newuser;

			if (i % 100000 == 0) {
				std::cout << i << " " << userSummary.size() << std::endl;
				cleanup(checkintime, window, isforward);
			}
		} //end of data for loop
		std::cout << "finished parsing " << timer.LiveElapsedSeconds()
		<< std::endl;
	}
	void queryAll() {
		Platform::Timer timer;
		timer.Start();
		ostringstream convert;

		outfile = outfile + ".result";
		ofstream rfile;
		rfile.open(outfile.c_str());
		typedef std::map<int, map<int, HyperLogLog>>::iterator locationit;

		for (locationit iterator = locationSummary.begin();
				iterator != locationSummary.end(); iterator++) {
			rfile << iterator->first << "," << iterator->second.size() << "\n";

		}
		rfile.close();
		std::cout << "finished querying " << timer.LiveElapsedMilliseconds()
		<< std::endl;
	}

	void queryAll(int freq) {
		Platform::Timer timer;
		timer.Start();
		ofstream rfile;
		outfile = outfile + "_" + to_string(freq) + ".result";
		rfile.open(outfile.c_str());
		typedef std::map<int, map<int, HyperLogLog>>::iterator locationit;
		typedef std::map<int, HyperLogLog>::iterator resit;
		int count = 0;
		for (locationit iterator = locationSummary.begin();
				iterator != locationSummary.end(); iterator++) {

			for (resit it = iterator->second.begin();
					it != iterator->second.end(); it++) {
				if (it->second.estimate() > freq) {
					count++;
				}

			}
			rfile << iterator->first << "," << count << "\n";
			count = 0;
		}
		rfile.close();
		std::cout << "finished parsing " << timer.LiveElapsedMilliseconds()
		<< std::endl;
	}
	void topK(int freq, int k) {
		Platform::Timer timer;
		timer.Start();
		ofstream rfile;

		typedef std::map<int, map<int, HyperLogLog>>::iterator locationit;
		typedef std::map<int, HyperLogLog>::iterator resit;
		int count = 0;
		for (locationit iterator = locationSummary.begin();
				iterator != locationSummary.end(); iterator++) {
			HyperLogLog hll(numberofbuckets);

			for (resit it = iterator->second.begin();
					it != iterator->second.end(); it++) {
				if (it->second.estimate() > freq) {
					hll.add(to_string(it->first).c_str(),
							to_string(it->first).size());
					count++;
				}

			}
			if (count > 2) {
				compactLocationSummary[iterator->first] = hll;
			}

			count = 0;
		}
		findseed(outfile + "_w" + to_string(window) + "_f" + to_string(freq),
				k);
		std::cout << "finished searching seeds " << timer.LiveElapsedSeconds()
		<< std::endl;
	}
private:
	void cleanup(long checkintime, long window, bool isforward) {

		map<string, map<int, long> > newuserSummary;
		map<int, long> newlist;
		long diff;
		typedef std::map<string, map<int, long>>::iterator it1;
		typedef std::map<int, long>::iterator it2;
		for (it1 it = userSummary.begin(); it != userSummary.end(); it++) {

			for (it2 itinner = userSummary[it->first].begin();
					itinner != userSummary[it->first].end(); itinner++) {
				if (isforward) {
					diff = checkintime - itinner->second;
				} else {
					diff = itinner->second - checkintime;
				}
				if (diff < window) {
					newlist[itinner->first] = itinner->second;
					//	userSummary[it->first].erase(itinner->first);

				}

			}
			userSummary[it->first].clear();

			if (newlist.size() > 0) {
				newuserSummary[it->first] = newlist;
			}
			newlist.clear();

		}
		userSummary.clear();
		userSummary = newuserSummary;
		//	for (unsigned i = 0; i < removelist.size() - 1; i++) {
		//	userSummary.erase(removelist[i]);
		//}
	}
	void findseed(string keyfile, int seedc) {

		node nd;

		//	std::cout << "finished compute" << std::endl;
		Platform::Timer timer;

		//	std::cout << "staring" << std::endl;
		timer.Start();
		vector<node> nodelist(seedc);
		vector<int> seedlist(seedc);
		set<int> selectednodes;
		typedef std::map<int, HyperLogLog>::iterator nodeit;
		for (nodeit iterator = compactLocationSummary.begin();
				iterator != compactLocationSummary.end(); iterator++) {
			nd.nodeid = iterator->first;
			nd.nodeset = iterator->second;
			nd.estimate = nd.nodeset.estimate();
			if (nd.estimate > 2)
			nodelist.push_back(nd);
		}
		sort(nodelist.begin(), nodelist.end(), sortByEstimate);
		seedlist[0] = nodelist[0].nodeid;
		HyperLogLog seed(numberofbuckets);

		double max = -1.0, tempsize = 0.0;

		int newseed;
		HyperLogLog is(numberofbuckets), temp(numberofbuckets), maxinf(
				numberofbuckets), newinf(numberofbuckets);
		is = nodelist[0].nodeset;
		selectednodes.insert(nodelist[0].nodeid);

		for (int i = 1; i < seedc; i++) {

			max = 0.0;

			for (node &n : nodelist) {
				if (selectednodes.find(n.nodeid) == selectednodes.end()) {
					if (n.nodeset.estimate() < max) {
						break;
					}
					temp = is;
					temp.merge(n.nodeset);
					tempsize = temp.estimate();
					if (tempsize > max) {
						max = tempsize;
						maxinf = temp;
						newseed = n.nodeid;
						newinf = n.nodeset;
					}
				}
			}
			seedlist[i] = newseed;
			selectednodes.insert(newseed);
			is.merge(newinf);
		}

		ofstream result;
		stringstream resultfile;
		resultfile << keyfile << "_s" << seedc << ".keys";
		std::cout << resultfile.str() << endl;
		result.open(resultfile.str().c_str());
		for (int n : seedlist) {
			result << n << "\n";
			std::cout << n << endl;

		}

		result.close();
	}
	void findWeigthedSeed(string keyfile, int seedc) {
		locationnode lnd;

				//	std::cout << "finished compute" << std::endl;
				Platform::Timer timer;

				//	std::cout << "staring" << std::endl;
				timer.Start();
				vector<locationnode> locationlist;
				vector<int> seedlist(seedc);
				set<int> selectednodes;
				typedef std::map<int, locationnode>::iterator nodeit;
				for (nodeit iterator = weigthedLocationSummary.begin();
						iterator != weigthedLocationSummary.end(); iterator++) {
					if(iterator->second.influenceset.size()>2){
						locationlist.push_back(iterator->second);
					}
				}
				sort(locationlist.begin(), locationlist.end(), sortByInfluenceSet);
				seedlist[0] = locationlist[0].locationid   ;
				HyperLogLog seed(numberofbuckets);

				double max = -1.0, tempsize = 0.0;

				int newseed;
				HyperLogLog is(numberofbuckets), temp(numberofbuckets), maxinf(
						numberofbuckets), newinf(numberofbuckets);
			//	is = nodelist[0].nodeset;
			//	selectednodes.insert(nodelist[0].nodeid);
/*
				for (int i = 1; i < seedc; i++) {

					max = 0.0;

					for (node &n : nodelist) {
						if (selectednodes.find(n.nodeid) == selectednodes.end()) {
							if (n.nodeset.estimate() < max) {
								break;
							}
							temp = is;
							temp.merge(n.nodeset);
							tempsize = temp.estimate();
							if (tempsize > max) {
								max = tempsize;
								maxinf = temp;
								newseed = n.nodeid;
								newinf = n.nodeset;
							}
						}
					}
					seedlist[i] = newseed;
					selectednodes.insert(newseed);
					is.merge(newinf);
				}

				ofstream result;
				stringstream resultfile;
				resultfile << keyfile << "_s" << seedc << ".keys";
				std::cout << resultfile.str() << endl;
				result.open(resultfile.str().c_str());
				for (int n : seedlist) {
					result << n << "\n";
					std::cout << n << endl;

				}

				result.close();
				*/
	}

};

}

#endif /* LOCATIONINFLUNCE_HPP_ */
