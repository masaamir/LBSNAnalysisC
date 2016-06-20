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
#include "modifiedhyperloglog.hpp"
#include "util.h"
using namespace hll;
using namespace std;
using namespace mhll;
using namespace util;
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

struct superlocation {
	map<int, HyperLogLog> locationInfluence;
	set<int> setoflocations;
	double totalweight;

};
struct locationnode {
	int locationid;
	HyperLogLog visitor;
	map<int, HyperLogLog> influenceset;
	double rankweight;
};

bool sortByEstimate(const node &lhs, const node &rhs) {
	return lhs.estimate > rhs.estimate;
}
bool sortByInfluenceSet(const locationnode &lhs, const locationnode &rhs) {
	return lhs.influenceset.size() > rhs.influenceset.size();
}
bool sortByRankWeight(const locationnode &lhs, const locationnode &rhs) {
	return lhs.rankweight > rhs.rankweight;
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
	map<int, set<int>> exactCompactLocationSummary;
	map<int, HyperLogLog> naiveSummary;
	map<string, ModifiedHyperLogLog > compactUsrSummary;
	map<string,HyperLogLog> friendmap;
	string datafolder;
	uint8_t numberofbuckets;
	string datafile;
	string outfile;
	long window = 0;
	LocationInfluence(uint8_t b = 7, string file = "", string ofile = "",string folder = "")
	throw (invalid_argument) {

		numberofbuckets = b;
		datafile = file;
		outfile = ofile;
		datafolder=folder;
	}
	void generateFriendshipData(string file) {
		ifstream infile(file.c_str());
		string line;
		vector<string> temp;
		Platform::Timer timer;
		timer.Start();

		while (infile >> line) {

			temp = Tools::Split(line, ',');
			if (friendmap.find(temp[0]) != friendmap.end()) {
				friendmap[temp[0]].add(temp[1].c_str(),temp[1].length());
			} else {
				HyperLogLog newhll(numberofbuckets);
				newhll.add(temp[1].c_str(),temp[1].length());
				friendmap[temp[0]]=newhll;
			}
		}
		std::cout << "build friendship network : "<<friendmap.size()<<" "<< timer.ElapsedMilliseconds()<<std::endl;
	}
	void FindInflunceExactUnitFreq(int wp, bool isforward,bool write) {
		window = 0;

		window = wp * 60 * 60;

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

		timer.Start();
		int locationid;
		long checkintime;
		string userid;
		long diff;
		int srcLoc, destLoc;
		map<int, long> newuser;
		set<int> newlocset;
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
						if (exactCompactLocationSummary.find(srcLoc)
								== exactCompactLocationSummary.end()) {
							newlocset.clear();

							exactCompactLocationSummary[srcLoc] = newlocset;
						}
						exactCompactLocationSummary[srcLoc].insert(destLoc);

					}
				} //end of for loop

			}

			newuser[locationid] = checkintime;

			userSummary[userid] = newuser;

			if (i % 100000 == 0) {
				//std::cout << i << " " << userSummary.size() << std::endl;
				cleanup(checkintime, window, isforward);
			}
		} //end of data for loop
		std::cout << "finished parsing " << timer.LiveElapsedSeconds()
		<< std::endl;
		if(write) {
			ofstream result;
			stringstream resultfile;
			resultfile << outfile << "exactUnit_w" << window << ".csv";

			result.open(resultfile.str().c_str());

			typedef std::map<int, set<int>>::iterator nodeit;
			for (nodeit iterator = exactCompactLocationSummary.begin();
					iterator != exactCompactLocationSummary.end(); iterator++) {
				result << iterator->first << ","<< iterator->second.size()<<"\n";

			}
			result.close();
		}

	}
	void findtopKusingNaive(string keyfile, int seedc) {
		vector<string> templine;
		ifstream infile(datafile.c_str());
		edge tempEdge;
		vector<edge> data;
		string line,user;
		int location;
		vector<int> seedlist(seedc);
		set<int> selectednodes;
		while (infile >> line) {

			templine = Tools::Split(line, ',');

			user = templine[0];
			location = atoi(templine[1].c_str());
			if(naiveSummary.find(location)==naiveSummary.end()) {
				HyperLogLog newusers(numberofbuckets);
				naiveSummary[location]=newusers;
			}
			naiveSummary[location].add(user.c_str(),user.length());
		}
		typedef std::map<int, HyperLogLog>::iterator nodeit;

		vector<node> nodelist;
		for (nodeit iterator = naiveSummary.begin();
				iterator != naiveSummary.end(); iterator++) {
			node nd;
			nd.nodeid=iterator->first;
			nd.nodeset=iterator->second;
			nd.estimate=nd.nodeset.estimate();
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
		resultfile << keyfile << "_naive" << seedc << ".keys";
		std::cout << resultfile.str() << endl;
		result.open(resultfile.str().c_str());
		for (int n : seedlist) {
			result << n << "\n";
		}

		result.close();

	}
	void FindInflunceApproxUnitFreqBackward(int wp, bool write) {
		long window = wp * 60 * 60;

		vector<string> temp;
		vector<edge> data;
		ifstream infile(datafile.c_str());
		edge tempEdge;
		map<string, ModifiedHyperLogLog> locSummary;

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
		<< timer.LiveElapsedSeconds() << std::endl;
		timer.Start();

		long checkintime;
		string userid;
		timer.Start();
		string srcLoc;
		ModifiedHyperLogLog newuser(numberofbuckets);
		ModifiedHyperLogLog newloc(numberofbuckets);
		for (int i = 0; i < datasize - 1; i++) {

			userid = data[i].user;

			checkintime = data[i].checkin;
			srcLoc=to_string(data[i].location);

			if (compactUsrSummary.find(userid) != compactUsrSummary.end()) {

				if(locSummary.find(srcLoc)==locSummary.end()) {

					locSummary[srcLoc]=newloc;
				}

				locSummary[srcLoc].merge(compactUsrSummary[userid],checkintime,window);

			} else {
				compactUsrSummary[userid]=newuser;

			}
			compactUsrSummary[userid].add(srcLoc.c_str(),srcLoc.length(),checkintime);

			if (i % 100000 == 0) {
				//	std::cout << i << " " << compactUsrSummary.size() << std::endl;
				//cleanupApprox(checkintime,window);
			}
		}
		//std::cout << "parsed in " << timer.LiveElapsedSeconds() << std::endl;
		std::cout << "finished parsing " << timer.LiveElapsedSeconds() << " memory "<<getValue() << std::endl;
		if(write) {
			ofstream result;
			stringstream resultfile;
			resultfile << outfile << "backUnit_w" << window << ".csv";

			result.open(resultfile.str().c_str());

			typedef std::map<string, ModifiedHyperLogLog>::iterator nodeit;
			for (nodeit iterator = locSummary.begin();
					iterator != locSummary.end(); iterator++) {
				result << iterator->first << ","<< iterator->second.estimate()<<"\n";

			}
			result.close();
		}

	}
	void FindInflunceApproxUnitFreqBackwardNew(int wp, bool write) {
		long window = wp * 60 * 60;

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
		<< timer.LiveElapsedSeconds() << std::endl;
		timer.Start();

		//cout << runCommand("systeminfo | find \"Virtual Memory: In Use:\"") << endl;
		long checkintime;
		string userid;
		timer.Start();
		int srcLoc;
		ModifiedHyperLogLog newuser(numberofbuckets);

		HyperLogLog compactuser;
		HyperLogLog newloc(numberofbuckets);
		for (int i = 0; i < datasize - 1; i++) {

			userid = data[i].user;

			checkintime = data[i].checkin;
			srcLoc=data[i].location;

			if (compactUsrSummary.find(userid) != compactUsrSummary.end()) {
				compactuser=compactUsrSummary[userid].convertToHLL(checkintime,window);
				//*compactuser->clear();

				if(compactuser.estimate()>0) {
					if(compactLocationSummary.find(srcLoc)==compactLocationSummary.end()) {

						compactLocationSummary[srcLoc]=newloc;
					}

					compactLocationSummary[srcLoc].merge(compactuser);
				}
			} else {
				//ModifiedHyperLogLog nu(numberofbuckets);
				//newuser=nu;
				compactUsrSummary[userid]=newuser;
			}

			compactUsrSummary[userid].add(to_string(srcLoc).c_str(),to_string(srcLoc).length(),checkintime);

			if (i % 100000 == 0) {
				//	std::cout << i << " " << compactUsrSummary.size() << std::endl;
				//cleanupApprox(checkintime,window);
			}
		}
		//std::cout << "parsed in " << timer.LiveElapsedSeconds() << std::endl;
		std::cout << "finished parsing " << timer.LiveElapsedSeconds()<<" "<<compactLocationSummary.size() << " memory "<<getValue() << std::endl;
		if(write) {
			ofstream result;
			stringstream resultfile;
			resultfile << outfile << "backNew_w" << window << ".csv";

			result.open(resultfile.str().c_str());

			typedef std::map<int, HyperLogLog>::iterator nodeit;
			for (nodeit iterator = compactLocationSummary.begin();
					iterator != compactLocationSummary.end(); iterator++) {
				result << iterator->first << ","<< iterator->second.estimate()<<"\n";

			}
			result.close();
		}

	}

	void FindInflunceApproxUnitFreq(int wp, int k, bool write, bool find) {
		long window = wp * 60 * 60;

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
		<< timer.LiveElapsedSeconds() << std::endl;
		timer.Start();
		int locationid;
		long checkintime;
		string userid;
		long diff;
		int srcLoc;
		string destLoc;
		map<int, long> newuser;
		//cout << runCommand("systeminfo | find \"Virtual Memory: In Use:\"") << endl;
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
				//	std::cout << i << " " << userSummary.size() << std::endl;
				//cleanup(checkintime, window, false);
			}
		} //end of data for loop
