# ⟁ TIMECOIN (TC)

> *"June 2026 – Central banks printed years, we print provable time. Timecoin."*

**Timecoin** is a decentralized cryptocurrency based on its own blockchain (fork of Bitcoin Core v26.0), where every parameter is governed by the **Golden Ratio** (φ = 1.6180339887) and the **Fibonacci sequence**.

## Fundamental Parameters

| Parameter | Value |
|-----------|-------|
| Ticker | TC |
| Maximum Supply | 16,180,339.887 TC (φ × 10⁷) |
| Genesis Reward | 377 TC/block (F14) |
| Algorithm | SHA-256D |
| Block time | 600 seconds |
| Difficulty adjustment | LWMA (144 blocks) |
| Premine | 0 TC — Fully fair launch |
| Mainnet Port | 16180 |
| Genesis | June 2026 |

## Timecoin Architecture

### 1. Fibonacci Emission (Spiral Subsidy)
Replaces Bitcoin's halving with a division by φ each season. The duration of seasons follows the Fibonacci sequence in months (1,1,2,3,5,8,13...). Total duration: ~127 years.

### 2. LWMA Difficulty Algorithm
Difficulty adjusts **every block** over a 144‑block window. Protection against hit‑and‑run attacks by large SHA‑256 mining pools.

### 3. Golden Vault Protocol
Native time‑lock based on `OP_CHECKLOCKTIMEVERIFY` exclusively using Fibonacci steps: 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144 months.

## Compilation

```bash
# Clone Bitcoin Core v26.0
git clone --depth 1 --branch v26.0 https://github.com/bitcoin/bitcoin.git timecoin-core
cd timecoin-core

# Copy Timecoin files
cp /path/to/Timecoin/src/* src/
cp /path/to/Timecoin/src/consensus/* src/consensus/

# Build
./autogen.sh
./configure --with-gui=no --disable-tests --disable-bench
make -j$(nproc)Status

    ✅ Testnet active

    ✅ 377 TC/block confirmed

    ✅ LWMA validated

    ✅ Golden Vault tested

    ⏳ Mainnet — June 2026License

MIT License — Copyright (c) 2026 Johnbillgate / Timecoin Developers

"In the Spiral, patience is the highest form of value." — Johnbillgate
