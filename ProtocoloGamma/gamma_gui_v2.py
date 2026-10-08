#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
=============================================================================
 GAMMA TEST GUI v2 - Monitor de comandos SPC para maquina Aristocrat (Gamma)
=============================================================================
Archivo AUTONOMO (no importa nada mas que pyserial + tkinter).

v2 (31/08/2026) - MISMA LOGICA DE PROTOCOLO que gamma_gui.py / gamma_gui_v1.py
(CRC, slots, QUERY->ACK->POLL->DATA, cadencias, resincronizacion: identicos).
Solo cambia la presentacion:
  * Comandos en un dropdown; en la lista solo aparece el nombre (sin codigo)
  * Panel LECTURAS (arriba): resultado de cada consulta como
      (02 03 01) = Current Credits = 00000000
  * Panel TRAMAS (abajo): solo las tramas crudas TX>>/RX<<
  * Sin tabla de valores
  * Boton "Guardar log...": guarda la sesion con el formato clasico del
    monitor (hex + interpretacion), igual que la version anterior
  * "Ver keep-alive en monitor" ahora si muestra los polls de fondo
    (en v1 el check existia pero no tenia efecto)
  * La trama hex manual ahora si se procesa (en v1 se encolaba y se ignoraba)
  * NUEVO "Escuchar todo (sniffer)": captura TODO lo que envia la maquina,
    incluso lo que llega fuera de la ventana de respuesta (marcado RX**) y
    separa tramas concatenadas dentro de una misma ventana
  * NUEVO v2.1 "Barrido automatico": checkbox + rangos editables
    (Clases/Sub/Meter en hex, soporta 01-09,80,81) + boton INICIAR.
    Recorre el grid completo consultando cada objeto (SOLO LECTURA),
    publica una linea por respuesta en LECTURAS y guarda la tabla
    completa en barrido_grid_<fecha>.txt junto al script, lista para
    analizar offline. Boton Detener para cancelar a mitad.
  * NUEVO v2.2 "Correccion denominacion" (04-09-2026): la denominacion
    (value of 1 credit, en centavos de la moneda interna) vive en
    02 01 05 (EGM Configuration P5), NO en 04 01 01 — ese es el value
    of 1 COIN (tambien en centavos; sigue el menú de la maquina).
    Alineado con GammaProLink.cpp (fuente unica 02 01 05). Para ello la
    GUI ahora decodifica datos BE16 (T07, LEN 08 -> valor en [8..9]) y
    distingue las palabras de estado T06 (objeto sin dato), antes
    mostradas como "DATA bloque (12 B)".
  * NUEVO v2.3 (04-09-2026): lecturas mucho mas descriptivas.
    - 04 01 01 muestra la conversion como la denominacion:
      "... = 00000100  ->  1 moneda = 1.00 $"
    - Los bloques de identidad se muestran como segmentos legibles
      separados por " | " ("G1607 | AH | Dolphin Treasure | 87.928%")
      en vez de un ascii lleno de puntos; aplica a cualquier
      bloque/string (02 01 02 moneda, 02 04 03, 03 01 02, 80 02 00...)
      que antes ni se publicaba en LECTURAS.
    - Datos BE32 (T08/09) y BE64 (T0C) decodificados por tipo.
    - 01 01 00 (directorio) decodificado: count + pares CL-SM con el
      nombre oficial del catalogo "Gamma Devices".
    - NOMBRES/dropdown ampliados con los objetos identificados en el
      barrido 2.024 que faltaban (02 01 02/03, 02 04 03/07, 02 05 07,
      03 01 02, 04 06 00, 06 02 01/02/03, 80 02 00, 01 01 00).
  * NUEVO v2.4 (04-09-2026, verificado en campo): 06 02 01 (EGM Doors
    P1) es un valor COMPUESTO: bit 16 = puerta ABIERTA en vivo (abierta
    lee contador+65536, cerrada lee el contador solo) y los bits bajos
    = contador de APERTURAS (+1 al abrir, el cierre no suma; sellado
    con ciclo completo cerrada->abierta->cerrada = +1 exacto). La GUI
    lo decodifica:
      (06 02 01) = ... = 67563  ->  puerta ABIERTA | aperturas de puerta = 2027
  * NUEVO v2.5 (04-09-2026, verificado contra pantalla MAIN1 y trama SPC de
    la maq. 3 Mk6): identidades del bloque 02 03 03 completadas/corregidas:
      [4]  = Coin In (Total Credits Bet)
      [5]  = Coin Out (Total Credits Won)
      [9]  = Drop / Bill In (Gross Drop)
      [13] = Attendant Paid Cancel Credits  (NO "Cashout Handpays")
      [17] = JACKPOT (Attendant Paid Jackpots)
    Y correccion de identidad individual: 02 05 03 NO es "Cashout Handpays"
    (en pantalla MAIN1 el handpay real esta en 0) — es Split Voucher Pays
    (ticket out). Ver ProtocoloGamma/docs/Analisis_SPC_Contadores.md

Uso:
  1. Selecciona el puerto y pulsa Conectar (hace sondeo de arranque 0x40)
  2. Elige un comando del dropdown y pulsa ENVIAR
  3. Arriba: lecturas decodificadas. Abajo: tramas crudas
  4. "Guardar log..." vuelca la sesion completa a un .txt
  5. "Escuchar todo (sniffer)" para cazar trafico no solicitado (RX**)

Incluye keep-alive de fondo (el enlace de la maquina se cae si deja de
recibir polls) y auto-resincronizacion por slot 0x40.

