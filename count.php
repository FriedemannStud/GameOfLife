<?php
/**
 * KI-Agent unterstützt: Secure Counter Bridge for Biotope
 */

$apiKey = getenv('MONGODB_API_KEY') ?: 'DEIN_API_KEY_HIER';
$appId = getenv('MONGODB_APP_ID') ?: 'DEIN_APP_ID_HIER';
$clusterName = 'Cluster0';
$database = 'biotope_db';
$collection = 'submissions';

header('Content-Type: application/json');

$url = "https://eu-central-1.aws.data.mongodb-api.com/app/{$appId}/endpoint/data/v1/action/countDocuments";

$payload = [
    'dataSource' => $clusterName,
    'database' => $database,
    'collection' => $collection,
    'filter' => new stdClass() // Empty filter to count all
];

$ch = curl_init($url);
curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
curl_setopt($ch, CURLOPT_POST, true);
curl_setopt($ch, CURLOPT_POSTFIELDS, json_encode($payload));
curl_setopt($ch, CURLOPT_HTTPHEADER, [
    'Content-Type: application/json',
    'api-key: ' . $apiKey
]);

$response = curl_exec($ch);
$data = json_decode($response, true);
curl_close($ch);

if (isset($data['count'])) {
    echo json_encode(['count' => $data['count']]);
} else {
    echo json_encode(['count' => 0, 'debug' => $data]);
}
?>
