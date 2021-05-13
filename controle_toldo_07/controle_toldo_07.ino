/*
<!-- The core Firebase JS SDK is always required and must be listed first -->
<script src="https://www.gstatic.com/firebasejs/8.6.0/firebase-app.js"></script>

<!-- TODO: Add SDKs for Firebase products that you want to use
     https://firebase.google.com/docs/web/setup#available-libraries -->

<script>
  // Your web app's Firebase configuration
  var firebaseConfig = {
    apiKey: "AIzaSyBFlqcoYQ07Xjpc3Bfbhuu799TYyhJI_V0",
    authDomain: "esp32-autocom.firebaseapp.com",
    projectId: "esp32-autocom",
    storageBucket: "esp32-autocom.appspot.com",
    messagingSenderId: "37325631743",
    appId: "1:37325631743:web:b7d9efd61c8c7930141cca"
  };
  // Initialize Firebase
  firebase.initializeApp(firebaseConfig);
</script>
*/
//Bibliotecas para WiFi e OTA
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <Update.h>

//Biblioteca com as funçoes do Firebase
#include <IOXhop_FirebaseESP32.h>

//Biblioteca com as funções de manipulação de JSON
#include <ArduinoJson.h>

// URL da base de dados fornecido pelo Firebase para a conexão http
#define FIREBASE_HOST  "https://esp32-autocom-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH  "" // Autenticação

const char *ssid     = "Autocom - Corporativo"; // Coloque o nome da sua rede wifi aqui
const char *password = "E3BG6019CA"; // Coloque a sua senha wifi aqui

const int Led = 2; //pino do Led
const int btnTouch = 4;//pino do botão de abertura

const String PATH_STATUS = "/Status"; //Caminho padrão para os comandos no Firebase

int timeOut = 10; //limite de tempo de acionamento do motor
bool btnTouch_executado = true; //indica se o comando da chave "Abrir" foi executado
unsigned long contador = 0;
String id_maquina = "1400/3";
String status_maquina = "OK";
String contador_str = "0";

//marca os instantes para controle de tentativas de conexão com o WiFi
//Valor máximmo alcançado em 50 dias de atividade ((2^32)-1) milissegundos
unsigned long instante = 0;


//intervalo para tentar reconectar no WiFi
unsigned long IntervaloParaTentarReconectar = 2 * 1000;

//Sinaliza a carga do FireBase
bool FireBaseIniciado = false;

// Página HTML para login
const char* loginIndex =
  "<head><meta charset='utf-8'></head>"
  "<form name='loginForm' method='post'>"
  "<table width='30%' bgcolor='00AAAA' align='center'>"
  "<tr>"
  "<td colspan=2>"
  "<center><font size=8><b>Login para ESP32 Fundicao Autocom v1.0</b></font></center>"
  "<br>"
  "</td>"
  "<br>"
  "<br>"
  "</tr>"
  "<td><font size=5>Usuário:<font size=5></td>"
  "<td><input type='text' size=50 name='userid'><br></td>"
  "</tr>"
  "<br>"
  "<br>"
  "<tr>"
  "<td><font size=5>Senha:<font size=5></td>"
  "<td><input type='Password' size=50 name='pwd'><br></td>"
  "<br>"
  "<br>"
  "</tr>"
  "<tr><td><br><br></td></tr>"
  "<tr>"
  "<td><br><br></td>"
  "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
  "<td><br><br></td>"
  "</tr>"
  "</table>"
  "</form>"
  "<script>"
  "function check(form)"
  "{"
  "if(form.userid.value=='usuario' && form.pwd.value=='senha')"
  "{"
  "window.open('/serverIndex')"
  "}"
  "else"
  "{"
  " alert('Nome ou senha inválidos')/*mostra uma menssagem de erro.*/"
  "}"
  "}"
  "</script>";

//Página do servidor
const char* serverIndex = 
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
   "<input type='file' name='update'>"
        "<input type='submit' value='Update'>"
    "</form>"
 "<div id='prg'>progress: 0%</div>"
 "<script>"
  "$('form').submit(function(e){"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  " $.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData:false,"
  "xhr: function() {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function(evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success:function(d, s) {"
  "console.log('success!')" 
 "},"
 "error: function (a, b, c) {"
 "}"
 "});"
 "});"
 "</script>";

WebServer server(80); //inicia o servidor

void setup()
{
  //Prepara a saída serial
  Serial.begin(115200);

  //Executa o setup do OTA (Over The Air updates)
  OTAsetup();

  //Ajusta as direções dos GPIO's
  pinMode(Led, OUTPUT);
  pinMode(btnTouch,INPUT);

  digitalWrite(Led, LOW); //inicia o Led de indicação
}

void loop()
{
  verificaWiFi();
  verificaFireBase();
  verificaOsBTNS();
}

// Função que sincroniza o valor do timeOut
//e Direção do ESP32 com o Firebase
void syncFirebase(String value)
{

  Serial.println("Sincronizando . . . ");
  Serial.println("Recebido: " + value);

  int indice = value.lastIndexOf(":") + 1; //procura pela 2ª string ":" no valor recebido
  Serial.println("INDICE: " + String(indice));
  value.remove(0, indice); //remove toda string antes da segunda ":" do valor recebido
  value.remove(value.length(), 1); //remove a "}" chave no fim da string
  Serial.println("TIMEOUT RECEBIDO: " + value); //envia para a serial o valor recebido (para debug)
  timeOut = value.toInt(); //converte a string para um inteiro
  if (timeOut > 60) timeOut = 60; //valor máximo peermitidopara o timeOut
  Serial.println("TIMEOUT: " + String(timeOut) + " (máx. 60s)"); //envia para a serial o novo valor
}

