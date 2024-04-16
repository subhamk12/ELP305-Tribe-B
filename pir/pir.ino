//esp - pir
//vin - 5v
//d4 - out
//gnd - gnd

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(4, INPUT_PULLUP);
}

void loop() {
  // put your main code here, to run repeatedly:
  int a = digitalRead(4);
  Serial.println(a);
  if(a==HIGH){
    Serial.println("motion");
  }
  if(a== LOW){
    Serial.println("no motion");
  }
  delay(10); // this speeds up the simulation
}
