/*-----------------------------------------------------------------------------------*/
/*
  ASIGNACIÓN DE PINES
*/
//Pines colorímetro
#define S0              A1
#define S1              A2
#define S2              A3
#define S3              A4
#define sensorOut       A5

/*
  PARÁMETROS DE AJUSTE DE COLORÍMETRO
*/
#define timeOut         1000   //Tiempo límite para leer
#define black_offset    8000   //Ajuste para límite de color negro
#define white_offset    2000   //Ajuste para límite de color blanco

/*-----------------------------------------------------------------------------------*/

//Estructura tipo de dato para almacenar color
typedef struct rgb {
  uint32_t R; //Rojo
  uint32_t G; //Verde
  uint32_t B; //Azul
  bool TO;    //Indicador de tiempo límite de sensor(0 lectura correcta, 1 tiempo superado)
} RGB;

//Estructura temporal de color en flotante para estimar promedios
typedef struct normf {
  float R;  //Rojo
  float G;  //Verde
  float B;  //Azul
} NORMFAC;


//Definición de variables
NORMFAC normalize   = {1, 1, 1};  //Factores de normalización de color
RGB color_blanco    = {0, 0, 0};   //Variable de referencia de color blanco
RGB color_negro     = {0, 0, 0};    //Variable de referencia de color negro

/*
  Función Colorimetro:
  Obtiene valores del sensor TCS3200
  Entrada: ninguna
  Retorno: Estructura RGB con datos de medición
*/

RGB colorimetro() {
  uint32_t time_exe = millis();//Tiempo de inicio de la medición
  RGB temp;//Variable temporal para almacenar datos de color

  //Configuración de pines para leer color Rojo
  delay(30);
  digitalWrite(S2, LOW);
  digitalWrite(S3, LOW);
  temp.R = pulseIn(sensorOut, LOW);//El tiempo de pulso indica la magnitud de color

  delay(30);
  //Configuración de pines para leer color Verde
  digitalWrite(S2, HIGH);
  digitalWrite(S3, HIGH);
  temp.G = pulseIn(sensorOut, LOW);
  delay(30);

  //Configuración de pines para leer color Azul
  digitalWrite(S2, LOW);
  digitalWrite(S3, HIGH);
  temp.B = pulseIn(sensorOut, LOW);

  //Se normalizan los valores de color
  temp.R = (normalize.R * temp.R);
  temp.G = (normalize.G * temp.G);
  temp.B = (normalize.B * temp.B);
  temp.TO = false;

  //Compara si se ha superado el tiempo límite para medir
  if ((millis() - time_exe) > timeOut) {
    Serial.println("\r\nTime Out!");
    temp.TO = true;
  }
  return temp;
}

/*
  Funcion waiting_cmd:
  Función utilitaria para esperar respuesta del usuario
*/
void waiting_cmd() {
  Serial.println("Envíe una letra para continuar");
  uint8_t count = 0;
  while (!Serial.available()) {
    count++;
    Serial.print(".");
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    delay(200);
    if (count > 40) {
      Serial.println();
      count = 0;
    }
  }
  while (Serial.available()) {
    Serial.read();
  }
}

/*
  Función print_color:
  Transmite por UART datos de color
  Entrada: data -> Estructura RGB
*/
void print_color(RGB data) {
  Serial.print("\r\nR= ");
  Serial.print(data.R);
  Serial.print("\t");
  Serial.print("G= ");
  Serial.print(data.G);
  Serial.print("\t");
  Serial.print("B= ");
  Serial.print(data.B);
}

/*
  Función colorimetro_iniciar:
  Rutina de configuración del hardware TCS3200 y computo de parámetros iniciales
*/
void colorimetro_iniciar() {
  long prom;
  bool cond_temporal = true;
  uint32_t black_threshold = 0;

  //Configura pines del sensor
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(sensorOut, INPUT);

  //Activa el sensor TCS3200
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);
  Serial.println("Calibración de colorimetro, siga las instrucciones\r\n");

  //Mide el valor de negro del ambiente como referencia
  while (cond_temporal) {
    Serial.println("\r\nPASO 1: Ubique un elemento de color NEGRO");
    waiting_cmd();
    color_negro = colorimetro();
    print_color(color_negro);
    //Se calcula el promedio de valores
    prom = (color_negro.R + color_negro.G + color_negro.B) / 3;
    black_threshold = prom;
    Serial.print("\r\nLimite de negro: ");
    Serial.println(black_threshold);

    //Si el valor promedio de negro es muy bajo es probable que exista un problema de luz
    if (black_threshold < 15000) {
      Serial.println("ALERTA!!!\nlímite de negro demasiado bajo, posible contaminación de luz,reintente");
    }
    else {
      cond_temporal = false;
    }
  }

  Serial.println("\r\nPASO 2: Ubique un elemento de color BLANCO");
  waiting_cmd();
  color_blanco = colorimetro();
  print_color(color_blanco);

  //Obtiene el valor diferencial de blanco y negro
  Serial.print("\r\n\r\nNormalizadores");
  normalize.R = color_negro.R - color_blanco.R;
  normalize.G = color_negro.G - color_blanco.G;
  normalize.B = color_negro.B - color_blanco.B;

  //Calculo del promedio de los valores diferenciales
  prom = (normalize.R + normalize.G + normalize.B) / 3;
  //Actualiza el valor de normalización para cada color según promedio
  normalize.R = prom / normalize.R;
  normalize.G = prom / normalize.G;
  normalize.B = prom / normalize.B;

  Serial.print("\r\nR= ");
  Serial.print(normalize.R);
  Serial.print("\t");
  Serial.print("G= ");
  Serial.print(normalize.G);
  Serial.print("\t");
  Serial.print("B= ");
  Serial.print(normalize.B);
  Serial.print("\r\n");

  //Aplica valores normalizados para las comparativas de blanco y negro
  color_negro.R *= normalize.R;
  color_negro.G *= normalize.G;
  color_negro.B *= normalize.B;
  color_blanco.R *= normalize.R;
  color_blanco.G *= normalize.G;
  color_blanco.B *= normalize.B;

  Serial.println("\r\nTERMINADO!\r\nEsperando...");
}

/*
 * Función Selector_color
 * Compara rangos de color de forma básica para establecer cinco colores base en retorno:
  Negro   = 0
  Blanco  = 1
  Rojo    = 2
  Verde   = 3
  Azul    = 4
  Error   = 5
*/
uint8_t selector_color(RGB input) {
  if (input.TO or (input.R > (color_negro.R - black_offset) and input.G > (color_negro.G - black_offset) and input.B > (color_negro.B - black_offset))) {
    return 0;
  }
  if (input.R < (color_blanco.R + white_offset) and input.G < (color_blanco.G + white_offset) and input.B < (color_blanco.B + white_offset)) {
    return 1;
  }
  if (input.R < input.B and input.R < input.G) {
    return 2;
  }
  if (input.G < input.B and input.G < input.R) {
    return 3;
  }
  if (input.B < input.G and input.B < input.R) {
    return 4;
  }
  return 5;
}
