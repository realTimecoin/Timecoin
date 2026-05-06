# ⟁ TIMECOIN (TC)

> *"June 2026 – Central banks printed years, we print provable time. Timecoin."*

**Timecoin** est une cryptomonnaie décentralisée basée sur sa propre blockchain (fork Bitcoin Core v26.0), dont chaque paramètre est régi par le **Nombre d'Or** (φ = 1.6180339887) et la **suite de Fibonacci**.

## Paramètres Fondamentaux

| Paramètre | Valeur |
|-----------|--------|
| Ticker | TC |
| Offre Maximale | 16 180 339.887 TC (φ × 10⁷) |
| Récompense Genesis | 377 TC/bloc (F14) |
| Algorithme | SHA-256D |
| Temps de bloc | 600 secondes |
| Ajustement difficulté | LWMA (144 blocs) |
| Pré-mine | 0 TC — Fair Launch total |
| Port Mainnet | 16180 |
| Genesis | Juin 2026 |

## Architecture Timecoin

### 1. Fibonacci Emission (Spiral Subsidy)
Remplace le halving Bitcoin par une division par φ à chaque saison. La durée des saisons suit la suite de Fibonacci en mois (1,1,2,3,5,8,13...). Durée totale : ~127 ans.

### 2. LWMA Difficulty Algorithm
Ajustement de la difficulté à **chaque bloc** sur une fenêtre de 144 blocs. Protection contre les attaques hit-and-run des grands pools SHA-256.

### 3. Golden Vault Protocol
Time-lock natif basé sur OP_CHECKLOCKTIMEVERIFY avec paliers Fibonacci exclusivement : 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144 mois.

## Compilation

```bash
# Cloner Bitcoin Core v26.0
git clone --depth 1 --branch v26.0 https://github.com/bitcoin/bitcoin.git timecoin-core
cd timecoin-core

# Copier les fichiers Timecoin
cp /path/to/Timecoin/src/* src/
cp /path/to/Timecoin/src/consensus/* src/consensus/

# Compiler
./autogen.sh
./configure --with-gui=no --disable-tests --disable-bench
make -j$(nproc)
```

## Statut

- ✅ Testnet actif
- ✅ 377 TC/bloc confirmé
- ✅ LWMA validé
- ✅ Golden Vault testé
- ⏳ Mainnet — Juin 2026

## Licence

MIT License — Copyright (c) 2026 Johnbillgate / Timecoin Developers

*"Dans la Spirale, la patience est la plus haute forme de valeur." — Johnbillgate*
