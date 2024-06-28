#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

// SSID dan Password WiFi
const char* ssid = "RIDHO FERNANDO";
const char* password = "ridho1710";

// Inisialisasi Bot Token
#define BOTtoken "7206639866:AAEK1GmIGdeKUc_CTlO3FPQKQIKZtFf2XSw"  // Bot Token dari BotFather
#define CHAT_ID "6293314881"

// Deklarasi objek untuk WiFi dan Telegram
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// Interval waktu antara permintaan ke bot dalam milidetik
int jedaPermintaanBot = 1000; 
unsigned long waktuTerakhirBotJalan;

// Mengatur SoftwareSerial pada pin D1 (RX) dan D2 (TX)
SoftwareSerial GPSSerial(D2, D1);
TinyGPSPlus gps;

// Mendefinisikan pin untuk SW-420 dan Buzzer
const int pinSW420 = D5;
const int pinBuzzer = D6;

// Flag untuk menghindari pengiriman alert ganda
bool getaranTerdeteksi = false;

// Flag untuk mengatur apakah buzzer dibisukan
bool buzzerDibisukan = false;

// Fungsi untuk menangani pesan baru
void tanganiPesanBaru(int jumlahPesanBaru) {
  Serial.println("tanganiPesanBaru");
  Serial.println(String(jumlahPesanBaru));

  for (int i = 0; i < jumlahPesanBaru; i++) {
    String idChat = String(bot.messages[i].chat_id);
    if (idChat != CHAT_ID) {
      bot.sendMessage(idChat, "Pengguna tidak valid", "");
      continue;
    }
    String teks = bot.messages[i].text;
    Serial.println("Pesan: " + teks);

    String namaPengirim = bot.messages[i].from_name;

    if (teks == "/start") {
      String kontrol = "Halo Paduka, " + namaPengirim + ". Ini Sistem MotoGuard.\n\n";
      kontrol += "Gunakan Perintah Di Bawah Untuk Monitoring Lokasi Kendaraan dan Mengontrol Alarm.\n\n";
      kontrol += "/Lokasi Untuk Mengetahui lokasi kendaraan saat ini. \n";
      kontrol += "/Bunyikan Untuk Membunyikan alarm selama 5 detik. \n";
      kontrol += "/Mute Untuk Membisukan buzzer saat getaran terdeteksi. \n";
      kontrol += "/UnMute Untuk Mengaktifkan kembali buzzer saat getaran terdeteksi. \n";
      bot.sendMessage(idChat, kontrol, "");
      Serial.println("Mengirim respon /start");
    } else if (teks == "/Lokasi") {
      if (gps.location.isValid()) {
        float latSekarang = gps.location.lat();
        float lngSekarang = gps.location.lng();
        String lokasi = "Lokasi Kendaraan Sekarang: https://www.google.com/maps/place/";
        lokasi += String(latSekarang, 6);
        lokasi += ",";
        lokasi += String(lngSekarang, 6);
        bot.sendMessage(idChat, lokasi, "");
        Serial.println("Mengirim respon /Lokasi");
      } else {
        Serial.println("Tidak ada data GPS yang tersedia");
        bot.sendMessage(idChat, "Data GPS Sedang tidak bisa diakses coba lagi sesaat.");
      }
    } else if (teks == "/Bunyikan") {
      for (int i = 0; i < 3; i++) {
        tone(pinBuzzer, 30, 5000);
        delay(300);
      }
      bot.sendMessage(idChat, "Alarm telah berbunyi selama 5 detik.", "");
      Serial.println("Buzzer berbunyi selama 5 detik");
    } else if (teks == "/Mute") {
      buzzerDibisukan = true;
      bot.sendMessage(idChat, "Buzzer telah dibisukan. Getaran tidak akan mengaktifkan alarm.\n /UnMute untuk menghidupkan kembali.", "");
      Serial.println("Buzzer dibisukan");
    } else if (teks == "/UnMute") {
      buzzerDibisukan = false;
      bot.sendMessage(idChat, "Buzzer telah diaktifkan kembali. Getaran akan mengaktifkan alarm.", "");
      Serial.println("Buzzer diaktifkan kembali");
    } else {
      bot.sendMessage(idChat, "Masukkan kata kunci yang valid");
      Serial.println("Masukkan kata kunci yang valid");
    }
  }
}

// Fungsi setup untuk inisialisasi
void setup() {
  Serial.begin(115200);
  GPSSerial.begin(9600);

  // Koneksi ke WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Menghubungkan ke WiFi..");
  }
  // Cetak Alamat IP Lokal ESP8266
  Serial.println("Terhubung ke WiFi");
  Serial.println(WiFi.localIP());

  // Set waktu root certificate dari server Telegram
  client.setInsecure();

  // Inisialisasi pin SW-420 dan Buzzer
  pinMode(pinSW420, INPUT);
  pinMode(pinBuzzer, OUTPUT);
}

// Fungsi loop utama
void loop() {
  // Perbarui data GPS secara terus-menerus tanpa menampilkan di Serial Monitor
  while (GPSSerial.available()) {
    char c = GPSSerial.read();
    gps.encode(c);
  }

  // Cek pesan baru dari Telegram setiap interval yang ditentukan
  if (millis() > waktuTerakhirBotJalan + jedaPermintaanBot) {
    int jumlahPesanBaru = bot.getUpdates(bot.last_message_received + 1);

    while (jumlahPesanBaru) {
      Serial.println("Mendapat respon dari Telegram");
      tanganiPesanBaru(jumlahPesanBaru);
      jumlahPesanBaru = bot.getUpdates(bot.last_message_received + 1);
    }
    waktuTerakhirBotJalan = millis();
  }

  // Cek sensor SW-420 
  if (digitalRead(pinSW420) == HIGH) {
    if (!getaranTerdeteksi) {
      getaranTerdeteksi = true;

      // Jika buzzer tidak dibisukan, aktifkan buzzer
      if (!buzzerDibisukan) {
          for (int i = 0; i < 3; i++) {
          tone(pinBuzzer, 30, 5000);
          delay(300);
        }
      }

      // Baca data GPS sampai lokasi yang valid diperoleh
      unsigned long waktuMulai = millis();
      while (millis() - waktuMulai < 10000) { // Coba selama 10 detik
        while (GPSSerial.available()) {
          char c = GPSSerial.read();
          gps.encode(c);
        }

        if (gps.charsProcessed() > 10 && gps.location.isValid()) {
          float latSekarang = gps.location.lat();
          float lngSekarang = gps.location.lng();

          String lokasi = "Ada Getaran terdeteksi di kendaraan anda!, Lokasi Kendaraan: https://www.google.com/maps/place/";
          lokasi += String(latSekarang, 6);
          lokasi += ",";
          lokasi += String(lngSekarang, 6);
          bot.sendMessage(CHAT_ID, lokasi, "");
          Serial.println("Mengirim alert getaran dengan lokasi");
          break;
        }
      }

      if (!gps.location.isValid()) {
        Serial.println("Tidak ada data GPS yang valid untuk alert getaran");
        bot.sendMessage(CHAT_ID, "Ada Getaran Terdeteksi namun data GPS Tidak Bisa diakses. Segera Cek kendaraan anda segera!", "");
      }
    }
  } else {
    getaranTerdeteksi = false;
  }
}
