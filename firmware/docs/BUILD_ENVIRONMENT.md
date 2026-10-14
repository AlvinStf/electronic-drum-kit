# Catatan lingkungan build yang berhasil

Isi berdasarkan komputer dan modul yang benar-benar dipakai. Jangan mengganti
library yang sudah bekerja hanya untuk mencocokkan template ini.

| Komponen | Catatan |
|---|---|
| Modul MCU | ESP32-S3 N16R8, menurut keterangan pengguna |
| Nama development board lengkap | Belum dicatat |
| Arduino IDE | Versi belum dicatat |
| Arduino-ESP32 | Versi persis belum dicatat; source menargetkan core 3.x |
| Board yang dipilih di menu Tools | Belum dicatat |
| Flash Size / Partition Scheme | Belum dicatat |
| PSRAM / CPU / Arduino Runs On | Belum dicatat |
| USB Mode | Source meminta USB-OTG (TinyUSB) |
| USB CDC On Boot | Catat pilihan yang dipakai |
| HelloDrum | Menggunakan library_patch/hellodrum.cpp modifikasi pengguna |
| HelloDrum header / library.properties | Versi persis belum dicatat |
| AudioTools | Versi persis belum dicatat |
| Adafruit GFX | Versi persis belum dicatat |
| Adafruit ST7735 and ST7789 | Versi persis belum dicatat |
| Adafruit BusIO | Versi persis belum dicatat |
| Sistem operasi komputer | Belum dicatat |
| Tanggal upload dan uji hardware | Belum dicatat untuk snapshot repository |

Catat juga hasil MIDI seluruh zona, pukulan cepat, master volume, menu layar,
dan Save/restart. Laporan simulasi komputer jangan dicatat sebagai uji hardware.

Setting yang disimpan di NVS pada board tidak otomatis masuk ke repository.
Repository menyimpan source dan dokumentasi yang diunggah, bukan dump flash
atau isi library lokal komputer. Cadangkan konfigurasi kalibrasi secara terpisah.
