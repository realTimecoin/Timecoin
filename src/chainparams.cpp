#include <common/args.h>
#include <chainparamsbase.h>
// Copyright (c) 2026 Johnbillgate / Timecoin Developers
// Distributed under the MIT software license.

#include <chainparams.h>
#include <chainparamsseeds.h>
#include <consensus/merkle.h>
#include <deploymentinfo.h>
#include <hash.h>
#include <script/interpreter.h>
#include <util/chaintype.h>
#include <util/strencodings.h>
#include <cassert>
#include <cstring>

static CBlock CreateGenesisBlock(const char* pszTimestamp,
    const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce,
    uint32_t nBits, int32_t nVersion, CAmount genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4)
        << std::vector<unsigned char>((const unsigned char*)pszTimestamp,
           (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;
    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

static CBlock CreateTimecoinGenesisBlock(uint32_t nTime, uint32_t nNonce,
    uint32_t nBits, int32_t nVersion, CAmount genesisReward)
{
    // Genesis message — prouve qu'aucun pré-mine n'a eu lieu avant ce jour
    const char* pszTimestamp =
        "June 2026 - Central banks printed years, we print provable time. Timecoin.";
    // OP_RETURN : sortie non-dépensable = 0 TC pré-miné
    const CScript genesisOutputScript = CScript() << OP_RETURN;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript,
                              nTime, nNonce, nBits, nVersion, genesisReward);
}

class CMainParams : public CChainParams {
public:
    CMainParams() {
        m_chain_type = ChainType::MAIN;

        // ── Consensus ──────────────────────────────────────────────
        consensus.nSubsidyHalvingInterval    = std::numeric_limits<int>::max();
        consensus.BIP34Height                = 1;
        consensus.BIP34Hash                  = uint256();
        consensus.BIP65Height                = 1;
        consensus.BIP66Height                = 1;
        consensus.CSVHeight                  = 1;
        consensus.SegwitHeight               = 1;
        consensus.MinBIP9WarningHeight       = 0;
        consensus.powLimit = uint256S(
            "00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan         = 144 * 600; // 1 jour
        consensus.nPowTargetSpacing          = 600;       // 10 minutes
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting          = false;
        consensus.nRuleChangeActivationThreshold = 1815;
        consensus.nMinerConfirmationWindow   = 2016;

        // ── Deployments ────────────────────────────────────────────
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime =
            Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout =
            Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0;

        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime =
            Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout =
            Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 0;

        consensus.nMinimumChainWork  = uint256S("0x00");
        consensus.defaultAssumeValid = uint256S("0x00");

        // ── Genesis Block ──────────────────────────────────────────
        // nNonce = 0 : PLACEHOLDER — remplacer après genesis_miner.py
        genesis = CreateTimecoinGenesisBlock(
            1780272000,  // 2026-06-01 00:00:00 UTC
            0,           // nNonce — à remplacer
            0x1d00ffff,  // difficulté initiale
            1,           // nVersion
            0            // 0 TC — OP_RETURN, aucun pré-mine
        );
        consensus.hashGenesisBlock = genesis.GetHash();

        // ── Réseau P2P ─────────────────────────────────────────────
        // Magic bytes : φ-b-s-a (phi, bona, spiral, aurum)
        pchMessageStart[0] = 0xF1;
        pchMessageStart[1] = 0xB0;
        pchMessageStart[2] = 0x5E;
        pchMessageStart[3] = 0x8A;
        nDefaultPort       = 16180; // φ × 10000
        nPruneAfterHeight  = 100000;
        m_assumed_blockchain_size  = 1;
        m_assumed_chain_state_size = 1;

        // ── Seeds ──────────────────────────────────────────────────
        vSeeds.emplace_back("seed1.timecoin.io");
        vSeeds.emplace_back("seed2.timecoin.io");
        vSeeds.emplace_back("seed3.timecoin.io");

        // ── Adresses ───────────────────────────────────────────────
        // Préfixe "T" pour les adresses Timecoin (version 65)
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 65);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 5);
        base58Prefixes[SECRET_KEY]     = std::vector<unsigned char>(1, 193);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};
        bech32_hrp = "tc"; // adresses natives : tc1q...

        fDefaultConsistencyChecks = false;
        m_is_mockable_chain       = false;

        checkpointData = {{ {0, consensus.hashGenesisBlock} }};
        m_assumeutxo_data = {};
        chainTxData = ChainTxData{ 1780272000, 0, 0 };
    }
};

