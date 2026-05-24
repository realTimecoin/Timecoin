#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════
# TIMECOIN — Script d'installation automatique
# Usage : bash install_timecoin_patches.sh
# Doit être lancé depuis le dossier Bitcoin Core cloné
# ═══════════════════════════════════════════════════════
set -e
GREEN='\033[0;32m'; GOLD='\033[0;33m'; NC='\033[0m'
ok() { echo -e "${GREEN}[✓]${NC} $1"; }
log() { echo -e "${GOLD}[TC]${NC} $1"; }

REPO="https://raw.githubusercontent.com/realTimecoin/Timecoin/master"

[[ -f "src/validation.cpp" ]] || { echo "Lance ce script depuis le dossier Bitcoin Core !"; exit 1; }

log "Téléchargement des fichiers Timecoin..."

# Fichiers consensus
curl -sL "$REPO/src/consensus/subsidy.cpp" -o src/consensus/subsidy.cpp; ok "subsidy.cpp"
curl -sL "$REPO/src/consensus/subsidy.h"   -o src/consensus/subsidy.h;   ok "subsidy.h"
curl -sL "$REPO/src/consensus/amount.h"    -o src/consensus/amount.h;    ok "amount.h"
curl -sL "$REPO/src/consensus/params.h"    -o src/consensus/params.h;    ok "params.h"

# Fichiers principaux
curl -sL "$REPO/src/chainparams.cpp"       -o src/chainparams.cpp;       ok "chainparams.cpp"
curl -sL "$REPO/src/pow.cpp"               -o src/pow.cpp;               ok "pow.cpp"
curl -sL "$REPO/src/golden_vault.cpp"      -o src/golden_vault.cpp;      ok "golden_vault.cpp"
curl -sL "$REPO/src/golden_vault.h"        -o src/golden_vault.h;        ok "golden_vault.h"
curl -sL "$REPO/src/timecoin_validation.cpp" -o src/timecoin_validation.cpp; ok "timecoin_validation.cpp"
curl -sL "$REPO/src/validation.cpp"        -o src/validation.cpp;        ok "validation.cpp"
curl -sL "$REPO/src/validation.h"          -o src/validation.h;          ok "validation.h"
curl -sL "$REPO/src/Makefile.am"           -o src/Makefile.am;           ok "Makefile.am"

# Fichiers patches
curl -sL "$REPO/src/index/coinstatsindex.cpp" -o src/index/coinstatsindex.cpp; ok "coinstatsindex.cpp"
curl -sL "$REPO/src/rpc/blockchain.cpp"       -o src/rpc/blockchain.cpp;       ok "rpc/blockchain.cpp"
curl -sL "$REPO/src/node/miner.cpp"           -o src/node/miner.cpp;           ok "node/miner.cpp"

# MAX_MONEY
sed -i 's/static constexpr CAmount MAX_MONEY = 21000000 \* COIN;/static constexpr CAmount MAX_MONEY = 1618033988700000000LL;/' src/consensus/amount.h
ok "MAX_MONEY patché"

echo ""
echo -e "${GOLD}✅ Tous les fichiers Timecoin installés !${NC}"
echo -e "${GOLD}Lance maintenant : make -j\$(nproc)${NC}"
