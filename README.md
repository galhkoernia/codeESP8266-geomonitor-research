# GeoMonitor Waterpass

## Deskripsi

GeoMonitor Waterpass adalah sistem berbasis mikrokontroler untuk mengukur kemiringan (tilt) secara real-time menggunakan sensor MPU6050. Sistem ini menampilkan data pada layar OLED serta memberikan indikator peringatan melalui buzzer berdasarkan tingkat kemiringan.

Perangkat ini dirancang untuk aplikasi monitoring kemiringan pada bidang konstruksi, alat ukur leveling, maupun sistem deteksi ketidakstabilan.

---

## Fitur Utama

* Pengukuran sudut kemiringan (tilt) dalam derajat
* Konversi kemiringan ke satuan mm/m (slope)
* Kalibrasi nol (zero calibration) dengan tombol
* Deteksi kondisi tidak stabil (unstable detection)
* Indikator status:

  * NORMAL
  * WARNING
  * DANGER
  * UNSTABLE
* Output visual melalui OLED
* Alarm buzzer berbasis kondisi

---

## Hardware yang Digunakan

* Mikrokontroler (ESP8266 / kompatibel)
* Sensor IMU MPU6050
* OLED Display SSD1306 (I2C, 128x64)
* Buzzer
* Push Button (untuk kalibrasi nol)

---

## Konfigurasi Pin

| Komponen    | Pin |
| ----------- | --- |
| SDA (I2C)   | D2  |
| SCL (I2C)   | D1  |
| Buzzer      | D6  |
| Tombol Zero | D5  |

---

## Library yang Dibutuhkan

Pastikan library berikut sudah terinstall:

* Adafruit GFX
* Adafruit SSD1306
* Adafruit MPU6050
* Adafruit Sensor

Install melalui Arduino Library Manager.

---

## Cara Kerja Sistem

### 1. Pembacaan Sensor

Sensor MPU6050 membaca percepatan pada sumbu X, Y, Z kemudian dihitung menjadi:

* Roll
* Pitch
* Total tilt

### 2. Filtering

Digunakan exponential smoothing untuk mengurangi noise:

* 97% data lama
* 3% data baru

### 3. Kalibrasi Nol

* Tekan tombol untuk menyimpan posisi saat ini sebagai referensi nol
* Sistem mengambil rata-rata 20 sampel untuk meningkatkan akurasi

### 4. Penentuan Status

| Kondisi  | Kriteria                    |
| -------- | --------------------------- |
| NORMAL   | ≤ 0.12°                     |
| WARNING  | ≤ 0.29°                     |
| DANGER   | > 0.29°                     |
| UNSTABLE | deviasi gravitasi > ±2 m/s² |

### 5. Output

* OLED menampilkan:

  * Sudut kemiringan
  * Slope (mm/m)
  * Status sistem
  * Nilai akselerasi
* Buzzer:

  * Normal: OFF
  * Warning: Beep berkala
  * Danger: ON terus
  * Unstable: OFF

---

## Cara Penggunaan

1. Upload program ke board
2. Nyalakan perangkat
3. Tunggu sensor stabil
4. Tekan tombol untuk set posisi nol
5. Amati nilai tilt dan status pada OLED

---

## Struktur Program

* `estimateTilt()` → menghitung sudut dari data sensor
* `buzzerUpdate()` → kontrol buzzer berdasarkan state
* `renderUI()` → menampilkan data ke OLED
* `loop()` → logika utama sistem

---

## Catatan

* Pastikan wiring I2C benar
* Gunakan supply stabil untuk menghindari noise sensor
* Kalibrasi ulang jika posisi alat berubah

---

## Pengembangan Lanjutan

Beberapa pengembangan yang dapat dilakukan:

* Logging data ke SD card
* Integrasi WiFi / MQTT untuk monitoring jarak jauh
* Kalibrasi multi-axis lebih akurat
* Penggunaan filter Kalman

---

## Lisensi

Proyek ini bebas digunakan untuk pengembangan dan pembelajaran.

