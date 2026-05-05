#include "consensus/amount.h"
// Copyright (c) 2026 Johnbillgate / Timecoin Developers
// Distributed under the MIT software license.
// SPDX-License-Identifier: MIT
//
// ══════════════════════════════════════════════════════════════════
//  TIMECOIN — The Spiral Subsidy Engine
//  Fibonacci-based block reward emission replacing Bitcoin's halving.
//
//  How it works:
//    - Season 1 lasts 1 month  (Fibonacci index 1) → 1 × BLOCKS_PER_MONTH
//    - Season 2 lasts 1 month  (Fibonacci index 2)
//    - Season 3 lasts 2 months (Fibonacci index 3)
//    - Season N lasts F(N) months
//    - Each season: new_reward = prev_reward / φ (1.6180339887...)
//    - Initial reward: 377 TC/block (Fibonacci #14)
//    - This continues for ~127 years until MAX_MONEY is approached
// ══════════════════════════════════════════════════════════════════

#include "subsidy.h"
#include "consensus/params.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <numeric>
#include <vector>

// ─── SEASON TABLE GENERATION ─────────────────────────────────────
//
// Generates the full season table at startup.
// Each season: duration = Fibonacci(n) months, reward = prev_reward / φ
//
// We stop generating seasons when:
//   1. The cumulative supply would exceed MAX_MONEY, OR
//   2. The per-block reward rounds to 0 Instants (dust threshold)
//
// Max precision: 1 Instant = 10^-8 TC

struct SeasonEntry {
    int     season_index;
    int64_t start_block;
    int64_t end_block;          // inclusive
    int64_t duration_blocks;
    int64_t reward_instants;    // reward per block in Instants
    double  cumulative_supply;  // running total in TC
};

// Global season table — built once at program start
static std::vector<SeasonEntry> g_season_table;
static bool g_table_built = false;

// Fibonacci generator — returns F(n), capped to prevent overflow
static int64_t FibonacciN(int n)
{
    if (n <= 0) return 0;
    if (n == 1 || n == 2) return 1;
    int64_t a = 1, b = 1;
    for (int i = 3; i <= n; ++i) {
        int64_t c = a + b;
        if (c < 0) return INT64_MAX; // overflow cap
        a = b;
        b = c;
    }
    return b;
}

// Build the complete season table.
// Called once during node init — takes < 1ms.
void BuildSeasonTable()
{
    if (g_table_built) return;
    g_table_built = true;
    g_season_table.clear();

    constexpr double PHI             = 1.6180339887498948482;
    constexpr double COIN_D          = 100000000.0;
    constexpr double MAX_MONEY_TC    = 16180339.887;
    constexpr int64_t BLOCKS_PER_MONTH = 4380; // 600s blocks, 30.4375 days/month

    double   reward_tc    = 377.0;          // Season 1 reward in TC
    double   total_supply = 0.0;
    int64_t  current_block = 0;

    for (int season = 1; ; ++season)
    {
        int64_t fib_months      = FibonacciN(season);
        int64_t duration_blocks = fib_months * BLOCKS_PER_MONTH;
        int64_t reward_instants = static_cast<int64_t>(reward_tc * COIN_D);

        // Stop if reward is below dust (< 1 Instant)
        if (reward_instants < 1) break;

        double season_supply = (double)duration_blocks * reward_tc;

        // Clamp last season if we'd exceed MAX_MONEY
        if (total_supply + season_supply > MAX_MONEY_TC) {
            double remaining = MAX_MONEY_TC - total_supply;
            if (remaining < 0.0) break;
            duration_blocks = std::max(static_cast<int64_t>(1),
                static_cast<int64_t>(remaining / reward_tc));
            season_supply = (double)duration_blocks * reward_tc;
        }

        SeasonEntry entry;
        entry.season_index    = season;
        entry.start_block     = current_block;
        entry.duration_blocks = duration_blocks;
        entry.end_block       = current_block + duration_blocks - 1;
        entry.reward_instants = reward_instants;
        entry.cumulative_supply = total_supply + season_supply;

        g_season_table.push_back(entry);

        total_supply  += season_supply;
        current_block += duration_blocks;

        // Advance to next Fibonacci season reward
        reward_tc /= PHI;

        // Safety: stop after 200 seasons (never reached)
        if (season >= 200) break;
    }
}

// ─── CORE SUBSIDY FUNCTION ────────────────────────────────────────
//
// GetBlockSubsidy(nHeight, params)
//   Returns reward in Instants for a given block height.
//   This replaces Bitcoin's GetBlockSubsidy() halving function.