//std::cout << "finished parsing " << timer.LiveElapsedSeconds() << std::endl;
		std::cout << "finished parsing " << timer.LiveElapsedSeconds() << " "
		<< compactLocationSummary.size() << " memory " << getValue()
		<< std::endl;
		if (write) {
			ofstream result;
			stringstream resultfile;
			resultfile << outfile << "Approx_w" << window << ".csv";

			result.open(resultfile.str().c_str());

			typedef std::map<int, HyperLogLog>::iterator nodeit;
			for (nodeit iterator = compactLocationSummary.begin();
					iterator != compactLocationSummary.end(); iterator++) {
				result << iterator->first << "," << iterator->second.estimate() << "\n";

			}
			result.close();
		}

//finding top k
		if (find) {
			findseed(outfile, k);
		}
	}
	void FindInflunceWeigthed(int wp, bool withFriend,
			bool monitorTime) {

		window = 0;

		window = wp * 60 * 60;

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

		sort(data.begin(), data.end(), forward);

		int datasize = data.size();
		std::cout << "read and sorted data " << datasize << " in "
		<< timer.LiveElapsedMilliseconds() << std::endl;
		std::map<int, HyperLogLog>::iterator ithll;
		vector<string> monitordata;
		Platform::Timer mtimer;
		timer.Start();
		int locationid;
		long checkintime;
		string userid;
		long diff;

		int srcLoc, destLoc;

		typedef std::map<int, long>::iterator it_type;
		if (monitorTime) {
			mtimer.Start();
		}
		for (int i = 0; i < datasize - 1; i++) {
			userid = data[i].user;
			locationid = data[i].location;
			checkintime = data[i].checkin;

			//	locationSummary[locationid];

			map<int, long> newuser;
			if (weigthedLocationSummary.find(locationid)
					== weigthedLocationSummary.end()) {
				map<int, HyperLogLog> newlochll;
				HyperLogLog nhll(numberofbuckets);
				locationnode newlocation;
				newlocation.influenceset = newlochll;
				newlocation.locationid = locationid;

				newlocation.visitor = nhll;

				weigthedLocationSummary[locationid] = newlocation;
			}
			if (userSummary.find(userid) != userSummary.end()) {

				for (it_type it = userSummary[userid].begin();
						it != userSummary[userid].end(); it++) {

					diff = checkintime - it->second;
					destLoc = locationid;
					srcLoc = it->first;

					if ((diff) < window) {

						weigthedLocationSummary[destLoc].visitor.add(userid.c_str(),
								userid.length());
						if (withFriend) {
							if (friendmap.find(userid) != friendmap.end()) {
								weigthedLocationSummary[destLoc].visitor.merge(
										friendmap[userid]);
							}
						}
						ithll = weigthedLocationSummary[srcLoc].influenceset.find(
								destLoc);
						if (ithll
								== weigthedLocationSummary[srcLoc].influenceset.end()) {
							HyperLogLog hll(numberofbuckets);
							hll.add(userid.c_str(), userid.size());
							if (withFriend) {
								if (friendmap.find(userid) != friendmap.end()) {
									hll.merge(friendmap[userid]);
								}
							}
							weigthedLocationSummary[srcLoc].influenceset[destLoc] = hll;
						} else {
							weigthedLocationSummary[srcLoc].influenceset[destLoc].add(
									userid.c_str(), userid.size());
							if (withFriend) {
								if (friendmap.find(userid) != friendmap.end()) {
									weigthedLocationSummary[srcLoc].influenceset[destLoc].merge(
											friendmap[userid]);
								}
							}
						}

					}
				} //end of for loop

			}

			newuser[locationid] = checkintime;

			userSummary[userid] = newuser;

			if (i % 100000 == 0) {
				//std::cout << i << " " << userSummary.size() << std::endl;
				//cleanup(checkintime, window, isforward);
			}
			if (monitorTime) {
				if (i % 1000 == 0) {
					monitordata.push_back(
							to_string(i) + "," + to_string(mtimer.LiveElapsedSeconds())
							+ to_string(getValue()) + "\n");
					//monitordata.push_back(to_string(i)+","+to_string(mtimer.LiveElapsedMilliseconds())+"\n");
					mtimer.Start();
				}
			}

		} //end of data for loop
		std::cout << "finished parsing " << timer.LiveElapsedSeconds() << std::endl;
		if (monitorTime) {
			writeData(monitordata, datafolder + "monitorNW_" + to_string(wp) + ".csv");
		}
	}
	void FindInflunceApprox(int wp, bool isforward, bool monitorTime) {
		window = 0;

		window = wp * 60 * 60;
		vector<string> monitordata;
		Platform::Timer mtimer;
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
//	std::cout << "read and sorted data " << datasize << " in "
//<< timer.LiveElapsedMilliseconds() << std::endl;
		std::map<int, HyperLogLog>::iterator ithll;

		timer.Start();
		int locationid;
		long checkintime;
		string userid;
		long diff;
		int srcLoc, destLoc;
		if (monitorTime) {
			mtimer.Start();
		}
		typedef std::map<int, long>::iterator it_type;
		for (int i = 0; i < datasize - 1; i++) {
			userid = data[i].user;
			locationid = data[i].location;
			checkintime = data[i].checkin;

			//	locationSummary[locationid];

			map<int, long> newuser;
			map<int, HyperLogLog> newlochll;

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
					if (diff < window) {
						newuser[it->first] = it->second;
						if (locationSummary.find(srcLoc) == locationSummary.end()) {
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
				//std::cout << i << " " << userSummary.size() << std::endl;
				cleanup(checkintime, window, isforward);
			}
			if (monitorTime) {
				if (i % 1000 == 0) {
					monitordata.push_back(
							to_string(i) + "," + to_string(mtimer.LiveElapsedSeconds())
							+ to_string(getValue()) + "\n");
					//monitordata.push_back(to_string(i)+","+to_string(mtimer.LiveElapsedMilliseconds())+"\n");
					mtimer.Start();
				}
			}

		} //end of data for loop
		std::cout << "finished parsing " << timer.LiveElapsedSeconds() << std::endl;
		if (monitorTime) {
			writeData(monitordata, datafolder + "monitorNW_" + to_string(wp) + ".csv");
		}
	}
	void FindInflunceExact(int wp, bool isforward) {
		window = 0;

		window = wp * 60 * 60;

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
				//	std::cout << i << " " << userSummary.size() << std::endl;
				cleanup(checkintime, window, isforward);
			}
		} //end of data for loop
		std::cout << "finished parsing " << timer.LiveElapsedSeconds() << std::endl;
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
		std::cout << "finished querying " << timer.LiveElapsedSeconds() << std::endl;
	}

	void queryAll(double tau) {
		Platform::Timer timer;
		timer.Start();
		ofstream rfile;
		outfile = outfile + "_" + to_string(tau) + ".csv";
		rfile.open(outfile.c_str());
		typedef std::map<int, map<int, HyperLogLog>>::iterator locationit;
		typedef std::map<int, HyperLogLog>::iterator resit;
		int count = 0;
		locationit iterator;
		resit it;
		for (iterator = locationSummary.begin(); iterator != locationSummary.end();
				iterator++) {

			for (it = iterator->second.begin(); it != iterator->second.end(); it++) {
				if (it->second.estimate() > tau) {
					count++;
				}

			}
			rfile << iterator->first << "," << count << "\n";
			rfile.flush();
			count = 0;
		}
		rfile.close();
		std::cout << "finished querying " << timer.LiveElapsedSeconds() << std::endl;
	}
	void queryExact(int freq) {
		Platform::Timer timer;
		timer.Start();
		typedef std::map<int, map<int, set<string>>> ::iterator locationit;
		typedef std::map<int, set<string>>::iterator resit;
		int count = 0;
		locationit iterator;
		resit it;

		for (iterator = exactLocationSummary.begin();
				iterator != exactLocationSummary.end(); iterator++) {
			set<int> locations;
			for (it = iterator->second.begin(); it != iterator->second.end(); it++) {
				if (it->second.size() > freq) {
					locations.insert(it->first);
					count++;

				}

			}
			exactCompactLocationSummary[iterator->first] = locations;
			count = 0;
		}

	}

	void queryInflunceSet(string file, int k, string outputfile) {

		vector<int> keys = readKeys(file, k);
		set<int> result;
		for (int temp : keys) {
			result.insert(exactCompactLocationSummary[temp].begin(),
					exactCompactLocationSummary[temp].end());
		}
		vector<string> data;
		for (int t : result) {
			data.push_back(to_string(t) + "\n");
		}
		writeData(data, outputfile);

	}
	void queryWeighted(double tau,bool isrelative) {
		Platform::Timer timer;
		timer.Start();
		ofstream rfile;
		outfile = outfile + "_" + to_string(tau) + ".result";
		rfile.open(outfile.c_str());
		typedef std::map<int, locationnode>::iterator locationit;
		typedef std::map<int, HyperLogLog>::iterator resit;
		int count = 0;
		locationit iterator;
		resit it;
		locationnode lntemp;
		double inf;
		for (iterator = weigthedLocationSummary.begin();
				iterator != weigthedLocationSummary.end(); iterator++) {
			lntemp = iterator->second;
			for (it = lntemp.influenceset.begin(); it != lntemp.influenceset.end();
					it++) {
				inf=it->second.estimate();
				if(isrelative) {
					inf=inf/weigthedLocationSummary[it->first].visitor.estimate();
				}
				if (inf > tau) {
					count++;
				}

			}
			rfile << iterator->first << "," << count << "\n";
			rfile.flush();
			count = 0;
		}
		rfile.close();
		std::cout << "finished querying " << timer.LiveElapsedSeconds() << std::endl;
	}

	void findseed(int freq, int k) {
		Platform::Timer timer;
		timer.Start();
		ofstream rfile;

		typedef std::map<int, map<int, HyperLogLog>>::iterator locationit;
		typedef std::map<int, HyperLogLog>::iterator resit;
		int count = 0;
		locationit iterator;
		resit it;
		for (iterator = locationSummary.begin(); iterator != locationSummary.end();
				iterator++) {
			HyperLogLog hll(numberofbuckets);

			for (it = iterator->second.begin(); it != iterator->second.end(); it++) {
				if (it->second.estimate() > freq) {
					hll.add(to_string(it->first).c_str(), to_string(it->first).size());
					count++;
				}

			}
			if (count > 2) {
				compactLocationSummary[iterator->first] = hll;
			}

			count = 0;
		}

		std::cout << "finished creating compact list " << timer.LiveElapsedSeconds()
		<< std::endl;
		timer.Start();
		findseed(outfile + "_w" + to_string(window) + "_f" + to_string(freq), k);
		std::cout << "finished searching seeds " << timer.LiveElapsedSeconds()
		<< std::endl;
	}

	void findWeigthedSeed(string keyfile, double tau, int seedc,double alpha,bool isrelative) {

		Platform::Timer timer;
		timer.Start();
		locationnode lnd;
		vector<locationnode> nodelist;
		superlocation superl;
		vector<int> seedlist(seedc);

		typedef std::map<int, locationnode>::iterator nodeit;

		for (nodeit iterator = weigthedLocationSummary.begin();
				iterator != weigthedLocationSummary.end(); iterator++) {

			lnd=iterator->second;

			lnd.rankweight=getRankWeight( lnd.influenceset,isrelative,alpha,tau);
			//		std::cout << lnd.locationid<<" : " << lnd.rankweight << endl;
			if (lnd.rankweight > 0) {
				nodelist.push_back(lnd);
			}
			if(lnd.locationid!=iterator->first) {
				std::cout << lnd.locationid<<" : " << iterator->first<<" : " << lnd.influenceset.size() << endl;
			}

		}
		sort(nodelist.begin(), nodelist.end(), sortByRankWeight);
		seedlist[0] = nodelist[0].locationid;
		superl.setoflocations.insert(nodelist[0].locationid);
		superl.locationInfluence=nodelist[0].influenceset;
		superl.totalweight=nodelist[0].rankweight;
		double previousweight=0.0;
		double maxgain=-1.0;
		double gain=0.0;
		int candidatenodes=nodelist.size();
		std::cout << "finished ranking " << nodelist.size() << " time "
		<< timer.LiveElapsedSeconds() << std::endl;
		int bestid=0;
		for (int i = 1; i < seedc; i++) {
			previousweight=superl.totalweight;
			//std::cout << previousweight<< std::endl;
			for (int j=1;j<candidatenodes;j++) {
				if(superl.setoflocations.find(nodelist[j].locationid)==superl.setoflocations.end()) {
					//std::cout <<"rank "<<nodelist[j].rankweight<< std::endl;
					if(nodelist[j].rankweight<maxgain) {
						break;
					}

					gain=getWeightIncrease(superl,nodelist[j],isrelative,alpha,tau)-previousweight;

					if(gain>maxgain) {
						//	std::cout <<"gain "<< gain<< "max gain "<<maxgain<<" j = "<<j<< std::endl;
						maxgain=gain;
						bestid=j;

					}

				}
			} //end of for loop
			  //add the survived candidate to the seed
			seedlist[i]=nodelist[bestid].locationid;
			if(bestid!=0) {
				addlocationToSuperLocation(superl,nodelist[bestid]);
				superl.totalweight=getRankWeight(superl.locationInfluence,isrelative,alpha,tau);
				bestid=0;
				gain=0;
				maxgain=-1;
			} else {
				break;
			}

		}
		std::cout <<superl.totalweight <<endl;
		std::cout << "finished finding seed " << timer.LiveElapsedSeconds()
		<< std::endl;
		ofstream result;
		stringstream resultfile;
		resultfile << keyfile << "_s" << seedc<<"_a"<<alpha << ".keys";
		std::cout << resultfile.str() << endl;
		result.open(resultfile.str().c_str());
		for (int n : seedlist) {
			result << n << "\n";

		}

		result.close();
		if(false) {
			ofstream rank;
			stringstream rankfile;
			rankfile << keyfile << "_s" << seedc<<"_a"<<alpha << ".rank";
			std::cout << rankfile.str() << endl;
			result.open(rankfile.str().c_str());
			typedef std::map<int, HyperLogLog>::iterator infsetit;
			double inf;
			double avg;
			int c=0;
			for (locationnode n : nodelist) {

				result << n.locationid<<","<<n.rankweight<<","<<n.visitor.estimate()<<","<<n.influenceset.size();
				for (infsetit it = n.influenceset.begin();
						it != n.influenceset.end(); it++) {

					inf=it->second.estimate();

					if(isrelative) {
						if(weigthedLocationSummary[it->first].visitor.estimate()>0) {
							inf=inf/weigthedLocationSummary[it->first].visitor.estimate();
						}
						else {
							inf=0;
							cout<< n.locationid<<","<<it->first<<" : "<<weigthedLocationSummary[it->first].visitor.estimate()<<" : "<<it->second.estimate()<<endl;
						}
					}
					c++;
					avg=avg+inf;
					result<<","<<it->first<<":"<<inf;

				}

				result<< "\n";

			}

			result.close();
		}
	}

