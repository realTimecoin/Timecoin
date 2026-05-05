#include "consensus/amount.h"
// Copyright (c) 2026 Johnbillgate / Timecoin Developers
// Distributed under the MIT software license.
#pragma once
#ifndef TIMECOIN_CONSENSUS_SUBSIDY_H
#define TIMECOIN_CONSENSUS_SUBSIDY_H

#include <cstdint>
#include "consensus/params.h"

/** Build the full Fibonacci season table (call once at startup). */
void BuildSeasonTable();

/** Return block reward in Instants for a given height. Replaces Bitcoin's GetBlockSubsidy(). */
CAmount GetBlockSubsidy(int nHeight, const Consensus::Params& consensusParams);

/** Return which Fibonacci season a block belongs to (1-based). -1 = post-emission. */
int GetBlockSeason(int nHeight);

/** Return cumulative supply in Instants at a given block height. */
int64_t GetCumulativeSupply(int nHeight);

/** Return true if nValue is within valid Instant range [0, MAX_MONEY]. */

/** Return number of Fibonacci difficulty steps needed given actual vs target timespan. */
int GetFibonacciDifficultyStep(int64_t nActualTimespan, int64_t nTargetTimespan);

#ifdef TIMECOIN_DEBUG
/** Print the full season table to stdout (debug builds only). */
void PrintSeasonTable();
#endif

#endif // TIMECOIN_CONSENSUS_SUBSIDY_H
