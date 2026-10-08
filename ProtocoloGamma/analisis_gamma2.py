#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Fase 2: validacion CRC en todas las tramas + clasificacion + emparejamiento TX->RX."""
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

def crc16_xmodem(data, crc=0x0000):
    for b in data:
        crc ^= (b << 8)
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc

def crc16_arc(data):  # alternativa reflejada (CRC-16/ARC poly 0xA001 reflejado de 0x8005)
    crc = 0x0000
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA001 if crc & 1 else crc >> 1
    return crc

def crc16_ccitt_ffff(data):  # init 0xFFFF
    crc = 0xFFFF
    for b in data:
        crc ^= (b << 8)
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc

def validate(frames, label):
    ok_x = ok_zero = ok_arc = ok_f = 0
    bad = []
    for i, f in enumerate(frames):
        raw = [b for _, b in f]
        if len(raw) < 4: continue
        data, crc_rx = raw[:-2], (raw[-2] << 8) | raw[-1]
        if crc16_xmodem(data) == crc_rx: ok_x += 1
        else: bad.append((i, raw))
        if crc16_xmodem(raw) == 0: ok_zero += 1
        if crc16_arc(data) == crc_rx: ok_arc += 1
        if crc16_ccitt_ffff(data) == crc_rx: ok_f += 1
    total = ok_x + len(bad)
    print(f"{label}: tramas >=4 bytes: {total}")
    print(f"  CRC-16/XMODEM (init 0, MSB first) : {ok_x}/{total}")
    print(f"  (check residual=0 sobre frame completo): {ok_zero}/{total}")
    print(f"  CRC-16/ARC (reflejado)            : {ok_arc}/{total}")
    print(f"  CRC-16/CCITT init FFFF            : {ok_f}/{total}")
    if bad:
        print(f"  FALLAN ({len(bad)}): primeras 10:")
        for i, raw in bad[:10]:
            print(f"   [{i}] {' '.join(f'{b:02X}' for b in raw)}  crc_calc={crc16_xmodem(raw[:-2]):04X}")
    return bad

def classify(frames, label):
    """Clasifica por (byte0, len) y muestra payloads unicos."""
    print(f"\n---- Clasificacion {label} ----")
    groups = Counter()
    examples = {}
    for i, f in enumerate(frames):
        raw = [b for _, b in f]
        if len(raw) < 4:
            groups[f'short({len(raw)})'] += 1
            continue
        seq, ln = raw[0], raw[1]
        expected = ln + 4
        key = f"seq={seq:02X} len={ln:02X} (frame {len(raw)}B, esperado {expected}B)"
        groups[key] += 1
        if key not in examples:
            examples[key] = (i, f[0][0], raw)
    for key, cnt in groups.most_common():
        print(f"  {cnt:4d}x  {key}")
        i, t, raw = examples[key]
        print(f"        ej.[{i}] t={t:.3f}: {' '.join(f'{b:02X}' for b in raw[:40])}{' ...' if len(raw) > 40 else ''}")
    return groups

if __name__ == '__main__':
    tx = frame_by_gap(load_csv(os.path.join(BASE, 'TX-27-08-202.txt')))
    rx = frame_by_gap(load_csv(os.path.join(BASE, 'RX-27-08-202.txt')))
    print("=== VALIDACION CRC TX ==="); validate(tx, 'TX')
    print("\n=== VALIDACION CRC RX ==="); validate(rx, 'RX')
    classify(tx, 'TX')
    classify(rx, 'RX')