CAmount GetBlockSubsidy(int nHeight, const Consensus::Params& consensusParams)
{
    // Ensure table is ready
    if (!g_table_built) BuildSeasonTable();

    // Genesis block — coinbase has no subsidy claim
    if (nHeight == 0) return 0;

    // Binary search through season table
    for (const auto& season : g_season_table) {
        if (nHeight >= season.start_block && nHeight <= season.end_block) {
            return season.reward_instants;
        }
    }

    // Past all seasons → tail emission = 0 (full scarcity)
    return 0;
}

// ─── SEASON INFO QUERY ────────────────────────────────────────────
// Returns which season a block belongs to.
int GetBlockSeason(int nHeight)
{
    if (!g_table_built) BuildSeasonTable();
    for (const auto& s : g_season_table) {
        if (nHeight >= s.start_block && nHeight <= s.end_block)
            return s.season_index;
    }
    return -1; // post-emission
}

// ─── CUMULATIVE SUPPLY QUERY ─────────────────────────────────────
// Returns approximate circulating supply (in Instants) at block height.
int64_t GetCumulativeSupply(int nHeight)
{
    if (!g_table_built) BuildSeasonTable();
    int64_t supply = 0;
    for (const auto& s : g_season_table) {
        if (nHeight >= s.end_block) {
            // Full season already emitted
            supply += s.duration_blocks * s.reward_instants;
        } else if (nHeight >= s.start_block) {
            // Partial season
            int64_t blocks_in_season = nHeight - s.start_block + 1;
            supply += blocks_in_season * s.reward_instants;
            break;
        }
    }
    return supply;
}


// ─── ANTI-ACCELERATION GATE ──────────────────────────────────────
//
// If blocks arrive faster than TARGET_BLOCK_TIME on average over the
// last RETARGET_INTERVAL blocks, difficulty multiplier is adjusted
// using the Fibonacci sequence (not linearly as in Bitcoin).
//
// fib_step = how many Fibonacci steps to advance difficulty

int GetFibonacciDifficultyStep(int64_t nActualTimespan, int64_t nTargetTimespan)
{
    // Standard ratio: how fast/slow we are
    double ratio = (double)nActualTimespan / (double)nTargetTimespan;

    if (ratio >= 1.0) {
        // Blocks are slower than target → reduce difficulty (1 Fibonacci step back)
        return -1;
    }

    // Blocks are faster than target
    // Map speed factor to Fibonacci steps (1φ, 2φ², 3φ³...)
    constexpr double PHI = 1.6180339887498948482;
    int steps = 0;
    double threshold = 1.0 / PHI;

    while (ratio < threshold && steps < 10) {
        steps++;
        threshold /= PHI;
    }
    return std::max(1, steps);
}

// Apply Fibonacci difficulty step to nBits target
// Returns new difficulty target
uint32_t ApplyFibonacciDifficulty(uint32_t nBits, int fib_step)
{
    // Convert compact nBits to a multiplier
    // Each step multiplies difficulty by φ^step
    constexpr double PHI = 1.6180339887498948482;
    double multiplier = std::pow(PHI, std::abs(fib_step));

    if (fib_step > 0) {
        // Increase difficulty (harder)
        // nTarget /= multiplier → lower target = higher difficulty
        // In compact form: shift mantissa
    } else {
        // Decrease difficulty (easier)
        // nTarget *= multiplier
    }
    // NOTE: Full implementation integrates with Bitcoin's arith_uint256 arithmetic.
    // See: src/pow.cpp :: CalculateNextWorkRequired()
    return nBits; // placeholder — implemented in pow.cpp
}

// ─── DIAGNOSTIC: Print Season Table ──────────────────────────────
#ifdef TIMECOIN_DEBUG
#include <cstdio>
void PrintSeasonTable()
{
    if (!g_table_built) BuildSeasonTable();
    printf("\n══════════════════════════════════════════════════════\n");
    printf(" TIMECOIN SPIRAL SUBSIDY TABLE\n");
    printf("══════════════════════════════════════════════════════\n");
    printf(" %-6s %-12s %-12s %-16s %-20s %-16s\n",
        "Season", "StartBlock", "EndBlock", "Reward(TC)", "Duration(months)", "CumSupply(TC)");
    printf("──────────────────────────────────────────────────────\n");
    for (const auto& s : g_season_table) {
        printf(" %-6d %-12lld %-12lld %-16.8f %-20lld %-16.3f\n",
            s.season_index,
            (long long)s.start_block,
            (long long)s.end_block,
            (double)s.reward_instants / 100000000.0,
            (long long)(s.duration_blocks / 4380),
            s.cumulative_supply);
    }
    printf("══════════════════════════════════════════════════════\n\n");
}
#endif
