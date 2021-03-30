/*  OPDRACHT     : Eindopdracht EDRSD studiejaar 2020-2021
 *  OPDRACHTGEVER: Bart Snijder
 *  AUTEUR       : Mark Huizenga
 *  STUDENTNUMMER: 1155088
 *  GEWIJZIGD    : 30 maart 2021
 *  Variant C.
 */
 
#include "mbed.h"

enum STATES{STOP, VOORUIT, ACHTERUIT, PWM};                                     // alle mogelijke toestanden benoemen
enum EVENTS {NO_EVENT, START_STOP_PUSHED, MODE_PUSHED};                         // alle mogelijke gebeurtenisen benoemen

STATES state;                                                                   // variabele definieren voor de toestanden
EVENTS event;                                                                   // variabele definieren voor de gebeurtenissen

AnalogIn potDutyCycle(PA_1);                                                    // potmeter
AnalogIn cny70(PA_3);                                                           // CNY70 sensor
InterruptIn startStop(PB_6, PullDown);                                          // stop/stop button (geel)
InterruptIn mode(PB_7, PullDown);                                               // mode button (blauw)

PwmOut motor(PB_4);                                                             // Pulse Width Modulated output naar ENABLE input van L293D
DigitalOut TPA_5(PF_0);                                                         // digitale output naar 1A input van L293D
DigitalOut TPA_6(PF_1);                                                         // digitale output naar 2A input van L293D

Timer timer;                                                                    // variabele definieren voor de timer

Serial pc(USBTX, USBRX);                                                        // variable definieren voor seriele communicatie

int dutyCycle;                                                                  // variable definieren voor het opslaan van de waarde van de potmeter
int newDutyCycle;                                                               // variable definieren voor het opslaan van een nieuwe waarde van de potmeter om later te kunnen vergelijken met de oude waarde

void setClockwise(){                                                            // functie die de digitale inputs van de L293D initieerd zodat de motor kloksgewijs gaat draaien
    TPA_5 = 0;
    TPA_6 = 1;
}

void setCounterClockwise(){                                                     // functie die de digitale inputs van de L293D initieerd zodat de motor tegen de klok in gaat draaien
    TPA_5 = 1;
    TPA_6 = 0;
}

void setStop(){                                                                 // functie die de digitale inputs van de L293D initieerd zodat de motor niet draaid
    TPA_5 = 0;
    TPA_6 = 0;
}

