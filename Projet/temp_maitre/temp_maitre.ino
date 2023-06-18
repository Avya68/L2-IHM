
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Wire.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_STMPE610.h>
#include <ThreeWire.h>      // permet de simplifier l'integration des ports pour l'horloge
#include <RtcDS1302.h>      // permet l'utilisation de l'horloge ds1302
#include <SD.h>             //permet l'utilisation de la carte SD
#include <DHT.h>

int TEMPERATURE = 0;  //initialisation temperature voulu a 0
int HUMIDITE = 0;     //initialisation humidite voulu a 0
int BRUMISATEUR_PIN = A5;   //pin brumisateur
int diff = 0;
unsigned long derniereLecturehumidite = 0; // Variable pour stocker le moment de la dernière lecture
unsigned long derniereLectureacquisition = 0;
const unsigned long delaiLecture = 2000; // Délai de 2 secondes entre les lectures
const unsigned long delaiLectureacqui = 1000; // Délai de 1 secondes pour l'aqcuisition
float Humidite_capteur = 0.0;

bool danstemp = false;
bool danshumi = false;
bool dansonoff = false;
bool acquisition = false;

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 150
#define TS_MINY 130
#define TS_MA80 3800
#define TS_MAXY 4000
  
#define STMPE_CS 8
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);
#define TFT_CS 10
#define TFT_DC 9
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

#define tempx 20
#define tempy 20
#define tempw 50
#define temph 50

#define humix tempx
#define humiy tempy + 80
#define humiw tempw
#define humih temph

#define onoffx humix
#define onoffy humiy + 80
#define onoffw humiw
#define onoffh humih

#define BUTTON_RADIUS 35
#define BUTTON_COLOR ILI9341_RED
#define BUTTONPLUS_X 60
#define BUTTONMOINS_X 250
#define BUTTON_Y 150

int RESISTANCE= 5;  //res com3       //pin resistance
int PELTIER= 7; //pel com4           // pin peltier
const int VENTILATEUR_HUMIDITE = 40; //pin ventilateur humi
const int VENTILATEUR_FILTRE = 49;   // pin ventilateur filtre
const int consigne = 0;

#define brocheBranchementDHT11 50
#define typeDeDHT11 DHT11
DHT dht11(brocheBranchementDHT11, typeDeDHT11);

ThreeWire myWire(4,5,2); // DAT, SCLK, RST      //defini les ports utilisé pour l'horloge
RtcDS1302<ThreeWire> Rtc(myWire);               // initialise l'horloge
File monFichier;                                // création d'un fichier pour la carte SD
unsigned long t0 = 0;                           // variable pour le timer
const unsigned long periode = 2000;             // période d'enregistrement (en ms)
const unsigned long etalonnage = 200*periode;   // durée de l'étalonnage

struct Point //cree class point pour faciliter la recuperation des donner de la position du doigt sur l'ecran
{
  int x;
  int y;
};

void enregistre(String fichier,String date, int tempv, float tempr, int humv, float humr) {   //création d'une fonction pour l'enristrement sur la carte SD
  Serial.begin(9600);

  monFichier = SD.open(fichier, FILE_WRITE);    //ouvre un fichier sur la carte SD au nom donnée dans le parametre (String fichier) en écriture

  if (monFichier){              // vérifie l'existance du fichier
    monFichier.print(date);     // inscit le parametre (date) dans la carte sd
    monFichier.print(";");      // sépare les données par ; permettant lz changement de colonne sur excel
    monFichier.print(tempv);    // inscrit le parametre dans la carte SD
    monFichier.print(";");    
    monFichier.print(tempr);  
    monFichier.print(";");
    monFichier.print(humv);
    monFichier.print(";");
    monFichier.println(humr);   // inscrit la valeur humr puis saute une ligne pour commencer le prochain enregistrement
    monFichier.close();         // ferme le fichier
  }
}

String printDateTime(const RtcDateTime& dt){    // creation d'une fonction pour récupérer la date et l'heure
        char datestring[20];        //défini la manière d'afficher la date et l'heure
        snprintf_P(datestring,countof(datestring),PSTR("%02u/%02u/%04u %02u:%02u:%02u"),dt.Day(),dt.Month(),dt.Year(),dt.Hour(),dt.Minute(),dt.Second() ); //recupere les données
        Serial.println(datestring);     // affiche dans le terminal
        return datestring;        //retourne la date et l'heure
}