Requiere:  pip install pyserial
"""

import os
import queue
import threading
import time
import tkinter as tk
from tkinter import ttk, messagebox, filedialog
from tkinter.scrolledtext import ScrolledText
from datetime import datetime

import serial
from serial.tools import list_ports

# Carpeta del script: aqui caen las tablas de barrido generadas
CARPETA = os.path.dirname(os.path.abspath(__file__))

# ============================================================================
# PROTOCOLO GAMMA (verificado contra maquina real - Manual ProtocoloGamma)
# ============================================================================
SLOTS = [0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78]

NOMBRES = {
    (0x01, 0x01, 0x00): "Directorio de familias validas",
    (0x01, 0x02, 0x01): "Version / serie",
    (0x01, 0x02, 0x02): "Fabricante / paytable / ID juego",
    (0x02, 0x01, 0x02): "Moneda interna (US CURRENCY / US$)",
    (0x02, 0x01, 0x03): "Jackpot limit (centavos)",
    (0x02, 0x01, 0x05): "DENOMINACION - Value of 1 credit (centavos)",
    (0x02, 0x02, 0x04): "Meter grande (02 02 04)",
    (0x02, 0x03, 0x01): "Current Credits",
    (0x02, 0x03, 0x03): "Bloque multi-meter (24 contadores)",
    (0x02, 0x04, 0x03): "Host Initiated Disable",
    (0x02, 0x04, 0x04): "Total Games Played",
    (0x02, 0x04, 0x05): "Total Credits Bet",
    (0x02, 0x04, 0x06): "Total Credits Won",
    (0x02, 0x04, 0x07): "Jackpot win limit",
    (0x02, 0x05, 0x03): "Split Voucher Pays (ticket out) - NO es handpay",
    (0x02, 0x05, 0x04): "Split Pay Out Coins Out? (candidato)",
    (0x02, 0x05, 0x07): "Max bet limit (centavos)",
    (0x03, 0x01, 0x00): "Juego / % payback",
    (0x03, 0x01, 0x02): "Bloque identidad LIMPIO (sin meters)",
    (0x04, 0x01, 0x01): "Value of 1 coin (centavos) - NO es la denominacion",
    (0x04, 0x01, 0x07): "Hopper P7 (candidato vencido; la fuente es 02 01 05)",
    (0x04, 0x01, 0x16): "Apuesta maxima?",
    (0x04, 0x04, 0x01): "Coin Drop / Gross Drop?",
    (0x04, 0x06, 0x00): "Tabla de billetes (centavos)",
    (0x04, 0x09, 0x01): "True In (Coins In)",
    (0x06, 0x02, 0x01): "EGM Doors P1 - estado de puerta (bit 16) + aperturas",
    (0x06, 0x02, 0x02): "EGM Doors P2 - cash box accesses",
    (0x06, 0x02, 0x03): "EGM Doors P3 - bill acceptor door accesses",
    (0x80, 0x02, 0x00): "SPC-2 (autoidentificacion)",
}

# Catálogo oficial "Gamma Devices" cruzado 1:1 con el directorio 01 01 00:
# CL-SM -> nombre oficial de la familia (C=clase, T=tipo, P[n]=parámetros)
FAMILIAS = {
    (0x01, 0x01): "Device List",        (0x01, 0x02): "EGM Identification",
    (0x01, 0x03): "Host Identification", (0x02, 0x01): "EGM Configuration",
    (0x02, 0x02): "EGM Control",         (0x02, 0x03): "EGM Audit",
    (0x02, 0x04): "EGM Game Summary",    (0x02, 0x05): "EGM Money Summary",
    (0x03, 0x01): "Juego (reel)",        (0x04, 0x01): "Hopper",
    (0x04, 0x02): "Ticket Printer",      (0x04, 0x03): "Hand-Pay",
    (0x04, 0x04): "Cash Box",            (0x04, 0x05): "Credit Transfer",
    (0x04, 0x06): "Bill Acceptor",       (0x04, 0x07): "EFT In",
    (0x04, 0x08): "EFT Out",             (0x04, 0x09): "Coin Acceptor",
    (0x04, 0x0A): "Bill Stacker",        (0x04, 0x0C): "Vouchers",
    (0x05, 0x11): "Link Progressive 1",  (0x05, 0x12): "Link Progressive 2",
    (0x05, 0x13): "Link Progressive 3",  (0x05, 0x14): "Link Progressive 4",
    (0x05, 0x20): "Link Mystery 8",      (0x06, 0x01): "Logic Seal",
    (0x06, 0x02): "EGM Doors",           (0x06, 0x03): "Software Signature",
    (0x07, 0x01): "Sound Effects",       (0x80, 0x01): "SPC smib",
    (0x80, 0x02): "SPC2 smib",
}

# Valores (en centavos) que contiene la tabla de billetes del Bill Acceptor
TABLA_BILLETES = (100, 200, 500, 1000, 2000, 5000, 10000)     # $1 .. $100


def _buscar_billetes(d: bytes):
    """Escanea el bloque 04 06 00 con ventanas BE32 de 1 byte (inmune a la
    alineacion/relleno) y devuelve los billetes presentes, en orden."""
    hallados = []
    for off in range(max(len(d) - 3, 0)):
        v = int.from_bytes(d[off:off + 4], "big")
        if v in TABLA_BILLETES and v not in hallados:
            hallados.append(v)
    return hallados


def _fmt_puerta(v: int) -> str:
    """Decodifica 06 02 01 (EGM Doors P1). Valor compuesto verificado en
    campo 04-09: bit 16 = puerta ABIERTA en vivo (abierta lee +65536,
    cerrada lee solo el contador); bits bajos = contador de APERTURAS de
    puerta — un ciclo completo cerrada->abierta->cerrada suma +1 (el
    incremento aparece ya en la lectura con la puerta abierta; el cierre
    no suma; estable sin actividad: no es un temporizador)."""
    return (f"puerta {'ABIERTA' if v & 0x10000 else 'CERRADA'}"
            f" | aperturas de puerta = {v & 0xFFFF}")


def _parse_directorio(d: bytes):
    """Decodifica el directorio 01 01 00: count BE32 + n pares CL-SM.
    Prueba las alineaciones 0 y 1 (byte de relleno) y exige que los pares
    caigan mayormente en familias conocidas del catalogo. None si no cuadra."""
    for off in (0, 1):
        if len(d) < off + 4:
            continue
        n = int.from_bytes(d[off:off + 4], "big")
        if not (0 < n <= (len(d) - off - 4) // 2):
            continue
        pares = [(d[off + 4 + 2 * i], d[off + 5 + 2 * i]) for i in range(n)]
        if sum(1 for p in pares if p in FAMILIAS) >= n * 0.8:
            return pares
    return None

CADENA_METERS = [
    (0x02, 0x04, 0x05), (0x02, 0x04, 0x06), (0x02, 0x05, 0x03), (0x02, 0x05, 0x04),
    (0x04, 0x09, 0x01), (0x02, 0x03, 0x01), (0x04, 0x04, 0x01), (0x02, 0x04, 0x04),
]

# Entradas del bloque multi-meter 02 03 03 (Manual Protocolo_Gamma, ANEXO I):
# la entrada i esta en RX[9+4i]. Solo las identificadas llevan nombre.
NOMBRES_ENTRADAS_020303 = {
    0:  "= 02 02 04 (bitmask/config?)",
    1:  "Current Credits",
    2:  "Total Games Played",
    3:  "sin asignar (meter vivo; tambien en 03 01 00)",
    4:  "Total Credits Bet (Coin In)",
    5:  "Total Credits Won (Coin Out)",
    8:  "sin asignar (familia drop)",
    9:  "Drop / Bill In (Gross Drop)",
    13: "Attendant Paid Cancel Credits (SPC: cancelled)",
    15: "sin asignar (familia drop; NO es True In)",
    17: "Jackpot (Attendant Paid Jackpots)",
}

# entrada -> objeto individual equivalente, para publicar la lectura con el
# objeto que representa (solo las confirmadas por cruce individual)
#  OJO: [13] ya NO se mapea a (02 05 03) — son meters DISTINTOS (02 05 03 es
#  Split Voucher Pays / ticket out; [13] es Attendant Paid Cancel Credits).
#  En la maq. 3 difieren por completo: [13]=2.303.708.112 vs 02 05 03=0
BLOQUE_A_OBJETO = {
    0:  (0x02, 0x02, 0x04),
    1:  (0x02, 0x03, 0x01),
    2:  (0x02, 0x04, 0x04),
    4:  (0x02, 0x04, 0x05),
    5:  (0x02, 0x04, 0x06),
}


def segmentos_ascii(blob: bytes, min_len: int = 2):
    """Runas de ASCII imprimible de un bloque, recortadas y sin vacios.
    ".....01.04.003      \\x00\\x00000001\\x00..." -> ["01.04.003", "000001"]
    (antes se mostraba con puntos por byte no imprimible: ilegible)."""
    segs, cur = [], bytearray()
    for b in blob:
        if 32 <= b < 127:
            cur.append(b)
        else:
            if len(cur) >= min_len:
                segs.append(bytes(cur).decode("ascii").strip())
            cur = bytearray()
    if len(cur) >= min_len:
        segs.append(bytes(cur).decode("ascii").strip())
    return [s for s in segs if s]


def gamma_crc16(data: bytes) -> int:
    """CRC-16/XMODEM: poly 0x1021, init 0x0000, MSB primero."""
    crc = 0x0000
    for b in data:
        crc ^= (b << 8)
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


def build_frame(seq: int, payload: bytes) -> bytes:
    head = bytes([seq, len(payload)]) + payload
    crc = gamma_crc16(head)
    return head + bytes([crc >> 8, crc & 0xFF])


def empty_frame(seq: int) -> bytes:
    return build_frame(seq, b"")


def query_frame(seq: int, obj) -> bytes:
    cl, sm, mt = obj
    return build_frame(seq, bytes([0x05, 0x30, 0x02, cl, sm, mt]))


def crc_ok(frame: bytes) -> bool:
    return len(frame) >= 4 and gamma_crc16(frame) == 0


def describe(frame: bytes) -> str:
    """Interpretacion de una trama (para el LOG guardado, formato clasico)."""
    if not crc_ok(frame):
        return "*** CRC INVALIDO ***"
    seq, ln = frame[0], frame[1]
    if ln == 0:
        return "POLL/ACK vacio"
    p = frame[2: 2 + ln]
    if ln == 6 and p[1] == 0x30 and p[2] == 0x02:
        obj = (p[3], p[4], p[5])
        return f"QUERY {NOMBRES.get(obj, 'objeto desconocido')} ({obj[0]:02X} {obj[1]:02X} {obj[2]:02X})"
    if p[1] == 0x31 and ln == 0x08 and p[0] == 0x07:
        # dato BE16 (T07): la DENOMINACION (02 01 05) y el Hopper P7 llegan asi.
        # Trama: [slot][08][07][31 82][eco CL SM MT][BE16][CRC] -> valor en [8..9]
        obj = (p[3], p[4], p[5])
        v = int.from_bytes(frame[8:10], "big")
        extra = ""
        if obj in ((0x02, 0x01, 0x05), (0x04, 0x01, 0x07)):
            extra = f"  (value of 1 credit: {v / 100:.2f} $)"
        return f"DATA BE16 -> {v:05d}  {NOMBRES.get(obj, 'objeto desconocido')}{extra}"
    if p[1] == 0x31 and ln == 0x08 and p[0] == 0x06:
        # T06 = palabra de ESTADO de objeto invalido/muerto, NO es un dato
        st = int.from_bytes(frame[8:10], "little")
        return f"DATA status T06 (objeto sin dato) LE16=0x{st:04X}"
    if p[1] == 0x31 and ln == 0x0A and p[0] in (0x08, 0x09):
        # BE32 (p. ej. Doors 06 02 xx)
        obj = (p[3], p[4], p[5])
        v = int.from_bytes(frame[8:12], "big")
        extra = f"  ({_fmt_puerta(v)})" if obj == (0x06, 0x02, 0x01) else ""
        return f"DATA BE32 -> {v}  {NOMBRES.get(obj, 'objeto desconocido')}{extra}"
    if p[1] == 0x31 and ln == 0x0E and p[0] == 0x0C:
        # BE64
        obj = (p[3], p[4], p[5])
        v = int.from_bytes(frame[8:16], "big")
        return f"DATA BE64 -> {v}  {NOMBRES.get(obj, 'objeto desconocido')}"
    if p[1] == 0x31 and ln >= 0x0C:
        obj = (p[3], p[4], p[5])
        if obj in NOMBRES and ln == 0x0C:
            v = int.from_bytes(frame[9:13], "big")
            vtxt = f"{v:,}".replace(",", ".")
            extra = ""
            if obj == (0x04, 0x01, 0x01):
                extra = f"  (value of 1 coin: {v / 100:.2f} $)"
            elif obj == (0x04, 0x01, 0x16):
                extra = f"  (= {v * 0.01:.2f} $ si son creditos)"
            return f"DATA -> {v:08d}  {NOMBRES[obj]}  (decimal: {vtxt}){extra}"
        if obj == (0x02, 0x03, 0x03):
            pass                                  # cae al listado de entradas abajo
        elif obj == (0x01, 0x01, 0x00):
            pares = _parse_directorio(frame[8:-2])
            if pares:
                fams = [f"{a:02X} {b:02X} {FAMILIAS.get((a, b), '?')}" for a, b in pares]
                lineas = [f"DATA directorio: {len(pares)} familias validas"]
                for i in range(0, len(fams), 4):
                    lineas.append("      " + " | ".join(fams[i:i + 4]))
                return "\n".join(lineas)
        elif obj[0] == 0x01 or obj == (0x03, 0x01, 0x00):
            segs = segmentos_ascii(frame[9:-2])
            return (f"DATA identidad: {' | '.join(segs)}"
                    if segs else "DATA identidad: (sin texto)")
        n = (ln - 8) // 4
        if n > 1 and (ln - 8) % 4 == 0:           # bloque multi-meter: N valores BE32
            nombres = NOMBRES_ENTRADAS_020303 if obj == (0x02, 0x03, 0x03) else {}
            titulo = ("DATA bloque multi-meter 02 03 03 - PAQUETE COMPLETO DE CONTADORES"
                      if obj == (0x02, 0x03, 0x03)
                      else f"DATA bloque {obj[0]:02X} {obj[1]:02X} {obj[2]:02X}")
            lineas = [f"{titulo} ({ln + 4} B) - {n} entradas:"]
            for i in range(min(n, 26)):
                v = int.from_bytes(frame[9 + 4 * i: 13 + 4 * i], "big")
                vt = f"{v:,}".replace(",", ".")
                lineas.append(f"      [{i:2d}] {vt:>13}  {nombres.get(i, '')}".rstrip())
            if n > 26:
                lineas.append(f"      ... ({n - 26} entradas mas)")
            return "\n".join(lineas)
        if obj == (0x04, 0x06, 0x00):
            hallados = _buscar_billetes(frame[8:-2])
            if hallados:
                return ("DATA tabla de billetes: "
                        + " ".join(f"${v // 100}" for v in hallados))
        segs = segmentos_ascii(frame[8:-2])
        if segs:
            return f"DATA bloque ({ln + 4} B): {' | '.join(segs)[:100]}"
        return f"DATA bloque ({ln + 4} B)"
    if ln == 4 and p[0] == 0x02:
        return f"STATUS 02 {p[1]:02X}"
    return f"comando (LEN={ln:02X})"


# ============================================================================
# HILO DE PROTOCOLO (todo el acceso serie ocurre aqui; la GUI solo lee colas)
# Logica IDENTICA a v1: solo cambia que publica "lectura" en vez de "meter"
# ============================================================================
class Protocolo(threading.Thread):
    def __init__(self, puerto, out_q):
        super().__init__(daemon=True)
        self.ser = serial.Serial(puerto, baudrate=19200, bytesize=8,
                                 parity="N", stopbits=1, timeout=0.01)
        self.out = out_q
        self.cmds = queue.Queue()
        self.slot_idx = 0
        self.keepalive = True          # por defecto ON: el enlace no debe caerse
        self.mostrar_ka = False        # no inundar el monitor con polls de fondo
        self.sniffer = False           # escuchar TODO lo que envia la maquina
        self.monitoreo = False
        self._prox_mon = 0.0
        self.running = True
        self.frames_ok = 0
        self.frames_bad = 0
        self.ultimo_ok = 0.0
        self.cadencia = 0.030
        self.barrido_cancel = False       # el boton Detener iza esta bandera
        self.barrido_activo = False

    def _log(self, direccion, frame):
        self.out.put(("log", direccion, frame))

    def _txt(self, texto, tipo="info"):
        self.out.put(("txt", texto, tipo))

    def _slot(self):
        s = SLOTS[self.slot_idx]
        self.slot_idx = (self.slot_idx + 1) % 8
        return s

    # ---- capa de trama -------------------------------------------------------
    def _partir_tramas(self, buf):
        """Divide un buffer en tramas validas caminando por SEQ/LEN/CRC
        (la misma regla del parser del ESP32). Devuelve la lista de tramas
        individuales; vacia si no hay ninguna."""
        tramas, i, n = [], 0, len(buf)
        while i < n:
            ln = buf[i + 1] if i + 1 < n else -1
            fin = i + ln + 4
            if buf[i] in SLOTS and 0 <= ln and fin <= n and crc_ok(buf[i:fin]):
                tramas.append(bytes(buf[i:fin]))
                i = fin
            else:
                i += 1                 # byte invalido: resincroniza al siguiente
        return tramas

    def _leer_pendiente(self):
        """Modo sniffer: lee y logea lo que haya llegado fuera de ventana
        (lo que v1 descartaba en silencio con reset_input_buffer). RX**."""
        d = self.ser.read(256)
        if not d:
            return
        fin = time.time() + 0.03       # sigue leyendo hasta silencio (gap > 2 ms)
        while time.time() < fin:
            mas = self.ser.read(256)
            if mas:
                d += mas
                fin = time.time() + 0.03
            else:
                break
        tramas = self._partir_tramas(d)
        if tramas:
            self.frames_ok += len(tramas)
            for t in tramas:
                self._log("RX**", t)
        else:
            self.frames_bad += 1
            self._log("RX**", bytes(d))

    def _recibir(self, timeout):
        buf = bytearray()
        fin = time.time() + timeout
        while time.time() < fin:
            d = self.ser.read(256)
            if d:
                buf += d
                fin = time.time() + 0.03
            elif buf:
                time.sleep(0.005)
                if not self.ser.read(256):
                    break
        if not buf:
            return None
        t = bytes(buf)
        # Cuidado: la concatenacion de dos tramas validas TAMBIEN deja CRC==0
        # (el residuo de la 1a es 0 = estado inicial de la 2a). Por eso se
        # exige ademas que el largo del blob sea exactamente LEN+4 de la 1a.
        esperado = t[1] + 4 if len(t) >= 2 else 0
        if crc_ok(t) and len(t) == esperado:
            self.frames_ok += 1
            return t
        tramas = self._partir_tramas(t) if self.sniffer else []
        if tramas:
            # varias tramas pegadas en una misma ventana (solo sniffer):
            # logear cada una y devolver la ultima (la mas reciente)
            self.frames_ok += len(tramas)
            for extra in tramas[:-1]:
                self._log("RX<<", extra)
            return tramas[-1]
        self.frames_bad += 1
        try:
            self.ser.reset_input_buffer()
        except Exception:
            pass
        return t

    def _xchg(self, frame, espera=0.15, quiet=False):
        if not quiet:
            self._log("TX>>", frame)
        if self.sniffer:
            self._leer_pendiente()     # no descartar lo pendiente: logearlo
        else:
            self.ser.reset_input_buffer()
        self.ser.write(frame)
        self.ser.flush()
        r = self._recibir(espera)
        if r is not None:
            self.ultimo_ok = time.time()
            if not quiet:
                self._log("RX<<", r)
            elif self.sniffer and len(r) > 4 and r[1] != 0x00:
                # Respuesta con DATOS a un poll de fondo silencioso: en este
                # protocolo la maquina NUNCA transmite sin poll, asi que un
                # evento solo puede viajar montado en la respuesta a un poll.
                # Con "Ver keep-alive" apagado esto se comia en silencio.
                self._log("RX**", r)
        return r

    # ---- transacciones ---------------------------------------------------------
    def sondeo(self, max_seg=30.0):
        self._txt(f"Sondeo de arranque: poll slot 0x40 hasta eco (max {max_seg:.0f} s)")
        t0 = time.time()
        while time.time() - t0 < max_seg and self.running:
            r = self._xchg(empty_frame(0x40), espera=0.25)
            if r is not None:
                self.slot_idx = 1        # tras el eco de 0x40 la maquina espera 0x48
                self._txt("MAQUINA EN LINEA", "ok")
                return True
        self._txt("Sin respuesta de la maquina", "err")
        return False

    def resincronizar(self, max_seg=10.0):
        self._txt("Enlace caido -> resincronizando con poll del slot 0x40", "err")
        t0 = time.time()
        while time.time() - t0 < max_seg and self.running:
            r = self._xchg(empty_frame(0x40), espera=0.25)
            if r is not None:
                self.slot_idx = 1
                self._txt("Enlace restablecido", "ok")
                return True
        return False

    def _esperar_data(self, obj, polls=4):
        for _ in range(polls):
            r = self.poll()
            if r and len(r) > 4:
                p = r[2: 2 + r[1]]
                if p[1] == 0x31 and (p[3], p[4], p[5]) == tuple(obj):
                    return r
        return None

    def poll(self):
        time.sleep(self.cadencia)
        return self._xchg(empty_frame(self._slot()), espera=0.1)

    def leer(self, obj):
        """QUERY -> ACK -> POLL... -> DATA. Publica la lectura hacia la GUI."""
        obj = tuple(obj)
        for _ in range(3):
            time.sleep(self.cadencia)
            r = self._xchg(query_frame(self._slot(), obj), espera=0.12)
            if r is None:
                if not self.resincronizar():
                    return None
                continue
            data = self._esperar_data(obj)
            if data is not None:
                self._publicar_lectura(obj, data)
                return data
            self._txt(f"... reintentando {NOMBRES.get(obj, obj)}")
        self._txt(f"Sin DATA para {obj}", "err")
        return None

    def _publicar_lectura(self, obj, data):
        """Capa de presentacion: vuelca la DATA al panel LECTURAS de la GUI.
        El tipo de dato se decide por T (payload[0]) y LEN, como en el barrido:
        T07=BE16, T08/09=BE32, T0A/0B=meter BE32 (byte de relleno en [8]),
        T0C=BE64, T06=palabra de estado (objeto sin dato), resto=bloque/string."""
        ln, t = data[1], data[2]
        otxt = f"({obj[0]:02X} {obj[1]:02X} {obj[2]:02X})"
        if ln == 0x0C and t in (0x0A, 0x0B):          # meter BE32 clasico
            v = int.from_bytes(data[9:13], "big")
            extra = ""
            if obj == (0x04, 0x01, 0x01):
                extra = f"  ->  1 moneda = {v / 100:.2f} $"
            elif obj == (0x04, 0x01, 0x16):
                extra = f"  ->  {v * 0.01:.2f} $ si son creditos"
            self.out.put(("lectura",
                          f"{otxt} = {NOMBRES.get(obj, 'objeto desconocido')} = {v:08d}{extra}"))
        elif ln == 0x0A and t in (0x08, 0x09):        # BE32 (p. ej. Doors 06 02)
            v = int.from_bytes(data[8:12], "big")
            extra = ""
            if obj == (0x06, 0x02, 0x01):             # estado vivo + eventos
                extra = f"  ->  {_fmt_puerta(v)}"
            self.out.put(("lectura",
                          f"{otxt} = {NOMBRES.get(obj, 'objeto desconocido')} = {v}{extra}"))
        elif ln == 0x0E and t == 0x0C:                # BE64
            v = int.from_bytes(data[8:16], "big")
            self.out.put(("lectura",
                          f"{otxt} = {NOMBRES.get(obj, 'objeto desconocido')} = {v}"))
        elif ln == 0x08 and t == 0x07:                # BE16: denominacion 02 01 05
            v = int.from_bytes(data[8:10], "big")
            extra = ""
            if obj in ((0x02, 0x01, 0x05), (0x04, 0x01, 0x07)):
                extra = f"  ->  1 credito = {v / 100:.2f} $"
            self.out.put(("lectura",
                          f"{otxt} = {NOMBRES.get(obj, 'objeto desconocido')} = {v:08d}{extra}"))
        elif ln == 0x08 and t == 0x06:
            # T06 = palabra de estado: el objeto no tiene dato (muerto/invalido)
            self.out.put(("lectura", f"{otxt} = objeto sin dato (status T06)"))
        elif obj == (0x02, 0x03, 0x03):
            n = (ln - 8) // 4
            self.out.put(("lectura",
                          f"{otxt} = PAQUETE COMPLETO DE CONTADORES ({n} entradas):"))
            for i in range(n):
                v = int.from_bytes(data[9 + 4 * i: 13 + 4 * i], "big")
                if i in BLOQUE_A_OBJETO:
                    o = BLOQUE_A_OBJETO[i]
                    self.out.put(("lectura",
                                  f"  ({o[0]:02X} {o[1]:02X} {o[2]:02X})"
                                  f" = {NOMBRES[o]} = {v:08d}"))
                else:
                    nom = NOMBRES_ENTRADAS_020303.get(i, "sin asignar")
                    self.out.put(("lectura", f"  {otxt}[{i:02d}] = {nom} = {v:08d}"))
        elif obj == (0x01, 0x01, 0x00) and ln >= 0x0A:
            # directorio de dispositivos: count BE32 + pares CL-SM (catalogo)
            pares = _parse_directorio(data[8:-2])
            if pares:
                fams = [f"{a:02X} {b:02X} {FAMILIAS.get((a, b), '?')}" for a, b in pares]
                self.out.put(("lectura",
                              f"{otxt} = DIRECTORIO: {len(pares)} familias validas"))
                for i in range(0, len(fams), 4):
                    self.out.put(("lectura", "  " + " | ".join(fams[i:i + 4])))
            else:
                self.out.put(("lectura",
                              f"{otxt} = Directorio (estructura no reconocida, "
                              f"{ln - 6} B)"))
        else:                                         # bloque / string generico
            d = data[8:-2]
            if obj == (0x04, 0x06, 0x00):            # tabla de billetes en ¢
                hallados = _buscar_billetes(d)
                if hallados:
                    self.out.put(("lectura",
                                  f"{otxt} = Tabla de billetes: "
                                  + "  ".join(f"${v // 100}" for v in hallados)))
                    return
            segs = segmentos_ascii(d)
            if segs:
                self.out.put(("lectura",
                              f"{otxt} = {NOMBRES.get(obj, 'bloque/string')}"
                              f" = {' | '.join(segs)}"))
            elif d:
                hx = " ".join(f"{b:02X}" for b in d[:24])
                self.out.put(("lectura",
                              f"{otxt} = {NOMBRES.get(obj, 'bloque')} = {hx}"
                              + (" ..." if len(d) > 24 else "")))
            else:
                self.out.put(("lectura", f"{otxt} = (sin datos)"))

    def leer_todos(self):
        self._txt("Lectura de todos los contadores")
        for obj in CADENA_METERS:
            self.leer(obj)

    def enviar_raw(self, frame):
        if not crc_ok(frame):
            self._txt("AVISO: trama con CRC invalido (enviada igual, a pedido del usuario)", "err")
        time.sleep(0.03)
        self._xchg(frame, espera=0.2)
        if frame[1] != 0x00:
            self._txt("(recuerda: la DATA llega con el siguiente POLL)")

    # ---- barrido automatico (grid CL-SM-MT) --------------------------------------
    def _leer_rapido(self, obj, reintentos=2):
        """QUERY -> ACK -> POLL... -> DATA, version ligera para el barrido:
        menos reintentos y sin publicar en LECTURAS (el barrido publica su
        propia linea por objeto). Devuelve la lista de tramas DATA (los
        bloques grandes pueden llegar en varias) o [] si no respondio."""
        obj = tuple(obj)
        for _ in range(reintentos):
            if self.barrido_cancel:
                return []
            time.sleep(self.cadencia)
            r = self._xchg(query_frame(self._slot(), obj), espera=0.12)
            if r is None:
                if not self.resincronizar():     # enlace caido insalvable
                    return []
                continue
            frames, vacios = [], 0
            while len(frames) < 4 and vacios < 3:
                r = self.poll()
                if r and len(r) > 4:
                    p = r[2: 2 + r[1]]
                    if p[1] == 0x31 and (p[3], p[4], p[5]) == obj:
                        frames.append(r)
                        vacios = 0
                        continue
                vacios += 1                     # poll sin DATA de este objeto
                if frames:
                    break                        # el bloque ya no trae mas partes
            if frames:
                return frames
        return []

    def _fmt_barrido(self, obj, frames):
        """Linea(s) resumen de un objeto: para el panel LECTURAS y el archivo
        de barrido. Decodifica por ancho (2=BE16+LE16, 4=BE32, 6=meter BE32,
        8=BE64, multiplo de 4=BLOQUE de entradas, resto=ascii) y adjunta el
        hex crudo cuando el dato es largo."""
        otxt = f"({obj[0]:02X} {obj[1]:02X} {obj[2]:02X})"
        nombre = NOMBRES.get(tuple(obj), "")
        if nombre:
            nombre = f"   [{nombre}]"
        if not frames:
            return [f"{otxt} -- sin DATA --"]
        d = bytearray()
        for fr in frames:            # data = payload tras cabecera TT 31 82 CL SM MT
            d += fr[8: 2 + fr[1]]
        d = bytes(d)
        n, t = len(d), frames[0][2]
        if n == 2:
            dec = f"BE16={int.from_bytes(d, 'big')} (LE16={int.from_bytes(d, 'little')})"
        elif n == 4:
            dec = f"BE32={int.from_bytes(d, 'big')}"
        elif n == 6:
            dec = f"meter BE32={int.from_bytes(d[1:5], 'big')}"
        elif n == 8:
            dec = f"BE64={int.from_bytes(d, 'big')}"
        elif n % 4 == 0:
            ents = [int.from_bytes(d[j:j + 4], "big") for j in range(0, n, 4)]
            ascii_ = "".join(chr(b) if 32 <= b < 127 else "." for b in d)
            dec = (f"BLOQUE {len(ents)} entradas BE32: "
                   + " ".join(f"[{j}]={v}" for j, v in enumerate(ents[:8]))
                   + (" ..." if len(ents) > 8 else "")
                   + f"  ascii: {ascii_[:56]}")
        else:
            ascii_ = "".join(chr(b) if 32 <= b < 127 else "." for b in d)
            dec = f"ascii: {ascii_[:70]}"
        lineas = [f"{otxt} LEN={frames[0][1]:02X} T={t:02X} {n}B  {dec}{nombre}"]
        if n > 6:
            hx = " ".join(f"{b:02X}" for b in d)
            lineas.append(f"{otxt} hex: {hx[:200]}" + (" ..." if len(hx) > 200 else ""))
        return lineas

    def barrer_grid(self, rangos):
        """Recorre el grid clases x subs x meters consultando cada objeto
        (SOLO LECTURA). Publica una linea por objeto en LECTURAS y guarda la
        tabla completa en barrido_grid_<fecha>.txt junto al script."""
        clases, subs, meters = rangos
        total = len(clases) * len(subs) * len(meters)
        self.barrido_cancel = False
        self.barrido_activo = True
        ruta = os.path.join(CARPETA,
                            f"barrido_grid_{datetime.now():%Y%m%d_%H%M%S}.txt")
        self._txt(f"BARRIDO INICIADO: {total} objetos, tabla en {ruta}")
        self.out.put(("barrido_prog", f"0/{total}"))
        t0, i, cancelado = time.time(), 0, False
        with open(ruta, "w", encoding="utf-8") as f:
            f.write(f"# Barrido grid Gamma - {datetime.now():%Y-%m-%d %H:%M:%S}\n")
            f.write(f"# clases={[hex(c) for c in clases]} subs={[hex(s) for s in subs]} "
                    f"meters={[hex(m) for m in meters]} -> {total} objetos\n")
            f.write("# formato: (CL SM MT) LEN=xx T=xx <bytes de datos> decodificacion\n\n")
            for cl in clases:
                for sm in subs:
                    for mt in meters:
                        if self.barrido_cancel:
                            cancelado = True
                            break
                        i += 1
                        lineas = self._fmt_barrido((cl, sm, mt),
                                                   self._leer_rapido((cl, sm, mt)))
                        for linea in lineas:
                            f.write(linea + "\n")
                        self.out.put(("lectura", lineas[0]))
                        if i % 20 == 0 or i == total:
                            dt = time.time() - t0
                            resto = (total - i) * (dt / i) if i else 0
                            self.out.put(("barrido_prog",
                                          f"{i}/{total}  ({dt:.0f} s, quedan ~{resto:.0f} s)"))
                    if cancelado:
                        break
                if cancelado:
                    break
            dt = time.time() - t0
            f.write(f"\n# FIN: {i}/{total} en {dt:.0f} s"
                    + (" (CANCELADO)" if cancelado else "") + "\n")
        self.barrido_activo = False
        resumen = f"BARRIDO {'CANCELADO' if cancelado else 'COMPLETO'}: {i}/{total} en {dt:.0f} s"
        self._txt(resumen + f" - tabla completa en {ruta}", "ok")
        self.out.put(("barrido_fin", resumen))

    # ---- bucle principal --------------------------------------------------------
    def run(self):
        self.out.put(("estado", "SONDEANDO"))
        if not self.sondeo():
            self.out.put(("estado", "SIN RESPUESTA"))
        else:
            self.out.put(("estado", "EN LINEA"))
        ultimo_stats = 0.0
        while self.running:
            cmd, arg = None, None
            try:
                cmd, arg = self.cmds.get(timeout=0.2)
            except queue.Empty:
                pass
            if cmd == "salir":
                break
            elif cmd == "sondeo":
                self.sondeo(30.0)
            elif cmd == "todos":
                self.leer_todos()
            elif cmd == "leer":
                self.leer(arg)
            elif cmd == "raw":                     # (en v1 quedaba encolado y sin efecto)
                self.enviar_raw(arg)
            elif cmd == "barrido":
                self.barrer_grid(arg)
            elif cmd == "cancelar_barrido":
                self.barrido_cancel = True

            if time.time() - ultimo_stats > 0.5:
                self.out.put(("stats", self.frames_ok, self.frames_bad,
                              SLOTS[self.slot_idx], self.ultimo_ok))
                ultimo_stats = time.time()

            try:
                if self.monitoreo and time.time() >= self._prox_mon:
                    self.leer_todos()
                    self._prox_mon = time.time() + 5.0
                elif self.keepalive:
                    self._xchg(empty_frame(self._slot()), espera=0.08,
                               quiet=not self.mostrar_ka)   # (v1: siempre quiet)
                if self.sniffer:
                    self._leer_pendiente()   # escucha activa en los huecos idle
            except (serial.SerialException, OSError):
                self._txt("ERROR de puerto serie (¿cable desconectado?)", "err")
                break
        try:
            self.ser.close()
        except Exception:
            pass

    def detener(self):
        self.running = False
        self.cmds.put(("salir", None))


# ============================================================================
# COMANDOS DEL DROPDOWN (solo el nombre; el codigo de objeto no se muestra)
# ============================================================================
COMANDOS = [
    ("Sondeo de arranque (poll 0x40)", "sondeo", None),
    ("Leer TODOS los contadores", "todos", None),
    ("Current Credits", "leer", (0x02, 0x03, 0x01)),
    ("Total Games Played", "leer", (0x02, 0x04, 0x04)),
    ("Total Credits Bet", "leer", (0x02, 0x04, 0x05)),
    ("Total Credits Won", "leer", (0x02, 0x04, 0x06)),
    ("Split Voucher Pays - ticket out (02 05 03)", "leer", (0x02, 0x05, 0x03)),
    ("Sin asignar (handpay?)", "leer", (0x02, 0x05, 0x04)),
    ("True In (Coins In)", "leer", (0x04, 0x09, 0x01)),
    ("Coin Drop / Gross Drop?", "leer", (0x04, 0x04, 0x01)),
    ("DENOMINACION: value of 1 credit (02 01 05)", "leer", (0x02, 0x01, 0x05)),
    ("Value of 1 coin (NO es la denominacion)", "leer", (0x04, 0x01, 0x01)),
    ("Hopper P7 (candidato vencido)", "leer", (0x04, 0x01, 0x07)),
    ("Apuesta maxima?", "leer", (0x04, 0x01, 0x16)),
    ("Meter grande", "leer", (0x02, 0x02, 0x04)),
    ("Paquete completo de contadores (24)", "leer", (0x02, 0x03, 0x03)),
    ("Version / serie", "leer", (0x01, 0x02, 0x01)),
    ("Fabricante / paytable", "leer", (0x01, 0x02, 0x02)),
    ("Juego / % payback", "leer", (0x03, 0x01, 0x00)),
    ("Identidad LIMPIA sin meters (03 01 02)", "leer", (0x03, 0x01, 0x02)),
    ("Directorio de familias (01 01 00)", "leer", (0x01, 0x01, 0x00)),
    ("Moneda interna (02 01 02)", "leer", (0x02, 0x01, 0x02)),
    ("Jackpot limit ¢ (02 01 03)", "leer", (0x02, 0x01, 0x03)),
    ("Jackpot win limit (02 04 07)", "leer", (0x02, 0x04, 0x07)),
    ("Max bet limit ¢ (02 05 07)", "leer", (0x02, 0x05, 0x07)),
    ("Host Initiated Disable (02 04 03)", "leer", (0x02, 0x04, 0x03)),
    ("Tabla de billetes $1-$100 (04 06 00)", "leer", (0x04, 0x06, 0x00)),
    ("Puertas P1: estado + aperturas (06 02 01)", "leer", (0x06, 0x02, 0x01)),
    ("Puertas P2: cash box accesses (06 02 02)", "leer", (0x06, 0x02, 0x02)),
    ("Puertas P3: bill acceptor door (06 02 03)", "leer", (0x06, 0x02, 0x03)),
    ("SPC-2 identidad (80 02 00)", "leer", (0x80, 0x02, 0x00)),
]


# ============================================================================
# INTERFAZ GRAFICA v2
# ============================================================================
class App:
    def __init__(self, root):
        self.root = root
        root.title("GAMMA TEST v2 - Monitor de comandos (protocolo Gamma Aristocrat) - SOLO LECTURA")
        root.geometry("1060x760")
        root.minsize(880, 560)
        self.worker = None
        self.out_q = queue.Queue()
        self.log_buf = []              # lineas del log (formato clasico, con desc)

        self._construir()
        self._refrescar_puertos()
        self.root.after(80, self._drenar)
        root.protocol("WM_DELETE_WINDOW", self._cerrar)

    # ---- construccion de widgets ------------------------------------------------
    def _construir(self):
        estilo = ttk.Style()
        for tema in ("vista", "clam", "alt"):
            if tema in estilo.theme_names():
                estilo.theme_use(tema)
                break

        # --- barra superior: conexion + estado ---
        top = ttk.Frame(self.root, padding=(8, 6))
        top.pack(fill="x")
        ttk.Label(top, text="Puerto:").pack(side="left")
        self.cb_puerto = ttk.Combobox(top, width=9, state="readonly")
        self.cb_puerto.pack(side="left", padx=4)
        ttk.Button(top, text="Refrescar", command=self._refrescar_puertos).pack(side="left")
        self.btn_con = ttk.Button(top, text="Conectar", command=self._conectar)
        self.btn_con.pack(side="left", padx=8)
        self.lbl_estado = ttk.Label(top, text="DESCONECTADO", foreground="gray",
                                    font=("Segoe UI", 10, "bold"))
        self.lbl_estado.pack(side="left", padx=12)
        self.lbl_stats = ttk.Label(top, text="", foreground="gray")
        self.lbl_stats.pack(side="right")

        # --- barra de comando ---
        barra = ttk.LabelFrame(self.root, text=" COMANDO ", padding=(8, 4))
        barra.pack(fill="x", padx=8, pady=(0, 4))

        fila1 = ttk.Frame(barra)
        fila1.pack(fill="x", pady=2)
        self.cb_cmd = ttk.Combobox(fila1, state="readonly", width=42,
                                   values=[c[0] for c in COMANDOS])
        self.cb_cmd.current(0)
        self.cb_cmd.pack(side="left")
        self.cb_cmd.bind("<Return>", lambda e: self._enviar_seleccion())
        ttk.Button(fila1, text="ENVIAR", command=self._enviar_seleccion).pack(side="left", padx=8)

        fila2 = ttk.Frame(barra)
        fila2.pack(fill="x", pady=2)
        ttk.Label(fila2, text="Objeto arbitrario (CL SM MT):").pack(side="left")
        self.ent_arb = ttk.Entry(fila2, width=9)
        self.ent_arb.insert(0, "02 05 05")
        self.ent_arb.pack(side="left", padx=4)
        self.ent_arb.bind("<Return>", lambda e: self._consultar_arb())
        ttk.Button(fila2, text="Consultar", command=self._consultar_arb).pack(side="left", padx=(0, 16))
        ttk.Label(fila2, text="Trama hex manual:").pack(side="left")
        self.ent_raw = ttk.Entry(fila2, width=22)
        self.ent_raw.pack(side="left", padx=4)
        self.ent_raw.bind("<Return>", lambda e: self._enviar_raw())
        ttk.Button(fila2, text="Enviar", command=self._enviar_raw).pack(side="left")

        fila3 = ttk.Frame(barra)
        fila3.pack(fill="x", pady=(6, 2))
        self.var_ka = tk.BooleanVar(value=True)
        ttk.Checkbutton(fila3, text="Keep-alive de fondo (recomendado)",
                        variable=self.var_ka,
                        command=self._set_ka).pack(side="left")
        self.var_ver_ka = tk.BooleanVar(value=False)
        ttk.Checkbutton(fila3, text="Ver keep-alive en monitor",
                        variable=self.var_ver_ka,
                        command=self._set_ver_ka).pack(side="left", padx=(0, 16))
        self.var_mon = tk.BooleanVar(value=False)
        ttk.Checkbutton(fila3, text="Monitoreo continuo (cada 5 s)",
                        variable=self.var_mon,
                        command=self._set_monitoreo).pack(side="left")
        self.var_snif = tk.BooleanVar(value=False)
        ttk.Checkbutton(fila3, text="Escuchar todo (sniffer)",
                        variable=self.var_snif,
                        command=self._set_sniffer).pack(side="left", padx=(0, 16))
        ttk.Button(fila3, text="Limpiar monitor", command=self._limpiar).pack(side="right")
        ttk.Button(fila3, text="Guardar log...", command=self._guardar_log).pack(side="right", padx=8)

        fila4 = ttk.Frame(barra)
        fila4.pack(fill="x", pady=(6, 2))
        self.var_barr = tk.BooleanVar(value=False)
        ttk.Checkbutton(fila4, text="Barrido automatico (grid CL SM MT)",
                        variable=self.var_barr).pack(side="left")
        ttk.Label(fila4, text="Clases:").pack(side="left", padx=(12, 0))
        self.ent_clases = ttk.Entry(fila4, width=12)
        self.ent_clases.insert(0, "01-09,80,81")
        self.ent_clases.pack(side="left", padx=2)
        ttk.Label(fila4, text="Sub:").pack(side="left", padx=(8, 0))
        self.ent_subs = ttk.Entry(fila4, width=7)
        self.ent_subs.insert(0, "01-08")
        self.ent_subs.pack(side="left", padx=2)
        ttk.Label(fila4, text="Meter:").pack(side="left", padx=(8, 0))
        self.ent_meters = ttk.Entry(fila4, width=8)
        self.ent_meters.insert(0, "00-16")
        self.ent_meters.pack(side="left", padx=2)
        self.btn_barr = ttk.Button(fila4, text="INICIAR BARRIDO",
                                   command=self._iniciar_barrido)
        self.btn_barr.pack(side="left", padx=10)
        self.btn_barr_stop = ttk.Button(fila4, text="Detener",
                                        command=self._detener_barrido,
                                        state="disabled")
        self.btn_barr_stop.pack(side="left")
        self.lbl_prog = ttk.Label(fila4, text="", foreground="gray")
        self.lbl_prog.pack(side="left", padx=10)

        # --- cuerpo: lecturas arriba / tramas abajo (panel redimensionable) ---
        cuerpo = ttk.PanedWindow(self.root, orient="vertical")
        cuerpo.pack(fill="both", expand=True, padx=8, pady=(0, 8))

        sup = ttk.LabelFrame(cuerpo, text=" LECTURAS -  (objeto) = nombre = valor  ")
        self.lecturas = ScrolledText(sup, wrap="none", font=("Consolas", 10),
                                     state="disabled", height=9)
        self.lecturas.pack(fill="both", expand=True)
        for tag, color in (("lect", "#1a1a1a"), ("hdr", "#1a3f8f")):
            self.lecturas.tag_configure(tag, foreground=color)
        cuerpo.add(sup, weight=1)

        inf = ttk.LabelFrame(cuerpo,
                             text=" MONITOR TX/RX - tramas crudas   (RX** = inesperada: evento montado en poll / no solicitada) ")
        self.monitor = ScrolledText(inf, wrap="none", font=("Consolas", 9),
                                    state="disabled", height=11)
        self.monitor.pack(fill="both", expand=True)
        for tag, color in (("tx", "#1a3f8f"), ("rx", "#0a6b24"), ("sni", "#b07000"),
                           ("info", "#555555"), ("ok", "#0a6b24"), ("err", "#c02020")):
            self.monitor.tag_configure(tag, foreground=color)
        cuerpo.add(inf, weight=2)

    # ---- insert con limite de lineas ---------------------------------------------
    @staticmethod
    def _insertar(widget, texto, tag):
        widget.config(state="normal")
        widget.insert("end", texto, tag)
        widget.see("end")
        if int(widget.index("end-1c").split(".")[0]) > 3000:
            widget.delete("1.0", "300.0")
        widget.config(state="disabled")

    # ---- cola GUI ---------------------------------------------------------------
    def _drenar(self):
        try:
            while True:
                ev = self.out_q.get_nowait()
                kind = ev[0]
                if kind == "log":
                    _, direccion, frame = ev
                    hextxt = " ".join(f"{b:02X}" for b in frame)
                    desc = describe(frame)
                    ts = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                    # al log va la linea clasica completa (hex + interpretacion)
                    self.log_buf.append(f"[{ts}] {direccion:<4} {hextxt:<64} {desc}\n")
                    # a pantalla solo la trama
                    if direccion.startswith("TX"):
                        tag = "tx"
                    elif "**" in direccion:
                        tag = "sni"          # no solicitada / fuera de ventana
                    else:
                        tag = "rx"
                    marca = ""
                    if "INVALIDO" in desc:
                        tag, marca = "err", "   *** CRC INVALIDO ***"
                    self._insertar(self.monitor,
                                   f"[{ts}] {direccion:<4} {hextxt}{marca}\n", tag)
                elif kind == "txt":
                    _, texto, tipo = ev
                    self.log_buf.append(f"--- {texto}\n")
                    self._insertar(self.monitor, f"--- {texto}\n", tipo)
                elif kind == "lectura":
                    _, texto = ev
                    tag = "hdr" if "PAQUETE COMPLETO" in texto else "lect"
                    self._insertar(self.lecturas, f"{texto}\n", tag)
                elif kind == "estado":
                    _, texto = ev
                    color = {"EN LINEA": "#0a6b24", "SONDEANDO": "#b07000",
                             "SIN RESPUESTA": "#c02020"}.get(texto, "gray")
                    self.lbl_estado.config(text=texto, foreground=color)
                elif kind == "barrido_prog":
                    self.lbl_prog.config(text=ev[1])
                elif kind == "barrido_fin":
                    self.lbl_prog.config(text=ev[1])
                    self.btn_barr.config(state="normal")
                    self.btn_barr_stop.config(state="disabled")
                elif kind == "stats":
                    _, ok, bad, slot, ultimo_ok = ev
                    vivo = time.time() - ultimo_ok < 5 if ultimo_ok else False
                    self.lbl_stats.config(
                        text=f"CRC ok={ok}  bad={bad}  slot prox=0x{slot:02X}  "
                             f"enlace={'ACTIVO' if vivo else 'sin trafico'}")
        except queue.Empty:
            pass
        self.root.after(80, self._drenar)

    # ---- log ----------------------------------------------------------------------
    def _guardar_log(self):
        if not self.log_buf:
            messagebox.showinfo("Log", "No hay nada que guardar todavia.")
            return
        sugerido = datetime.now().strftime("log_gamma_%Y%m%d_%H%M%S.txt")
        ruta = filedialog.asksaveasfilename(
            title="Guardar log de la sesion",
            defaultextension=".txt",
            initialfile=sugerido,
            filetypes=[("Log de texto", "*.txt"), ("Todos", "*.*")])
        if not ruta:
            return
        try:
            with open(ruta, "w", encoding="utf-8") as f:
                f.writelines(self.log_buf)
                f.write(f"=== Log guardado {datetime.now():%d/%m/%Y %H:%M:%S} ===\n")
        except OSError as e:
            messagebox.showerror("Log", f"No se pudo guardar:\n{e}")
            return
        messagebox.showinfo("Log", f"Guardadas {len(self.log_buf)} lineas en:\n{ruta}")

    # ---- acciones ------------------------------------------------------------------
    def _refrescar_puertos(self):
        puertos = [p.device for p in list_ports.comports()]
        self.cb_puerto["values"] = puertos
        if puertos and not self.cb_puerto.get():
            self.cb_puerto.current(0)

    def _conectar(self):
        if self.worker is not None:
            self._desconectar()
            return
        puerto = self.cb_puerto.get()
        if not puerto:
            messagebox.showwarning("Puerto", "Selecciona un puerto serie.")
            return
        try:
            self.worker = Protocolo(puerto, self.out_q)
        except serial.SerialException as e:
            messagebox.showerror("Puerto", f"No se pudo abrir {puerto}:\n{e}")
            self.worker = None
            return
        # Sincronizar el estado de los checks con el worker recien creado:
        # si el usuario los marco ANTES de conectar, no habian llegado a nadie
        # (_set_* solo aplica si ya habia worker) y el sniffer quedaba OFF.
        self.worker.keepalive = self.var_ka.get()
        self.worker.mostrar_ka = self.var_ver_ka.get()
        self.worker.monitoreo = self.var_mon.get()
        self.worker.sniffer = self.var_snif.get()
        self.worker.start()
        self.btn_con.config(text="Desconectar")
        linea = f"=== Conectado a {puerto} (19200 8N1) ===\n"
        self.log_buf.append(linea)
        self._insertar(self.monitor, linea, "info")

    def _desconectar(self):
        if self.worker is None:
            return
        self.worker.detener()
        self.worker.join(timeout=3)
        self.worker = None
        self.btn_con.config(text="Conectar")
        self.lbl_estado.config(text="DESCONECTADO", foreground="gray")
        self.lbl_stats.config(text="")
        self.btn_barr.config(state="normal")
        self.btn_barr_stop.config(state="disabled")
        self.lbl_prog.config(text="")
        linea = "=== Desconectado ===\n"
        self.log_buf.append(linea)
        self._insertar(self.monitor, linea, "info")

    def _enviar_seleccion(self, evt=None):
        w = self.worker
        if w is None:
            messagebox.showwarning("Sin conexion", "Conecta primero el puerto.")
            return
        idx = self.cb_cmd.current()
        if idx < 0:
            messagebox.showinfo("Comando", "Selecciona un comando de la lista.")
            return
        label, kind, arg = COMANDOS[idx]
        if kind is None:
            return
        w.cmds.put((kind, arg))

    def _consultar_arb(self):
        w = self.worker
        if w is None:
            messagebox.showwarning("Sin conexion", "Conecta primero el puerto.")
            return
        try:
            b = bytes.fromhex(self.ent_arb.get().strip().replace(" ", ""))
            if len(b) != 3:
                raise ValueError
        except Exception:
            messagebox.showerror("Hex", "Objeto invalido. Formato: 02 05 05")
            return
        w.cmds.put(("leer", (b[0], b[1], b[2])))

    def _enviar_raw(self):
        w = self.worker
        if w is None:
            messagebox.showwarning("Sin conexion", "Conecta primero el puerto.")
            return
        try:
            frame = bytes.fromhex(self.ent_raw.get().strip().replace(" ", ""))
        except ValueError:
            messagebox.showerror("Hex", "Trama invalida.")
            return
        if len(frame) < 4:
            messagebox.showerror("Hex", "Trama demasiado corta.")
            return
        if not crc_ok(frame) and not messagebox.askyesno(
                "CRC", "El CRC de esa trama NO es valido. Enviar de todos modos?"):
            return
        w.cmds.put(("raw", frame))

    # ---- barrido automatico -------------------------------------------------------
    def _parse_rango(self, txt, defecto):
        """Parsea rangos hex tipo '01-09,80,81' (comas, guion = rango inclusive)."""
        vals = []
        for parte in txt.replace(" ", "").split(","):
            if not parte:
                continue
            try:
                if "-" in parte:
                    a, b = parte.split("-", 1)
                    lo, hi = int(a, 16), int(b, 16)
                    if lo > hi:
                        lo, hi = hi, lo
                    vals += list(range(lo, hi + 1))
                else:
                    vals.append(int(parte, 16))
            except ValueError:
                messagebox.showerror(
                    "Rango", f"Rango invalido: '{parte}' (formato: 01-09,80,81)")
                return None
        return sorted(set(vals)) if vals else list(defecto)

    def _iniciar_barrido(self):
        w = self.worker
        if w is None:
            messagebox.showwarning("Sin conexion", "Conecta primero el puerto.")
            return
        if not self.var_barr.get():
            messagebox.showwarning(
                "Barrido", "Marca el checkbox 'Barrido automatico' primero.")
            return
        clases = self._parse_rango(self.ent_clases.get(), range(0x01, 0x0A))
        if clases is None:
            return
        subs = self._parse_rango(self.ent_subs.get(), range(0x01, 0x09))
        if subs is None:
            return
        meters = self._parse_rango(self.ent_meters.get(), range(0x00, 0x17))
        if meters is None:
            return
        total = len(clases) * len(subs) * len(meters)
        est = total * 0.45 / 60
        if not messagebox.askyesno(
                "Barrido",
                f"{total} objetos a consultar.\n"
                f"Tiempo estimado: ~{est:.0f}-{est * 1.5:.0f} min.\n\n"
                "Solo lecturas (no se escribe nada en la maquina).\n"
                "La tabla completa queda en barrido_grid_<fecha>.txt "
                "junto al script.\n\nIniciar?"):
            return
        self.btn_barr.config(state="disabled")
        self.btn_barr_stop.config(state="normal")
        self.lbl_prog.config(text="0/" + str(total))
        w.cmds.put(("barrido", (clases, subs, meters)))

    def _detener_barrido(self):
        if self.worker:
            self.worker.cmds.put(("cancelar_barrido", None))
            self.lbl_prog.config(text="deteniendo...")

    def _set_ka(self):
        if self.worker:
            self.worker.keepalive = self.var_ka.get()

    def _set_ver_ka(self):
        if self.worker:
            self.worker.mostrar_ka = self.var_ver_ka.get()

    def _set_monitoreo(self):
        if self.worker:
            self.worker.monitoreo = self.var_mon.get()
            self.worker._prox_mon = time.time() + 0.5

    def _set_sniffer(self):
        if self.worker:
            self.worker.sniffer = self.var_snif.get()

    def _limpiar(self):
        for w in (self.lecturas, self.monitor):
            w.config(state="normal")
            w.delete("1.0", "end")
            w.config(state="disabled")
        self.log_buf.clear()

    def _cerrar(self):
        self._desconectar()
        self.root.destroy()


def main():
    root = tk.Tk()
    App(root)
    root.mainloop()


if __name__ == "__main__":
    main()
