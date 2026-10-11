# ESP32-S3 Drum Module

Firmware modul drum elektrik berbasis **ESP32-S3 N16R8**, dengan **USB MIDI**,
**audio WAV polyphonic melalui PCM5102**, **master volume potensiometer GPIO15**,
dan **menu setting ST7789**.

**Baru menggunakan GitHub?** Mulai dari [Panduan GitHub pertama](docs/GITHUB_PERTAMA.md).
Untuk pemasangan dan penggunaan drum, baca [Panduan firmware](FIRMWARE_GUIDE.md).

## Fitur dalam kode

- USB MIDI untuk kick, snare, tiga tom, hihat, pedal, crash, dan ride.
- Mixer audio 16 suara, dengan keluaran I2S stereo 44.100 Hz.
- Master volume GPIO15, Master trim, Local audio, dan Audio velocity.
- Menu ST7789 untuk parameter pad, curve, dan MIDI note.
- Penyimpanan setting melalui Preferences/NVS, dengan validasi dan CRC.
- Test kick lokal, serta pengujian logika di komputer menggunakan stub hardware.

**Sampel bawaan hanya kick.** Zona lain tetap mengirim MIDI, tetapi memerlukan
WAV sendiri agar berbunyi melalui PCM5102. Audio tidak dibaca dari kartu SD.
Mengubah MIDI note tidak mengganti sampel WAV.

## Membuka di Arduino IDE

Buka berkas berikut, dengan seluruh header tetap berada di folder yang sama:

```text
DrumModule/DrumModule.ino
```
Dependensi firmware: Arduino-ESP32, HelloDrum dengan patch pengguna, AudioTools,
Adafruit GFX, Adafruit ST7735 and ST7789 Library, dan Adafruit BusIO.
`Preferences`, `USB`, `USBMIDI`, dan `SPI` digunakan dari Arduino-ESP32.

Versi persis instalasi pengguna belum dicatat. Isi [catatan lingkungan build](docs/BUILD_ENVIRONMENT.md)
berdasarkan instalasi yang berhasil, bukan dengan mengasumsikan versi terbaru.

## Pemetaan GPIO dalam firmware

| Perangkat | Sinyal | GPIO |
|---|---|---|
| PCM5102 | BCK / WS / DIN | 42 / 47 / 21 |
| Potensiometer master | Wiper | 15 |
| ST7789 | SCK / MOSI | 12 / 11 |
| ST7789 | CS / DC / RST | 39 / 40 / 41 |
| Tombol | UP / SELECT / DOWN | 48 / 45 / 0 |
| Kick | Piezo | 10 |
| Snare | Head / rim | 7 / 8 |
| Tom | High / hi-mid / floor | 4 / 5 / 6 |
| Hihat / pedal | Sensor | 1 / 2 |
| Crash | Piezo / sensor | 3 / 13 |
| Ride | Piezo / sensor | 9 / 14 |

Ini adalah pemetaan dari source, bukan pengganti skematik rangkaian.
Perhatikan catatan catu daya, ground, dan pin strapping di [panduan firmware](FIRMWARE_GUIDE.md).
Matikan daya sebelum mengubah wiring. Pot menggunakan 3V3 dan GND, bukan 5 V.

## HelloDrum modifikasi

`library_patch/hellodrum.cpp` adalah salinan berkas modifikasi yang digunakan
proyek ini, bukan seluruh library. Cadangkan lalu pasang pada `src/hellodrum.cpp`
di instalasi HelloDrum yang aktif, menggunakan header dan berkas pendukung yang
sesuai. Instalasi yang sudah menggunakan berkas ini tidak perlu ditimpa lagi.

**Jangan menyalin patch ke folder sketch sekaligus mengompilasi library yang sama.**
Baca [petunjuk patch](library_patch/README.txt) dan [kredit pihak ketiga](THIRD_PARTY_NOTICES.md).

## Struktur

```text
DrumMIDI_ST7789/      Sketch utama dan seluruh header firmware
library_patch/       hellodrum.cpp modifikasi, petunjuk, lisensi upstream
reference/           Contoh yang menjadi bahan integrasi; bukan sketch utama
artifacts/           Hasil pemeriksaan pengemasan di komputer
tests/              Pengujian logika dan stub hardware
tools/              Konverter WAV PCM16 menjadi header C++
docs/               Panduan GitHub dan catatan build
```

`tests/stubs` bukan pengganti library Arduino asli.

## Pengujian

Pada lingkungan Linux dengan Python 3, Bash, dan g++ yang mendukung sanitizer:

```bash
bash tests/run_tests.sh
```

Tes memakai API hardware tiruan. Kelulusan tes bukan bukti timing ADC, DMA,
USB, tampilan fisik, atau keberhasilan compile dengan library ESP32 lokal.
[TEST_RESULTS.txt](TEST_RESULTS.txt) adalah laporan dari paket sebelumnya;
[PACKAGING_REPORT.md](PACKAGING_REPORT.md) mencatat pemeriksaan tahap repository.

Beberapa tes mengunci pemetaan dan hash baseline. Setelah perubahan firmware
atau sampel yang disengaja, tinjau dan perbarui ekspektasi tes secara sadar.

## Status lisensi dan sampel

Mulai dengan repository **Private**. Belum ditetapkan lisensi umum untuk seluruh
proyek. Lisensi upstream HelloDrum MIT dan kredit Ryo Kosaka tetap dipertahankan;
ketentuan pihak ketiga tidak dihapus oleh status lisensi proyek.

Asal serta izin distribusi sampel suara belum dicatat. Jangan menganggap
sampel otomatis bebas dibagikan karena sudah diubah menjadi array C++.
Baca [LICENSE_STATUS.md](LICENSE_STATUS.md) sebelum publikasi atau distribusi.