void stateMachine(EVENTS inputEvent){                                           // functie die de bijbehorende code van de huidige toestand uitvoert
    switch(state){
        case VOORUIT:                                                           // code voor de toestant VOORUIT
            setClockwise();                                                         // roept functie aan die de outputs goed zet om de motor vooruit te laten bewegen
            timer.stop();                                                           // stopt en reset de timer, zodat deze de volgende keer weer gestart kan worden als de toestand veranderd naar ACHTERUIT
            timer.reset();
            if (inputEvent == START_STOP_PUSHED){                                   // functie die checkt of de start/stop button ingedrukt is/is geweest
                state = STOP;                                                           // update de toestand
                event = NO_EVENT;                                                       // reset de gebeurtenis
                pc.printf("De motor staat nu stil.\n");                                 // print de huidige toestand van de motor naar de seriele monitor
            } else if (inputEvent == MODE_PUSHED){                                  // functie die checkt of de mode button ingedrukt is/is geweest
                state = PWM;                                                            // update de toestand
                event = NO_EVENT;                                                       // reset de gebeurtenis
                pc.printf("PWM instellingen geopend.\n");                               // print de huidige toestand van de motor naar de seriele monitor
                pc.printf("PWM duty cycle = %d%%\n", dutyCycle);                        // print de waarde van de potmeter die is opgeslagen in dutyCycle
            } else if (cny70.read() < 0.2f){                                        // functie die checkt of de CNY70 een witte lijn detecteerd
                setStop();                                                              // roept functie aan die de outpust goed zet om de stop te zetten
                wait(0.5);                                                              // wacht 0.5 seconden, zodat de as van de motor tot stilstand kan komen
                state = ACHTERUIT;                                                      // update de toestand
                pc.printf("De motor draait nu tegen de klok in.\n");                    // print de huidige toestand van de motor naar de seriele monitor
            } else {
                state = VOORUIT;                                                
            }
        break;
        case ACHTERUIT:                                                         // code voor de toestant ACHTERUIT
            setCounterClockwise();                                                  // roept functie aan die de outputs goed zet om de motor achteruit te laten bewegen
            timer.start();
            if (inputEvent == START_STOP_PUSHED){                                   // functie die checkt of de start/stop button ingedrukt is/is geweest
                state = STOP;                                                           // update de toestand
                event = NO_EVENT;                                                       // reset de gebeurtenis
                pc.printf("De motor staat nu stil.\n");                                 // print de huidige toestand van de motor naar de seriele monitor
            } else if (inputEvent == MODE_PUSHED){                                  // functie die checkt of de mode button ingedrukt is/is geweest
                state = PWM;                                                            // update de toestand
                event = NO_EVENT;                                                       // reset de gebeurtenis
                pc.printf("PWM instellingen geopend.\n");                               // print de huidige toestand van de motor naar de seriele monitor 
                pc.printf("PWM duty cycle = %d%%\n", dutyCycle);                        // print de waarde van de potmeter die is opgeslagen in dutyCycle
            } else if (timer > 4){                                                  // functie die checkt of motor al 4 seconden achteruit geeft gedraaid
                setStop();                                                              // roept functie aan die de outpust goed zet om de stop te zetten
                wait(0.5);                                                              // wacht 0.5 seconden, zodat de as van de motor tot stilstand kan komen
                state = VOORUIT;                                                        // update de toestand
                pc.printf("De motor draait nu kloksgewijs.\n");                         // print de huidige toestand van de motor naar de seriele monitor
            } else {
                state = ACHTERUIT;                                              
            }
        break;
        case STOP:                                                              // code voor de toestant STOP
            setStop();                                                              // roept functie aan die de outpust goed zet om de stop te zetten
            timer.stop();                                                           // stopt en reset de timer, zodat deze de volgende keer weer gestart kan worden als de toestand veranderd naar ACHTERUIT
            timer.reset();
            if (inputEvent == START_STOP_PUSHED){                                   // functie die checkt of de start/stop button ingedrukt is/is geweest
                state = VOORUIT;                                                        // update de toestand
                event = NO_EVENT;                                                       // reset de gebeurtenis
                pc.printf("De motor draait nu kloksgewijs.\n");                         // print de huidige toestand van de motor naar de seriele monitor
            } else if (inputEvent == MODE_PUSHED){                                  // functie die checkt of de mode button ingedrukt is/is geweest
                state = PWM;                                                            // update de toestand
                event = NO_EVENT;                                                       // reset de gebeurtenis
                pc.printf("PWM instellingen geopend.\n");                               // print de huidige toestand van de motor naar de seriele monitor
                pc.printf("PWM duty cycle = %d%%\n", dutyCycle);                        // print de waarde van de potmeter die is opgeslagen in dutyCycle
            } else {
                state = STOP;                                                   
            }
        break;
        case PWM:                                                               // code voor de toestant PWM
            setStop();                                                              // roept functie aan die de outpust goed zet om de stop te zetten
            timer.stop();                                                           // stopt en reset de timer, zodat deze de volgende keer weer gestart kan worden als de toestand veranderd naar ACHTERUIT
            timer.reset();
            if (inputEvent == START_STOP_PUSHED){                                   // functie die checkt of de start/stop button ingedrukt is/is geweest
                state = STOP;                                                           // update de toestand
                event = NO_EVENT;                                                       // reset de gebeurtenis
                pc.printf("PWM instellingen gesloten.\n");                              // print de huidige toestand van de motor naar de seriele monitor
                pc.printf("De motor staat nu stil.\n");                                 // print de huidige toestand van de motor naar de seriele monitor
            } else if (inputEvent == MODE_PUSHED){                                  // functie die checkt of de mode button ingedrukt is/is geweest                            
                state = VOORUIT;                                                        // update de toestand
                event = NO_EVENT;                                                       // reset de gebeurtenis
                motor.write(potDutyCycle.read());                                       // zet het PWM signaal naar de huidige waarde van de potmeter
                pc.printf("PWM instellingen opgeslagen en gesloten.\n");                              // print de huidige toestand van de motor naar de seriele monitor
                pc.printf("De motor draait nu kloksgewijs.\n");                         // print de huidige toestand van de motor naar de seriele monitor
            } else {
                newDutyCycle = potDutyCycle.read()*100;                             // slaat de nieuwe waarde van de potmeter op in newDutyCycle
                if (newDutyCycle != dutyCycle){                                     // functie die wordt aangeroepen als de nieuwe waarde van de potmeter verschilt van de oude waarde van de potmeter
                    dutyCycle = newDutyCycle;                                           // slaat de nieuwe waarde van de potmeter op in dutyCycle
                    pc.printf("PWM duty cycle = %d%%\n", dutyCycle);                    // print de waarde van de potmeter die is opgeslagen in dutyCycle
                }   
                state = PWM;                                                    
            }
        break;
    }
}

void startStopButtonPushed(){                                                   // functie die het event veranderd naar start/stop button ingedrukt
    event = START_STOP_PUSHED;
}

void modeButtonPushed(){                                                        // funcite die het event veranderd naar mode button ingedrukt
    event = MODE_PUSHED;
}

int main() {                                                                    // hoofdfunctie, vanuit hier worden alles onderdelen aangeroepen
    motor.period(0.05);                                                             // zet de frequentie van de moter op 20 Hz
    motor.write(potDutyCycle.read());                                               // zet het PWM signaal naar de huidige waarde van de potmeter
    
    dutyCycle = potDutyCycle.read()*100;                                            // slaat de huidige waarde van de potmeter op in dutyCycle
    
    startStop.rise(startStopButtonPushed);                                          // wanneer de start/stop button wordt ingedrukt, wordt de startStopButtonPushed-functie aangeroepen
    mode.rise(modeButtonPushed);                                                    // wanneer de mode button wordt ingedrukt, wordt de modeButtonPushed-functie aangeroepen

    pc.printf("De motor staat nu stil.\n");                                         // print de huidige toestand van de motor naar de seriele monitor
    
    while (true) {                                                                  // loop functie die constant wordt aangeroepen
        stateMachine(event);                                                            // roept de funcitie stateMachine aan en geeft de huidige gebeurtenis event mee
        wait_ms(10);                                                                    // wacht 10 miliseconden, zodat analoge waarden tussendoor kunnen stabiliseren, anders worden bijvoorbeeld de PWM veranderingen te vaak geprint als die zich op een kantelpunt van twee waarden begeven
    }
}
