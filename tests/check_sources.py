from pathlib import Path
import hashlib
import re
root = Path(__file__).resolve().parents[1]
original = (root/'tests/fixtures/DrumMIDI_original.ino.txt').read_text()
new = (root/'DrumMIDI_ST7789/DrumMIDI_ST7789.ino').read_text()
def normalize(s):
    return re.sub(r'\s+', '', re.sub(r'/\*.*?\*/|//[^\n]*', '', s, flags=re.S))
def extract(pattern, text):
    return [normalize(x) for x in re.findall(pattern, text, re.S)]
pattern = r'MIDI\.(?:noteOn|noteOff|controlChange|polyPressure)\(.*?\);'
assert extract(pattern, original) == extract(pattern, new)
assert len(extract(pattern, original)) == 45
assert extract(r'byte \w+\[\d+\] = \{.*?\};', original) == extract(r'byte \w+\[\d+\] = \{.*?\};', new)
assert extract(r'(?m)^HelloDrum \w+\([^;\n]+;', original) == extract(r'(?m)^HelloDrum \w+\([^;\n]+;', new)
cpp = (root/'library_patch/hellodrum.cpp').read_bytes()
assert hashlib.sha256(cpp).hexdigest() == 'b712c8808129ecfc52d5b6a69663e7c554ee2b78dedabaf64d7eca7d29205d3a'
assert len(re.findall(r'drum_runtime::padSound\(', new)) == 16
print('Source preservation: all 45 MIDI calls, 9 tuning arrays, 9 pad constructors unchanged')
print('Custom hellodrum.cpp: SHA-256 identical to supplied file; 16 local-audio routes added')

# Verify user sample bytes are preserved and loop additions cannot call the TFT.
assert hashlib.sha256((root/'DrumMIDI_ST7789/WavData.h').read_bytes()).hexdigest() == "fd681793c92fb60e04c99394cf61901d8a83708adebf074ad29d3a2ae43ae1a3"
main_loop = new[new.index('void loop()'): ]
assert 'display.' not in main_loop and 'while (' not in main_loop
assert 'drum_settings::serviceBeforeScan();' in main_loop
assert 'drum_settings::publish();' in main_loop
assert not re.search(r'\bdelay\s*\(', main_loop)
print('WavData.h unchanged; no TFT calls, new delay, or wait-for-button loop in sensing loop')
