#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Fase 5: resumen final - comportamiento idle, tablas TX generadas para el manual."""
import csv, os
from collections import Counter

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
    return [(f[0][0], [b for _, b in f]) for f in frames]

def H(raw): return ' '.join(f'{b:02X}' for b in raw)

def crc16(data):
    crc = 0
    for b in data:
        crc ^= (b << 8)
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc

tx = frame_by_gap(load_csv(os.path.join(BASE, 'TX-27-08-202.txt')))
rx = frame_by_gap(load_csv(os.path.join(BASE, 'RX-27-08-202.txt')))

# --- Comportamiento en ventana idle 45-51.5s ---
print("=== VENTANA 45.0-51.5s (que hace la SPC entre rafagas) ===")
c = Counter()
for t, r in tx:
    if 45.0 <= t <= 51.5 and len(r) >= 4:
        c[H(r)] += 1
for k, v in c.most_common():
    print(f"  {v:3d}x  {k}")
print(f"  (total TX en ventana: {sum(c.values())})")
dts = [t2[0]-t1[0] for t1, t2 in zip([x for x in tx if 45.0 <= x[0] <= 51.5], [x for x in tx if 45.0 <= x[0] <= 51.5][1:])]
if dts:
    print(f"  periodo medio entre TX: {sum(dts)/len(dts)*1000:.1f}ms")

# --- Ventana 59.5-63s (final) ---
print("\n=== VENTANA 59.5-63s ===")
c = Counter()
for t, r in tx:
    if t >= 59.5 and len(r) >= 4:
        c[H(r)] += 1
for k, v in c.most_common():
    print(f"  {v:3d}x  {k}")

# --- Tablas generadas para el manual ---
print("\n=== POLLS VACIOS (8 slots, precalculados) ===")
for s in range(8):
    seq = 0x40 + 8*s
    c = crc16([seq, 0x00])
    print(f"  slot {s} (0x{seq:02X}): {seq:02X} 00 {c>>8:02X} {c&0xFF:02X}")

print("\n=== CONSULTA DE METER EN CADA SLOT (10 bytes listos para enviar) ===")
meters = [
    ("Current Credits",     0x02, 0x03, 0x01),
    ("Total Games Played",  0x02, 0x04, 0x04),
    ("Total Credits Bet",   0x02, 0x04, 0x05),
    ("Total Credits Won",   0x02, 0x04, 0x06),
    ("Cashout Handpays",    0x02, 0x05, 0x03),
    ("True In",             0x04, 0x09, 0x01),
    ("CoinDrop/GrossDrop?", 0x04, 0x04, 0x01),
]
for name, cl, sm, mt in meters:
    print(f"\n  {name} (obj {cl:02X} {sm:02X} {mt:02X}):")
    row = []
    for s in range(8):
        seq = 0x40 + 8*s
        frame = [seq, 0x06, 0x05, 0x30, 0x02, cl, sm, mt]
        c = crc16(frame)
        row.append(f"0x{seq:02X}:{seq:02X} 06 05 30 02 {cl:02X} {sm:02X} {mt:02X} {c>>8:02X} {c&0xFF:02X}")
    for r in row: print(f"    {r}")

# --- Validacion cruzada: frames de la captura ---
print("\n=== VERIFICACION CONTRA CAPTURA (frames reales) ===")
tests = [
    ("Credits Bet @78 TX",   [0x78,0x06,0x05,0x30,0x02,0x02,0x04,0x05,0x5F,0x75]),
    ("Credits Bet RX",       [0x40,0x0C,0x0A,0x31,0x82,0x02,0x04,0x05,0x00,0x02,0x21,0x1E,0xFF,0x00,0xCB,0xF3]),
    ("True In @78 TX",       [0x78,0x06,0x05,0x30,0x02,0x04,0x09,0x01,0xDB,0x0D]),
    ("CurrentCredits @48TX", [0x48,0x06,0x05,0x30,0x02,0x02,0x03,0x01,0xC0,0xBA]),
]
for name, f in tests:
    ok = (crc16(f) == 0)
    print(f"  {name}: {'OK' if ok else 'FALLA'} ({H(f)})")
