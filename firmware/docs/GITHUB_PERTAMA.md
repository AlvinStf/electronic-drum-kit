# Repository GitHub pertama untuk proyek drum

Panduan ini memakai browser di komputer. Tidak perlu menjalankan Git atau
terminal untuk unggahan pertama. Pembuatan repository online tetap dilakukan
melalui akun GitHub Anda; ZIP ini belum membuat repository di akun mana pun.

## 1. Siapkan folder

Ekstrak `esp32-s3-drum-module-github-ready.zip`. Buka folder
`esp32-s3-drum-module` sampai terlihat `README.md`, `DrumMIDI_ST7789`,
`library_patch`, `docs`, `tests`, dan berkas lainnya.

Ini salinan paket terakhir dalam percakapan. Bila source atau sampel pada
komputer Anda sudah berubah setelahnya, bandingkan dan salin versi terbaru yang
memang ingin disimpan sebelum upload. Jangan menimpa pekerjaan lokal tanpa backup.

## 2. Buat repository kosong

Masuk ke akun GitHub pribadi. Bila belum ada akun, buat akun terlebih dahulu.
Buka `https://github.com/new` atau menu tanda tambah > New repository.

Isi sebagai berikut:

| Kolom | Nilai yang disarankan |
|---|---|
| Owner | Akun pribadi Anda, bukan organisasi bersama |
| Repository name | esp32-s3-drum-module |
| Description | ESP32-S3 electronic drum module with USB MIDI, PCM5102 audio, and ST7789 settings. |
| Visibility | Private |
| Add README | Off / tidak dicentang; paket sudah memuat README |
| Add .gitignore | None; paket sudah memuat .gitignore |
| Choose a license | None; lihat LICENSE_STATUS.md sebelum memilih |

Klik Create repository. Jangan memilih template atau meminta generator kode
mengganti isi proyek yang sudah bekerja. Nama repository boleh berbeda; nama
folder sketch `DrumMIDI_ST7789` dan berkas `.ino` utamanya tetap berpasangan.

Private membatasi akses; untuk akun pribadi, akses diberikan kepada pemilik dan
pihak yang diberi izin. Pemilik organisasi dapat memiliki akses pada repository
organisasi. Karena itu, gunakan akun pribadi dan jangan undang kolaborator bila
proyek belum ingin dibagikan. Tinjau izin aplikasi sebelum menghubungkannya. [1][2]

## 3. Upload isi folder, bukan ZIP

Pada repository kosong, pilih tautan untuk mengunggah existing file. Setelah
repository berisi file, jalur umumnya Add file > Upload files. [3]

Buka folder hasil ekstrak di file manager dan pilih ISINYA. Seret file serta
subfolder di dalamnya ke area upload. Jangan menyeret folder pembungkus
`esp32-s3-drum-module` sebagai satu tingkat tambahan dan jangan mengunggah ZIP
sebagai pengganti source. Pertahankan subfolder sketch dan header di dalamnya.

Pastikan path pratinjau seperti:

```text
README.md
DrumMIDI_ST7789/DrumMIDI_ST7789.ino
DrumMIDI_ST7789/DrumConfig.h
library_patch/hellodrum.cpp
```

Bukan:

```text
esp32-s3-drum-module/README.md
```

Bila file tersembunyi tidak terlihat, aktifkan tampilan hidden files agar
`.gitignore` dan `.gitattributes` ikut dipilih. Alternatifnya, tambahkan setelah
upload dengan Add file > Create new file, nama persis `.gitignore` atau
`.gitattributes`, lalu salin isinya dari paket. Jangan menambahkan akhiran `.txt`.

Aturan `.gitignore` bukan pemeriksa upload browser. Tetap periksa sendiri bahwa
tidak ada password, token, dump data pribadi, atau file di luar proyek yang ikut.

## 4. Simpan sebagai commit pertama

Tunggu proses transfer file pada halaman upload selesai dan periksa daftar
path. Isi pesan commit:

```text
Initial import: ESP32-S3 drum with MIDI, PCM5102, and ST7789
```

Untuk unggahan awal repository baru milik sendiri, simpan ke branch utama
(`main` bila itu nama defaultnya). Gunakan tombol konfirmasi commit yang
tersedia. Bila memilih branch baru, perubahan baru muncul pada branch utama
setelah pull request digabungkan. [3][4]

Commit adalah catatan perubahan yang disimpan bersama pesannya. Mengunggah
file tidak sama dengan mengunggah firmware ke ESP32; upload firmware tetap
melalui Arduino IDE. [4]

## 5. Pastikan hasilnya benar

Di halaman utama repository, periksa label Private, nama pemilik, README yang
muncul, dan folder `DrumMIDI_ST7789`. Buka `.ino` serta beberapa header untuk
memastikan semuanya ada. Repository bukan hanya satu file ZIP.

GitHub mendukung unggahan browser maksimal 100 file sekaligus dan 25 MiB per
file. Paket ini jauh di bawah batas ukuran per file dan berisi kurang dari
100 file. Bila sampel bertambah banyak/besar, tinjau lagi metode unggahnya. [3]

Simpan juga backup lokal yang sudah bekerja. Repository tidak otomatis
menyimpan library Arduino yang terpasang di komputer atau setting NVS board.

## 6. Pembaruan berikutnya

Edit dan uji source di komputer. Unggah berkas yang berubah ke path yang sama,
lalu berikan pesan commit yang menjelaskan perubahan, misalnya
`Tune kick threshold` atau `Add snare sample`.

Bila memperbarui satu header, masuk dahulu ke folder `DrumMIDI_ST7789` sebelum
memakai Upload files agar berkas tidak berada di root. Untuk perubahan besar,
gunakan branch terpisah dari versi utama; gabungkan setelah diperiksa. [3][4]

Upload browser bukan sinkronisasi otomatis: file yang dipindah atau dihapus
secara lokal tidak otomatis hilang dari repository. Bila pekerjaan makin rutin,
GitHub Desktop dapat dipelajari kemudian; tidak diperlukan untuk tahap ini.

## 7. Publikasi dan lisensi

Mulai Private. Jangan memilih MIT untuk semua file tanpa meninjau
`LICENSE_STATUS.md`. Lisensi MIT HelloDrum sudah disertakan secara terpisah.
Asal dan izin distribusi sampel kick belum tercatat.

Public dan open-source bukan hal yang sama: pemilihan lisensi merupakan
keputusan tersendiri. Jangan mengunggah materi yang tidak boleh didistribusikan
ke publik hanya karena repository bisa diubah menjadi Private lagi. Riwayat
commit dan salinan/fork yang sudah dibuat perlu diperhitungkan. [5]

## Referensi resmi

Dokumentasi GitHub diperiksa pada 11 Oktober 2026. Nama atau letak tombol dapat
berubah; gunakan tujuan langkah sebagai acuan.

[1] Creating a new repository:
`https://docs.github.com/en/repositories/creating-and-managing-repositories/creating-a-new-repository`

[2] About repositories, ownership, and visibility:
`https://docs.github.com/en/repositories/creating-and-managing-repositories/about-repositories`

[3] Adding a file to a repository:
`https://docs.github.com/en/repositories/working-with-files/managing-files/adding-a-file-to-a-repository`

[4] Hello World, commits and branches:
`https://docs.github.com/en/get-started/using-github/hello-world`

[5] Licensing a repository:
`https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/licensing-a-repository`
