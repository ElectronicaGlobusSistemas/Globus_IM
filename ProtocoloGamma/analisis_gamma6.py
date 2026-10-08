#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
=============================================================================
 analisis_gamma6 - La respuesta misteriosa RX** capturada por el sniffer (v2)
=============================================================================
Entrada: la trama no-Gamma de 93 B que la maquina emite (CRC invalido para
XMODEM), documentada en el Manual §6.3-4 y capturada completa el 31/08/2026
a las 15:57:54.878 con el modo sniffer de gamma_gui_v2.py:

  26 59 41 31 0A 00 D0 EA (x7) 95 F8 DB 92 05 99 46 25 C8 08 29 10 18 00 CE
  E9 95 FA 7B 96 45 99 E6 2D C8 48 2D 14 18 02 CE F9 95 FA FB BF 65 BB EE
  2F E8 08 BF 16 BA 22 CE F9 95 FE FB B7 65 FB EE 2F EA 68 AD 16 BE

Hipotesis a probar:
  A) Encuadre propio: [26]=marca  [59]=longitud (89)  [payload 89 B] [trailer 2 B]
     -> 2 + 89 + 2 = 93 exacto. Cazar el CRC del trailer con 12 variantes.
  B) Gamma desplazado 1-7 bits (UART mal alineado / 8E1 vs 8N1) o invertido.
  C) Emision a otro baud (9600 leido a 19200): decodificar tomando 1 bit de cada 2.
