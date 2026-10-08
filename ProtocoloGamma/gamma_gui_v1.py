#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
=============================================================================
 GAMMA TEST GUI - Monitor de comandos SPC para maquina Aristocrat (protocolo Gamma)
=============================================================================
Archivo AUTONOMO (no importa nada mas que pyserial + tkinter).

Uso:
  1. Selecciona el puerto y pulsa Conectar (hace sondeo de arranque 0x40)
  2. Elige un comando de la lista y pulsa Enviar (o doble clic)
  3. Observa en el monitor la trama enviada y la respuesta de la maquina
     (hex crudo + interpretacion). Los contadores se muestran con su valor
     de 8 digitos y en decimal.

Incluye keep-alive de fondo (el enlace de la maquina se cae si deja de
recibir polls) y auto-resincronizacion por slot 0x40.

Requiere:  pip install pyserial
"""

import queue
import threading
import time
import tkinter as tk
from tkinter import ttk, messagebox
from tkinter.scrolledtext import ScrolledText
from datetime import datetime

import serial
from serial.tools import list_ports

# ============================================================================
# PROTOCOLO GAMMA (verificado contra maquina real - Manual ProtocoloGamma)
# ============================================================================
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
    (0x01, 0x02, 0x01): "Version / serie",
    (0x01, 0x02, 0x02): "Fabricante / paytable / ID juego",
    (0x03, 0x01, 0x00): "Juego / % payback",
    (0x02, 0x03, 0x03): "Bloque multi-meter (24 contadores)",
}

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
    4:  "Total Credits Bet",
    5:  "Total Credits Won",
    8:  "sin asignar (familia drop)",
    13: "Cashout Handpays",
    15: "sin asignar (familia drop; NO es True In)",
}

# entrada -> objeto individual equivalente, para refrescar la tabla VALORES
# con la respuesta del bloque (solo las confirmadas por cruce individual)
BLOQUE_A_OBJETO = {
    0:  (0x02, 0x02, 0x04),
    1:  (0x02, 0x03, 0x01),
    2:  (0x02, 0x04, 0x04),
    4:  (0x02, 0x04, 0x05),
    5:  (0x02, 0x04, 0x06),
    13: (0x02, 0x05, 0x03),
}


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
    """Interpretacion de una trama para el monitor."""
    if not crc_ok(frame):
        return "*** CRC INVALIDO ***"
    seq, ln = frame[0], frame[1]
    if ln == 0:
        return "POLL/ACK vacio"
    p = frame[2: 2 + ln]
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
            return f"DATA -> {v:08d}  {NOMBRES[obj]}  (decimal: {vtxt}){extra}"
        if obj == (0x02, 0x03, 0x03):
            pass                                  # cae al listado de entradas abajo
        elif obj[0] == 0x01 or obj == (0x03, 0x01, 0x00):
            txt = "".join(chr(b) if 32 <= b < 127 else "." for b in frame[9:-2])[:70]
            return f"DATA identidad: {txt}"
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
        return f"DATA bloque ({ln + 4} B)"
    if ln == 4 and p[0] == 0x02:
        return f"STATUS 02 {p[1]:02X}"
    return f"comando (LEN={ln:02X})"


# ============================================================================
# HILO DE PROTOCOLO (todo el acceso serie ocurre aqui; la GUI solo lee colas)
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
        self.monitoreo = False
        self._prox_mon = 0.0
        self.running = True
        self.frames_ok = 0
        self.frames_bad = 0
        self.ultimo_ok = 0.0
        self.cadencia = 0.030

    def _log(self, direccion, frame):
        self.out.put(("log", direccion, frame))

    def _txt(self, texto, tipo="info"):
        self.out.put(("txt", texto, tipo))

    def _slot(self):
        s = SLOTS[self.slot_idx]
        self.slot_idx = (self.slot_idx + 1) % 8
        return s

    # ---- capa de trama -------------------------------------------------------
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
        if crc_ok(t):
            self.frames_ok += 1
        else:
            self.frames_bad += 1
            try:
                self.ser.reset_input_buffer()
            except Exception:
                pass
        return t

    def _xchg(self, frame, espera=0.15, quiet=False):
        if not quiet:
            self._log("TX>>", frame)
        self.ser.reset_input_buffer()
        self.ser.write(frame)
        self.ser.flush()
        r = self._recibir(espera)
        if r is not None:
            self.ultimo_ok = time.time()
            if not quiet:
                self._log("RX<<", r)
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
        """QUERY -> ACK -> POLL... -> DATA. Publica el valor hacia la GUI."""
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
                if data[1] == 0x0C:
                    self.out.put(("meter", obj, int.from_bytes(data[9:13], "big")))
                elif obj == (0x02, 0x03, 0x03):
                    # refresca las filas confirmadas de la tabla (ANEXO I)
                    n = (data[1] - 8) // 4
                    for i, o in BLOQUE_A_OBJETO.items():
                        if i < n:
                            self.out.put(("meter", o,
                                          int.from_bytes(data[9 + 4 * i: 13 + 4 * i], "big")))
                return data
            self._txt(f"... reintentando {NOMBRES.get(obj, obj)}")
        self._txt(f"Sin DATA para {obj}", "err")
        return None

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

            if time.time() - ultimo_stats > 0.5:
                self.out.put(("stats", self.frames_ok, self.frames_bad,
                              SLOTS[self.slot_idx], self.ultimo_ok))
                ultimo_stats = time.time()

            try:
                if self.monitoreo and time.time() >= self._prox_mon:
                    self.leer_todos()
                    self._prox_mon = time.time() + 5.0
                elif self.keepalive:
                    self._xchg(empty_frame(self._slot()), espera=0.08, quiet=True)
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
# LISTA DE COMANDOS QUE SE MUESTRAN EN LA GUI
# ============================================================================
COMANDOS = [
    ("--- GENERALES ---", None, None),
    ("Sondeo de arranque (poll 0x40)", "sondeo", None),
    ("Leer TODOS los contadores", "todos", None),
    ("--- CONTADORES ---", None, None),
    ("Current Credits           (02 03 01)", "leer", (0x02, 0x03, 0x01)),
    ("Total Games Played        (02 04 04)", "leer", (0x02, 0x04, 0x04)),
    ("Total Credits Bet         (02 04 05)", "leer", (0x02, 0x04, 0x05)),
    ("Total Credits Won         (02 04 06)", "leer", (0x02, 0x04, 0x06)),
    ("Cashout Handpays          (02 05 03)", "leer", (0x02, 0x05, 0x03)),
    ("Sin asignar (handpay?)    (02 05 04)", "leer", (0x02, 0x05, 0x04)),
    ("True In (Coins In)        (04 09 01)", "leer", (0x04, 0x09, 0x01)),
    ("Coin Drop / Gross Drop?   (04 04 01)", "leer", (0x04, 0x04, 0x01)),
    ("Denominacion SAS          (04 01 01)", "leer", (0x04, 0x01, 0x01)),
    ("Apuesta maxima?           (04 01 16)", "leer", (0x04, 0x01, 0x16)),
    ("Meter grande              (02 02 04)", "leer", (0x02, 0x02, 0x04)),
    ("Paquete contadores (24)   (02 03 03)", "leer", (0x02, 0x03, 0x03)),
    ("--- INFORMACION ---", None, None),
    ("Version / serie           (01 02 01)", "leer", (0x01, 0x02, 0x01)),
    ("Fabricante / paytable     (01 02 02)", "leer", (0x01, 0x02, 0x02)),
    ("Juego / % payback         (03 01 00)", "leer", (0x03, 0x01, 0x00)),
]


# ============================================================================
# INTERFAZ GRAFICA
# ============================================================================
class App:
    def __init__(self, root):
        self.root = root
        root.title("GAMMA TEST - Monitor de comandos (protocolo Gamma Aristocrat) - SOLO LECTURA")
        root.geometry("1150x700")
        self.worker = None
        self.out_q = queue.Queue()

        self._construir()
        self._refrescar_puertos()
        self.root.after(80, self._drenar)
        root.protocol("WM_DELETE_WINDOW", self._cerrar)

    # ---- construccion de widgets ------------------------------------------------
    def _construir(self):
        # --- barra superior ---
        top = ttk.Frame(self.root, padding=5)
        top.pack(fill="x")
        ttk.Label(top, text="Puerto:").pack(side="left")
        self.cb_puerto = ttk.Combobox(top, width=10, state="readonly")
        self.cb_puerto.pack(side="left", padx=4)
        ttk.Button(top, text="Refrescar", command=self._refrescar_puertos).pack(side="left")
        self.btn_con = ttk.Button(top, text="Conectar", command=self._conectar)
        self.btn_con.pack(side="left", padx=8)
        self.lbl_estado = ttk.Label(top, text="DESCONECTADO", foreground="gray",
                                    font=("Segoe UI", 10, "bold"))
        self.lbl_estado.pack(side="left", padx=12)
        self.lbl_stats = ttk.Label(top, text="", foreground="gray")
        self.lbl_stats.pack(side="right")

        # --- cuerpo: comandos | monitor ---
        cuerpo = ttk.Frame(self.root, padding=5)
        cuerpo.pack(fill="both", expand=True)

        izq = ttk.LabelFrame(cuerpo, text=" COMANDOS ", padding=6)
        izq.pack(side="left", fill="y", padx=(0, 6))

        self.lb_cmd = tk.Listbox(izq, width=38, height=18, exportselection=False,
                                 font=("Consolas", 9))
        for label, _, _ in COMANDOS:
            self.lb_cmd.insert("end", label)
        self.lb_cmd.pack(pady=3)
        self.lb_cmd.bind("<Double-Button-1>", lambda e: self._enviar_seleccion())
        ttk.Button(izq, text="ENVIAR", width=38, command=self._enviar_seleccion).pack(pady=4)

        ttk.Separator(izq).pack(fill="x", pady=6)
        ttk.Label(izq, text="Objeto arbitrario (CL SM MT):").pack()
        fila_arb = ttk.Frame(izq)
        fila_arb.pack(pady=2)
        self.ent_arb = ttk.Entry(fila_arb, width=10)
        self.ent_arb.insert(0, "02 05 05")
        self.ent_arb.pack(side="left")
        ttk.Button(fila_arb, text="Consultar", command=self._consultar_arb).pack(side="left", padx=4)

        ttk.Label(izq, text="Trama hex manual:").pack()
        fila_raw = ttk.Frame(izq)
        fila_raw.pack(pady=2)
        self.ent_raw = ttk.Entry(fila_raw, width=20)
        self.ent_raw.pack(side="left")
        ttk.Button(fila_raw, text="Enviar", command=self._enviar_raw).pack(side="left", padx=4)

        ttk.Separator(izq).pack(fill="x", pady=6)
        self.var_ka = tk.BooleanVar(value=True)
        ttk.Checkbutton(izq, text="Keep-alive de fondo (recomendado)",
                        variable=self.var_ka,
                        command=self._set_ka).pack(anchor="w")
        self.var_ver_ka = tk.BooleanVar(value=False)
        ttk.Checkbutton(izq, text="Ver keep-alive en monitor",
                        variable=self.var_ver_ka,
                        command=self._set_ver_ka).pack(anchor="w")
        self.var_mon = tk.BooleanVar(value=False)
        ttk.Checkbutton(izq, text="Monitoreo continuo (cada 5 s)",
                        variable=self.var_mon,
                        command=self._set_monitoreo).pack(anchor="w")
        ttk.Button(izq, text="Limpiar monitor", width=38,
                   command=self._limpiar).pack(pady=6)

        # --- monitor ---
        der = ttk.LabelFrame(cuerpo, text=" MONITOR TX/RX (hex crudo + interpretacion) ", padding=6)
        der.pack(side="left", fill="both", expand=True)
        self.monitor = ScrolledText(der, wrap="none", font=("Consolas", 9),
                                    state="disabled", width=72, height=22)
        self.monitor.pack(fill="both", expand=True)
        for tag, color in (("tx", "#1a3f8f"), ("rx", "#0a6b24"), ("info", "#555555"),
                           ("ok", "#0a6b24"), ("err", "#c02020")):
            self.monitor.tag_configure(tag, foreground=color)

        # --- tabla de valores (sin comparacion: solo lo ultimo leido) ---
        abajo = ttk.LabelFrame(self.root, text=" VALORES (ultima lectura) ", padding=6)
        abajo.pack(fill="x", padx=5, pady=5)
        cols = ("objeto", "nombre", "v8", "decimal", "hora")
        self.tabla = ttk.Treeview(abajo, columns=cols, show="headings", height=6)
        enc = {"objeto": ("Objeto", 70, "center"), "nombre": ("Contador", 210, "w"),
               "v8": ("Valor (8 dig)", 110, "center"), "decimal": ("Decimal", 120, "center"),
               "hora": ("Hora", 90, "center")}
        for c, (t, w, a) in enc.items():
            self.tabla.heading(c, text=t)
            self.tabla.column(c, width=w, anchor=a)
        self.tabla.pack(fill="x")
        self.filas = {}
        for obj in NOMBRES:
            iid = self.tabla.insert("", "end", values=(
                f"{obj[0]:02X} {obj[1]:02X} {obj[2]:02X}", NOMBRES[obj], "-", "-", "-"))
            self.filas[obj] = iid

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
                    tag = "tx" if direccion.startswith("TX") else ("err" if "INVALIDO" in desc else "rx")
                    self._monitor_insert(f"[{ts}] {direccion:<4} {hextxt:<64} {desc}\n", tag)
                elif kind == "txt":
                    _, texto, tipo = ev
                    self._monitor_insert(f"--- {texto}\n", tipo)
                elif kind == "meter":
                    _, obj, v = ev
                    self._actualizar_fila(obj, v)
                elif kind == "estado":
                    _, texto = ev
                    self.lbl_estado.config(text=texto)
                elif kind == "stats":
                    _, ok, bad, slot, ultimo_ok = ev
                    vivo = time.time() - ultimo_ok < 5 if ultimo_ok else False
                    self.lbl_stats.config(
                        text=f"CRC ok={ok}  bad={bad}  slot prox=0x{slot:02X}  "
                             f"enlace={'ACTIVO' if vivo else 'sin trafico'}")
        except queue.Empty:
            pass
        self.root.after(80, self._drenar)

    def _monitor_insert(self, texto, tag):
        self.monitor.config(state="normal")
        self.monitor.insert("end", texto, tag)
        self.monitor.see("end")
        if int(self.monitor.index("end-1c").split(".")[0]) > 3000:
            self.monitor.delete("1.0", "200.0")
        self.monitor.config(state="disabled")

    def _actualizar_fila(self, obj, v):
        obj = tuple(obj)
        iid = self.filas.get(obj)
        if iid is None:
            return
        vtxt = f"{v:,}".replace(",", ".")
        if obj == (0x04, 0x01, 0x01):
            vtxt += f"  (={v * 0.0001:.2f} $)"
        elif obj == (0x04, 0x01, 0x16):
            vtxt += f"  (={v * 0.01:.2f} $)"
        self.tabla.item(iid, values=(
            f"{obj[0]:02X} {obj[1]:02X} {obj[2]:02X}", NOMBRES.get(obj, "?"),
            f"{v:08d}", vtxt, datetime.now().strftime("%H:%M:%S")))

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
        self.worker.start()
        self.btn_con.config(text="Desconectar")
        self._monitor_insert(f"=== Conectado a {puerto} (19200 8N1) ===\n", "info")

    def _desconectar(self):
        if self.worker is None:
            return
        self.worker.detener()
        self.worker.join(timeout=3)
        self.worker = None
        self.btn_con.config(text="Conectar")
        self.lbl_estado.config(text="DESCONECTADO", foreground="gray")
        self.lbl_stats.config(text="")
        self._monitor_insert("=== Desconectado ===\n", "info")

    def _enviar_seleccion(self, evt=None):
        w = self.worker
        if w is None:
            messagebox.showwarning("Sin conexion", "Conecta primero el puerto.")
            return
        sel = self.lb_cmd.curselection()
        if not sel:
            messagebox.showinfo("Comando", "Selecciona un comando de la lista.")
            return
        label, kind, arg = COMANDOS[sel[0]]
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

    def _limpiar(self):
        self.monitor.config(state="normal")
        self.monitor.delete("1.0", "end")
        self.monitor.config(state="disabled")

    def _cerrar(self):
        self._desconectar()
        self.root.destroy()


def main():
    root = tk.Tk()
    App(root)
    root.mainloop()


if __name__ == "__main__":
    main()
