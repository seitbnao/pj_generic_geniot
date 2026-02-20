#include <Wire.h>
#include <Adafruit_BME280.h>
#include <BH1750.h>
#include <WiFiS3.h>
#include <ArduinoHttpClient.h>

Adafruit_BME280 bme; // I2C
BH1750 medidorLuz;

// --- Wi-Fi / HTTP ---
// Preencha com seu Wi-Fi
char WIFI_SSID[] = "SEU_WIFI";
char WIFI_PASS[] = "SUA_SENHA";

// Servidor que vai gravar no MySQL (rode no PC/Raspberry/Host)
// Dica: use o IP do seu PC na rede local, ex: "192.168.0.50"
const char SERVER_HOST[] = "192.168.0.50";
const int SERVER_PORT = 80;
const char SERVER_PATH[] = "/ingest.php";

// Chave simples para evitar que qualquer um poste dados no seu endpoint
const char API_KEY[] = "troque_esta_chave";

WiFiClient wifi;
HttpClient http(wifi, SERVER_HOST, SERVER_PORT);

static unsigned long ultimoPostMs = 0;
static const unsigned long INTERVALO_POST_MS = 10000; // 10s

static unsigned long ultimaTentativaWifiMs = 0;
static const unsigned long WIFI_TENTAR_NOVAMENTE_MS = 15000; // 15s

static void garantirWifiConectado(bool verboso = false) {
  if (WiFi.status() == WL_CONNECTED) return;

  unsigned long agora = millis();
  if (agora - ultimaTentativaWifiMs < WIFI_TENTAR_NOVAMENTE_MS) return;
  ultimaTentativaWifiMs = agora;

  if (verboso) {
    Serial.print("Conectando no WiFi: ");
    Serial.println(WIFI_SSID);
  }

  // Tentativa rápida (não bloqueia o loop por muito tempo)
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

static uint8_t ler8(uint8_t addr, uint8_t reg) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return 0xFF;
  if (Wire.requestFrom(addr, (uint8_t)1) != 1) return 0xFF;
  return Wire.read();
}

static bool i2cPresente(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

static void escanearI2C() {
  Serial.println("\nEscaneando I2C...");
  uint8_t encontrados = 0;

  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    uint8_t erro = Wire.endTransmission();
    if (erro == 0) {
      Serial.print("Encontrado I2C em 0x");
      if (addr < 16) Serial.print('0');
      Serial.println(addr, HEX);
      encontrados++;
    }
  }

  if (encontrados == 0) {
    Serial.println("Nenhum dispositivo I2C encontrado.");
    Serial.println("Cheque: VCC=3.3V, GND comum, SDA/SCL nos pinos dedicados, e se o módulo tem pull-ups.");
  }
}

// Pressão ao nível do mar (hPa). Use um valor local mais preciso se quiser melhorar a altitude.
#define SEALEVELPRESSURE_HPA 1013.25

