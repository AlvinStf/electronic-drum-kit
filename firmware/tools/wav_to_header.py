#!/usr/bin/env python3
"""Wrap an already prepared PCM16 WAV in an Arduino PROGMEM array.
No audio conversion, resampling, trimming, or normalization is performed.
"""
from __future__ import annotations
import argparse
from pathlib import Path
import re
import struct
import sys


def validate_wav(data: bytes, required_rate: int) -> tuple[int, int, int]:
    if len(data) < 12 or data[:4] != b'RIFF' or data[8:12] != b'WAVE':
        raise ValueError('Bukan RIFF/WAVE.')
    end = struct.unpack_from('<I', data, 4)[0] + 8
    if not 12 <= end <= len(data):
        raise ValueError('Panjang RIFF tidak valid atau berkas terpotong.')
    fmt = None
    payload_size = None
    pos = 12
    while pos < end:
        if end - pos < 8:
            raise ValueError('Header chunk tidak lengkap.')
        tag, size = struct.unpack_from('<4sI', data, pos)
        pos += 8
        if size > end - pos:
            raise ValueError('Chunk melewati batas RIFF.')
        if tag == b'fmt ':
            if fmt is not None or size < 16:
                raise ValueError('Chunk fmt tidak valid atau ganda.')
            fmt = struct.unpack_from('<HHIIHH', data, pos)
        elif tag == b'data':
            if payload_size is not None:
                raise ValueError('Data chunk ganda tidak didukung.')
            payload_size = size
        pos += size + (size & 1)
        if pos > end:
            raise ValueError('Padding chunk tidak lengkap.')
    if fmt is None or payload_size is None:
        raise ValueError('Chunk fmt/data tidak ditemukan.')
    encoding, channels, rate, byte_rate, align, bits = fmt
    if encoding != 1 or channels not in (1, 2) or bits != 16:
        raise ValueError('Ekspor sebagai WAV PCM integer 16-bit, mono/stereo; bukan float/ADPCM/extensible.')
    if align != channels * 2 or byte_rate != rate * align or not payload_size or payload_size % align:
        raise ValueError('Payload PCM atau block alignment tidak valid.')
    if rate != required_rate:
        raise ValueError(f'Sample rate {rate} Hz; perlu {required_rate} Hz. Ekspor/resample dahulu.')
    return rate, channels, payload_size // align


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--name', required=True, help='C++ symbol, misalnya SnareHeadWav')
    parser.add_argument('--rate', type=int, default=44100)
    parser.add_argument('--force', action='store_true', help='Izinkan menimpa header yang sudah ada')
    args = parser.parse_args()
    try:
        if not re.fullmatch(r'[A-Za-z][A-Za-z0-9_]*', args.name):
            raise ValueError('--name harus identifier C++ yang dimulai dengan huruf.')
        if not 8000 <= args.rate <= 96000:
            raise ValueError('--rate harus 8000..96000 Hz.')
        if args.input.resolve() == args.output.resolve():
            raise ValueError('Berkas input dan output tidak boleh sama.')
        if args.output.exists() and not args.force:
            raise ValueError('Output sudah ada; gunakan --force untuk menimpa.')
        data = args.input.read_bytes()
        rate, channels, frames = validate_wav(data, args.rate)
        rows = ['  ' + ', '.join(f'0x{b:02X}' for b in data[i:i+12]) + ','
                for i in range(0, len(data), 12)]
        text = ('#pragma once\n#include <Arduino.h>\n\n'
                f'// PCM16 WAV: {rate} Hz, {channels} channel(s), {frames} frames.\n'
                f'const uint8_t {args.name}[] PROGMEM = {{\n' + '\n'.join(rows) + '\n};\n'
                f'const size_t {args.name}Len = sizeof({args.name});\n')
        args.output.parent.mkdir(parents=True, exist_ok=True)
        # Exclusive create by default: a concurrent writer cannot be overwritten.
        with args.output.open('w' if args.force else 'x', encoding='ascii') as handle:
            handle.write(text)
        print(f'{args.output}: {len(data)} byte WAV, {rate} Hz, {channels} kanal, {frames/rate:.3f} detik')
        return 0
    except (OSError, ValueError, struct.error) as exc:
        print(f'ERROR: {exc}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
