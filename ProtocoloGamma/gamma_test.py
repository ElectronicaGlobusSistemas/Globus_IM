#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
=============================================================================
 GAMMA TEST - Herramienta SPC (maestro) para maquina Aristocrat / protocolo Gamma
=============================================================================
Se conecta a la MAQUINA REAL por un puerto serie (adaptador USB-RS232) y le
envia los comandos de consulta de contadores, mostrando por consola:

  * la trama cruda que se envia (TX>>) y la que responde la maquina (RX<<)
  * la interpretacion de cada trama (QUERY / ACK / POLL / DATA / CRC)
  * el valor de cada contador en formato de 8 digitos:  01353790  Total Games Played
  * comparacion contra los valores de la captura del 27-08-2026

Sirve para VERIFICAR EXPERIMENTALMENTE todo lo deducido del analisis
(docs/Manual_Protocolo_Gamma.md).

CONEXION (la maquina es RS-232 verdadera, +-V):
  Adaptador USB-RS232          Conector SPC de la maquina
    TXD (sale datos)  ───────►  linea RX de la maquina (por donde hablaba la SPC)
    RXD (entra datos) ◄───────  linea TX de la maquina (por donde respondia)
    GND               ───────  GND

USO:
  python gamma_test.py --port COM10
  python gamma_test.py --port COM10 --slot 78     (empezar en slot 0x78, como la captura)
  (requiere:  pip install pyserial )

Modelo implementado (Manual, seccion 6):
  QUERY(10B) -> ACK(4B) -> POLL(4B) -> DATA(16B o bloque)
  CRC-16/XMODEM poly 0x1021 init 0x0000, MSB primero.
  Slots 0x40..0x78 rotando +8 por transaccion. Cadencia ~30 ms.
  HERRAMIENTA DE SOLO LECTURA: no escribe nada en la maquina.