//pisca o led a cada 2x o valor de meioCiclo
void piscarLed(int meioCiclo)
{
  digitalWrite(Led, HIGH);
  delay(meioCiclo);
  digitalWrite(Led, LOW);
  delay(meioCiclo);
}

//Executa a abertura e sincronização
void incrementa_contador ()
{ 
    contador = contador + 1;
    Serial.println("CONTADOR: " + contador); //envia para a serial qual será a execução.
    Serial.println(" ");
    delay(500); //aguarda meio segundo
    contador_str = String(contador);

    int instante = millis(); //inicia o cronômetro para o timeout
    while ((millis() - instante) < timeOut * 1000) //verifica o estouro do timeout
    {
      piscarLed(250);//pisca o led a cada meio segundo para indicar a execução
    }

    if ((WiFi.status() == WL_CONNECTED) and (FireBaseIniciado == true)) atualizaFireBase("true");
}


//Atualiza o FireBase com os novos estados
void atualizaFireBase(String estado)
{
  StaticJsonBuffer<1> jsonBufferEstado; //reserva de memória para oobjeto JSON que contem os dados dos sensores
  JsonObject& novoEstado = jsonBufferEstado.createObject(); //cria o objeto JSON que conterá os dados dos sensores

  Firebase.set("/Status/IdentificacaoMaquina", id_maquina); //envia para o Firebase
  Firebase.set("Status/IP", WiFi.localIP().toString());
  Firebase.set("Status/Contador", contador_str);
  Firebase.set("Status/Status", status_maquina);

  jsonBufferEstado.clear(); //limpa os objetos JSON criados
}

//verifica a conexão WiFi
void verificaWiFi()
{

  //Enquanto não conectado, faz o Led indicador de rede piscar e envia uma mensagem pela serial
  //(Na inicialização, tentará conectar-se apenas durante aproximadamente 20s, depois desistirá.)

  verificaInstante();
  while ((WiFi.status() != WL_CONNECTED) and ((millis() - instante) < IntervaloParaTentarReconectar))
  {
    Serial.println("Conectando ao WiFi ..." + String(((IntervaloParaTentarReconectar - (millis() - instante)) / 1000)) + "s");
    WiFi.begin(ssid, password);
    delay(1000);
    piscarLed(100);
  }

  delay(2000); //Aguarda 2s
}

//Verifica a carga do FireBase
void verificaFireBase()
{
  if ((WiFi.status() == WL_CONNECTED) and (FireBaseIniciado == false))
  {
    //inicia a conexão com o Firebase
    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
    FireBaseIniciado = true;

    //Inicia a função de callback usando o caminho de COMANDOS
    Firebase.stream(PATH_STATUS, [](FirebaseStream stream)
    {
      String path, value; //Variáveis que recebem o caminho e valor retornado pelo callback

      // Se o evento que vem do callback é de alteração "put"
      if (stream.getEvent() == "put")
      {
        // Obtemos os valores de path e value
        path = stream.getPath();
        value = stream.getDataString();

        if (path.equals("/")) //se é o callback inicial quando a conexão é estabelecida
        {
          // Sincronizamos o valor de timeout
          syncFirebase(value); //executa a função que interpreta o valor recebido e atualiza o timeout
          Serial.println("Firebase sincronizado"); //envia a mensagem pela serial de sincronização
        }

        Serial.println(path + " - " + value); //imprime na serial o caminho e o valor recebido (para debug)

        if (path == "/timeout") //verifica o timeout
        {
          timeOut = value.toInt(); //obtém o valor atual
          if (timeOut > 60) timeOut = 60; //valor máximo peermitido para o timeOut
          Serial.println("Timeout:" + String(timeOut) + " (máx. 60s)"); //envia para a serial o novo valor
        }
      }
    });
  }
}

//Verifica mudanças de estado nos botões
void verificaOsBTNS()
{

  verificaInstante();
  delay(500);
  incrementa_contador();
  if (contador % 2 == 0){
    status_maquina = "OK";
  }
  else {
    status_maquina = "Manutencao";  
  }
  
  while ((millis() - instante) < (5 * IntervaloParaTentarReconectar))
  {
    server.handleClient();
    if ((digitalRead(btnTouch) == HIGH) and (btnTouch_executado == false))
    {
      btnTouch_executado = true;
      incrementa_contador();
    }

    delay(200);//debounce - minimiza os efeitos da rebatida dos botões

    if (digitalRead(btnTouch) == LOW) btnTouch_executado = false; //btnTouch deve se tornar LOW antes de poder ser executado novamente
    
    delay(200);//debounce - minimiza os efeitos da rebatida dos botões
  }
}

//Verifica se o instante está próximo do overflow
//e se o ESP está executando a muito tempo
void verificaInstante()
{
  // reinicia o ESP uma vez por semana
  const unsigned long intervaloDeRestart = 168 * 60 * 60 * 1000; // 168 horas por semana

  instante = millis();
  //if (instante > (((2 ^ 32) - 1) - (60 * 1000)))
  if (instante > intervaloDeRestart)
  {
    Serial.println("ESP será reiniciado devido ao longo período de funcionamento initerrupto.");
    delay(2000);
    ESP.restart();
  }
}
