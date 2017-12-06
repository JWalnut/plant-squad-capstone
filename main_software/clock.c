#import clock.h

void ConfigureTimerA(void)
{
	/* Preconfig */
	TA0CTL = (MC_0 | TACLR); //Stop timer, Clear timer

	/* Configure Timer A */
	//Below code taken from Project 3
	TA0CTL = (ID_3 | TASSEL_2 | TAIE); //Set to divide counter by 8, Set source to SMCLK, Enable Interrupts
	TA0CTL &= ~TAIFG; //Reset Timer A flag

	/* Start Timer */
	TA0CTL |= MC_1; //Set to count UP (i.e., start timer a)
}

#pragma vector = TIMER0_A0_VECTOR
// Interrupt service routine for CCIFG0
__interrupt void Timer0_A0_routine(void) {
  if (counter >= 50000) {
    OSTimer();
    counter = 0;
  } else {
    counter++;
  }
}
