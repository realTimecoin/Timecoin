// Copyright (c) 2026 Johnbillgate / Timecoin Developers
// Distributed under the MIT software license.
//
// TIMECOIN — Proof of Work : LWMA Difficulty Algorithm
//
// Remplace l'ajustement Fibonacci basique par LWMA
// (Linear Weighted Moving Average) — standard de l'industrie
// pour les coins SHA-256 susceptibles aux attaques de pools.
//
// Fenêtre : 144 blocs (~1 jour)
// Recalcul : à chaque bloc
// Effet : monte vite, descend vite → pas de "hit-and-run" possible

#include <pow.h>
#include <arith_uint256.h>
#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>
#include <algorithm>
#include <cmath>

// ─── LWMA PARAMETERS ─────────────────────────────────────────────
static constexpr int64_t LWMA_WINDOW       = 144;   // blocs dans la fenêtre
static constexpr int64_t TARGET_BLOCK_TIME = 600;   // 10 minutes en secondes
static constexpr int64_t MAX_FUTURE_TIME   = 2 * TARGET_BLOCK_TIME;

// ─── LWMA CORE FUNCTION ──────────────────────────────────────────
//
// Calcule la nouvelle cible de difficulté selon LWMA.
//
// Principe :
//   - On regarde les N derniers blocs (LWMA_WINDOW)
//   - Chaque bloc reçoit un poids proportionnel à son rang (bloc récent = poids élevé)
//   - On calcule la moyenne pondérée des temps de bloc
//   - On ajuste la cible proportionnellement
//
// Résultat :
//   - Si les blocs arrivent trop vite → difficulté monte rapidement
//   - Si les blocs arrivent trop lentement → difficulté descend rapidement
//   - Réaction en ~10 blocs au lieu de 2016 (Bitcoin) ou 144 (notre ancien système)

unsigned int LWMACalculateNextWorkRequired(
    const CBlockIndex* pindexLast,
    const Consensus::Params& params)
{
    const arith_uint256 powLimit = UintToArith256(params.powLimit);

    // Pas assez de blocs pour LWMA → retourner powLimit
    if (pindexLast->nHeight < LWMA_WINDOW) {
        return powLimit.GetCompact();
    }

    // ── Collecter les temps et cibles des N derniers blocs ────────
    arith_uint256 sumTarget;
    int64_t       sumWeightedTime = 0;
    int64_t       totalWeight     = 0;
    int64_t       t               = 0;

    const CBlockIndex* pindex = pindexLast;

    // On remonte LWMA_WINDOW blocs
    std::vector<const CBlockIndex*> blocks;
    blocks.reserve(LWMA_WINDOW + 1);
    for (int i = 0; i <= LWMA_WINDOW && pindex != nullptr; i++) {
        blocks.push_back(pindex);
        pindex = pindex->pprev;
    }

    // ── Calculer LWMA ─────────────────────────────────────────────
    for (int i = 1; i <= LWMA_WINDOW; i++) {
        if ((int)blocks.size() <= i) break;

        // Temps entre deux blocs consécutifs
        int64_t solveTime = blocks[i-1]->GetBlockTime() - blocks[i]->GetBlockTime();

        // Clamp : éviter les temps aberrants (négatifs ou trop grands)
        solveTime = std::max(std::min(solveTime, MAX_FUTURE_TIME), (int64_t)1);

        // Poids linéaire : bloc le plus récent a le poids le plus élevé
        int64_t weight = i; // poids = rang (1 à LWMA_WINDOW)

        sumWeightedTime += solveTime * weight;
        totalWeight     += weight;

        // Accumuler les cibles (pondérées également)
        arith_uint256 target;
        target.SetCompact(blocks[i-1]->nBits);
        sumTarget += target / LWMA_WINDOW;
    }

    if (totalWeight == 0) return powLimit.GetCompact();

    // ── Calculer la moyenne pondérée des temps ────────────────────
    int64_t lwmaTime = sumWeightedTime / totalWeight;
    lwmaTime = std::max(lwmaTime, (int64_t)1);

    // ── Ajuster la cible ──────────────────────────────────────────
    // nouvelle_cible = moyenne_cibles × (lwmaTime / TARGET_BLOCK_TIME)
    // Si lwmaTime > TARGET → blocs trop lents → cible monte (plus facile)
    // Si lwmaTime < TARGET → blocs trop rapides → cible baisse (plus dur)
    arith_uint256 newTarget = sumTarget;
    newTarget *= lwmaTime;
    newTarget /= TARGET_BLOCK_TIME;

    // ── Appliquer les limites ─────────────────────────────────────
    if (newTarget > powLimit) newTarget = powLimit;

    // Limite basse : ne jamais être 4x plus difficile que le bloc précédent
    // (protection contre les oscillations extrêmes)
    arith_uint256 lastTarget;
    lastTarget.SetCompact(pindexLast->nBits);
    arith_uint256 minTarget = lastTarget / 4;
    if (newTarget < minTarget) newTarget = minTarget;

    return newTarget.GetCompact();
}

// ─── ENTRY POINT ─────────────────────────────────────────────────
unsigned int GetNextWorkRequired(
    const CBlockIndex* pindexLast,
    const CBlockHeader* pblock,
    const Consensus::Params& params)
{
    assert(pindexLast != nullptr);
    const arith_uint256 powLimit = UintToArith256(params.powLimit);

    // Genesis + premiers blocs → difficulté minimale
    if (pindexLast->nHeight == 0) {
        return powLimit.GetCompact();
    }

    // Testnet : si le bloc prend plus de 2x le temps cible
    // → autoriser un bloc à difficulté minimale (évite le blocage)
    if (params.fPowAllowMinDifficultyBlocks) {
        if (pblock->GetBlockTime() >
            pindexLast->GetBlockTime() + params.nPowTargetSpacing * 2) {
            return powLimit.GetCompact();
        }
    }

    // Regtest : pas de recalcul
    if (params.fPowNoRetargeting) {
        return pindexLast->nBits;
    }

    // LWMA : recalcul à chaque bloc
    return LWMACalculateNextWorkRequired(pindexLast, params);
}

// ─── VALIDATION ──────────────────────────────────────────────────
bool CheckProofOfWork(uint256 hash, unsigned int nBits,
                      const Consensus::Params& params)
{
    bool fNegative, fOverflow;
    arith_uint256 bnTarget;
    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);
    if (fNegative || bnTarget == 0 || fOverflow ||
        bnTarget > UintToArith256(params.powLimit))
        return false;
    return UintToArith256(hash) <= bnTarget;
}

// ─── PERMITTED DIFFICULTY TRANSITION ─────────────────────────────
bool PermittedDifficultyTransition(
    const Consensus::Params& params,
    int64_t height,
    uint32_t old_nbits,
    uint32_t new_nbits)
{
    // LWMA recalcule à chaque bloc → toute transition est permise
    if (params.fPowNoRetargeting) return old_nbits == new_nbits;
    return true;
}

// ─── COMPATIBILITY ALIAS ─────────────────────────────────────────
// Requis par test/fuzz/pow.cpp
unsigned int CalculateNextWorkRequired(
    const CBlockIndex* pindexLast,
    int64_t nFirstBlockTime,
    const Consensus::Params& params)
{
    return LWMACalculateNextWorkRequired(pindexLast, params);
}