private:
	void addlocationToSuperLocation(superlocation &sl,locationnode &ln) {
		typedef std::map<int, HyperLogLog>::iterator infsetit;

		for (infsetit iterator = ln.influenceset.begin();
				iterator != ln.influenceset.end(); iterator++) {
			if(sl.locationInfluence.find(iterator->first)==sl.locationInfluence.end()) {
				sl.locationInfluence[iterator->first]=iterator->second;
			} else {
				sl.locationInfluence[iterator->first].merge(iterator->second);
			}
		}
		sl.setoflocations.insert(ln.locationid);

	}
	double getWeightIncrease(superlocation sl,locationnode &ln,bool isrelative,double alpha,double tau) {
		addlocationToSuperLocation(sl,ln);
		return getRankWeight(sl.locationInfluence,isrelative,alpha,tau);
	}

	double getRankWeight(map<int,HyperLogLog> &influenceset,bool isrelative,double alpha,double tau) {
		double weight=0.0;
		double count=0.0;
		double inf;
		typedef std::map<int, HyperLogLog>::iterator infsetit;
		for (infsetit it = influenceset.begin();
				it != influenceset.end(); it++) {

			inf=it->second.estimate();
			if(isrelative) {
				if(weigthedLocationSummary[it->first].visitor.estimate()>0) {
					inf=inf/weigthedLocationSummary[it->first].visitor.estimate();
				}
				else {
					inf=0;
					//cout<<it->first<<" : "<<weigthedLocationSummary[it->first].visitor.estimate()<<" : "<<it->second.estimate()<<endl;
				}
			}
			if (inf < tau) {

				weight=weight+alpha*inf;
			} else {
				weight=weight+alpha*tau;
				count++;
			}

		}
		weight=weight+(1-alpha)*count;
		return weight;
	}
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

