# HelloDrum + PCM5102 + ST7789 settings

## Mulai

Pertahankan board, flash, PSRAM, USB dan versi AudioTools/HelloDrum yang sudah bekerja sebelumnya. Jalur USB MIDI menggunakan USB-OTG (TinyUSB). Gunakan USB CDC On Boot bila Serial Monitor memang digunakan melalui USB native tersebut.

Tambahan dependensi dari Library Manager:

- Adafruit GFX Library.
- Adafruit ST7735 and ST7789 Library.
- Adafruit BusIO, bila belum terpasang sebagai dependensi.

`library_patch/hellodrum.cpp` merupakan berkas HelloDrum yang di modifikasi agar dapat bekerja dengan ESP32-S3 dan kompatibel dengan hardware drum elektrik Yamaha (belum diuji). Buat salinan header/berkas pendukung dari instalasi HelloDrum asli sebelum ditimpa.

## Wiring yang dipakai firmware

Matikan daya sebelum mengubah kabel. Angka berikut adalah **nomor GPIO**, bukan nomor urut pin header.

| Perangkat | Sinyal | GPIO / sambungan |
|---|---|---|
| ST7789 | SCL / SCK | 12 |
| ST7789 | SDA / MOSI | 11 |
| ST7789 | CS | 39 |
| ST7789 | DC | 40 |
| ST7789 | RES / RST | 41 |
| ST7789 | VCC | 3V3 |
| ST7789 | GND | GND |
| ST7789 | BLK | 3V3 |
| ST7789 | MISO / SDO | Tidak disambungkan / tidak digunakan |
| Tombol UP | Tombol ke GND | 48 |
| Tombol SELECT | Tombol ke GND | 45 |
| Tombol DOWN | Tombol ke GND | 0 |
| PCM5102 | BCK | 42 |
| PCM5102 | LCK / WS | 47 |
| PCM5102 | DIN | 21 |
| Pot master volume | Wiper | 15 |
| Pot master volume | Kedua ujung | 3V3 dan GND |

Bus layar diinisialisasi eksplisit:

```cpp
displaySPI.begin(12, -1, 11, 39); // SCK, MISO, MOSI, CS
```

`MISO=-1` mencegah inisialisasi input SPI pada GPIO13 yang dipakai sensor crash. SCL/SDA pada modul ST7789 ini adalah penamaan clock/data **SPI**, bukan instruksi memakai I2C.

### Perhatian tombol saat boot

GPIO0 dan GPIO45 adalah pin strapping ESP32-S3. Keduanya dipertahankan karena pemetaan itu sudah diuji pengguna, bukan dipilih ulang sebagai rekomendasi umum. Jangan menahan DOWN/GPIO0 ketika power-on/reset/upload normal. Jangan menambahkan pull-up eksternal pada GPIO45 tanpa memeriksa skematik dan konfigurasi VDD_SPI modul. Kode hanya menggunakan `INPUT_PULLUP` setelah startup.

Apabila perlu menghindari kedua pin tersebut, ubah SELECT dan DOWN ke GPIO lain di `UiConfig.h`, lalu pindahkan kabelnya; pastikan pin pengganti tersedia pada board dan tidak dipakai periferal lain.

### Orientasi

```cpp
display.init(240, 280);
display.setRotation(3);
```

Dengan driver Adafruit, rotation 3 memberi koordinat logis **280 x 240 (landscape)**. Layout membaca `width()`/`height()`, bukan memaksakan tinggi 280. Untuk portrait, ubah `ROTATION` menjadi 0 atau 2 di `UiConfig.h`.

Default clock SPI 20 MHz. Tidak ada touch atau pengaturan brightness baru.

## Menu dan tombol

Halaman awal menampilkan status audio, jumlah WAV siap, hit terakhir, velocity, master level, suara aktif dan counter drop/error I2S. Ini telemetri dari program, bukan pengukuran sinyal analog atau konfirmasi komputer menerima MIDI.

