#include <WiFi.h>
#include <HTTPClient.h>
#include "Arduino.h"
#include <MFRC522.h> //biblioteca responsável pela comunicação com o módulo RFID-RC522
#include <SPI.h> //biblioteca para comunicação do barramento SPI
#include <ArduinoJson.h>


const char* ssid = "Ingrid";
const char* password = "123456789";

#define SIZE_BUFFER     18
#define MAX_SIZE_BLOCK  16

#define SS_PIN    21
#define RST_PIN   22

#define pinVerde     12
#define pinVermelho  32

const int ledPin = 13; // Defina o pino do LED

String uidString = "";

StaticJsonDocument<200> doc;

String getUID(MFRC522::Uid uid);
void leituraDados();

//esse objeto 'chave' é utilizado para autenticação
MFRC522::MIFARE_Key key;
//código de status de retorno da autenticação
MFRC522::StatusCode status;

// Definicoes pino modulo RC522
MFRC522 mfrc522(SS_PIN, RST_PIN);

void makeHttpRequest();

void setup() {
  pinMode(ledPin, OUTPUT); // Configura o pino como saída
  // Inicia a serial
  Serial.begin(9600);
  SPI.begin(); // Init SPI bus

  pinMode(pinVerde, OUTPUT);
  pinMode(pinVermelho, OUTPUT);

  // Conectar-se à rede Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao WiFi...");
  }
  Serial.println("Conectado ao WiFi");

   // Inicia MFRC522
  mfrc522.PCD_Init();
  // Mensagens iniciais no serial monitor
  Serial.println("Aproxime o seu cartao do leitor...");
  Serial.println();
}

void loop() {
  // Aguarda a aproximacao do cartao
  if ( ! mfrc522.PICC_IsNewCardPresent())
  {
    return;
  }
  // Seleciona um dos cartoes
  if ( ! mfrc522.PICC_ReadCardSerial())
  {
    return;
  }

  leituraDados();

  // Fazer a requisição à API
  makeHttpRequest();

  delay(5000);

  // instrui o PICC quando no estado ACTIVE a ir para um estado de "parada"
  mfrc522.PICC_HaltA();
  // "stop" a encriptação do PCD, deve ser chamado após a comunicação com autenticação, caso contrário novas comunicações não poderão ser iniciadas
  mfrc522.PCD_StopCrypto1();
}

void makeHttpRequest() {
  // Crie uma instância da classe HTTPClient
  HTTPClient http;

  // Especifique o endpoint da sua API
  String url = "https://portaria-api-lime.vercel.app/usuario/" + uidString;

  // Inicie a conexão com a URL
  http.begin(url);

  // Obtenha a resposta da requisição
  int httpCode = http.GET();

  String payload = "";

  // Se a requisição foi bem-sucedida, exiba a resposta
  if (httpCode > 0) {
    payload = http.getString();
    Serial.println("Resposta da API:");
    Serial.println(payload);
  } else {
    Serial.println("Erro na requisição à API");
  }

  // Libere os recursos
  http.end();

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, payload);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  if(doc["existe"]) {
    digitalWrite(ledPin, HIGH); // Liga o LED
    delay(5000); // Aguarda 1 segundo
    digitalWrite(ledPin, LOW); // Desliga o LED
    return;
  }
  digitalWrite(ledPin, HIGH); // Liga o LED
  delay(500); // Aguarda 1 segundo
  digitalWrite(ledPin, LOW); // Desliga o LED
  delay(500);
  digitalWrite(ledPin, HIGH); // Liga o LED
  delay(500); // Aguarda 1 segundo
  digitalWrite(ledPin, LOW); // Desliga o LED
}

//faz a leitura dos dados do cartão/tag
void leituraDados(){
  //imprime os detalhes tecnicos do cartão/tag
  mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid));

  //Prepara a chave - todas as chaves estão configuradas para FFFFFFFFFFFFh (Padrão de fábrica).
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  //buffer para colocar os dados ligos
  byte buffer[SIZE_BUFFER] = {0};

  //bloco que faremos a operação
  byte bloco = 1;
  byte tamanho = SIZE_BUFFER;

  //imprime os dados lidos
  for (uint8_t i = 0; i < MAX_SIZE_BLOCK; i++)
  {
      Serial.write(buffer[i]);
  }
  Serial.println(" ");

  uidString = getUID(mfrc522.uid);  // Armazena o UID na variável uidString
}

String getUID(MFRC522::Uid uid) {
  String uidString = "";
  for (byte i = 0; i < uid.size; i++) {
    uidString += (uid.uidByte[i] < 0x10 ? "0" : "");
    uidString += String(uid.uidByte[i], HEX);
  }
  return uidString;
}