#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Analisis del protocolo Gamma (Aristocrat) a partir de capturas del analizador logico.
TX = SPC -> Maquina | RX = Maquina -> SPC  (segun descripcion del usuario; verificar ambos sentidos)

Reconstruye tramas por timing (gap > umbral = nueva trama), valida CRC-16/XMODEM,
y clasifica los tipos de mensaje.
"""
import csv, sys, os
from collections import Counter, OrderedDict

BASE = os.path.dirname(os.path.abspath(__file__))

def load_csv(path):
    """Devuelve lista de (time, byte)"""
    out = []
    with open(path, newline='') as f:
        r = csv.reader(f)
        header = next(r)
        for row in r:
            if not row or not row[0].strip():
                continue
            t = float(row[0])
            v = row[1].strip()
            if not v.startswith('0x'):
                continue
            try:
                b = int(v, 16)
            except ValueError:
                continue
            out.append((t, b))
    return out

def frame_by_gap(samples, gap_us=0.004):
    """Agrupa por gap de tiempo. Serial 19200: 1 byte ~ 520us. Gap > 4ms = nueva trama (o timeout interbyte)."""
    frames = []
    cur = []
    prev_t = None
    for t, b in samples:
        if prev_t is not None and (t - prev_t) > gap_us:
            frames.append(cur)
            cur = []
        cur.append((t, b))
        prev_t = t
    if cur:
        frames.append(cur)
    return frames

def crc16_xmodem(data, crc=0x0000):
    for b in data:
        crc ^= (b << 8)
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc

def crc16_check(data_with_crc):
    """Check: CRC sobre todo (incluyendo el CRC) con init 0 == 0."""
    crc = crc16_xmodem(data_with_crc)
    return crc == 0x0000

def analyze_file(path, label):
    print("=" * 100)
    print(f"ARCHIVO: {label}")
    print("=" * 100)
    samples = load_csv(path)
    print(f"Total bytes: {len(samples)}")
    if not samples:
        return []

    # duration
    print(f"Duracion: {samples[0][0]:.3f}s - {samples[-1][0]:.3f}s")

    frames = frame_by_gap(samples)
    # Merge frames that are too short (1-2 bytes) with neighbors if gap was small-ish
    # First look at frame size distribution
    sizes = Counter(len(f) for f in frames)
    print(f"Tramas (por gap {0.004*1000:.0f}ms): {len(frames)}  distribucion de tamanos: {dict(sorted(sizes.items()))}")

    # print first 40 frames in hex
    print("\n-- Primeras 40 tramas (hex) --")
    for i, f in enumerate(frames[:40]):
        t0 = f[0][0]
        hexs = ' '.join(f'{b:02X}' for _, b in f)
        print(f"[{i:4d}] t={t0:10.3f}s  len={len(f):3d}  {hexs}")

    return frames

if __name__ == '__main__':
    tx_frames = analyze_file(os.path.join(BASE, 'TX-27-08-202.txt'), 'TX (SPC -> Maquina)')
    print()
    rx_frames = by_gap = analyze_file(os.path.join(BASE, 'RX-27-08-202.txt'), 'RX (Maquina -> SPC)')
