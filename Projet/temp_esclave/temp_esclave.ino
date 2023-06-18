
int RESISTANCE= 5;
int PELTIER = 7;
int diff = 0;

void chauffe() {//fonction qui permet d'augmenter la temperature de la boite en activant la resistance et en desactivant le peltier
  digitalWrite(RESISTANCE,HIGH);
  digitalWrite(PELTIER,LOW);
}

void refroid(){//fonction qui permet de diminuer la temperature de la boite en activant le peltier et en desactivant la resistance
  digitalWrite(PELTIER,HIGH);
  digitalWrite(RESISTANCE,LOW);
}

void arrettemp(){//fonction qui stop tout de la temperature
  digitalWrite(PELTIER,LOW);
  digitalWrite(RESISTANCE,LOW);
}

void setup() {
  Serial.begin(9600);  // Démarrer la communication série
  pinMode(RESISTANCE, OUTPUT);
  pinMode(PELTIER, OUTPUT);
}

void loop() {
    if (Serial.available() > 0) {      //verifi qu'une donnee a bien ete envoyer
      int diff = Serial.parseInt();    // Lit les données d'humidité envoyées par l'Arduino esclave
    }

    if (diff > 0) {                    //active le peltier en "mode froid" pour refroidire la boite quand la temperature voulu est inferieur à la temperature reel
      refroid();
    }

    else if (diff < 0) {               //active la resistance chauffer la boite quand la température voulu est superieur à la temperature reel
      chauffe();
    }

    else{                             //si la temperature est bonne tout s'arrete
      arrettemp();
    }
}