class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        m_chain_type = ChainType::TESTNET;
        consensus.nSubsidyHalvingInterval    = std::numeric_limits<int>::max();
        consensus.BIP34Height = 1;
        consensus.BIP34Hash   = uint256();
        consensus.BIP65Height = 1;
        consensus.BIP66Height = 1;
        consensus.CSVHeight   = 1;
        consensus.SegwitHeight = 1;
        consensus.MinBIP9WarningHeight = 0;
        consensus.powLimit = uint256S(
            "7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 144 * 600;
        consensus.nPowTargetSpacing  = 600;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting  = false;
        consensus.nRuleChangeActivationThreshold = 1512;
        consensus.nMinerConfirmationWindow = 2016;

        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime =
            Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout =
            Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime =
            Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout =
            Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 0;
        consensus.nMinimumChainWork  = uint256S("0x00");
        consensus.defaultAssumeValid = uint256S("0x00");

        genesis = CreateTimecoinGenesisBlock(
            1780272000, 0, 0x207fffff, 1, 0);
        consensus.hashGenesisBlock = genesis.GetHash();

        pchMessageStart[0] = 0x0B;
        pchMessageStart[1] = 0x11;
        pchMessageStart[2] = 0x09;
        pchMessageStart[3] = 0x07;
        nDefaultPort = 26180;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size  = 1;
        m_assumed_chain_state_size = 1;

        vSeeds.emplace_back("testnet-seed.timecoin.io");

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 127);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 196);
        base58Prefixes[SECRET_KEY]     = std::vector<unsigned char>(1, 255);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};
        bech32_hrp = "tct";

        fDefaultConsistencyChecks = false;
        m_is_mockable_chain = false;
        checkpointData = {{ {0, consensus.hashGenesisBlock} }};
        m_assumeutxo_data = {};
        chainTxData = ChainTxData{ 1780272000, 0, 0 };
    }
};

class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
        m_chain_type = ChainType::REGTEST;
        consensus.nSubsidyHalvingInterval    = std::numeric_limits<int>::max();
        consensus.BIP34Height = 1;
        consensus.BIP34Hash   = uint256();
        consensus.BIP65Height = 1;
        consensus.BIP66Height = 1;
        consensus.CSVHeight   = 0;
        consensus.SegwitHeight = 0;
        consensus.MinBIP9WarningHeight = 0;
        consensus.powLimit = uint256S(
            "7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 144 * 600;
        consensus.nPowTargetSpacing  = 600;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting  = true;
        consensus.nRuleChangeActivationThreshold = 108;
        consensus.nMinerConfirmationWindow = 144;

        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout =
            Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime =
            Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout =
            Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 0;
        consensus.nMinimumChainWork  = uint256S("0x00");
        consensus.defaultAssumeValid = uint256S("0x00");

        genesis = CreateTimecoinGenesisBlock(
            1780272000, 0, 0x207fffff, 1, 0);
        consensus.hashGenesisBlock = genesis.GetHash();

        pchMessageStart[0] = 0xFA;
        pchMessageStart[1] = 0xBF;
        pchMessageStart[2] = 0xB5;
        pchMessageStart[3] = 0xDA;
        nDefaultPort = 36180;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size  = 0;
        m_assumed_chain_state_size = 0;

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 196);
        base58Prefixes[SECRET_KEY]     = std::vector<unsigned char>(1, 239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};
        bech32_hrp = "tcrt";

        fDefaultConsistencyChecks = true;
        m_is_mockable_chain = true;
        checkpointData = {{ {0, consensus.hashGenesisBlock} }};
        m_assumeutxo_data = {};
        chainTxData = ChainTxData{ 0, 0, 0 };
    }
};

std::unique_ptr<const CChainParams> CreateChainParams(
    const ArgsManager& args, const ChainType chain)
{
    switch (chain) {
        case ChainType::MAIN:    return std::make_unique<CMainParams>();
        case ChainType::TESTNET: return std::make_unique<CTestNetParams>();
        case ChainType::REGTEST: return std::make_unique<CRegTestParams>();
        case ChainType::SIGNET:  return std::make_unique<CTestNetParams>(); // fallback
    }
    assert(false);
}

// ── Fonctions globales requises par Bitcoin Core ──────────────────
static std::unique_ptr<const CChainParams> globalChainParams;

const CChainParams& Params() {
    assert(globalChainParams);
    return *globalChainParams;
}

void SelectParams(const ChainType chain) {
    SelectBaseParams(chain);
    globalChainParams = CreateChainParams(gArgs, chain);
}