void Ecriture_Accueil()
{
  tft.fillScreen(ILI9341_BLACK);  //initialise l'écran noir

  Creation_des_boutons_acceuil(); //appel la fonction pour créée les boutons de l'acceuil

  tft.setCursor(80,  tempy + (temph/2));// place le texte
  tft.setTextColor(ILI9341_WHITE);      // mets l'écriture blanc
  tft.setTextSize(2);                   // initalise la taille
  tft.println("Temperature");           // écrit "Temperature"

  tft.setCursor(80, humiy + (humih/2));
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.println("% Humidite");

  tft.setCursor(60, onoffy + (onoffh/2));
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.println("Depart experience");
}

void Creation_des_boutons_acceuil()
{
  tft.drawRect(tempx, tempy, tempw, temph, ILI9341_RED);                //cree le carrer pour le bouton température
  tft.drawRect(humix, humiy, humiw, humih, ILI9341_RED);                //cree le carrer pour le bouton humidite
  tft.drawRect(onoffx + 20, onoffy, humiw+190, humih + 5, ILI9341_RED); //cree le carrer pour le bouton demarrage
}

void fenetre(const char* texte)
{
  //mettre l'ecran noir
  tft.fillScreen(ILI9341_BLACK);
  
  //cree le carre et le retangle
  tft.drawRect(tempx, tempy, tempw, temph, ILI9341_RED);
  tft.drawRect(tempx + 60, tempy, tempw + 180, temph,ILI9341_RED);
  
  //cree la croix
  tft.drawLine(25, 25, 65, 65, ILI9341_WHITE);
  tft.drawLine(25, 65, 65, 25, ILI9341_WHITE);
  
  //ecrit le texte
  tft.setCursor(90 , 35);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(3);
  tft.println(texte);
}

Point pointage() {
  Point p;
  TS_Point point = ts.getPoint();   //verifie la position du doigt
  point.x = map(point.x, TS_MINY, TS_MAXY, 0, tft.height());
  point.y = map(point.y, TS_MINX, TS_MA80, 0, tft.width());
  p.y = tft.height() - point.x;
  p.x = point.y;
  return p;
}

void drawButton_plus_moins() {
  tft.drawCircle(BUTTONPLUS_X, BUTTON_Y, BUTTON_RADIUS, BUTTON_COLOR);  //cree le bouton rond du bouton plus
  tft.drawCircle(BUTTONMOINS_X, BUTTON_Y, BUTTON_RADIUS, BUTTON_COLOR); //cree le bouton rond du bouton moins

  tft.setCursor(235, 130);          //
  tft.setTextColor(ILI9341_WHITE);  //cree le +
  tft.setTextSize(6);               //
  tft.println("+");                 //

  tft.setCursor(45, 130);           //
  tft.setTextColor(ILI9341_WHITE);  //cree le -
  tft.setTextSize(6);               //
  tft.println("-");                 //
}

bool Button_moins_pressed(Point touchPoint) { //verifie si le bouton - est appuyer
  int distance = sqrt(pow(touchPoint.x - BUTTONPLUS_X, 2) + pow(touchPoint.y - BUTTON_Y, 2));
  return distance <= BUTTON_RADIUS;
}

bool Button_plus_pressed(Point touchPoint) {  //verifie si le bouton + est appuyer
  int distance = sqrt(pow(touchPoint.x - BUTTONMOINS_X, 2) + pow(touchPoint.y - BUTTON_Y, 2));
  return distance <= BUTTON_RADIUS;
}

