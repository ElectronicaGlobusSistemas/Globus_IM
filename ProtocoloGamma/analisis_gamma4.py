#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Fase 4: modelo transaccional exacto. Queries completas con CRC, timeline intercalado,
status exchange LEN=04, respuestas LEN=08, y que precede a cada respuesta de meter."""
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

# fusionar en timeline unico
events = [('TX', t, r) for t, r in tx] + [('RX', t, r) for t, r in rx]
events.sort(key=lambda e: e[1])

# ---- A. Queries 10B completas (con CRC real) ----
print("=" * 110)
print("A. TODAS LAS CONSULTAS TX DE 10 BYTES (unicas, primera aparicion): SEQ LEN 05 30 SP CL SM MT CRC")
print("=" * 110)
seen = set()
for d, t, r in events:
    if d == 'TX' and len(r) == 10 and r[1] == 0x06:
        key = tuple(r[2:8])
        if key in seen: continue
        seen.add(key)
        ok = 'CRC-OK' if crc16(r[:8]) == (r[8] << 8) | r[9] else 'CRC-BAD!!'
        print(f"  t={t:8.3f}  {H(r)}   [{ok}]  payload={H(r[2:8])}  (space={r[4]:02X} clase={r[5]:02X} sub={r[6]:02X} meter={r[7]:02X})")

# ---- B. Respuestas 16B de meter: completa + que TX la precede ----
print("=" * 110)
print("B. RESPUESTAS RX DE 16 BYTES (unicas por objeto) + TX inmediatamente anterior")
print("=" * 110)
seen = set()
for i, (d, t, r) in enumerate(events):
    if d == 'RX' and len(r) == 16 and r[1] == 0x0C:
        key = tuple(r[3:8])
        if key in seen: continue
        seen.add(key)
        # buscar TX anterior
        prev_tx = None
        for j in range(i - 1, -1, -1):
            if events[j][0] == 'TX':
                prev_tx = events[j]; break
        ptx = f"t={prev_tx[1]:8.3f} {H(prev_tx[2])}" if prev_tx else "???"
        val = (r[9] << 24) | (r[10] << 16) | (r[11] << 8) | r[12]
        print(f"  RX t={t:8.3f}  {H(r)}   obj={r[5]:02X} {r[6]:02X} {r[7]:02X}  valor={val:,}")
        if prev_tx:
            print(f"      TX previo: {ptx}  (dt={(t - prev_tx[1])*1000:.1f}ms)")

# ---- C. Timeline 51.5 - 52.7s (rafaga de meters) ----
print("=" * 110)
print("C. TIMELINE INTERCALADO t=51.50-52.70s")
print("=" * 110)
for d, t, r in events:
    if 51.50 <= t <= 52.70 and len(r) >= 4:
        ann = ''
        if len(r) == 4: ann = '(poll/ACK vacio)'
        elif len(r) == 10: ann = f"(QUERY clase={r[5]:02X} sub={r[6]:02X} meter={r[7]:02X})"
        elif len(r) == 16 and r[1] == 0x0C: ann = f"(DATA meter {r[5]:02X}{r[6]:02X}{r[7]:02X}={((r[9]<<24)|(r[10]<<16)|(r[11]<<8)|r[12]):,})"
        elif len(r) == 8: ann = '(STATUS)'
        print(f"  {d} t={t:10.3f}  {H(r)}  {ann}")

# ---- D. STATUS exchange LEN=04 (8 bytes) ----
print("=" * 110)
print("D. INTERCAMBIO STATUS (frames de 8 bytes, LEN=04): todos, unicos por contenido y direccion")
print("=" * 110)
c = Counter()
for d, t, r in events:
    if len(r) == 8 and r[1] == 0x04:
        c[(d, tuple(r))] += 1
for (d, r), n in sorted(c.items()):
    print(f"  {d} {n:3d}x  {H(list(r))}  payload={H(list(r[2:6]))}")

# ---- E. RX 12B LEN=08 (respuestas a escrituras) ----
print("=" * 110)
print("E. RX 12 BYTES (LEN=08) - todas las unicas")
print("=" * 110)
c = Counter()
for d, t, r in events:
    if d == 'RX' and len(r) == 12:
        c[tuple(r)] += 1
for r, n in c.most_common(30):
    print(f"  {n:3d}x  {H(list(r))}")

# ---- F. Verificar modelo: cada DATA RX de meter, es su TX previo un poll simple? ----
print("=" * 110)
print("F. MODELO: para cada respuesta de meter 16B, tipo de TX previo (poll simple vs query)")
print("=" * 110)
c = Counter()
for i, (d, t, r) in enumerate(events):
    if d == 'RX' and len(r) == 16 and r[1] == 0x0C:
        for j in range(i - 1, -1, -1):
            if events[j][0] == 'TX':
                pt = events[j][2]
                if len(pt) == 4: c['poll simple (4B)'] += 1
                elif len(pt) == 10: c['query 10B'] += 1
                elif len(pt) == 12: c['write 12B'] += 1
                else: c[f'otros len={len(pt)}'] += 1
                break
for k, v in c.most_common():
    print(f"  {k}: {v}")

# ---- G. Con que retraso llega la data tras la query ----
print("=" * 110)
print("G. LATENCIA query->ACK y query->DATA")
print("=" * 110)
pending = {}
lat_ack = []
lat_data = []
for d, t, r in events:
    if d == 'TX' and len(r) == 10:
        pending[tuple(r[2:8])] = t
    elif d == 'RX' and len(r) == 16:
        key = tuple(r[5:8])
        if key in pending:
            lat_data.append(t - pending.pop(key))
print(f"  Queries con DATA recibida despues: {len(lat_data)}")
if lat_data:
    print(f"  dt query->data: min={min(lat_data)*1000:.0f}ms med={sorted(lat_data)[len(lat_data)//2]*1000:.0f}ms max={max(lat_data)*1000:.0f}ms")
    print(f"  n slots de rotacion entre query y data (aprox dt/30ms): {[round(x/0.030) for x in sorted(lat_data)][:30]}")

# ---- H. Baud rate: medir ancho de bytes ----
print("=" * 110)
print("H. ESTIMACION DE BAUDIOS (gap medio entre bytes consecutivos)")
print("=" * 110)
gaps = []
raw_tx = load_csv(os.path.join(BASE, 'TX-27-08-202.txt'))
for (t1, _), (t2, _) in zip(raw_tx, raw_tx[1:]):
    g = t2 - t1
    if g < 0.002: gaps.append(g)
import statistics
mean_b = statistics.mean(gaps)
print(f"  gap medio entre bytes: {mean_b*1e6:.1f} us  -> ~{1/mean_b/10:.0f} baudios (10 bits/byte)")
