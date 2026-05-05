#!/usr/bin/env python3
"""
╔══════════════════════════════════════════════════════════════════╗
║  TIMECOIN — Genesis Block Miner                                  ║
║  Finds the valid nNonce for the Timecoin genesis block.          ║
║                                                                  ║
║  Usage: python3 genesis_miner.py                                 ║
║  Output: Valid nNonce + genesis block hash                       ║
╚══════════════════════════════════════════════════════════════════╝
"""

import hashlib
import struct
import time

# ─── GENESIS PARAMETERS ───────────────────────────────────────────
GENESIS_TIME     = 1745452800          # 2026-04-24 00:00:00 UTC
GENESIS_NBITS    = 0x207fffff          # Testnet : difficulte minimale (mine en secondes)
GENESIS_VERSION  = 1
GENESIS_MESSAGE  = b"June 2026 - Central banks printed years, we print provable time. Timecoin."

# SHA-256 double-hash
def hash256(data: bytes) -> bytes:
    return hashlib.sha256(hashlib.sha256(data).digest()).digest()

def compact_to_target(nbits: int) -> int:
    """Convert compact nBits to 256-bit integer target."""
    exponent = nbits >> 24
    mantissa = nbits & 0x007fffff
    if nbits & 0x00800000:
        mantissa = -mantissa
    return mantissa * (256 ** (exponent - 3))

def serialize_varint(n: int) -> bytes:
    if n < 0xfd:
        return struct.pack('B', n)
    elif n <= 0xffff:
        return b'\xfd' + struct.pack('<H', n)
    elif n <= 0xffffffff:
        return b'\xfe' + struct.pack('<I', n)
    else:
        return b'\xff' + struct.pack('<Q', n)

def serialize_script_num(n: int) -> bytes:
    """Serialize a script integer (CScriptNum)."""
    if n == 0:
        return b''
    result = []
    neg = n < 0
    absval = abs(n)
    while absval:
        result.append(absval & 0xff)
        absval >>= 8
    if result[-1] & 0x80:
        result.append(0x80 if neg else 0x00)
    elif neg:
        result[-1] |= 0x80
    return bytes(result)

def build_coinbase_tx() -> bytes:
    """Builds the genesis coinbase transaction."""
    # ScriptSig: <nBits> <4> <message>
    nbits_bytes = serialize_script_num(GENESIS_NBITS)
    four_bytes   = serialize_script_num(4)
    msg_len      = serialize_varint(len(GENESIS_MESSAGE))

    script_sig = (
        bytes([len(nbits_bytes)]) + nbits_bytes +
        bytes([len(four_bytes)]) + four_bytes +
        msg_len + GENESIS_MESSAGE
    )

    # Output: value=0, scriptPubKey=OP_RETURN (0x6a)
    output_value = struct.pack('<q', 0)  # 0 Instants (no pre-mine)
    script_pubkey = bytes([0x6a])        # OP_RETURN
    output_script_len = serialize_varint(len(script_pubkey))

    tx = (
        struct.pack('<i', 1)                  +  # nVersion
        serialize_varint(1)                   +  # input count
        b'\x00' * 32                          +  # prevout hash (null)
        struct.pack('<I', 0xffffffff)          +  # prevout index (UINT_MAX)
        serialize_varint(len(script_sig))     +  # scriptSig length
        script_sig                            +  # scriptSig
        struct.pack('<I', 0xffffffff)          +  # sequence
        serialize_varint(1)                   +  # output count
        output_value                          +  # output value (0 TC)
        output_script_len                     +  # scriptPubKey length
        script_pubkey                         +  # OP_RETURN
        struct.pack('<I', 0)                     # nLockTime
    )
    return tx

def merkle_root(txids: list) -> bytes:
    """Computes Merkle root from list of txid bytes."""
    if len(txids) == 0:
        return b'\x00' * 32
    if len(txids) == 1:
        return txids[0]
    while len(txids) > 1:
        if len(txids) % 2 == 1:
            txids.append(txids[-1])
        new_level = []
        for i in range(0, len(txids), 2):
            new_level.append(hash256(txids[i] + txids[i+1]))
        txids = new_level
    return txids[0]

def mine_genesis(nbits=GENESIS_NBITS, start_nonce=0, max_nonce=0xffffffff):
    """
    Mine the genesis block by finding a valid nNonce.
    Prints progress every 1M iterations.
    """
    print("╔══════════════════════════════════════════════════════╗")
    print("║  TIMECOIN Genesis Block Miner                        ║")
    print("╠══════════════════════════════════════════════════════╣")
    print(f"║  Message: {GENESIS_MESSAGE.decode()[:42]}")
    print(f"║  Time:    {GENESIS_TIME} (2026-04-24 UTC)")
    print(f"║  nBits:   0x{nbits:08x}")
    print("╚══════════════════════════════════════════════════════╝")
    print()

    target = compact_to_target(nbits)
    print(f"Target: {target:064x}")
    print()

    # Build coinbase tx and merkle root
    coinbase_tx = build_coinbase_tx()
    txid = hash256(coinbase_tx)
    genesis_merkle = merkle_root([txid])

    print(f"Coinbase TX hash: {txid[::-1].hex()}")
    print(f"Merkle root:      {genesis_merkle[::-1].hex()}")
    print()
    print("Mining... (this may take a few minutes)")

    start_time = time.time()
    nonce = start_nonce

    while nonce <= max_nonce:
        # Serialize block header (80 bytes)
        header = struct.pack('<i', GENESIS_VERSION)           # nVersion (4)
        header += b'\x00' * 32                                 # hashPrevBlock (32) — null
        header += genesis_merkle                               # hashMerkleRoot (32)
        header += struct.pack('<I', GENESIS_TIME)              # nTime (4)
        header += struct.pack('<I', nbits)                     # nBits (4)
        header += struct.pack('<I', nonce)                     # nNonce (4)

        block_hash = hash256(header)
        hash_int   = int.from_bytes(block_hash[::-1], 'big')  # Little-endian to big

        if hash_int < target:
            elapsed = time.time() - start_time
            print()
            print("╔══════════════════════════════════════════════════════╗")
            print("║  ✓ GENESIS BLOCK FOUND!                              ║")
            print("╠══════════════════════════════════════════════════════╣")
            print(f"║  nNonce:     {nonce}")
            print(f"║  Hash:       {block_hash[::-1].hex()}")
            print(f"║  MerkleRoot: {genesis_merkle[::-1].hex()}")
            print(f"║  Time:       {elapsed:.1f} seconds")
            print("╚══════════════════════════════════════════════════════╝")
            print()
            print("Now update chainparams.cpp:")
            print(f"  genesis = CreateTimecoinGenesisBlock(")
            print(f"      {GENESIS_TIME},   // nTime")
            print(f"      {nonce},          // nNonce  ← UPDATE THIS")
            print(f"      0x{nbits:08x},    // nBits")
            print(f"      1,               // nVersion")
            print(f"      0                // genesisReward (no pre-mine)")
            print(f"  );")
            print(f"  // Expected hash: {block_hash[::-1].hex()}")
            return nonce, block_hash[::-1].hex()

        nonce += 1
        if nonce % 1_000_000 == 0:
            elapsed = time.time() - start_time
            rate = nonce / elapsed if elapsed > 0 else 0
            print(f"  Tried {nonce:,} nonces | {rate:,.0f} H/s | "
                  f"Best: {block_hash[::-1].hex()[:16]}...")

    print("No valid nonce found in range. Try extending max_nonce.")
    return None, None

if __name__ == "__main__":
    mine_genesis()