| Aksi | Hasil |
|---|---|
| SELECT singkat pada halaman Play | Membuka SETTINGS |
| UP / DOWN | Memindahkan pilihan |
| SELECT singkat pada parameter | Memulai edit; bar menjadi kuning |
| UP / DOWN saat edit | Mengubah nilai sementara; tahan untuk repeat |
| SELECT singkat saat edit | Menerapkan nilai ke RAM, efektif pada iterasi sensing berikutnya |
| Tahan SELECT sekitar 0,8 detik saat edit | Membatalkan nilai sementara |
| Tahan SELECT sekitar 0,8 detik saat navigasi | Kembali satu tingkat |
| Baris `< Back` / `< Play` | Cara lain untuk kembali |

Warna cyan menandai pilihan navigasi. Warna kuning menandai edit yang belum diterapkan. **Mengubah angka dengan UP/DOWN belum mengubah drum sampai SELECT ditekan lagi.** Setelah itu pad dapat diuji tanpa keluar dari menu. Tanda `*` menunjukkan konfigurasi RAM berbeda dari konfigurasi tersimpan (atau baseline default ketika belum ada simpanan).

Susunan utama: **Pad settings**, **Output**, **Save settings**, **Reload saved**, **Restore defaults**, **About**, **Play**. Menu menggulir untuk item yang tidak muat di satu halaman.

### Pad settings

Tersedia kesembilan input: Kick, Snare, High Tom, Hi-Mid Tom, Floor Tom, Hihat, Hihat Pedal, Crash, Ride. Urutan tampilan tidak mengubah urutan sensing pada sketch.

| Jenis input | Parameter |
|---|---|
| Kick dan tom | Sensitivity, Threshold, Scan time, Mask time, Curve, MIDI note |
| Snare dual piezo | Parameter dasar, Rim sensitivity, Rim threshold, Note head/rim/cross |
| Hihat | Parameter dasar, Curve, Note open/closed |
| Hihat Pedal | Sensitivity, Threshold, Scan start, Scan end, Pedal sensitivity, Curve, Note pedal |
| Crash dan Ride | Parameter dasar, Edge threshold, Cup threshold, Curve, Note bow/edge/cup |

Scan time dan Mask time pada pad menggunakan milidetik. **Scan start/end pedal adalah level ambang, bukan durasi milidetik.** Nilai tetap dikirim ke fungsi HelloDrum dengan urutan dan skala aslinya. Curve: 0 Linear, 1 Exp 1, 2 Exp 2, 3 Log 1, 4 Log 2.

Validator menolak nilai di luar rentang, sensitivity <= threshold, scan start pedal >= scan end, dan edge threshold cymbal >= cup threshold. Nilai yang ditolak tidak diterapkan dan tidak disimpan. MIDI note 0..127 tetap valid. Rentang nilai sensor/time pada menu 1..100, mengikuti parameter contoh; curve 0..4.

Mengubah MIDI note **tidak mengganti WAV lokal**. Slot suara tetap berdasarkan zona fisik, bukan nomor note. Crash cup dan ride cup tetap merupakan slot terpisah meskipun default MIDI note keduanya 53.

### Output

- **Local audio:** mute/unmute keluaran lokal. Mixer tetap berjalan; MIDI tidak dimatikan. Mengaktifkan kembali output bisa membuat ekor sampel yang masih aktif terdengar.
- **Audio velocity:** gain pukulan WAV berdasarkan velocity. Perubahan berlaku pada suara baru; suara yang sudah aktif mempertahankan gain pukulannya. Default Off seperti versi sebelumnya. MIDI velocity selalu tetap dikirim seperti sketch asli.
- **Master trim %:** 0..100%, pengali tambahan setelah pot dan ceiling lama. Default 100%, bukan pengganti pot GPIO15.
- **Test kick:** satu pukulan uji lokal, tanpa pesan USB MIDI.

Master gain efektif:

```text
kurva pot GPIO15 x MASTER_CEILING_Q15 x (Master trim / 100)
```

