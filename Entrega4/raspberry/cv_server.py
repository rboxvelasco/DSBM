from __future__ import annotations

#!/usr/bin/env python3
"""
recognition_server.py  –  Servidor de reconeixement de lletres per socket Unix.

Arrenca una vegada, carrega Tesseract, i espera connexions del programa C.

Protocol (client → server):
  [4B]  magic   : 0xDEADBEEF  (little-endian uint32)
  [1B]  light   : 0 = dark, 1 = bright
  [4B]  size_x  : amplada de la matriu (int32 LE)
  [4B]  size_y  : alçada de la matriu  (int32 LE)
  [size_x * size_y bytes]  matriu aplanada, ordre row-major (y extern, x intern), 0/1

Protocol (server → client):
  [1B]  status  : 0 = OK, 1 = error

Instal·lació:
  sudo apt install tesseract-ocr
  pip install pytesseract opencv-python-headless pillow numpy
"""

import socket
import struct
import os
import json
import base64
import io
import logging
from datetime import datetime, timezone

import cv2
import numpy as np
import pytesseract
from PIL import Image

# ── Configuració ───────────────────────────────────────────────────────────────
SOCKET_PATH  = "/tmp/letter_recognition.sock"
OUTPUT_JSON  = "/var/www/html/data/state.json"
MAGIC        = 0xDEADBEEF
IMG_SIZE     = 128   # mida a la qual escalem per a Tesseract
PADDING      = 20    # marge al voltant del contingut dibuixat

# Configuració Tesseract per a un sol caràcter manuscrit:
#   --psm 10  → tracta la imatge com un únic caràcter
#   --oem 1   → usa LSTM (més precís per a lletres aïllades)
TESS_CONFIG = (
    "--psm 10 --oem 1 "
    "-c tessedit_char_whitelist=ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
)

logging.basicConfig(
    level=logging.INFO,
    format="[%(asctime)s] %(levelname)s  %(message)s",
    datefmt="%H:%M:%S",
)
log = logging.getLogger("recognition")


# ── Comprova que Tesseract és accessible ───────────────────────────────────────
def check_tesseract() -> None:
    try:
        ver = pytesseract.get_tesseract_version()
        log.info("Tesseract %s trobat.", ver)
    except pytesseract.pytesseract.TesseractNotFoundError:
        raise RuntimeError(
            "Tesseract no trobat. Executa: sudo apt install tesseract-ocr"
        )


# ── Helpers d'imatge ───────────────────────────────────────────────────────────

def matrix_to_image(flat: bytes, size_x: int, size_y: int) -> np.ndarray:
    """
    Converteix els bytes rebuts a una imatge numpy grayscale.
    La matriu marca els píxels 'actius' (dibuixats) com a 1.
    Retornem lletra blanca sobre fons negre (el que millor funciona amb Tesseract).
    """
    arr = np.frombuffer(flat, dtype=np.uint8).reshape(size_y, size_x)
    return (arr * 255).astype(np.uint8)


def crop_and_pad(img: np.ndarray) -> np.ndarray:
    """Retalla al bounding-box del contingut i afegeix un marge uniforme."""
    coords = cv2.findNonZero(img)
    if coords is None:
        return img                              # canvas buit
    x, y, w, h = cv2.boundingRect(coords)
    cropped = img[y:y + h, x:x + w]
    return cv2.copyMakeBorder(
        cropped, PADDING, PADDING, PADDING, PADDING,
        cv2.BORDER_CONSTANT, value=0
    )


def preprocess(img: np.ndarray) -> np.ndarray:
    """
    Prepara la imatge per a Tesseract:
      1. Retalla i afegeix marge
      2. Escala a quadrat (letterbox) de IMG_SIZE × IMG_SIZE
      3. Llindar binari
      4. Dilata lleugerament per engrossir traços fins
      5. Inverteix: lletra negra sobre fons blanc (format esperat per Tesseract)
    """
    img = crop_and_pad(img)

    # Letterbox: escalar mantenint proporció dins un quadrat
    h, w = img.shape
    side = max(h, w)
    canvas = np.zeros((side, side), dtype=np.uint8)
    y_off = (side - h) // 2
    x_off = (side - w) // 2
    canvas[y_off:y_off + h, x_off:x_off + w] = img

    canvas = cv2.resize(canvas, (IMG_SIZE, IMG_SIZE), interpolation=cv2.INTER_AREA)

    # Llindar per netejar soroll de la redimensió
    _, canvas = cv2.threshold(canvas, 30, 255, cv2.THRESH_BINARY)

    # Dilació per engrossir traços
    kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (3, 3))
    canvas = cv2.dilate(canvas, kernel, iterations=1)

    # Tesseract prefereix lletra fosca sobre fons clar
    canvas = cv2.bitwise_not(canvas)

    return canvas


