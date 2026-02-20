<p align="center">
  <img src="https://github.com/user-attachments/assets/81314a73-d417-411e-a345-f3b815548eae" width="650" alt="Arduino UNO R4 WiFi">
  <br><em>Figura 1 - Arduino UNO R4 WiFi (exemplo de placa utilizada).</em>
</p>

# Documentação (iniciantes) - Leitura de sensores + envio via Wi-Fi (UNO R4 WiFi)

Este projeto lê **temperatura, umidade, pressão e altitude estimada** (sensor **BME280**) e **luminosidade em lux** (sensor **BH1750**) via **I2C**, mostra os valores no **Serial Monitor** e, a cada **10 segundos**, envia um **HTTP POST** com um **JSON** para um servidor na sua rede (ex.: um PC/Raspberry) que pode gravar em MySQL.

<p align="center">
  <img src="https://github.com/user-attachments/assets/35e4598e-5ea5-4d9d-9080-d385a99d18f1" width="420" alt="Módulo BME280">
  <br><em>Figura 2 - Sensor BME280 (temperatura, umidade e pressão) em módulo I2C.</em>
</p>

<p align="center">
  <img src="https://github.com/user-attachments/assets/b05143c4-71e2-4604-b5d9-14bbaa93f3f0" width="420" alt="Módulo BH1750">
  <br><em>Figura 3 - Sensor BH1750 (luminosidade em lux) em módulo I2C.</em>
</p>

> Placa alvo sugerida pelo código: **Arduino UNO R4 WiFi** (por causa da biblioteca `WiFiS3`).

---

## 1) Componentes (o que é o quê)

### Microcontrolador / placa
- **Arduino UNO R4 WiFi**: placa com microcontrolador **Renesas RA4M1** + módulo Wi-Fi integrado. Usa a biblioteca `WiFiS3` para rede.

### Sensores (I2C)
- **BME280 (Bosch)**: mede **temperatura (°C)**, **umidade (%)** e **pressão (hPa)**. A **altitude (m)** é *calculada* a partir da pressão (não é um sensor de altitude real).
- **BH1750**: mede **luz ambiente (lux)**.

---

## 2) Ligações (fiação) - I2C

Os dois sensores usam **I2C**, então eles compartilham os mesmos fios **SDA** e **SCL**.

### Pinos I2C na placa
- Use os pinos **marcados na placa como `SDA` e `SCL`** (conector próprio do I2C).
- Também conecte **GND com GND** (terra comum) e **VCC**.

### Tabela de ligação (BME280 + BH1750 ? UNO R4 WiFi)

| Sensor | VCC | GND | SDA | SCL |
|---|---|---|---|---|
| BME280 | 3.3V (recomendado) | GND | SDA | SCL |
| BH1750 | 3.3V (recomendado) | GND | SDA | SCL |

#### Importante sobre alimentação (3.3V vs 5V)
- O código assume **3.3V** (mensagens de debug também sugerem isso).
- Muitos módulos vendidos como “BME280/BH1750” têm **regulador e conversão de nível** e aceitam 5V, mas **nem todos**.
- Se você não tiver certeza, use **3.3V** para evitar danos ao sensor.

#### Pull-ups (resistores do I2C)
- Em I2C, normalmente existem resistores “pull-up” no SDA/SCL. Muitos módulos já trazem isso.
- Se o scanner I2C não achar nada, pode ser falta de pull-up, fiação errada ou alimentação incorreta.

---

## 3) Endereços I2C esperados (muito importante)

O código tenta achar e validar os sensores pelos endereços mais comuns.

### BME280
- Endereços típicos: `0x76` ou `0x77`
- O código lê o registrador `0xD0` (Chip ID)
  - **BME280 verdadeiro**: Chip ID `0x60`
- Se não inicializar, o programa **para** (fica em loop infinito), porque o BME280 é obrigatório no projeto.

### BH1750
- Endereços típicos: `0x23` (padrão) ou `0x5C`
- Pino **ADDR** do módulo define o endereço:
  - ADDR em **GND** ? `0x23`
  - ADDR em **VCC** ? `0x5C`
- Se não encontrar, o código **não para**; apenas mostra um aviso.

---

## 4) Bibliotecas usadas (o que cada `#include` faz)

O topo do código tem:

- `#include <Wire.h>`
  - Biblioteca padrão do Arduino para comunicação **I2C**.

- `#include <Adafruit_BME280.h>`
  - Biblioteca da Adafruit para o sensor **BME280**.
  - Observação: ao instalar, o Arduino IDE pode pedir dependências como **Adafruit Unified Sensor**.

- `#include <BH1750.h>`
  - Biblioteca para o sensor de luz **BH1750** (comum no Library Manager).

- `#include <WiFiS3.h>`
  - Biblioteca de Wi-Fi usada na **UNO R4 WiFi**.

- `#include <ArduinoHttpClient.h>`
  - Cliente HTTP simples (para fazer GET/POST) em cima do `WiFiClient`.

### Como instalar bibliotecas no Arduino IDE
1. Arduino IDE ? **Sketch** ? **Include Library** ? **Manage Libraries…**
2. Pesquise e instale:
   - **Adafruit BME280 Library**
   - **Adafruit Unified Sensor** (se solicitado)
   - **BH1750**
   - **ArduinoHttpClient**
3. A biblioteca `Wire` já vem com o core.
4. `WiFiS3` normalmente vem junto com o core da UNO R4, mas pode aparecer como dependência/instalável.

---

## 4.1) Como compilar e enviar (Arduino IDE)