"""

import argparse
import sys
import threading
import time
from datetime import datetime

try:
    import serial
except ImportError:
    sys.exit("Falta pyserial. Instalar con:  pip install pyserial")

# ---------------------------------------------------------------------------
# Protocolo
# ---------------------------------------------------------------------------
SLOTS = [0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78]

NOMBRES = {
    (0x02, 0x03, 0x01): "Current Credits",
    (0x02, 0x04, 0x04): "Total Games Played",
    (0x02, 0x04, 0x05): "Total Credits Bet",
    (0x02, 0x04, 0x06): "Total Credits Won",
    (0x02, 0x05, 0x03): "Cashout Handpays",
    (0x02, 0x05, 0x04): "Sin asignar (handpay?)",
    (0x04, 0x09, 0x01): "True In (Coins In)",
    (0x04, 0x04, 0x01): "Coin Drop / Gross Drop?",
    (0x04, 0x01, 0x01): "Denominacion SAS",
    (0x04, 0x01, 0x16): "Apuesta maxima?",
    (0x02, 0x02, 0x04): "Meter grande (02 02 04)",
}

# Valores reales leidos en la captura del 27-08-2026 (para comparar)
VALORES_CAPTURA = {
    (0x02, 0x03, 0x01): 1194,
    (0x02, 0x04, 0x04): 1353790,
    (0x02, 0x04, 0x05): 35725055,
    (0x02, 0x04, 0x06): 31052141,
    (0x02, 0x05, 0x03): 8080392,
    (0x02, 0x05, 0x04): 0,
    (0x04, 0x09, 0x01): 12754500,
    (0x04, 0x04, 0x01): 12754500,
    (0x04, 0x01, 0x01): 100,
    (0x04, 0x01, 0x16): 20000,
}

# Cadena del Anexo F.10 del manual (orden de la captura)
CADENA_METERS = [
    (0x02, 0x04, 0x05),
    (0x02, 0x04, 0x06),
    (0x02, 0x05, 0x03),
    (0x02, 0x05, 0x04),
    (0x04, 0x09, 0x01),
    (0x02, 0x03, 0x01),
    (0x04, 0x04, 0x01),
    (0x02, 0x04, 0x04),
]

# Informacion de la maquina (Anexo G)
CADENA_INFO = [
    (0x04, 0x01, 0x01),   # denominacion SAS
    (0x01, 0x02, 0x02),   # fabricante / paytable / ID juego
    (0x03, 0x01, 0x00),   # juego / % payback / meters
    (0x01, 0x02, 0x01),   # version / serie
]


def gamma_crc16(data: bytes) -> int:
    """CRC-16/XMODEM (poly 0x1021, init 0x0000, sin reflexion). Verificado 1561/1561 tramas."""
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


# ---------------------------------------------------------------------------
# Visualizacion
# ---------------------------------------------------------------------------
def H(frame: bytes) -> str:
    return " ".join(f"{b:02X}" for b in frame)


def describe(frame: bytes) -> str:
    if not crc_ok(frame):
        return "*** CRC INVALIDO ***"
    seq, ln = frame[0], frame[1]
    if ln == 0:
        return "POLL/ACK vacio"
    p = frame[2 : 2 + ln]
    if ln == 6 and p[1] == 0x30 and p[2] == 0x02:
        obj = (p[3], p[4], p[5])
        return f"QUERY {NOMBRES.get(obj, 'objeto desconocido')} ({obj[0]:02X} {obj[1]:02X} {obj[2]:02X})"
    if p[1] == 0x31 and ln >= 0x0C:
        obj = (p[3], p[4], p[5])
        if obj in NOMBRES and ln == 0x0C:
            v = int.from_bytes(frame[9:13], "big")
            vtxt = f"{v:,}".replace(",", ".")
            extra = ""
            if obj == (0x04, 0x01, 0x01):
                extra = f"  (= {v * 0.0001:.4f} $)"
            elif obj == (0x04, 0x01, 0x16):
                extra = f"  (= {v * 0.01:.2f} $ si son creditos)"
            return f"DATA -> {v:08d}  {NOMBRES[obj]}  (valor decimal: {vtxt}){extra}"
        if obj == (0x02, 0x03, 0x03):
            return f"DATA bloque multi-meter ({ln + 4} B)"
        if obj[0] == 0x01 or obj == (0x03, 0x01, 0x00):
            txt = "".join(chr(b) if 32 <= b < 127 else "." for b in frame[9:-2])[:70]
            return f"DATA identidad: {txt}"
        return f"DATA bloque ({ln + 4} B)"
    if ln == 4 and p[0] == 0x02:
        return f"STATUS 02 {p[1]:02X}"
    return f"comando (LEN={ln:02X})"


def log(direction: str, frame: bytes):
    ts = datetime.now().strftime("%H:%M:%S.%f")[:-3]
    print(f"[{ts}] {direction:<4} {H(frame):<66} {describe(frame)}")


def banner(texto: str):
    print("\n" + "-" * 78)
    print(f" {texto}")
    print("-" * 78)


# ---------------------------------------------------------------------------
# SPC (maestro) - el corazon de la herramienta
# ---------------------------------------------------------------------------
class SPC:
    def __init__(self, ser, slot_inicial=0x40, cadencia=0.030):
        self.ser = ser
        self.slot_idx = SLOTS.index(slot_inicial)
        self.cadencia = cadencia          # pausa entre transacciones (como la SPC real)
        self.frames_ok = 0
        self.frames_bad = 0
        self.lock = threading.Lock()
        self._ka_stop = threading.Event()
        self._ka_thread = None

    # ---- slots -------------------------------------------------------------
    def _slot(self) -> int:
        s = SLOTS[self.slot_idx]
        self.slot_idx = (self.slot_idx + 1) % 8
        return s

    # ---- capa de trama -------------------------------------------------------
    def _recibir_trama(self, timeout=0.15):
        """Lee bytes hasta un silencio >= 5 ms (fin de trama). None si timeout."""
        buf = bytearray()
        fin = time.time() + timeout
        while time.time() < fin:
            data = self.ser.read(256)
            if data:
                buf += data
                fin = time.time() + 0.03      # extiende: la trama sigue llegando
            elif buf:
                # silencio: verificar si ya paso el gap minimo desde el ultimo byte
                time.sleep(0.005)
                if not self.ser.read(256):
                    break
        if not buf:
            return None
        trama = bytes(buf)
        if crc_ok(trama):
            self.frames_ok += 1
        else:
            self.frames_bad += 1
            self.ser.reset_input_buffer()   # descartar basura acumulada
        return trama

    def _xchg(self, frame: bytes, espera=0.15, quiet=False):
        """Envia una trama y espera la respuesta. Muestra ambas por consola."""
        if not quiet:
            log("TX>>", frame)
        self.ser.reset_input_buffer()
        self.ser.write(frame)
        self.ser.flush()
        resp = self._recibir_trama(espera)
        if resp is not None and not quiet:
            log("RX<<", resp)
        return resp

    # ---- transacciones de alto nivel -------------------------------------------
    def poll(self) -> bytes:
        time.sleep(self.cadencia)
        with self.lock:
            return self._xchg(empty_frame(self._slot()), espera=0.1)

    def resincronizar(self, max_seg=10.0):
        """Enlace caido (la maquina deja de responder tras silencio prolongado):
        solo readquiere al ver un poll del slot 0x40. Tras el eco, espera 0x48."""
        print("   ... enlace caido -> resincronizando con poll del slot 0x40")
        t0 = time.time()
        while time.time() - t0 < max_seg:
            with self.lock:
                resp = self._xchg(empty_frame(0x40), espera=0.25)
            if resp is not None:
                self.slot_idx = 1          # tras el eco de 0x40, la maquina espera 0x48
                print("   ... enlace restablecido")
                return True
        print("   ... no se pudo resincronizar")
        return False

    # ---- keep-alive de fondo (evita que el enlace caiga en el menu) -----------
    def ka_iniciar(self, periodo=0.2):
        if self._ka_thread is not None:
            return
        self._ka_stop.clear()

        def bucle():
            while not self._ka_stop.is_set():
                try:
                    with self.lock:
                        self._xchg(empty_frame(self._slot()), espera=0.08, quiet=True)
                except Exception:
                    break
                self._ka_stop.wait(periodo)

        self._ka_thread = threading.Thread(target=bucle, daemon=True)
        self._ka_thread.start()

    def ka_detener(self):
        if self._ka_thread is None:
            return
        self._ka_stop.set()
        self._ka_thread.join()
        self._ka_thread = None

    def sondeo_arranque(self, max_seg=30.0):
        """Como la SPC real: poll del slot 0x40 cada ~250 ms hasta que la maquina conteste."""
        banner("SONDEO DE ARRANQUE (poll slot 0x40 cada 250 ms hasta eco)")
        t0 = time.time()
        while time.time() - t0 < max_seg:
            with self.lock:
                resp = self._xchg(empty_frame(0x40), espera=0.25)
            if resp is not None:
                self.slot_idx = 1          # tras el eco de 0x40, la maquina espera 0x48
                print(" >> MAQUINA EN LINEA")
                return True
            print("   ... sin respuesta (maquina apagada/boteando?)")
        print(" >> Sin respuesta. Abortando.")
        return False

    def _esperar_data(self, obj, polls=4):
        """Envia polls hasta recibir la DATA del objeto (modelo diferido)."""
        for _ in range(polls):
            resp = self.poll()
            if resp and len(resp) > 4:
                p = resp[2 : 2 + resp[1]]
                if p[1] == 0x31 and (p[3], p[4], p[5]) == tuple(obj):
                    return resp
        return None

    def leer(self, obj, reintentos=3):
        """Transaccion completa: QUERY -> ACK -> POLL... -> DATA. Devuelve la DATA o None."""
        obj = tuple(obj)
        for _ in range(reintentos):
            time.sleep(self.cadencia)
            with self.lock:
                resp = self._xchg(query_frame(self._slot(), obj), espera=0.12)
            if resp is None:
                # la maquina no respondio: enlace caido por inactividad -> resincronizar
                if not self.resincronizar():
                    return None
                continue
            data = self._esperar_data(obj)
            if data is not None:
                return data
            print(f"   ... reintentando {NOMBRES.get(obj, obj)}")
        return None

    def leer_valor(self, obj):
        data = self.leer(obj)
        if data and data[1] == 0x0C:
            return int.from_bytes(data[9:13], "big")
        return None


# ---------------------------------------------------------------------------
# Operaciones de menu
# ---------------------------------------------------------------------------
def op_todos(spc: SPC):
    banner("LECTURA DE TODOS LOS CONTADORES (cadena Anexo F.10) + comparacion con captura")
    leidos = {}
    for obj in CADENA_METERS:
        data = spc.leer(obj)
        print("")
        if data and data[1] == 0x0C:
            leidos[obj] = int.from_bytes(data[9:13], "big")
        else:
            print(f" !! Sin DATA para {obj}")
    print("\n" + "=" * 66)
    print(" VALOR LEIDO AHORA           CAPTURA 27-08    OBJETO")
    print("=" * 66)
    for obj in CADENA_METERS:
        v = leidos.get(obj)
        ref = VALORES_CAPTURA.get(obj)
        if v is None:
            marca, txt = "??", "--------- FALLÓ ---------"
        elif ref is None:
            marca, txt = "  ", f"{v:08d}"
        elif v == ref:
            marca, txt = "=", f"{v:08d}   =   {ref:08d}   IDENTICO a la captura"
        else:
            marca, txt = "~", f"{v:08d}   vs  {ref:08d}   (difiere: {v - ref:+,})"
        print(f"  {txt:<46} {marca} {NOMBRES.get(obj, obj)}")
    print("=" * 66)
    print(" (= identico | ~ difiere (normal si la maquina ha jugado desde la captura)")


def op_info(spc: SPC):
    banner("INFORMACION DE LA MAQUINA (Anexo G)")
    for obj in CADENA_INFO:
        data = spc.leer(obj)
        print("")
        if data is None:
            print(f" !! Sin DATA para {obj}")


def op_multibloque(spc: SPC):
    banner("BLOQUE MULTI-METER 02 03 03 (24 contadores en una consulta)")
    data = spc.leer((0x02, 0x03, 0x03))
    if data is None:
        print(" !! Sin DATA")
        return
    if data[1] == 0x68:
        idx_a_obj = {1: (0x02, 0x03, 0x01), 2: (0x02, 0x04, 0x04), 4: (0x02, 0x04, 0x05),
                     5: (0x02, 0x04, 0x06), 13: (0x02, 0x05, 0x03), 15: (0x04, 0x09, 0x01)}
        print("\n entry  VALOR       CONTADOR (layout deducido de la captura)")
        for i, obj in sorted(idx_a_obj.items()):
            v = int.from_bytes(data[9 + 4 * i : 13 + 4 * i], "big")
            print(f"  [{i:2d}]  {v:08d}    {NOMBRES[obj]}")
        print("\n (las demas entradas: 3=476780, 8=12828900, resto 0 en la captura)")
    else:
        print(f" !! Longitud inesperada: LEN={data[1]:02X} ({len(data)} B)")


def op_individual(spc: SPC):
    objs = list(NOMBRES.keys())
    banner("LECTURA INDIVIDUAL")
    for i, obj in enumerate(objs, 1):
        print(f"  {i}) {NOMBRES[obj]:<28} ({obj[0]:02X} {obj[1]:02X} {obj[2]:02X})")
    sel = input(" Numero (enter = cancelar): ").strip()
    if not sel.isdigit() or not (1 <= int(sel) <= len(objs)):
        return
    obj = objs[int(sel) - 1]
    print("")
    v = spc.leer_valor(obj)
    if v is not None:
        print(f"\n >> {v:08d}  {NOMBRES[obj]}")


def op_arbitrario(spc: SPC):
    banner("CONSULTA DE OBJETO ARBITRARIO (para explorar objetos no identificados)")
    txt = input(" Objeto en hex CL SM MT (ej: 02 05 05): ").strip().replace(" ", "")
    try:
        b = bytes.fromhex(txt)
        if len(b) != 3:
            raise ValueError
    except ValueError:
        print(" Hex invalido.")
        return
    data = spc.leer(b)
    if data and data[1] == 0x0C:
        v = int.from_bytes(data[9:13], "big")
        print(f"\n >> {v:08d}  (objeto {b[0]:02X} {b[1]:02X} {b[2]:02X})")
    elif data:
        print(f"\n >> Bloque de {len(data)} B (LEN={data[1]:02X})")


def op_monitoreo(spc: SPC):
    banner("MONITOREO CONTINUO (re-lectura cada 5 s, Ctrl+C para detener)")
    try:
        while True:
            print(f"\n===== {datetime.now().strftime('%H:%M:%S')} =====")
            for obj in CADENA_METERS:
                v = spc.leer_valor(obj)
                if v is not None:
                    print(f"  {v:08d}   {NOMBRES[obj]}")
                print("")
    except KeyboardInterrupt:
        print("\n (monitoreo detenido)")


def op_keepalive(spc: SPC):
    banner("KEEP-ALIVE: polls vacios rotando slots cada 30 ms (Ctrl+C para detener)")
    print(" (asi se mantiene viva la comunicacion cuando no se lee nada)")
    try:
        n = 0
        while True:
            spc.poll()
            n += 1
            if n % 33 == 0:
                print(f"   ... {n} transacciones, CRC ok={spc.frames_ok} bad={spc.frames_bad}")
    except KeyboardInterrupt:
        print(f"\n (detenido: {n} transacciones, CRC ok={spc.frames_ok} bad={spc.frames_bad})")


def op_manual(spc: SPC):
    banner("ENVIO DE TRAMA HEX MANUAL (expertos - formato exacto, incluye SEQ y CRC)")
    txt = input(" Trama hex (ej: 78 06 05 30 02 02 04 05 5F 75): ").strip()
    try:
        frame = bytes.fromhex(txt.replace(" ", ""))
    except ValueError:
        print(" Hex invalido.")
        return
    if not crc_ok(frame):
        print(" AVISO: el CRC de esa trama NO es valido. Enviar de todos modos? (s/n)")
        if input().strip().lower() != "s":
            return
    time.sleep(0.03)
    resp = spc._xchg(frame)
    if resp is None:
        print(" (sin respuesta)")
    if frame[1] != 0x00:      # si fue QUERY, la DATA vendra en el siguiente poll
        print(" (recuerda: la DATA llega con el siguiente POLL)")


MENU = """
==================== GAMMA TEST - SPC maestro ====================
  1) Sondeo de arranque (poll hasta eco de la maquina)
  2) Leer TODOS los contadores (cadena F.10) + comparar con captura
  3) Leer informacion de la maquina (Anexo G)
  4) Leer bloque multi-meter 02 03 03
  5) Leer contador individual
  6) Consultar objeto arbitrario (CL SM MT)
  7) Monitoreo continuo de contadores (5 s)
  8) Keep-alive (polls rotando)
  9) Enviar trama hex manual
  q) Salir