def img_to_b64(img: np.ndarray) -> str:
    """Codifica la imatge processada en base64 PNG."""
    pil = Image.fromarray(img)
    buf = io.BytesIO()
    pil.save(buf, format="PNG")
    return base64.b64encode(buf.getvalue()).decode()


# ── Reconeixement amb Tesseract ────────────────────────────────────────────────

def recognize(img: np.ndarray) -> tuple[str, float]:
    """
    Crida Tesseract en mode caràcter únic (--psm 10).

    Retorna (lletra, confiança) on:
      - lletra    és un caràcter majúscul, o '?' si Tesseract no detecta res
      - confiança és un float entre 0.0 i 1.0
    """
    # image_to_data retorna un dict amb una columna 'conf' per caràcter
    data = pytesseract.image_to_data(
        img,
        config=TESS_CONFIG,
        output_type=pytesseract.Output.DICT,
    )

    # Filtrem entrades amb confiança vàlida (≥ 0) i text no buit
    valid = [
        (data["text"][i].strip(), int(data["conf"][i]))
        for i in range(len(data["text"]))
        if data["conf"][i] >= 0 and data["text"][i].strip()
    ]

    if not valid:
        log.warning("Tesseract no ha detectat cap caràcter.")
        return "?", 0.0

    # Agafem el caràcter amb la confiança més alta
    text, conf_pct = max(valid, key=lambda t: t[1])
    letter = text[0].upper() if text else "?"
    confidence = round(conf_pct / 100.0, 4)   # de 0–100 a 0.0–1.0

    return letter, confidence


# ── Gestió d'una connexió ──────────────────────────────────────────────────────

def recv_exact(conn: socket.socket, n: int) -> bytes | None:
    """Llegeix exactament n bytes, o retorna None si la connexió es tanca."""
    buf = bytearray()
    while len(buf) < n:
        chunk = conn.recv(n - len(buf))
        if not chunk:
            return None
        buf.extend(chunk)
    return bytes(buf)


def handle_connection(conn: socket.socket) -> None:
    try:
        # Capçalera: magic(4) + light(1) + size_x(4) + size_y(4) = 13 bytes
        header = recv_exact(conn, 13)
        if header is None:
            log.warning("Connexió tancada abans de rebre la capçalera.")
            return

        magic, light, size_x, size_y = struct.unpack("<IBII", header)

        if magic != MAGIC:
            log.error("Magic incorrecte: 0x%08X (esperat 0x%08X)", magic, MAGIC)
            conn.sendall(b"\x01")
            return

        log.info("Rebent matriu %dx%d  light=%d", size_x, size_y, light)

        # Matriu: size_x × size_y bytes
        flat = recv_exact(conn, size_x * size_y)
        if flat is None:
            log.error("Connexió tancada durant la recepció de la matriu.")
            conn.sendall(b"\x01")
            return

        # Processem i reconeixem
        raw_img  = matrix_to_image(flat, size_x, size_y)
        proc_img = preprocess(raw_img)
        letter, conf = recognize(proc_img)
        now = datetime.now(timezone.utc).isoformat(timespec="seconds")

        log.info("Reconegut: '%s'  confiança=%.3f", letter, conf)

        # Escrivim el JSON
        result = {
            "image_b64"  : img_to_b64(proc_img),
            "light_state": bool(light),
            "recognition": {
                "letter"    : letter,
                "confidence": conf,
                "timestamp" : now,
            },
            "last_update": now,
        }
        with open(OUTPUT_JSON, "w") as f:
            json.dump(result, f, indent=2)

        log.info("JSON escrit a %s", OUTPUT_JSON)
        conn.sendall(b"\x00")   # ACK OK

    except Exception:
        log.exception("Error inesperat processant la connexió.")
        try:
            conn.sendall(b"\x01")
        except Exception:
            pass


# ── Bucle principal del servidor ───────────────────────────────────────────────

def main() -> None:
    check_tesseract()

    # Elimina el socket si quedava d'una execució anterior
    if os.path.exists(SOCKET_PATH):
        os.unlink(SOCKET_PATH)

    srv = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    srv.bind(SOCKET_PATH)
    os.chmod(SOCKET_PATH, 0o666)    # accés des del procés C
    srv.listen(1)

    log.info("Servidor llest. Esperant connexions a %s …", SOCKET_PATH)

    try:
        while True:
            conn, _ = srv.accept()
            with conn:
                handle_connection(conn)
    except KeyboardInterrupt:
        log.info("Aturant servidor (Ctrl+C).")
    finally:
        srv.close()
        if os.path.exists(SOCKET_PATH):
            os.unlink(SOCKET_PATH)


if __name__ == "__main__":
    main()
