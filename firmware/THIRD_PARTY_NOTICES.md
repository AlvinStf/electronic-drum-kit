# Kredit dan materi pihak ketiga

## HelloDrum Arduino Library

- Penulis upstream: Ryo Kosaka.
- Repository: `https://github.com/RyoKosaka/HelloDrum-arduino-Library`
- Lisensi upstream yang diperiksa: MIT, Copyright (c) 2018 Ryo Kosaka.
- Teks lisensi: [library_patch/LICENSE.txt](library_patch/LICENSE.txt).
- Berkas turunan yang disertakan: `library_patch/hellodrum.cpp`, modifikasi pengguna.
- Contoh LCD `reference/minimalDrumkit.ino.txt` menyebut repository HelloDrum
  dalam header asalnya. Pemberitahuan MIT juga dipertahankan untuk materi
  HelloDrum yang termuat dalam contoh atau bagian lain proyek.

Berkas modifikasi bukan rilis resmi atau dukungan dari penulis upstream.
Lisensi pihak ketiga tidak diluaskan secara otomatis ke seluruh kode baru,
contoh display, atau sampel audio dalam repository ini.

## Library eksternal yang dipasang terpisah

Arduino-ESP32 (Espressif), Arduino AudioTools (Phil Schatzmann), Adafruit GFX,
Adafruit ST7735 and ST7789 Library, dan Adafruit BusIO digunakan sebagai
dependensi. Salinan lengkap library tersebut tidak dibundel di sini.
Gunakan dan pertahankan pemberitahuan lisensi dari versi yang benar-benar dipasang.

`tests/stubs` hanya berisi antarmuka tiruan untuk tes komputer; folder tersebut
bukan library asli dari vendor dan tidak boleh dipasang menggantikannya.
