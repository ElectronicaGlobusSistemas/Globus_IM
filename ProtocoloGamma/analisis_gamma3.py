#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Fase 3: correlacion temporal TX->RX, regla de secuencia, catalogo de meters, tramas largas."""
import csv, os
from collections import Counter, OrderedDict

BASE = os.path.dirname(os.path.abspath(__file__))

def load_csv(path):
    out = []
    with open(path, newline='') as f:
        r = csv.reader(f)
        next(r)
        for row in r:
            if not row or not row[0].strip() or not row[1].strip().startswith('0x'):
                continue
            out.append((float(row[0]), int(row[1], 16)))
    return out

def frame_by_gap(samples, gap=0.004):
    frames, cur, prev = [], [], None
    for t, b in samples:
        if prev is not None and (t - prev) > gap:
            frames.append(cur); cur = []
        cur.append((t, b)); prev = t
    if cur: frames.append(cur)
    return frames

def hexs(raw, maxlen=99):
    return ' '.join(f'{b:02X}' for b in raw[:maxlen])

tx = [(f[0][0], [b for _, b in f]) for f in frame_by_gap(load_csv(os.path.join(BASE, 'TX-27-08-202.txt')))]
rx = [(f[0][0], [b for _, b in f]) for f in frame_by_gap(load_csv(os.path.join(BASE, 'RX-27-08-202.txt')))]

# ---- 1. Regla de secuencia: para cada TX frame, buscar el primer RX con |dt| < 60ms ----
print("=" * 110)
print("1. CORRELACION TX -> RX (primer RX dentro de 60 ms despues del TX)")
print("=" * 110)
match_same_seq = match_next_seq = 0
pairs = []
for t, raw in tx:
    if len(raw) < 4: continue
    seq = raw[0]
    best = None
    for t2, raw2 in rx:
        if len(raw2) < 4: continue
        if 0 <= (t2 - t) <= 0.060:
            best = (t2, raw2); break
    if best:
        t2, raw2 = best
        seq2 = raw2[0]
        nxt = ((seq - 0x40 + 8) & 0x38) + 0x40
        tag = 'SAME' if seq2 == seq else ('NEXT' if seq2 == nxt else f'??{seq2:02X}')
        if seq2 == seq: match_same_seq += 1
        elif seq2 == nxt: match_next_seq += 1
        pairs.append((t, raw, t2, raw2, tag))

print(f"Respuesta con MISMO seq: {match_same_seq}, con seq SIGUIENTE: {match_next_seq}")
print("\nMuestra de pares TX->RX (10-byte y 12-byte TX unicamente):")
shown = Counter()
for t, raw, t2, raw2, tag in pairs:
    kind = f'TX{len(raw)}->RX{len(raw2)}'
    if len(raw) <= 4: continue  # solo comandos, no polls simples
    key = (len(raw), len(raw2), tuple(raw[1:5]), tuple(raw2[1:5]))
    shown[key] += 1
    if shown[key] <= 2:
        print(f"  TX t={t:8.3f} [{hexs(raw)}]")
        print(f"  RX t={t2:8.3f} [{hexs(raw2)}]   dt={(t2-t)*1000:.1f}ms  seq:{tag}")
        print()

# ---- 2. Rotacion del seq en TX ----
print("=" * 110)
print("2. ROTACION DE SECUENCIA EN TX (todos los frames >= 4B)")
print("=" * 110)
seqs = [raw[0] for t, raw in tx if len(raw) >= 4]
transitions = Counter()
for a, b in zip(seqs, seqs[1:]):
    transitions[(a, b)] += 1
print("Transiciones seq[n] -> seq[n+1] mas frecuentes:")
for (a, b), c in transitions.most_common(12):
    print(f"  {a:02X} -> {b:02X} : {c}x")

# ---- 3. Catalogo de meters: RX frames LEN=0C (16B) ----
print("=" * 110)
print("3. RESPUESTAS DE METER (RX 16B, LEN=0C): class/sub/meter -> valor")
print("=" * 110)
meters = {}
for t, raw in rx:
    if len(raw) == 16 and raw[1] == 0x0C:
        cls, sub, met = raw[5], raw[6], raw[7]
        val = (raw[9] << 24) | (raw[10] << 16) | (raw[11] << 8) | raw[12]
        key = (cls, sub, met)
        meters.setdefault(key, set()).add(val)
for key in sorted(meters):
    vals = sorted(meters[key])
    print(f"  {key[0]:02X} {key[1]:02X} {key[2]:02X} : {len(vals)} valor(es): {[f'{v:,} (0x{v:08X})' for v in vals[:4]]}")

# ---- 4. Que TX queries existen y con que seq ----
print("=" * 110)
print("4. CONSULTAS TX (10B LEN=06): combinaciones unicas class/sub/meter + seqs usados")
print("=" * 110)
queries = {}
for t, raw in tx:
    if len(raw) == 10 and raw[1] == 0x06:
        key = (raw[5], raw[6], raw[7])
        queries.setdefault(key, []).append((t, raw[0]))
for key in sorted(queries):
    seqs_used = sorted(set(s for _, s in queries[key]))
    print(f"  class/sub/meter={key[0]:02X} {key[1]:02X} {key[2]:02X}  ({len(queries[key])}x)  seqs usados: {[f'{s:02X}' for s in seqs_used]}")

# ---- 5. Escrituras TX (12B LEN=08) ----
print("=" * 110)
print("5. ESCRITURAS/COMANDOS TX (12B LEN=08): unicos")
print("=" * 110)
writes = {}
for t, raw in tx:
    if len(raw) == 12 and raw[1] == 0x08:
        key = tuple(raw[2:])
        writes[key] = writes.get(key, 0) + 1
for key, c in sorted(writes.items(), key=lambda kv: -kv[1]):
    print(f"  {c:3d}x  payload={hexs(list(key))}")

# ---- 6. Respuestas a escrituras: RX 12B ----
print("=" * 110)
print("6. RESPUESTAS RX 12B (LEN=08): unicas (primeras 15)")
print("=" * 110)
r12 = {}
for t, raw in rx:
    if len(raw) == 12 and raw[1] == 0x08:
        key = tuple(raw[2:])
        r12[key] = r12.get(key, 0) + 1
for key, c in sorted(r12.items(), key=lambda kv: -kv[1])[:15]:
    print(f"  {c:3d}x  payload={hexs(list(key))}")

# ---- 7. Tramas RX largas ----
print("=" * 110)
print("7. TRAMAS RX LARGAS (>16B) - decodificacion completa")
print("=" * 110)
seen_long = set()
for t, raw in rx:
    if len(raw) > 16:
        key = len(raw)
        if key in seen_long: continue
        seen_long.add(key)
        print(f"\n  t={t:.3f}s len={len(raw)} LENbyte=0x{raw[1]:02X}")
        print(f"  {hexs(raw)}")
        # payload como ASCII
        payload = raw[2:-2]
        asc = ''.join(chr(b) if 32 <= b < 127 else '.' for b in payload)
        print(f"  ASCII payload: {asc}")