//userSummary.clear();
		userSummary = newuserSummary;
//	for (unsigned i = 0; i < removelist.size() - 1; i++) {
//	userSummary.erase(removelist[i]);
//}
	}
	void cleanupApprox(long checkintime, long window) {

		map<string, ModifiedHyperLogLog> newuserSummary;
		ModifiedHyperLogLog newlist;
		long diff;
		typedef std::map<string, ModifiedHyperLogLog>::iterator it1;

		for (it1 it = compactUsrSummary.begin(); it != compactUsrSummary.end(); it++) {
			newlist = it->second;
			newlist.cleanup(checkintime, window);

			if (newlist.estimate() > 0) {
				newuserSummary[it->first] = newlist;
			}

		}

//delete[] pt;
//userSummary.clear();
		compactUsrSummary = newuserSummary;
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
		HyperLogLog is(numberofbuckets), temp(numberofbuckets), maxinf(numberofbuckets),
		newinf(numberofbuckets);
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
		std::cout << "finished finding seed " << timer.LiveElapsedSeconds()
		<< std::endl;
		ofstream result;
		stringstream resultfile;
		resultfile << keyfile << "_s" << seedc << ".keys";
		std::cout << resultfile.str() << endl;
		result.open(resultfile.str().c_str());
		for (int n : seedlist) {
			result << n << "\n";

		}

		result.close();
	}
	void writeData(vector<string> data, string path) {
		ofstream result;

		result.open(path.c_str());
		for (string n : data) {
			result << n;

		}

		result.close();
	}
	vector<int> readKeys(string file, int k) {
		ifstream infile(file.c_str());
		vector<int> keys;
		string line;
		int count = 0;
		while (infile >> line) {
			keys.push_back(atoi(line.c_str()));
			count++;
			if (count > k) {
				break;
			}
		}
		return keys;
	}
}
;

}

#endif /* LOCATIONINFLUNCE_HPP_ */
