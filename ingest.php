<?php
declare(strict_types=1);

// ingest.php
// Endpoint HTTP para receber JSON do Arduino e gravar no MySQL.
// Espera:
// - Método: POST
// - Header: X-API-Key: <sua_chave>
// - Body: application/json
//
// Exemplo de JSON:
// {
//   "device": "uno_r4_wifi",
//   "temp_c": 24.13,
//   "pressure_hpa": 1008.32,
//   "humidity": 55.20,
//   "lux": 123.45,
//   "altitude_m": 42.80
// }

// ==============================
// CONFIGURAÇÕES
// ==============================

// Mesma chave do Arduino (API_KEY no .ino)
const API_KEY = 'troque_esta_chave';

// MySQL
const DB_HOST = '127.0.0.1';
const DB_PORT = 3306;
const DB_NAME = 'sensores';
const DB_USER = 'root';
const DB_PASS = '';

// Tabela (veja schema.sql)
const DB_TABLE = 'leituras';

// ==============================
// FUNÇÕES AUXILIARES
// ==============================

function respondJson(int $statusCode, array $payload): void {
    http_response_code($statusCode);
    header('Content-Type: application/json; charset=utf-8');
    echo json_encode($payload, JSON_UNESCAPED_UNICODE);
    exit;
}

function getHeader(string $name): ?string {
    // Em PHP/Apache/Nginx, headers costumam vir em $_SERVER com prefixo HTTP_
    $key = 'HTTP_' . strtoupper(str_replace('-', '_', $name));
    if (!isset($_SERVER[$key])) return null;
    $value = trim((string)$_SERVER[$key]);
    return ($value === '') ? null : $value;
}

function requirePostMethod(): void {
    $method = strtoupper($_SERVER['REQUEST_METHOD'] ?? '');
    if ($method !== 'POST') {
        respondJson(405, [
            'ok' => false,
            'error' => 'Método não permitido. Use POST.'
        ]);
    }
}

function requireApiKey(): void {
    $key = getHeader('X-API-Key');
    if ($key === null || !hash_equals(API_KEY, $key)) {
        respondJson(401, [
            'ok' => false,
            'error' => 'API key inválida ou ausente.'
        ]);
    }
}

function readJsonBody(): array {
    $raw = file_get_contents('php://input');
    if ($raw === false || trim($raw) === '') {
        respondJson(400, [
            'ok' => false,
            'error' => 'Body vazio.'
        ]);
    }

    $data = json_decode($raw, true);
    if (!is_array($data)) {
        respondJson(400, [
            'ok' => false,
            'error' => 'JSON inválido.'
        ]);
    }

    return $data;
}

function requireField(array $data, string $field): mixed {
    if (!array_key_exists($field, $data)) {
        respondJson(400, [
            'ok' => false,
            'error' => "Campo obrigatório ausente: {$field}"
        ]);
    }
    return $data[$field];
}

function toStringValue(mixed $v): string {
    if (is_string($v) && trim($v) !== '') return trim($v);
    if (is_numeric($v)) return (string)$v;
    respondJson(400, [
        'ok' => false,
        'error' => 'Campo device inválido.'
    ]);
}

function toFloatValue(mixed $v, string $field): float {
    if (is_int($v) || is_float($v)) return (float)$v;
    if (is_string($v) && is_numeric($v)) return (float)$v;

    respondJson(400, [
        'ok' => false,
        'error' => "Campo {$field} inválido (esperado número)."
    ]);
}

function pdo(): PDO {
    $dsn = sprintf('mysql:host=%s;port=%d;dbname=%s;charset=utf8mb4', DB_HOST, DB_PORT, DB_NAME);

    try {
        $pdo = new PDO($dsn, DB_USER, DB_PASS, [
            PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,
            PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
        ]);
        return $pdo;
    } catch (Throwable $e) {
        respondJson(500, [
            'ok' => false,
            'error' => 'Falha ao conectar no MySQL.',
            // Para produção, remova detalhes.
            'details' => $e->getMessage(),
        ]);
    }
}

// ==============================
// HANDLER
// ==============================

requirePostMethod();
requireApiKey();

$data = readJsonBody();

$device = toStringValue(requireField($data, 'device'));
$tempC = toFloatValue(requireField($data, 'temp_c'), 'temp_c');
$pressureHpa = toFloatValue(requireField($data, 'pressure_hpa'), 'pressure_hpa');
$humidity = toFloatValue(requireField($data, 'humidity'), 'humidity');
$lux = toFloatValue(requireField($data, 'lux'), 'lux');
$altitudeM = toFloatValue(requireField($data, 'altitude_m'), 'altitude_m');

$pdo = pdo();

$sql = "INSERT INTO " . DB_TABLE . " (device, temp_c, pressure_hpa, humidity, lux, altitude_m, created_at)
        VALUES (:device, :temp_c, :pressure_hpa, :humidity, :lux, :altitude_m, NOW())";

try {
    $stmt = $pdo->prepare($sql);
    $stmt->execute([
        ':device' => $device,
        ':temp_c' => $tempC,
        ':pressure_hpa' => $pressureHpa,
        ':humidity' => $humidity,
        ':lux' => $lux,
        ':altitude_m' => $altitudeM,
    ]);

    respondJson(200, [
        'ok' => true,
        'id' => (int)$pdo->lastInsertId(),
    ]);
} catch (Throwable $e) {
    respondJson(500, [
        'ok' => false,
        'error' => 'Falha ao inserir no banco.',
        // Para produção, remova detalhes.
        'details' => $e->getMessage(),
    ]);
}