void temperature(){   //fonction concernant la fenetre temperature
  fenetre("Temperature"); //cree la fenetre avec ecrit "Temperature"

  drawButton_plus_moins(); //cree le bouton + et -

  tft.setCursor(150, BUTTON_Y-20);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(4);
  tft.println(TEMPERATURE);

  tft.setCursor(115, 220);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.println("En degree Celsius");

  while (danstemp == true){ //permet de rester dans la fenetre temperature
      delay(500);           //attend le temps que l'utilisateur retire son doigt pour pas que ça appuie sur la croix par erreur
      Point myPoint = pointage();
      int x = myPoint.x;    //recupère la position du doigt
      int y = myPoint.y;    //

      tft.setCursor(150, BUTTON_Y-20);
      tft.setTextColor(ILI9341_WHITE);
      tft.setTextSize(4);

      if (Button_moins_pressed(myPoint)){                       //diminue la temperature voulu de 1 si le bouton - est appuyer
        tft.fillRect(150, BUTTON_Y-30, 51, 40, ILI9341_BLACK);  //supprime l'ancienne temperature
        TEMPERATURE -= 1;
        tft.println(TEMPERATURE);                                //initialise la nouvelle valeur de la temperature voulu
      }

      if (Button_plus_pressed(myPoint)){                         //augmente la temperature voulu de 1 si le bouton + est appuyer
        tft.fillRect(150, BUTTON_Y-30, 51, 40, ILI9341_BLACK);
        TEMPERATURE += 1;
        tft.println(TEMPERATURE);
      }

      if((x > tempx) && (x < (tempx + tempw))) {                //  
        if ((y > tempy) && (y <= (tempy + temph))) {            // vérifie quand la X est appuyer

          danstemp=false;                                       //permet de sortir de la fenetre temperature

          delay(500);
      }
    }
  }
}

void humidite(){      //la meme chose que temperature sauf pour l'humidite
  fenetre("Humidite");

  drawButton_plus_moins();

  tft.setCursor(150, BUTTON_Y-20);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(4);
  tft.println(HUMIDITE);

  tft.setCursor(150, 220);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.println("En pourcentage");

  while (danshumi == true){
      delay(500);

      Point myPoint = pointage();
      int x = myPoint.x;
      int y = myPoint.y;
     
      tft.setCursor(150, BUTTON_Y-20);
      tft.setTextColor(ILI9341_WHITE);
      tft.setTextSize(4);

      if (Button_moins_pressed(myPoint)){
        tft.fillRect(150, BUTTON_Y-30, 51, 40, ILI9341_BLACK);
        HUMIDITE -= 1;
        tft.println(HUMIDITE);
      }

      if (Button_plus_pressed(myPoint)){
        tft.fillRect(150, BUTTON_Y-30, 51, 40, ILI9341_BLACK);
        HUMIDITE += 1;
        tft.println(HUMIDITE);
      } 

      if((x > tempx) && (x < (tempx + tempw))) {
        if ((y > tempy) && (y <= (tempy + temph))) {   

          danshumi=false;

          delay(500);
      }
    }
  }
}

