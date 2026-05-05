// Copyright (c) 2026 Johnbillgate / Timecoin Developers
// Distributed under the MIT software license.
#pragma once
#ifndef TIMECOIN_GOLDEN_VAULT_H
#define TIMECOIN_GOLDEN_VAULT_H

#include <cstdint>
#include <string>
#include "script/script.h"
#include "pubkey.h"
#include "uint256.h"

namespace GoldenVault {

/** Valid Fibonacci lock durations in months: 1,2,3,5,8,13,21,34,55,89,144 */
extern const int64_t VAULT_LOCK_MONTHS[];
extern const int     NUM_VAULT_TIERS;

struct LockTier {
    int64_t     months;
    int64_t     blocks;
    int         tier_index;
    std::string label;
};

struct VaultInfo {
    uint256     txid;
    uint32_t    vout_index;
    int64_t     amount_instants;
    int64_t     lock_months;
    int64_t     lock_start_block;
    int64_t     unlock_block;
    int64_t     fee_instants;
    bool        is_unlocked;
    std::string tier_label;
};

/** Returns the LockTier for a given month count, or nullptr if invalid. */
const LockTier* ValidateLockPeriod(int64_t lock_months);

/** Calculates vault fee in Instants for a given tier: BASE_FEE × φ^tier_index */
int64_t CalculateVaultFee(const LockTier* tier);

/** Builds the locking scriptPubKey for a Golden Vault output. */
CScript BuildVaultLockScript(int64_t current_height,
                             int64_t lock_months,
                             const CKeyID& owner_pubkey_hash);

/** Builds the scriptSig/witness to spend a Golden Vault output. */
CScript BuildVaultUnlockScript(const std::vector<unsigned char>& signature,
                               const std::vector<unsigned char>& pubkey);

/** Validates that a vault output can be spent at nSpendHeight. */
bool VerifyVaultSpend(const CScript& lockScript,
                      int64_t nSpendHeight,
                      std::string& error);

/** Builds the OP_RETURN sequestration output that holds fee metadata on-chain. */
CScript BuildFeeSequestrationScript(const uint256& vault_txid,
                                    int64_t unlock_block,
                                    int64_t total_fee_instants);

/**
 * Returns Instants owed to the miner at current_block from this vault's fee pool.
 * Last block absorbs integer division remainder — no Instant is ever lost.
 */
int64_t GetVaultFeeForBlock(int64_t total_fee_instants,
                            int64_t lock_start_block,
                            int64_t unlock_block,
                            int64_t current_block);

/** Creates a VaultInfo descriptor for wallet/explorer use. */
VaultInfo CreateVaultInfo(const uint256& txid,
                          uint32_t vout,
                          int64_t amount_instants,
                          int64_t lock_months,
                          int64_t current_block);

/** Human-readable vault status string. */
std::string VaultStatusString(const VaultInfo& v, int64_t current_block);

#ifdef TIMECOIN_DEBUG
bool VerifyFeeDistributionInvariant(int64_t total_fee_instants,
                                    int64_t lock_start_block,
                                    int64_t unlock_block);
#endif

} // namespace GoldenVault

#endif // TIMECOIN_GOLDEN_VAULT_H
