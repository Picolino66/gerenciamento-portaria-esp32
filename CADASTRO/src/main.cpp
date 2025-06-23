#include "Arduino.h"
#include <MFRC522.h> //biblioteca responsável pela comunicação com o módulo RFID-RC522
#include <SPI.h> //biblioteca para comunicação do barramento SPI
#include <WiFi.h>
#include <HTTPClient.h>

#define SS_PIN    21
#define RST_PIN   22

#define SIZE_BUFFER     18
#define MAX_SIZE_BLOCK  16

#define pinVerde     12
#define pinVermelho  32

const int ledPin = 13; // Defina o pino do LED

String uidString = ""; 

const char* ssid = "Ingrid";
const char* password = "123456789";
void makeHttpPostRequest(String uidString);
String getUID(MFRC522::Uid uid);
void leituraDados();

//esse objeto 'chave' é utilizado para autenticação
MFRC522::MIFARE_Key key;
//código de status de retorno da autenticação
MFRC522::StatusCode status;

// Definicoes pino modulo RC522
MFRC522 mfrc522(SS_PIN, RST_PIN); 

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
  digitalWrite(ledPin, HIGH); // Liga o LED
}

void loop() 
{
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
  // instrui o PICC quando no estado ACTIVE a ir para um estado de "parada"
  mfrc522.PICC_HaltA(); 
  // "stop" a encriptação do PCD, deve ser chamado após a comunicação com autenticação, caso contrário novas comunicações não poderão ser iniciadas
  mfrc522.PCD_StopCrypto1();  
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
  // Fazer a requisição à API
    digitalWrite(ledPin, LOW); // Liga o LED
  makeHttpPostRequest(uidString);
}

void makeHttpPostRequest(String uidString) {
  // Crie uma instância da classe HTTPClient
  HTTPClient http;

  // Especifique o endpoint da sua API
  String url = "https://portaria-api-lime.vercel.app/cadastro/carteirinha/";

  // Inicie a conexão com a URL
  http.begin(url);

  // Configure o cabeçalho da requisição para indicar que é um POST
  http.addHeader("Content-Type", "application/json");

  // Crie o JSON que você deseja enviar na requisição
  String jsonPayload = "{\"carteirinha\":\""+uidString+"\"}";

  // Faça a requisição POST, enviando os dados
  int httpCode = http.POST(jsonPayload);

  // Se a requisição foi bem-sucedida, exiba a resposta
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("Resposta da API:");
    Serial.println(payload);
  } else {
    Serial.println("Erro na requisição à API");
  }

  // Libere os recursos
  http.end();
  digitalWrite(ledPin, HIGH); // Liga o LED
}

String getUID(MFRC522::Uid uid) {
  String uidString = "";
  for (byte i = 0; i < uid.size; i++) {
    uidString += (uid.uidByte[i] < 0x10 ? "0" : "");
    uidString += String(uid.uidByte[i], HEX);
  }
  return uidString;
}

