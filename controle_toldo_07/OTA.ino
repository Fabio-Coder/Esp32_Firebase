void OTAsetup()
{
  // Conecta no WiFi
  WiFi.begin(ssid, password);
  Serial.println("");

  //aguarda pela conexão
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Conectado em ");
  Serial.println(ssid);
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());

//O servidor responde a solicitação de conexão
//enviando a página "loginIndex"
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", loginIndex);
  });
  
//O servidor responde a solicitação de conexão
//enviando a página "serverIndex"
  server.on("/serverIndex", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });
  
//Controla o Upload do firmware
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "ERRO" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Atualizando: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      //Grava o firmware no ESP
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      //Sinaliza o fim do UPDATE
      if (Update.end(true)) {
        Serial.printf("Atualização bem sucedida: %u\nReiniciando...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });

  //inicia o servidor
  server.begin();
}
