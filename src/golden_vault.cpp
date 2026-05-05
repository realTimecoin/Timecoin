// Copyright (c) 2026 Johnbillgate / Timecoin Developers
// Distributed under the MIT software license.
//
// TIMECOIN — Golden Vault Protocol
// Time-lock natif basé sur la suite de Fibonacci.
// Paliers : 1,2,3,5,8,13,21,34,55,89,144 mois
// Fee = BASE_FEE × φ^tier_index
// Invariant : SUM(tous les blocs) == total_fee_instants (zéro Instant perdu)

#include "golden_vault.h"
#include <cmath>
#include <stdexcept>
#include <string>

namespace GoldenVault {

static constexpr double  PHI               = 1.6180339887498948482;
static constexpr int64_t BASE_FEE_INSTANTS = 1000LL;

static const LockTier LOCK_TIERS[] = {
    {   1,    4380,  0, "1 month  [F1]"  },
    {   2,    8760,  1, "2 months [F3]"  },
    {   3,   13140,  2, "3 months [F4]"  },
    {   5,   21900,  3, "5 months [F5]"  },
    {   8,   35040,  4, "8 months [F6]"  },
    {  13,   56940,  5, "13 months [F7]" },
    {  21,   91980,  6, "21 months [F8]" },
    {  34,  148920,  7, "34 months [F9]" },
    {  55,  240900,  8, "55 months [F10]"},
    {  89,  389820,  9, "89 months [F11]"},
    { 144,  630720, 10, "144 months [F12]"},
};
static constexpr int NUM_TIERS = sizeof(LOCK_TIERS)/sizeof(LOCK_TIERS[0]);

const LockTier* ValidateLockPeriod(int64_t lock_months) {
    for (int i = 0; i < NUM_TIERS; ++i)
        if (LOCK_TIERS[i].months == lock_months) return &LOCK_TIERS[i];
    return nullptr;
}

int64_t CalculateVaultFee(const LockTier* tier) {
    if (!tier) return -1;
    int64_t fee = static_cast<int64_t>(BASE_FEE_INSTANTS * std::pow(PHI, tier->tier_index));
    return std::max(fee, BASE_FEE_INSTANTS);
}

CScript BuildVaultLockScript(int64_t current_height, int64_t lock_months, const CKeyID& owner) {
    const LockTier* tier = ValidateLockPeriod(lock_months);
    if (!tier) throw std::invalid_argument("GoldenVault: palier Fibonacci invalide.");
    int64_t unlock_height = current_height + tier->blocks;
    CScript script;
    script << OP_IF
               << unlock_height << OP_CHECKLOCKTIMEVERIFY << OP_DROP
               << OP_DUP << OP_HASH160 << ToByteVector(owner)
               << OP_EQUALVERIFY << OP_CHECKSIG
           << OP_ELSE
               << OP_RETURN << std::vector<unsigned char>{'G','V','1'}
           << OP_ENDIF;
    return script;
}

CScript BuildVaultUnlockScript(const std::vector<unsigned char>& sig,
                               const std::vector<unsigned char>& pubkey) {
    CScript script;
    script << sig << pubkey << OP_1;
    return script;
}

bool VerifyVaultSpend(const CScript& lockScript, int64_t nSpendHeight, std::string& error) {
    CScript::const_iterator it = lockScript.begin();
    opcodetype opcode;
    std::vector<unsigned char> data;
    if (!lockScript.GetOp(it, opcode, data) || opcode != OP_IF) {
        error = "GoldenVault: OP_IF attendu"; return false;
    }
    if (!lockScript.GetOp(it, opcode, data)) {
        error = "GoldenVault: hauteur de verrouillage manquante"; return false;
    }
    int64_t lock_height = 0;
    for (size_t i = 0; i < data.size() && i < 8; ++i)
        lock_height |= (static_cast<int64_t>(data[i]) << (8*i));
    if (nSpendHeight < lock_height) {
        error = "GoldenVault: coffre verrouille jusqu'au bloc #" +
                std::to_string(lock_height) +
                " (actuel: #" + std::to_string(nSpendHeight) + ")";
        return false;
    }
    return true;
}

CScript BuildFeeSequestrationScript(const uint256& vault_txid,
                                    int64_t unlock_block,
                                    int64_t total_fee) {
    CScript script;
    std::vector<unsigned char> marker = {'G','V','F','E','E'};
    std::vector<unsigned char> txid_bytes(vault_txid.begin(), vault_txid.end());
    std::vector<unsigned char> unlock_bytes(8), fee_bytes(8);
    for (int i = 0; i < 8; ++i) {
        unlock_bytes[i] = (unlock_block >> (8*i)) & 0xFF;
        fee_bytes[i]    = (total_fee   >> (8*i)) & 0xFF;
    }
    script << OP_RETURN << marker << txid_bytes << unlock_bytes << fee_bytes;
    return script;
}

// Invariant garanti : SUM de tous les blocs == total_fee exactement.
// Le dernier bloc absorbe le reste de la division entière.
int64_t GetVaultFeeForBlock(int64_t total_fee, int64_t lock_start,
                            int64_t unlock_block, int64_t current_block) {
    if (current_block < lock_start || current_block > unlock_block) return 0;
    int64_t total_blocks = unlock_block - lock_start + 1;
    if (total_blocks <= 1) return total_fee;
    int64_t base_fee  = total_fee / total_blocks;
    int64_t remainder = total_fee % total_blocks;
    if (current_block == unlock_block) return base_fee + remainder;
    return base_fee;
}

VaultInfo CreateVaultInfo(const uint256& txid, uint32_t vout,
                          int64_t amount, int64_t lock_months,
                          int64_t cur_block) {
    const LockTier* tier = ValidateLockPeriod(lock_months);
    if (!tier) throw std::invalid_argument("Palier invalide");
    VaultInfo info;
    info.txid             = txid;
    info.vout_index       = vout;
    info.amount_instants  = amount;
    info.lock_months      = lock_months;
    info.lock_start_block = cur_block;
    info.unlock_block     = cur_block + tier->blocks;
    info.fee_instants     = CalculateVaultFee(tier);
    info.is_unlocked      = false;
    info.tier_label       = tier->label;
    return info;
}

std::string VaultStatusString(const VaultInfo& v, int64_t cur_block) {
    int64_t remaining = v.unlock_block - cur_block;
    if (remaining <= 0)
        return "DEVERROUILLE au bloc #" + std::to_string(v.unlock_block);
    return "VERROUILLE — s'ouvre au bloc #" + std::to_string(v.unlock_block) +
           " (~" + std::to_string(static_cast<int>(remaining / 4380.0)) +
           " mois restants)";
}

} // namespace GoldenVault
