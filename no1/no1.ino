
const int buttonPIN = 36;
const int ledPIN = 42;

int buttonState = 0;
void setup() {
  // put your setup code here, to run once:
  pinMode(ledPIN, OUTPUT);
  pinMode(buttonPIN, INPUT_PULLUP);
}

void loop() {
  // put your main code here, to run repeatedly:
  buttonState = digitalRead(buttonPIN);

  if(buttonState==LOW)
  {
    
    digitalWrite(ledPIN, HIGH);
    
  }
  else
  {
    digitalWrite(ledPIN, LOW);
    
  }
}
