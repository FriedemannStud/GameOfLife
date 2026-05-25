<?php
/**
 * KI-Agent unterstützt: Secure Bridge between Biotope Editor and MongoDB Atlas
 * 
 * This script should be placed on the university server alongside bericht_und_editor.html.
 * It uses the MongoDB Atlas Data API to securely store submissions without exposing 
 * API keys to the browser.
 */

// --- CONFIGURATION ---
// 1. Create a .env file in the same directory OR set these variables here if .env is not supported.
// 2. You need an "API Key" from MongoDB Atlas (Access Manager -> API Keys).
// 3. You need the "Data API App ID" from the Data API section in Atlas.

$apiKey = getenv('MONGODB_API_KEY') ?: 'DEIN_API_KEY_HIER';
$appId = getenv('MONGODB_APP_ID') ?: 'DEIN_APP_ID_HIER'; // e.g. data-abcde
$clusterName = 'Cluster0'; 
$database = 'biotope_db';
$collection = 'submissions';
$region = 'aws-eu-central-1'; // Check your Atlas Data API URL for this

// --- HEADER ---
header('Content-Type: application/json');

// --- READ INPUT ---
$input = file_get_contents('php://input');
$data = json_decode($input, true);

if (!$data) {
    http_response_code(400);
    echo json_encode(['error' => 'Invalid JSON input']);
    exit;
}

// --- PREPARE DATA API REQUEST ---
// URL for "insertOne" action
$url = "https://eu-central-1.aws.data.mongodb-api.com/app/{$appId}/endpoint/data/v1/action/insertOne";

// Add timestamp on server side for integrity
$data['created_at'] = date('c'); 
$data['status'] = 'active';
$data['elo_rating'] = 1200;

$payload = [
    'dataSource' => $clusterName,
    'database' => $database,
    'collection' => $collection,
    'document' => $data
];

// --- EXECUTE CURL ---
$ch = curl_init($url);
curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
curl_setopt($ch, CURLOPT_POST, true);
curl_setopt($ch, CURLOPT_POSTFIELDS, json_encode($payload));
curl_setopt($ch, CURLOPT_HTTPHEADER, [
    'Content-Type: application/json',
    'api-key: ' . $apiKey
]);

$response = curl_exec($ch);
$httpCode = curl_getinfo($ch, CURLINFO_HTTP_CODE);
curl_close($ch);

// --- RESPONSE ---
if ($httpCode >= 200 && $httpCode < 300) {
    echo $response;
} else {
    http_response_code($httpCode);
    echo json_encode([
        'error' => 'Atlas Data API error',
        'details' => json_decode($response, true)
    ]);
}
?>