void OnOff(){
  fenetre("Demarrage...");
  tft.drawRect(15, 200, 285, 35,ILI9341_RED); //cree le bouton pour l'acquisition

  tft.setCursor(175, 85);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.println("Reel         Voulu");

  tft.setCursor(20, BUTTON_Y-40);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.println("Temperature");

  tft.setCursor(20, BUTTON_Y+20);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.println("Humidite");

  tft.setCursor(260, BUTTON_Y-40);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.println(TEMPERATURE);

  tft.setCursor(260, BUTTON_Y+20);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.println(HUMIDITE);


  tft.setCursor(20, 210);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.println("Lancer l'Acquisition");

  while (dansonoff == true)
  {
    delay(500);

    Point myPoint = pointage();
    int x = myPoint.x;
    int y = myPoint.y;
    unsigned long tempsActuel = millis(); // Obtenir le temps actuel

    if (tempsActuel - derniereLecturehumidite >= delaiLecture) {
      // Attendre 2 secondes écoulées depuis la dernière lecture
      derniereLecturehumidite = tempsActuel; // Mettre à jour le moment de la dernière lecture

      float Humidite_capteur = dht11.readHumidity(); // Lire l'humidité relative du capteur
      Serial.print(Humidite_capteur); // Envoyer la valeur d'humidité à l'Arduino maître
    }

    float Temperature_capteur=temp();                              //on récupère la température grâce au capteur de temperature

    RtcDateTime now = Rtc.GetDateTime();                           //récupère l'heure (now)

    tft.fillRect(180, BUTTON_Y-50, 35, BUTTON_Y-50, ILI9341_BLACK);//supprimer les valeur précédente grâce à un écran noir
    
    tft.setCursor(180, BUTTON_Y-40);  //
    tft.setTextColor(ILI9341_WHITE);  //
    tft.setTextSize(2);               //  affiche la temperature obtenu grâce au capteur
    tft.println(Temperature_capteur); //

    tft.setCursor(180, BUTTON_Y+20);  //
    tft.setTextColor(ILI9341_WHITE);  //
    tft.setTextSize(2);               //  affiche l'humidite obtenu grâce au capteur
    tft.println(Humidite_capteur);    //

    if (Humidite_capteur > HUMIDITE) {            //active le ventilateur pour amener l'air vers le filtre pour diminuer l'humidite si l'humidite voulu est inférieur a l'humidite reel
      filtre();
    }

    else if (Humidite_capteur < HUMIDITE) {       //active le ventilateur pour amener l'humidite dans la boite d'experience si l'humidite voulu est superieur a l'humidite reel
      humidifie();
    }

    else{                                         //si l'humidite est bonne tout s'arrete
      arrethumi();
    }

    if (Temperature_capteur > TEMPERATURE) {      //active le peltier en "mode froid" pour refroidire la boite quand la temperature voulu est inferieur à la temperature reel
      int diff = 1;
    }

    else if (Temperature_capteur < TEMPERATURE) { //active la resistance chauffer la boite quand la température voulu est superieur à la temperature reel
      int diff = -1;
    }

    else{                                         //si la temperature est bonne tout s'arrete
      int diff = 0;
    }

    Serial.print(diff);                           //envoie ce que doit faire la resistance et/ou le peltier a l'esclave

    if (acquisition == true){                       //si acquisition activer grace au bouton
      if (tempsActuel - derniereLectureacquisition >= delaiLectureacqui) {
        //attendre 1 sec entre chaque acquisition
        derniereLectureacquisition = tempsActuel;     // Mettre à jour le moment de la dernière lecture
        #define countof(a) (sizeof(a) / sizeof(a[0])) //defini le nombre de place que occupe l'horodatage
        enregistre("DONNEES.csv",printDateTime(now), TEMPERATURE,Temperature_capteur,HUMIDITE,Humidite_capteur); //enregistre les données dans la carte SD
        printDateTime(now);                           // affiche l'horodatage dans le terminal
      }
    }

    if((x > tempx) && (x < (tempx + tempw))) {     //verifie si on appuie sur la croix
      if ((y > tempy) && (y <= (tempy + temph))){  //

        dansonoff = false;                         //permet de sortir de la fenetre
        arrethumi();                               //stop tout pour l'humidite (au cas ou quelque chose etait en cours)
        delay(500);
      }
    }

    if((x > 15) && (x < 285)) {     //verifie si on appuie sur le bouton pour lancer l'aquisition
      if ((y > 200) && (y <= 235)){ // 

        if (acquisition == false){  //verifie si l'acquisition est fausse avant de la passer en vraie pour savoir si on veux lancer ou arreter l'acquisition

          acquisition = true;       //active l'acquisiton

          tft.fillRect(16, 201, 283, 33, ILI9341_BLACK);//supprime "lancer l'aqcuisition"

          tft.setCursor(20, 210);
          tft.setTextColor(ILI9341_WHITE);
          tft.setTextSize(2);
          tft.println("Arreter l'Acquisition"); //ecrit "arreter l'acquisition" a la place de "lancer l'aqcuisition"

        }  
        else{

          acquisition = false;                  //stop l'acquisiton+

          tft.fillRect(16, 201, 283, 33, ILI9341_BLACK);//supprime "lancer l'aqcuisition"

          tft.setCursor(20, 210);
          tft.setTextColor(ILI9341_WHITE);
          tft.setTextSize(2);
          tft.println("Lancer l'Acquisition");  //ecrit "lancer l'acquisition" a la place de "arreter l'aqcuisition"
        }
      }
    }
  }
}

float temp() {
  float val = analogRead(0);
  float fenya = (val / 1023) * 5;
  float r = fenya / (5 - fenya) * 10000;
  float a = ( 1 / ( log(r / 10000) / 3950 + 1 / (25 + 273.15)) - 273.15);
  Serial.println(a); 
  return (a);
}

