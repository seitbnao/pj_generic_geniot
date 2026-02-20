# Documentação do ingest.php (API + MySQL)

Este documento explica como usar o endpoint [ingest.php](ingest.php) para receber o JSON enviado pelo Arduino (UNO R4 WiFi) e gravar os dados no MySQL.

---

## 1) O que o endpoint faz

O [ingest.php](ingest.php) é um endpoint HTTP que:
- Aceita apenas `POST`
- Exige o header `X-API-Key`
- Lê um corpo JSON (`application/json`)
- Valida os campos obrigatórios e tipos (string/números)
- Insere os dados em uma tabela MySQL via **PDO** (prepared statement)
- Retorna JSON com `ok: true/false`

---

## 2) Pré-requisitos

- PHP 8.x (funciona em PHP 7.4+ na maioria dos casos, mas o projeto está em modo estrito)
- Extensão **PDO** habilitada
- Driver **pdo_mysql** habilitado
- Servidor web (Apache/Nginx) ou PHP built-in server (para testes)
- MySQL/MariaDB

---

## 3) Banco de dados (tabela)

Use o arquivo [schema.sql](schema.sql) para criar o banco `sensores` e a tabela `leituras`.

Campos gravados:
- `device` (texto)
- `temp_c`, `pressure_hpa`, `humidity`, `lux`, `altitude_m` (números)
- `created_at` (data/hora do servidor)

---

## 4) Configuração do ingest.php

No topo do arquivo [ingest.php](ingest.php), ajuste:

### API Key
- `API_KEY`
  - Deve ser **igual** à constante `API_KEY` no Arduino.
  - O Arduino envia essa chave no header `X-API-Key`.

### MySQL
- `DB_HOST` (ex.: `127.0.0.1`)
- `DB_PORT` (padrão: `3306`)
- `DB_NAME` (padrão no schema: `sensores`)
- `DB_USER`
- `DB_PASS`
- `DB_TABLE` (padrão no schema: `leituras`)

---

## 5) Contrato da API (HTTP)

### URL
Exemplo:
- `http://192.168.0.50/ingest.php`

> No Arduino, isso corresponde a `SERVER_HOST`, `SERVER_PORT` e `SERVER_PATH`.

### Método
- `POST`

### Headers obrigatórios
- `Content-Type: application/json`
- `X-API-Key: <sua_chave>`

### Body (JSON)
Campos obrigatórios:
- `device` (string)
- `temp_c` (number)
- `pressure_hpa` (number)
- `humidity` (number)
- `lux` (number)
- `altitude_m` (number)

Exemplo:
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

---

## 6) Respostas (HTTP)

### Sucesso
- Status: `200`
- Body:
```json
{ "ok": true, "id": 123 }
```

### Erros comuns
- `405` — método diferente de POST
- `401` — `X-API-Key` ausente ou inválido
- `400` — JSON inválido, body vazio ou campo obrigatório ausente/inválido
- `500` — falha de conexão com o MySQL ou erro ao inserir

---

## 7) Como testar sem o Arduino

### Teste com curl
```bash
curl -i \
  -X POST "http://127.0.0.1/ingest.php" \
  -H "Content-Type: application/json" \
  -H "X-API-Key: troque_esta_chave" \
  -d '{"device":"teste","temp_c":25.1,"pressure_hpa":1000.2,"humidity":50.0,"lux":10.0,"altitude_m":12.3}'
```

### Teste no PowerShell (Windows)
```powershell
$uri = "http://127.0.0.1/ingest.php"
$headers = @{ "X-API-Key" = "troque_esta_chave" }
$body = @{ device="teste"; temp_c=25.1; pressure_hpa=1000.2; humidity=50.0; lux=10.0; altitude_m=12.3 } | ConvertTo-Json
Invoke-RestMethod -Uri $uri -Method Post -Headers $headers -ContentType "application/json" -Body $body
```

---

## 8) Segurança (importante)

- A `API_KEY` é um controle simples (evita posts acidentais), mas **não é segurança forte**.
- Para expor na internet, use:
  - HTTPS
  - autenticação robusta (tokens/assinaturas)
  - regras de firewall/rede
- O endpoint retorna `details` em erros 500 (útil para depurar). Em produção, remova `details` para não vazar informação sensível.

---

## 9) Troubleshooting

### Recebo 401 (API key inválida)
- Confira se o Arduino está enviando o header `X-API-Key` igual ao `API_KEY` do [ingest.php](ingest.php).

### Recebo 500 (Falha ao conectar no MySQL)
- Verifique `DB_HOST`, `DB_USER`, `DB_PASS`
- Confirme se o serviço MySQL está rodando
- Confirme se o PHP tem `pdo_mysql` habilitado

### Recebo 400 (JSON inválido)
- Confirme `Content-Type: application/json`
- Confirme se o body é JSON válido