`MASTER_CEILING_Q15` tetap 8192, sekitar 0,25. Master level 100% di layar berarti 100% dari ceiling lama tersebut, **bukan** gain digital unity, bukan dB, dan bukan volume MIDI CC7. Pada pot yang memakai kurva kuadrat, persentase level tidak sama dengan persentase sudut putaran.

Tanpa pot terpasang, gunakan `USE_VOLUME_POT=false` di `DrumConfig.h`, seperti versi sebelumnya. Jangan mengaktifkan pembacaan GPIO15 dengan pin mengambang.

## Save, Reload dan Defaults

Penyimpanan memakai **Preferences / NVS** milik ESP32, namespace `drum_st7789`, key `settings_v1`. Satu record berukuran 105 byte: header, versi format, 93 byte konfigurasi, CRC32. Pembacaan memeriksa panjang, identitas, versi, CRC dan rentang seluruh parameter sebelum menerapkan record. Setelah Save, record dibaca ulang dan dibandingkan.

- **Save settings:** konfirmasi `Yes, continue` untuk menyimpan konfigurasi RAM. Hanya proses ini yang melakukan penulisan konfigurasi.
- **Reload saved:** memuat record tersimpan dan membuang perubahan RAM; perlu konfirmasi. Bila data tidak valid atau belum ada, konfigurasi berjalan tidak diubah.
- **Restore defaults:** mengembalikan angka dari array pada sketch, **hanya di RAM**. Simpan setelahnya bila default itu ingin dipertahankan sesudah restart. Ini tidak menghapus seluruh NVS, tidak memformat flash dan tidak mengubah namespace milik aplikasi lain.

Pada startup, record valid dimuat otomatis. Bila belum ada atau tidak valid, kode menggunakan angka asli dari sketch. Tidak ada migrasi otomatis dari EEPROM HelloDrum, dan tidak ada `initMemory()` yang menimpa konfigurasi setiap boot. Pot fisik tidak disimpan; posisi aktualnya selalu dibaca kembali.

**Berhenti bermain sebelum Save/Reload.** Kode menolak operasi ketika ada hit baru dalam 750 ms terakhir, tes berulang masih berjalan, atau mixer yang sehat masih memiliki suara/event aktif. Biarkan ekor suara selesai lalu ulangi Save. Ketika audio sudah gagal, antrean audio yang macet tidak mengunci fungsi penyimpanan selamanya.

Flash/NVS tetap dapat menimbulkan jeda sistem saat operasi berlangsung. Pemeriksaan idle ini tidak membuat firmware menjadi IRAM-safe dan tidak menjamin hit yang datang persis saat penulisan akan tertangkap. Karena itu tidak ada auto-save saat memainkan pad atau mengubah angka.

## Pemisahan tugas

`loop()` tetap menjalankan sensing, USB MIDI, pembacaan pot dan pengiriman event audio. Di awal iterasi, maksimal satu command menu diproses. Di akhir iterasi, snapshot status dikirim paling sering setiap 100 ms (atau segera setelah command).

Task audio tetap mencampur suara dan menulis I2S. Task UI menangani tombol, state menu dan SPI display dengan prioritas lebih rendah (audio 3, UI 1). Pada konfigurasi dual-core, keduanya dipasang pada core yang berbeda dari task `setup()/loop()`.

Komunikasi antartask menggunakan antrean command dan mailbox snapshot berupa salinan data. Task UI tidak membaca/menulis array HelloDrum langsung dan tidak mengakses ADC, MIDI, Preferences, posisi suara atau instance mixer. Target gain/velocity lintas-core menggunakan atomics. Pembaruan display hanya menggambar baris yang berubah menggunakan canvas strip 16-bit, bukan buffer penuh layar setiap loop.

`delay(500)` dari contoh LCD dan loop penunggu pelepasan tombol dari contoh menu **tidak dipakai** pada sensing. `delay(1)` dan filter FSR yang sudah ada pada `hellodrum.cpp` pengguna **tetap tidak diubah**. Beban display tetap perlu diperiksa pada hardware; pemisahan tugas bukan jaminan latensi tertentu.