=============================================================================
"""

BLOB = bytes.fromhex(
    "26 59 41 31 0A 00 D0 EA 0A 00 D0 EA 0A 00 D0 EA 0A 00 D0 EA "
    "0A 00 D0 EA 0A 00 D0 EA 0A 00 D0 EA 95 F8 DB 92 05 99 46 25 "
    "C8 08 29 10 18 00 CE E9 95 FA 7B 96 45 99 E6 2D C8 48 2D 14 "
    "18 02 CE F9 95 FA FB BF 65 BB EE 2F E8 08 BF 16 BA 22 CE F9 "
    "95 FE FB B7 65 FB EE 2F EA 68 AD 16 BE")

SLOTS = [0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78]


def rev(x, n):
    return int(f"{x:0{n}b}"[::-1], 2)


def crc16(data, poly, init, refin, refout, xorout):
    c = init
    for b in data:
        if refin:
            b = rev(b, 8)
        c ^= b << 8
        for _ in range(8):
            c = ((c << 1) ^ poly) & 0xFFFF if c & 0x8000 else (c << 1) & 0xFFFF
    if refout:
        c = rev(c, 16)
    return c ^ xorout


VARIANTES = [
    ("XMODEM",       0x1021, 0x0000, False, False, 0x0000),
    ("CCITT-FALSE",  0x1021, 0xFFFF, False, False, 0x0000),
    ("AUG-CCITT",    0x1021, 0x1D0F, False, False, 0x0000),
    ("KERMIT",       0x1021, 0x0000, True,  True,  0x0000),
    ("X25",          0x1021, 0xFFFF, True,  True,  0xFFFF),
    ("ARC",          0x8005, 0x0000, True,  True,  0x0000),
    ("MODBUS",       0x8005, 0xFFFF, True,  True,  0x0000),
    ("USB",          0x8005, 0xFFFF, True,  True,  0xFFFF),
    ("MAXIM",        0x8005, 0x0000, True,  True,  0xFFFF),
    ("DNP",          0x3D65, 0x0000, True,  True,  0xFFFF),
    ("MCRF4XX",      0x1021, 0xFFFF, True,  True,  0x0000),
    ("GENIBUS",      0x1021, 0xFFFF, False, False, 0xFFFF),
]


def crc8(data, poly, init, refin, refout, xorout):
    c = init
    for b in data:
        if refin:
            b = rev(b, 8)
        c ^= b
        for _ in range(8):
            c = ((c << 1) ^ poly) & 0xFF if c & 0x80 else (c << 1) & 0xFF
    return (rev(c, 8) if refout else c) ^ xorout


def bits_a_bytes(bits):
    return bytes(int("".join(map(str, bits[i:i + 8])), 2)
                 for i in range(0, len(bits) - 7, 8))


def bytes_a_bits(data):
    out = []
    for b in data:
        out += [(b >> i) & 1 for i in range(7, -1, -1)]
    return out


def escanear_gamma(buf, nombre):
    """Busca tramas Gamma validas (SEQ slot + LEN + CRC XMODEM) en buf."""
    hallazgos, i, n = [], 0, len(buf)
    while i < n:
        ln = buf[i + 1] if i + 1 < n else -1
        fin = i + ln + 4
        if buf[i] in SLOTS and 0 <= ln and fin <= n and \
                crc16(buf[i:fin], 0x1021, 0, False, False, 0) == 0:
            hallazgos.append((i, bytes(buf[i:fin])))
            i = fin
        else:
            i += 1
    for pos, t in hallazgos:
        print(f"    *** TRAMA GAMMA VALIDA en {nombre} pos {pos}: "
              f"{' '.join(f'{b:02X}' for b in t)}")
    ascii_runs = []
    run = b""
    for b in buf:
        if 32 <= b < 127:
            run += bytes([b])
        else:
            if len(run) >= 6:
                ascii_runs.append(run.decode())
            run = b""
    if len(run) >= 6:
        ascii_runs.append(run.decode())
    for s in ascii_runs:
        print(f"    ASCII en {nombre}: {s!r}")
    return hallazgos


print("=" * 74)
print(f"1. HECHOS BASICOS")
print(f"   longitud total        : {len(BLOB)} B")
print(f"   byte[0]=0x{BLOB[0]:02X}  byte[1]=0x{BLOB[1]:02X} ({BLOB[1]} dec)")
print(f"   bytes tras byte[1]    : {len(BLOB) - 2}")
print(f"   payload si 2+2 cab/tr : {len(BLOB) - 4}  == byte[1]? {len(BLOB) - 4 == BLOB[1]}")
print(f"   trailer (ultimos 2)   : {BLOB[-2]:02X} {BLOB[-1]:02X}")
print(f"   patron 0A 00 D0 EA    : {BLOB.count(bytes.fromhex('0A00D0EA'))} repeticiones")

print("=" * 74)
print("2. CAZA DEL TRAILER (CRC/suma sobre rangos, ambos ordenes de bytes)")
encontrado = False
tr = (BLOB[-2] << 8) | BLOB[-1]
tr_swap = (BLOB[-1] << 8) | BLOB[-2]
for nombre, *args in VARIANTES:
    for ini, etiqueta in ((0, "desde 0x26"), (2, "solo payload"), (1, "desde 0x59")):
        c = crc16(BLOB[ini:-2], *args)
        if c in (tr, tr_swap):
            print(f"   *** {nombre} {etiqueta}: CRC=0x{c:04X} coincide con trailer!")
            encontrado = True
if not encontrado:
    print("   ninguna variante CRC-16 coincide con el trailer 16 BE")
# residuo-cero sobre la trama COMPLETA (la propiedad que delato el XMODEM de Gamma)
for nombre, *args in VARIANTES:
    if crc16(BLOB, *args) == 0:
        print(f"   *** {nombre}: residuo 0 sobre la trama completa -> es SU CRC")
# Fletcher-16 (suma y complemento) sobre payload
for ini, etiqueta in ((0, "desde 0x26"), (2, "solo payload")):
    seg = BLOB[ini:-2]
    s1 = s2 = 0
    for b in seg:
        s1 = (s1 + b) % 255
        s2 = (s2 + s1) % 255
    f = (s2 << 8) | s1
    if f in (tr, tr_swap):
        print(f"   *** Fletcher-16 {etiqueta}: 0x{f:04X} coincide con trailer!")
# checksums simples sobre todo menos trailer
for ini in (0, 1, 2):
    seg = BLOB[ini:-2]
    if sum(seg) & 0xFFFF in (tr, tr_swap):
        print(f"   *** suma16 desde {ini} coincide")
    if sum(seg) & 0xFF in (BLOB[-2], BLOB[-1]):
        print(f"   *** suma8 desde {ini} coincide con un byte del trailer")
for poly, nom in ((0x07, "SMBus"), (0x31, "MAXIM8"), (0x9B, "CDMA2000")):
    for ini in (0, 2):
        c8 = crc8(BLOB[ini:-2], poly, 0, False, False, 0)
        if c8 in (BLOB[-2], BLOB[-1]):
            print(f"   *** CRC-8 {nom} desde {ini}: 0x{c8:02X} coincide")
print("   (sin *** arriba: trailer todavia sin explicar)")

print("=" * 74)
print("3. GAMMA DESPLAZADO k BITS / INVERTIDO / REFLEJADO")
bits = bytes_a_bits(BLOB)
transformaciones = {}
for k in range(1, 8):
    transformaciones[f"shift{k}"] = bits_a_bytes(bits[k:])
    transformaciones[f"shift-{k}"] = bits_a_bytes(
        [(0, 1)[i % 2] for i in range(k)] + bits)[: len(bits) // 8]
inv = bytes(b ^ 0xFF for b in BLOB)
transformaciones["XOR FF"] = inv
bi = bytes_a_bits(inv)
for k in range(1, 4):
    transformaciones[f"XORFF shift{k}"] = bits_a_bytes(bi[k:])
transformaciones["bit-rev"] = bytes(rev(b, 8) for b in BLOB)
total = 0
for nombre, buf in transformaciones.items():
    total += len(escanear_gamma(buf, nombre))
if total == 0:
    print("    (ninguna trama Gamma valida bajo transformacion de bits)")

print("=" * 74)
print("4. OTRO BAUD: 9600 emitido y leido a 19200 (tomar 1 bit de cada 2)")
for fase in (0, 1):
    sub = bits[fase::2]
    buf = bits_a_bytes(sub)
    escanear_gamma(buf, f"fase{fase}")
    print(f"    fase {fase}: primeros 16 B -> {' '.join(f'{b:02X}' for b in buf[:16])}")

print("=" * 74)
print("5. ESTRUCTURA INTERNA (separadores CE y bloques)")
seg, inicio = [], 0
for i, b in enumerate(BLOB):
    if b == 0xCE:
        seg.append((inicio, i))
        inicio = i + 1
seg.append((inicio, len(BLOB)))
for a, b in seg:
    trozo = BLOB[a:b]
    print(f"   [{a:2d}:{b:2d}] ({len(trozo):2d} B) "
          f"{' '.join(f'{x:02X}' for x in trozo)}")

print("=" * 74)
print("6. CONTEXTO DEL PATRON CABEZA")
print(f"   ASCII de los 5 primeros: {''.join(chr(b) if 32 <= b < 127 else '.' for b in BLOB[:5])!r}")
print("   (0x0A='\\n': '26 59 41 31 0A' se lee '&YA1<LF>' en ASCII)")