1. Conecte a placa no PC via USB.
2. Arduino IDE ? **Tools** ? **Board** ? selecione **Arduino UNO R4 WiFi**.
3. Arduino IDE ? **Tools** ? **Port** ? selecione a porta da sua placa.
4. Clique em **Upload**.
5. Abra o **Serial Monitor** e ajuste para **9600 baud**.

---

## 5) Configurações que você precisa editar

No código, edite estas variáveis:

### Wi-Fi
- `WIFI_SSID` ? nome da rede Wi-Fi
- `WIFI_PASS` ? senha

### Servidor HTTP (onde os dados serão enviados)
- `SERVER_HOST` ? IP ou host do servidor na rede local (ex.: `192.168.0.50`)
- `SERVER_PORT` ? porta (geralmente `80` para HTTP)
- `SERVER_PATH` ? caminho do endpoint (ex.: `/ingest.php`)

### Chave simples (API Key)
- `API_KEY` ? uma senha simples enviada no header `X-API-Key`
  - Serve para evitar que qualquer dispositivo “aleatório” poste dados.
  - Não é segurança forte; para internet use HTTPS e autenticação melhor.

---

## 6) O que o código faz (passo a passo)

### `setup()` (roda 1 vez ao ligar)
1. Abre o Serial a `9600`.
2. Tenta conectar no Wi-Fi, mas **sem travar** o programa:
   - Ele espera ~5 segundos por uma conexão inicial.
   - Se não conectar, segue lendo sensores e tenta reconectar “em background”.
3. Inicializa I2C (`Wire.begin()`) e roda um **scanner I2C**.
4. Inicializa o **BME280**:
   - Testa `0x76` e `0x77`
   - Confere Chip ID `0x60`
   - Se falhar, para o programa (BME280 é obrigatório).
5. Configura amostragem do BME280 (modo normal, filtros etc.).
6. Inicializa o **BH1750** em `0x23` e, se não achar, tenta `0x5C`.

### `loop()` (roda para sempre)
1. Lê:
   - Temperatura (°C)
   - Pressão (hPa)
   - Umidade (%)
   - Altitude estimada (m)
   - Luz (lux)
2. Imprime tudo no Serial Monitor.
3. Tenta manter o Wi-Fi conectado chamando `garantirWifiConectado()` a cada ciclo.
4. A cada **10 s** (`INTERVALO_POST_MS`), se Wi-Fi estiver conectado:
   - Monta um JSON
   - Envia via **HTTP POST** para `SERVER_PATH`
   - Mostra o **status HTTP** e a resposta do servidor

---

## 7) Formato do envio HTTP (para quem vai fazer o servidor)

### Requisição
- Método: `POST`
- URL: `http://SERVER_HOST:SERVER_PORT/SERVER_PATH`
- Headers:
  - `Content-Type: application/json`
  - `X-API-Key: <API_KEY>`
  - `Content-Length: <tamanho>`
- Body (JSON):

Exemplo de JSON enviado:
```json
{
  "device": "uno_r4_wifi",
  "temp_c": 24.13,
  "pressure_hpa": 1008.32,
  "humidity": 55.20,
  "lux": 123.45,
  "altitude_m": 42.80
}
```

### Resposta
O código imprime:
- `Status HTTP: <código>` (ex.: 200)
- `Resposta: <texto>` (se existir)

---

## 8) Exemplo mínimo de servidor (opcional)

A ideia do `ingest.php` é:
1. Ler o header `X-API-Key` e comparar com a sua chave.
2. Ler o corpo JSON (`php://input`) e fazer `json_decode`.
3. Inserir no banco (MySQL) e responder `200 OK`.

---

## 9) Dicas de uso e diagnóstico (troubleshooting)

### Serial Monitor
- Configure o Serial Monitor para **9600 baud**.
- No início, você deve ver:
  - “Escaneando I2C…”
  - Um ou mais endereços encontrados (por exemplo `0x76`, `0x23`)
  - “BME280 OK (0x76)”

### Caracteres “quebrados” (Ã£, Ã³, etc.) no Serial
Se algumas mensagens aparecerem com acentos estranhos, isso normalmente é **codificação do arquivo** (salvo como ANSI/Latin-1 em vez de UTF-8, por exemplo).
- Tente re-salvar o sketch no editor como **UTF-8**.
- Alternativa simples: trocar textos com acento por versões sem acento.

### “Nenhum dispositivo I2C encontrado”
Cheque:
- VCC e GND corretos (terra comum)
- Pinos SDA/SCL corretos (os pinos dedicados I2C da placa)
- Sensor alimentado (LED do módulo, se existir)
- Pull-ups no barramento

### BME280 não inicializa
- O módulo pode ser **BMP280** (parecido, mas Chip ID diferente).
- O endereço pode ser 0x76/0x77 e a fiação precisa estar correta.

### BH1750 não encontrado
- Verifique o pino **ADDR** (GND ? 0x23, VCC ? 0x5C)
- Se não tiver o BH1750 conectado, o resto do projeto continua.

### Wi-Fi não conecta
- Confirme SSID/senha
- Rede 2.4 GHz vs 5 GHz (depende do roteador e suporte do módulo)
- O código tenta reconectar a cada **15 s** (`WIFI_TENTAR_NOVAMENTE_MS`).

### Segurança
- Esse modelo é pensado para **rede local**.
- Para expor na internet, evite HTTP simples; prefira HTTPS e autenticação adequada.

---

## 10) Personalizações rápidas

- Intervalo de envio: altere `INTERVALO_POST_MS` (ex.: `30000` para 30 s).
- Nome do dispositivo: altere o campo `"device"` no JSON.
- Altitude mais realista: ajuste `SEALEVELPRESSURE_HPA` para a pressão ao nível do mar na sua região.

