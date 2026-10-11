from pathlib import Path
import importlib.util
import struct
root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('converter',root/'tools/wav_to_header.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
fmt = struct.pack('<HHIIHH',1,1,44100,88200,2,16)
b = b'RIFF'+struct.pack('<I',40)+b'WAVEfmt '+struct.pack('<I',16)+fmt+b'data'+struct.pack('<I',4)+b'\0'*4
assert module.validate_wav(b,44100) == (44100,1,2)
for invalid in [b[:i] for i in range(len(b))]+[b[:20]+b'\3\0'+b[22:]]:
    try: module.validate_wav(invalid,44100)
    except ValueError: pass
    else: raise AssertionError('Invalid WAV accepted')
try: module.validate_wav(b,48000)
except ValueError: pass
else: raise AssertionError('Mismatched sample rate accepted')
print('WAV-to-header validator: valid PCM, truncated inputs, float and rate mismatch PASS')
