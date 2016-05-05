/*
 * modifiedhyperloglog.hpp
 *
 *  Created on: Dec 30, 2015
 *      Author: Rohit
 */

#ifndef MODIFIEDHYPERLOGLOG_HPP_
#define MODIFIEDHYPERLOGLOG_HPP_

#include <vector>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include "murmur3.h"
#include "hyperloglog.hpp"

#define HLL_HASH_SEED 313

#if defined(__has_builtin) && (defined(__GNUC__) || defined(__clang__))

#define _GET_CLZ(x, b) (uint8_t)std::min(b, ::__builtin_clz(x)) + 1

#else

#define _GET_CLZ(x, b) _get_leading_zero_count(x, b)
#endif /* defined(__GNUC__) */
struct data {
	uint8_t value;
	long time;
};
using namespace std;
using namespace hll;
namespace mhll {

static const double pow_2_32 = 4294967296.0; ///< 2^32
static const double neg_pow_2_32 = -4294967296.0; ///< -(2^32)

/** @class HyperLogLog
 *  @brief Implement of 'HyperLogLog' estimate cardinality algorithm
 */
class ModifiedHyperLogLog {
	vector<vector<data> > M_;

public:

	/**
	 * Constructor
	 *
	 * @param[in] b bit width (register size will be 2 to the b power).
	 *            This value must be in the range[4,30].Default value is 4.
	 *
	 * @exception std::invalid_argument the argument is out of range.
	 */
	ModifiedHyperLogLog(uint8_t b = 8) throw (std::invalid_argument) :
			b_(b), m_(1 << b) {
		///< registers
		M_.resize(m_);

		if (b < 4 || 30 < b) {
			throw std::invalid_argument(
					"bit width must be in the range [4,30]");
		}

		double alpha;
		switch (m_) {
		case 16:
			alpha = 0.673;
			break;
		case 32:
			alpha = 0.697;
			break;
		case 64:
			alpha = 0.709;
			break;
		default:
			alpha = 0.7213 / (1.0 + 1.079 / m_);
			break;
		}
		alphaMM_ = alpha * m_ * m_;
	}

	/**
	 * Adds element to the estimator
	 *
	 * @param[in] str string to add
	 * @param[in] len length of string
	 */
	void add(const char* str, uint32_t len, long time) {

		uint32_t hash;
		MurmurHash3_x86_32(str, len, HLL_HASH_SEED, (void*) &hash);
		uint32_t index = hash & ((1 << b_) - 1);
		uint8_t rank = _GET_CLZ((hash << b_), 32 - b_);

		updateBucket(index, rank, time);
	}

	bool updateBucket(uint32_t index, uint8_t value, long time) {
		data newdata;
		newdata.time = time;
		newdata.value = value;

		if (M_[index].empty()) {

			M_[index].push_back(newdata);

			return true;
		} else {
			vector<data> newlist;
			for (uint32_t i = 0; i < M_[index].size(); i++) {
				if (M_[index][i].value == newdata.value) {
					if (M_[index][i].time <= newdata.time) {
						return false;
					} else if (M_[index][i].time > newdata.time) {
						M_[index][i] = newdata;
						return true;
					}

				} else if (M_[index][i].time == newdata.time) {

					if (M_[index][i].value < newdata.value) {
						M_[index][i] = newdata;
						return true;
					} else {
						return false;
					}
				} else {
					if (value < M_[index][i].value
							&& time > M_[index][i].time) {
						return false;
					} else {
						if (value < M_[index][i].value
								&& time < M_[index][i].time) {
							newlist.push_back(M_[index][i]);
						} else if (value > M_[index][i].value
								&& time > M_[index][i].time) {
							newlist.push_back(M_[index][i]);
						}
					}

				}

			}		//end of for
			newlist.push_back(newdata);
			M_[index] = newlist;
			return true;
		}

	}
	double getCurrentSum() const {
		double sum = 0.0;
		uint8_t max = 0;

		for (uint32_t i = 0; i < m_; i++) {
			if (!M_[i].empty()) {
				max = 0;
				for (uint32_t j = 0; j < M_[i].size(); j++) {
					if (max < M_[i][j].value) {
						max = M_[i][j].value;
					}
				}
				sum += 1.0 / (1 << max);

			} else {
				sum += 1.0 / (1 << 0);
			}

		}

		return sum;
	}
	/**
	 * Merges the estimate from 'other' into this object, returning the estimate of their union.
	 * The number of registers in each must be the same.
	 *
	 * @param[in] other HyperLogLog instance to be merged
	 *
	 * @exception std::invalid_argument number of registers doesn't match.
	 */
	void merge(const ModifiedHyperLogLog &other, long time, long window)
			throw (std::invalid_argument) {
		if (m_ != other.m_) {
			std::stringstream ss;
			ss << "number of registers doesn't match: " << m_ << " != "
					<< other.m_;
			throw std::invalid_argument(ss.str().c_str());
		} else {

			for (uint32_t i = 0; i < m_; i++) {

				for (uint32_t j = 0; j < other.M_[i].size(); j++) {
					if (other.M_[i][j].time - time <= window) {
						updateBucket(i, other.M_[i][j].value,
								other.M_[i][j].time);
					}
				}
			}
		}

	}

	void cleanup(long time, long window) {
		for (uint32_t i = 0; i < m_; i++) {
			vector<data> newlist;

			for (uint32_t j = 0; j < M_[i].size(); j++) {

				if (M_[i][j].time - time <= window) {
					data newdata;
					newdata.time=M_[i][j].time;
					newdata.value=M_[i][j].value;
					newlist.push_back(newdata);
				}

			}
			M_[i]=newlist;
		}
	}
	/**
	 * Estimates cardinality value.
	 *
	 * @return Estimated cardinality value.
	 */
	double estimate() const {
		double estimate;
		double sum = 0.0;

		sum = getCurrentSum();

		estimate = alphaMM_ / sum; // E in the original paper

		if (estimate <= 2.5 * m_) {
			uint32_t zeros = 0;
			for (uint32_t i = 0; i < m_; i++) {
				if (M_[i].empty()) {
					zeros++;
				}
			}
			if (zeros != 0) {
				estimate = m_ * std::log(static_cast<double>(m_) / zeros);
			}
		} else if (estimate > (1.0 / 30.0) * pow_2_32) {
			estimate = neg_pow_2_32 * log(1.0 - (estimate / pow_2_32));
		}
		return estimate;
	}

	HyperLogLog convertToHLL() {

		HyperLogLog newhll(7);

		uint8_t max = 0;

		for (uint32_t i = 0; i < m_; i++) {

			max = 0;

			for (uint32_t j = 0; j < M_[i].size(); j++) {
				if (max < M_[i][j].value) {
					max = M_[i][j].value;
				}
			}

			newhll.set(i, max);
		}

		return newhll;
	}

	/**
	 * Returns size of register.
	 *
	 * @return Register size
	 */
	uint32_t registerSize() const {
		return m_;
	}

protected:
	uint8_t b_; ///< register bit width
	uint32_t m_; ///< register size
	double alphaMM_; ///< alpha * m^2

};

} // namespace hll

#endif /* HYPERLOGLOG_HPP_ */
