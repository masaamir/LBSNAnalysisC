/*
 * CountDistinctSketch.hpp
 *
 *  Created on: Mar 11, 2016
 *      Author: Rohit
 */

#ifndef COUNTDISTINCTSKETCH_HPP_
#define COUNTDISTINCTSKETCH_HPP_
# define LONG_PRIME 32993
# include <ctime>
#include <vector>
#include "hyperloglog.hpp"
#include <string>

using namespace std;
using namespace hll;
/** CountDistinctSketch class definition here **/
namespace cds {
class CountDistinctSketch {
	// width, depth

public:
	// constructor
	CountDistinctSketch(float ep = 0.01, float gamm = 0.90, int bucket = 7) {
		eps = ep;
		gamma = gamm;
		w = ceil(exp(1) / eps);
		d = ceil(log(1 / gamma));
		total = 0;
		buck = bucket;
		// initialize counter array of arrays, C
		C.resize(d);
		unsigned int i, j;
		for (i = 0; i < d; i++) {
			C[i].resize(w);
			for (j = 0; j < w; j++) {
				HyperLogLog hll(buck);
				C[i][j] = hll;
			}
		}
		// initialize d pairwise independent hashes
		srand(time(NULL));
		hashes = new int*[d];
		for (i = 0; i < d; i++) {
			hashes[i] = new int[2];
			genajbj(hashes, i);
		}
	}

	// update item (int) by count c
	void update(int item, string value) {

		unsigned int hashval = 0;
		for (unsigned int j = 0; j < d; j++) {
			hashval = (hashes[j][0] * item + hashes[j][1]) % w;
			C[j][hashval].add(value.c_str(), value.size());
		}
	}

	// estimate count of item i and return count
	unsigned int estimate(int item) {
		HyperLogLog result(buck);
		unsigned int hashval = (hashes[0][0] * item + hashes[0][1]) % w;
		result.replace(C[0][hashval]);
		for (unsigned int j = 1; j < d; j++) {
			hashval = (hashes[j][0] * item + hashes[j][1]) % w;

			result.minimum(C[j][hashval]);
		}
		return result.estimate();
	}
	// generate "new" aj,bj
	void genajbj(int **hashes, int i) {
		hashes[i][0] = int(
				float(rand()) * float(LONG_PRIME) / float(RAND_MAX) + 1);
		hashes[i][1] = int(
				float(rand()) * float(LONG_PRIME) / float(RAND_MAX) + 1);
	}
protected:
	unsigned int w, d;
	int buck;
	// eps (for error), 0.01 < eps < 1
	// the smaller the better
	float eps;

	// gamma (probability for accuracy), 0 < gamma < 1
	// the bigger the better
	float gamma;

	// aj, bj \in Z_p
	// both elements of fild Z_p used in generation of hash
	// function
	unsigned int aj, bj;

	// total count so far
	unsigned int total;

	// array of arrays of counters
	vector<vector<hll::HyperLogLog> > C;

	// array of hash values for a particular item
	// contains two element arrays {aj,bj}
	int **hashes;

};
}
#endif /* COUNTDISTINCTSKETCH_HPP_ */
