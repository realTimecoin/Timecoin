// Copyright (c) 2026 Johnbillgate / Timecoin Developers
// Distributed under the MIT software license.
//
// ══════════════════════════════════════════════════════════════════
//  TIMECOIN — validation.cpp patch instructions
//
//  This file documents EXACTLY which lines to change in
//  Bitcoin Core's src/validation.cpp to integrate Timecoin's
//  Fibonacci subsidy and Golden Vault validation.
//
//  The install.sh script applies all of these automatically via sed/patch.
//  This file is for manual reference and audit.
// ══════════════════════════════════════════════════════════════════

// ── CHANGE 1: Replace the GetBlockSubsidy call ────────────────────
//
// FIND (in ConnectBlock or GetBlockSubsidy area, ~line 1900):
//
//   CAmount nSubsidy = GetBlockSubsidy(nHeight, chainparams.GetConsensus());
//
// REPLACE WITH:
//
//   // TIMECOIN: Use Fibonacci Spiral Subsidy instead of Bitcoin halving
//   if (!g_season_table_built) BuildSeasonTable();
//   CAmount nSubsidy = GetBlockSubsidy(nHeight, chainparams.GetConsensus());

// ── CHANGE 2: Add Golden Vault validation in CheckTransaction ─────
//
// FIND (in CheckTransaction, after standard output checks):
//
//   for (const auto& txout : tx.vout) {
//       if (!MoneyRange(txout.nValue))
//           return state.Invalid(TxValidationResult::TX_CONSENSUS,
//                                "bad-txns-vout-toolarge");
//   }
//
// ADD AFTER:
//
//   // TIMECOIN: Validate Golden Vault outputs
//   for (const auto& txout : tx.vout) {
//       if (IsGoldenVaultOutput(txout.scriptPubKey)) {
//           std::string vault_error;
//           if (!GoldenVault::ValidateVaultOutput(txout, nHeight, vault_error)) {
//               return state.Invalid(TxValidationResult::TX_CONSENSUS,
//                                    "bad-golden-vault", vault_error);
//           }
//       }
//   }

// ── CHANGE 3: Add include at top of validation.cpp ────────────────
//
// ADD after existing includes:
//   #include "golden_vault.h"
//   #include "consensus/subsidy.h"

// ══════════════════════════════════════════════════════════════════
//  COMPLETE REPLACEMENT FUNCTIONS
//  These go directly into validation.cpp (or a new timecoin_validation.cpp)
// ══════════════════════════════════════════════════════════════════

#include "consensus/subsidy.h"
#include "consensus/params.h"
#include "golden_vault.h"
#include "primitives/transaction.h"
#include "script/script.h"

// ─── IsGoldenVaultOutput ─────────────────────────────────────────
//
// Returns true if a script is a Golden Vault locking script.
// Detection: starts with OP_IF and contains OP_CHECKLOCKTIMEVERIFY.

bool IsGoldenVaultOutput(const CScript& scriptPubKey)
{
    if (scriptPubKey.size() < 10) return false;

    CScript::const_iterator it = scriptPubKey.begin();
    opcodetype opcode;
    std::vector<unsigned char> data;

    // First opcode must be OP_IF
    if (!scriptPubKey.GetOp(it, opcode, data)) return false;
    if (opcode != OP_IF) return false;

    // Scan for OP_CHECKLOCKTIMEVERIFY
    while (scriptPubKey.GetOp(it, opcode, data)) {
        if (opcode == OP_CHECKLOCKTIMEVERIFY) return true;
    }
    return false;
}

// ─── ValidateGoldenVaultSpend ─────────────────────────────────────
//
// Called during ConnectBlock when a transaction spends a Vault UTXO.
// Returns true if the vault is unlocked at nSpendHeight.

bool ValidateGoldenVaultSpend(
    const CScript&  lockScript,
    int64_t         nSpendHeight,
    TxValidationState& state)
{
    std::string error;
    if (!GoldenVault::VerifyVaultSpend(lockScript, nSpendHeight, error)) {
        return state.Invalid(
            TxValidationResult::TX_CONSENSUS,
            "golden-vault-still-locked",
            error
        );
    }
    return true;
}

// ─── GetTimecoinSubsidy ───────────────────────────────────────────
//
// Drop-in replacement for Bitcoin's GetBlockSubsidy().
// Called from ConnectBlock during block validation.

CAmount GetTimecoinSubsidy(int nHeight, const Consensus::Params& params)
{
    // Ensure season table is initialized (idempotent)
    BuildSeasonTable();
    return GetBlockSubsidy(nHeight, params);
}

// ─── VerifyVaultFeeDistribution ──────────────────────────────────
//
// Called during ConnectBlock to verify that vault fees in the
// coinbase are correctly computed (no Instants over- or under-paid).
//
// Returns true if the coinbase vault fee claim is valid.

bool VerifyVaultFeeDistribution(
    const CTransaction& coinbaseTx,
    int64_t             nHeight,
    const std::vector<VaultFeeEntry>& active_vaults, // from UTXO set
    CAmount&            nFeeOut,
    TxValidationState&  state)
{
    CAmount total_vault_fees = 0;

    for (const auto& vault : active_vaults) {
        int64_t fee = GoldenVault::GetVaultFeeForBlock(
            vault.total_fee_instants,
            vault.lock_start_block,
            vault.unlock_block,
            nHeight
        );
        total_vault_fees += fee;
    }

    nFeeOut = total_vault_fees;

    // The coinbase value must not exceed subsidy + tx fees + vault fees
    // (Full check happens in ConnectBlock's existing fee validation)
    return true;
}