## Urutan uji di board

1. Cocokkan wiring SPI eksplisit, lalu upload sebagai proyek terpisah. Mulai dengan volume amplifier rendah. Pastikan MIDI seluruh pad dan kick lokal masih bekerja.
2. Coba tombol: SELECT, UP/DOWN, edit, SELECT lagi untuk apply, tahan SELECT untuk cancel/back. Tahan UP/DOWN saat memainkan kick cepat: cek respons drum dan counter `dropped`/`writeErr`.
3. Buka Pad settings > Kick. Ubah Sensitivity 100 menjadi 99, terapkan, pukul pad. Buka Curve dan MIDI note untuk memastikan perubahan benar-benar berpengaruh, lalu kembalikan nilai sesuai kebutuhan.
4. Berhenti bermain, biarkan seluruh ekor audio habis, lakukan Save. Tunggu pesan `Settings saved and verified`, kemudian restart dan periksa nilai termuat lagi. Jangan memutus daya sebelum konfirmasi selesai.
5. Uji pot dan Master trim; MIDI velocity tidak boleh ikut berubah. Uji output mute/unmute. Bila memakai instrumen virtual komputer, pot hanya memengaruhi DAC lokal.

Perintah Serial Monitor tetap: `k` satu kick lokal, `t` 16 hit berjarak 100 ms, `x` menghentikan tes berulang, `s` status. Baud 115200. Tidak ada logging per pukulan. `gapUs` adalah maksimum kumulatif sejak boot dan dapat mencakup aktivitas Save/Serial; bukan hanya waktu sensing saat bermain.

Bila layar kosong: cek backlight, common ground, SCK12/MOSI11, CS39/DC40/RST41 dan library. Bila perlu mengisolasi beban display, set `ENABLE_DISPLAY=false` di `UiConfig.h`; konfigurasi yang telah tersimpan tetap dimuat. Tetap diperlukan instalasi library display karena header tetap dikompilasi.

## Pengujian yang disertakan

Jalankan `bash tests/run_tests.sh` pada komputer dengan Python 3 dan g++. Tes menggunakan AddressSanitizer/UndefinedBehaviorSanitizer dan **stub hardware**, bukan firmware ESP32. Rinciannya ada pada `TEST_RESULTS.txt`.

`tests/stubs` adalah tiruan antarmuka untuk pengujian komputer. Jangan memasukkannya ke folder library Arduino atau mengganti library asli dengannya. Tes grafis memeriksa panggilan/geometri dan cache baris dengan metrik huruf tiruan; tidak memvalidasi rupa font atau piksel LCD asli.

## Referensi teknis

Materi pengguna: `reference/contoh_menu_st7789_240x280.ino.txt`, `reference/minimalDrumkit.ino.txt`, `tests/fixtures/DrumMIDI_original.ino.txt`, `library_patch/hellodrum.cpp`, dan sampel asli.

Verifikasi API/dokumentasi eksternal, 11 Oktober 2026:

- Adafruit ST7789 source, constructor SPIClass, init dan rotation: `https://github.com/adafruit/Adafruit-ST7735-Library/blob/master/Adafruit_ST7789.cpp` dan `.h`.
- Adafruit GFX / SPITFT: `https://github.com/adafruit/Adafruit-GFX-Library`.
- Arduino-ESP32 SPI initialization: `https://github.com/espressif/arduino-esp32/blob/master/libraries/SPI/src/SPI.cpp`.
- Preferences: `https://docs.espressif.com/projects/arduino-esp32/en/latest/api/preferences.html`.
- Strapping / boot: `https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32s3/schematic-checklist.html`.
- Flash concurrency: `https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/spi_flash/spi_flash_concurrency.html`.

Keputusan baru pada integrasi: schema tervalidasi, Preferences/CRC, task UI terpisah, antrean command/snapshot, Master trim, Local audio mute, serta konfirmasi Save/Reload/Defaults.
