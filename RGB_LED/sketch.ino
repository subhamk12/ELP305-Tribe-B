int red_led = 14;
int green_led = 27;
int blue_led = 26;
int flickerDelay = 500;  // delay between intensity changes (in milliseconds)

void setup() {
  pinMode(red_led, OUTPUT);
  pinMode(green_led, OUTPUT);
  pinMode(blue_led, OUTPUT);
  // put your setup code here, to run once:

}

void loop() {
  blinking_red_color();
  blinking_green_color();
  red_color();
  green_color();
  white_color();
}
void led_color(int red_value, int green_value,int blue_value)
{
  analogWrite(red_led,red_value);
  analogWrite(green_led,green_value);
  analogWrite(blue_led,blue_value);
}

void red_color()
{
  digitalWrite(red_led, LOW);
  digitalWrite(green_led, HIGH);
  digitalWrite(blue_led, HIGH);
  delay(1000);
}

void blinking_red_color()
{
  digitalWrite(red_led, LOW);
  digitalWrite(green_led, HIGH);
  digitalWrite(blue_led, HIGH);
  delay(flickerDelay);
  digitalWrite(red_led,HIGH);
  digitalWrite(blue_led,HIGH);
  digitalWrite(green_led,HIGH);
  delay(flickerDelay);
}

void green_color()
{
  digitalWrite(red_led, HIGH);
  digitalWrite(green_led, LOW);
  digitalWrite(blue_led, HIGH);
  delay(1000);
}

void blinking_green_color()
{
  digitalWrite(red_led, HIGH);
  digitalWrite(green_led, LOW);
  digitalWrite(blue_led, HIGH);
  delay(flickerDelay);
  digitalWrite(red_led,HIGH);
  digitalWrite(blue_led,HIGH);
  digitalWrite(green_led,HIGH);
  delay(flickerDelay);
}

void white_color()
{
  digitalWrite(red_led, LOW);
  digitalWrite(green_led, LOW);
  digitalWrite(blue_led, LOW);
  delay(1000);
}