void setup() {
  Serial.begin(9600);
  while (!Serial) { } // útil em placas com Serial via USB nativa

  // Wi-Fi: tenta conectar, mas NÃO trava se falhar
  Serial.print("Tentando WiFi (não bloqueante): ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // Espera no máximo ~5s por conexão inicial; se não conectar, segue normal.
  unsigned long inicio = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - inicio) < 5000) {
    delay(200);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi OK. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi não conectou agora; vou continuar lendo sensores e tentando reconectar em background.");
    ultimaTentativaWifiMs = millis();
  }

  Wire.begin();
  delay(50);
  escanearI2C();

  // --- BME280 ---
  bool bmeOk = false;
  uint8_t bmeAddr = 0;
  for (uint8_t candidato : { (uint8_t)0x76, (uint8_t)0x77 }) {
    if (!i2cPresente(candidato)) continue;

    // Registro de ID do chip (0xD0). BME280 = 0x60
    uint8_t chipId = ler8(candidato, 0xD0);
    Serial.print("Chip ID em 0x");
    if (candidato < 16) Serial.print('0');
    Serial.print(candidato, HEX);
    Serial.print(": 0x");
    if (chipId < 16) Serial.print('0');
    Serial.println(chipId, HEX);

    if (chipId == 0x60) {
      bmeOk = bme.begin(candidato);
      bmeAddr = candidato;
      break;
    }
  }

  if (!bmeOk) {
    Serial.println("\nErro: BME280 não inicializou.");
    Serial.println("- Confirme VCC=3.3V, GND comum, SDA/SCL nos pinos dedicados.");
    Serial.println("- O BME280 normalmente está em 0x76 ou 0x77, com Chip ID 0x60.");
    while (1) delay(10);
  }

  bme.setSampling(
    Adafruit_BME280::MODE_NORMAL,
    Adafruit_BME280::SAMPLING_X2,   // temperatura
    Adafruit_BME280::SAMPLING_X16,  // pressão
    Adafruit_BME280::SAMPLING_X1,   // umidade
    Adafruit_BME280::FILTER_X16,
    Adafruit_BME280::STANDBY_MS_500
  );
  Serial.print("BME280 OK (0x");
  if (bmeAddr < 16) Serial.print('0');
  Serial.print(bmeAddr, HEX);
  Serial.println(")");

  // --- BH1750 ---
  // Endereços comuns do BH1750:
  // - 0x23 (ADDR em GND ou padrão)
  // - 0x5C (ADDR em VCC)
  bool bhOk = false;
  if (i2cPresente(0x23)) {
    bhOk = medidorLuz.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23);
  }
  if (!bhOk && i2cPresente(0x5C)) {
    bhOk = medidorLuz.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x5C);
  }
  if (bhOk) {
    Serial.println("BH1750 OK!");
  } else {
    Serial.println("Aviso: BH1750 não encontrado (0x23/0x5C). Ligue ADDR em GND (0x23) ou 3.3V (0x5C).");
  }
}

void loop() {
  float tempC = bme.readTemperature();
  float pressHpa = bme.readPressure() / 100.0F; // Pa -> hPa
  float hum = bme.readHumidity();
  float altM = bme.readAltitude(SEALEVELPRESSURE_HPA);
  float lux = medidorLuz.readLightLevel();

  Serial.print("Temp: ");
  Serial.print(tempC);
  Serial.print(" *C | Pressão: ");
  Serial.print(pressHpa);
  Serial.print(" hPa | Umidade: ");
  Serial.print(hum);
  Serial.print(" % | Luz: ");
  Serial.print(lux);
  Serial.print(" lx | Altitude: ");
  Serial.print(altM);
  Serial.println(" m");

  // Mantém o Wi-Fi tentando conectar em background
  garantirWifiConectado(false);

  // Envia para o servidor a cada INTERVALO_POST_MS (somente se Wi-Fi conectado)
  unsigned long agora = millis();
  if (agora - ultimoPostMs >= INTERVALO_POST_MS) {
    ultimoPostMs = agora;

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi offline; pulando envio (vou tentar reconectar automaticamente).");
      delay(1000);
      return;
    }

    String json = "{";
    json += "\"device\":\"uno_r4_wifi\",";
    json += "\"temp_c\":" + String(tempC, 2) + ",";
    json += "\"pressure_hpa\":" + String(pressHpa, 2) + ",";
    json += "\"humidity\":" + String(hum, 2) + ",";
    json += "\"lux\":" + String(lux, 2) + ",";
    json += "\"altitude_m\":" + String(altM, 2);
    json += "}";

    Serial.println("Enviando POST para servidor...");

    http.beginRequest();
    http.post(SERVER_PATH);
    http.sendHeader("Content-Type", "application/json");
    http.sendHeader("X-API-Key", API_KEY);
    http.sendHeader("Content-Length", json.length());
    http.beginBody();
    http.print(json);
    http.endRequest();

    int statusCode = http.responseStatusCode();
    String resp = http.responseBody();
    Serial.print("Status HTTP: ");
    Serial.println(statusCode);
    if (resp.length() > 0) {
      Serial.print("Resposta: ");
      Serial.println(resp);
    }
  }

  delay(1000);
}