void humidifie() {//fonction qui permet d'augmenter l'humidite de la boite en activant le ventilateur de l'humidite et en desactivant le ventilateur du filtre
  digitalWrite(VENTILATEUR_FILTRE,LOW);
  digitalWrite(VENTILATEUR_HUMIDITE,HIGH);
}

void filtre() {//fonction qui permet de diminuer l'humidite de la boite en activant le ventilateur du filtre et en desactivant le ventilateur de l'humidite
  digitalWrite(VENTILATEUR_HUMIDITE,LOW);
  digitalWrite(VENTILATEUR_FILTRE,HIGH);
}

void arrethumi() {//fonction qui stop tout de l'humidite
  digitalWrite(VENTILATEUR_HUMIDITE,LOW);
  digitalWrite(VENTILATEUR_FILTRE,LOW);
}

void setup(void)
{
  Serial.begin(9600);

  if (!ts.begin()) { 
    Serial.println("Unable to start touchscreen.");
  } 

  else { 
    Serial.println("Touchscreen started."); 
  }

  t0 = millis();                    // Mise à zéro du timer

  Serial.print(__DATE__);           // affiche dans le terminale la date
  Serial.println(__TIME__);         //affiche dans le terminal l'heure

  Rtc.Begin();                      // démare l'horloge

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__); // permet d'affiche la date et l'heure en même temps en appelant "compiled"
  printDateTime(compiled);          // afiche et retourne la date et l'heure
  Serial.println();                 // saute une ligne

  if (!Rtc.IsDateTimeValid()){      // vérifi si l'horloge est opérationnelle
    Rtc.SetDateTime(compiled);      //si oui met l'horloge à l"heure
  }

  if (Rtc.GetIsWriteProtected()){   //vérifie si elle est protégée
    Rtc.SetIsWriteProtected(false); // si oui, la déprotège
  } 

  if (!Rtc.GetIsRunning()){         // vérifie si elle fonctionne
    Rtc.SetIsRunning(true);         // si non la fait fonctionner
  }

  RtcDateTime now = Rtc.GetDateTime();//récupère l'heure (now)

  if (now < compiled){                // si l'heure de l'horloge est différent de celui de celle compilée
    Rtc.SetDateTime(compiled);        // remet à l'heure
  }

  pinMode(VENTILATEUR_HUMIDITE, OUTPUT);//broche connectée au ventilateur qui gère l'humidité
  pinMode(VENTILATEUR_FILTRE, OUTPUT);  //broche connectée au ventilateur qui gère le filtre

  pinMode(RESISTANCE, OUTPUT);      //initilialise le pin de la resistance comme sortie
  pinMode(PELTIER, OUTPUT);         //initilialise le pin du peltier comme sortie

  tft.begin();
  tft.fillScreen(ILI9341_BLACK);        //mets l'ecran noir
  tft.setRotation(1);                   //initiliasie l'ecran en mode paysage

  Ecriture_Accueil();
}

void loop()
{
  delay(500); 

  if (!ts.bufferEmpty())
  {      
    delay(500);
    Point myPoint = pointage();
    int x = myPoint.x;
    int y = myPoint.y;
    
    if((x > tempx) && (x < (tempx + tempw))) {      //verifie si le bouton temperature est appuyer
      if ((y > tempy) && (y <= (tempy + temph))) {  //

        danstemp=true;                              //permettra de rester dans la boucle pour rester dans la fenetre temperature

        temperature();                             
        Ecriture_Accueil();
      }
    }

    if((x > humix) && (x < (humix + humiw))) {    //verifie si le bouton humiditer est appuyer
      if ((y > humiy) && (y <= (humiy + humih))) {

        danshumi=true;

        humidite();
        Ecriture_Accueil();
      }
    }

    if((x > onoffx+20) && (x < (onoffw+190))) {   //verifie si le demarrage temperature est appuyer
      if ((y > onoffy) && (y <= (onoffy + onoffh))) {

        dansonoff = true;

        OnOff();
        Ecriture_Accueil();
      }
    }
  }
}