=================================================================="""


def elegir_puerto() -> str:
    """Sin --port: lista los puertos disponibles y pregunta por consola."""
    from serial.tools import list_ports
    puertos = list(list_ports.comports())
    print(" Puertos serie detectados:")
    if puertos:
        for p in puertos:
            print(f"   {p.device}  -  {p.description}")
    else:
        print("   (ninguno detectado - conecta el adaptador USB-RS232)")
    sel = input(" Puerto a usar (ej COM10): ").strip()
    if not sel:
        sys.exit("Sin puerto no hay partida.")
    return sel


def main():
    ap = argparse.ArgumentParser(
        description="GAMMA TEST - SPC maestro para maquina Aristocrat (protocolo Gamma). SOLO LECTURA.")
    ap.add_argument("--port", default=None, metavar="COMx", help="puerto del adaptador USB-RS232 (si se omite, se pregunta)")
    ap.add_argument("--baud", type=int, default=19200, help="baudios (defecto 19200)")
    ap.add_argument("--slot", default=None, help="slot inicial en hex (defecto 40; usar 78 para reproducir la captura)")
    args = ap.parse_args()

    puerto = args.port or elegir_puerto()

    if args.slot is None:
        s = input(" Slot inicial en hex [40] (78 = reproducir la captura): ").strip().lower()
        slot_txt = s if s else "40"
    else:
        slot_txt = args.slot

    try:
        ser = serial.Serial(puerto, baudrate=args.baud, bytesize=8, parity="N",
                            stopbits=1, timeout=0.01)
    except serial.SerialException as e:
        sys.exit(f"Error abriendo {puerto}: {e}")

    slot_inicial = int(slot_txt, 16)
    if slot_inicial not in SLOTS:
        sys.exit(f"Slot inicial invalido: {args.slot} (validos: 40 48 50 58 60 68 70 78)")

    spc = SPC(ser, slot_inicial=slot_inicial)

    print("=" * 78)
    print(f" GAMMA TEST - conectado a la MAQUINA REAL por {puerto} ({args.baud} 8N1)")
    print(f" Slot inicial 0x{slot_inicial:02X} | herramienta de SOLO LECTURA")
    print(" Recordatorio de cableado: TXD adaptador -> RX maquina, RXD <- TX maquina, GND comun")
    print("=" * 78)

    acciones = {
        "1": lambda: spc.sondeo_arranque(),
        "2": lambda: op_todos(spc),
        "3": lambda: op_info(spc),
        "4": lambda: op_multibloque(spc),
        "5": lambda: op_individual(spc),
        "6": lambda: op_arbitrario(spc),
        "7": lambda: op_monitoreo(spc),
        "8": lambda: op_keepalive(spc),
        "9": lambda: op_manual(spc),
    }

    while True:
        print(MENU)
        spc.ka_iniciar()
        print(" (keep-alive de fondo activo: el enlace no se cae mientras estas en el menu)")
        try:
            op = input(" Opcion: ").strip().lower()
        except (KeyboardInterrupt, EOFError):
            break
        finally:
            spc.ka_detener()
        if op == "q":
            break
        fn = acciones.get(op)
        if fn:
            try:
                fn()
            except KeyboardInterrupt:
                print("\n (operacion interrumpida)")
        else:
            print(" Opcion invalida.")

    print(f"\nEstadisticas: tramas validas={spc.frames_ok}  con error={spc.frames_bad}")
    ser.close()


if __name__ == "__main__":
    main()
