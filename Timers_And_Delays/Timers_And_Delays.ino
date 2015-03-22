const int LEDPIN1 = 5;
const int LEDPIN2 = 4;
int timer1_counter;

void setup()
{
  pinMode(LEDPIN1, OUTPUT);
  pinMode(LEDPIN2, OUTPUT);

  // initialize timer1 
  noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  
  int hertz = 16;

  // Set timer1_counter to the correct value for our interrupt interval
  timer1_counter = 65536 - (12000000 / 256 / hertz);
  
  TCNT1 = timer1_counter;   // preload timer
  TCCR1B |= (1 << CS12);    // 256 prescaler 
  TIMSK1 |= (1 << TOIE1);   // enable timer overflow interrupt
  interrupts();             // enable all interrupts
}

ISR(TIMER1_OVF_vect)        // interrupt service routine 
{
  TCNT1 = timer1_counter;   // preload timer
  digitalWrite(LEDPIN1, digitalRead(LEDPIN1) ^ 1);
}

void loop()
{
  digitalWrite(LEDPIN2, digitalRead(LEDPIN2) ^ 1);
  delay(500);
}

