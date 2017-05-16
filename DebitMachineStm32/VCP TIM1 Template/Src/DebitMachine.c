/**
 ******************************************************************************
 *  FILE          : DebitMachine.c
 *  PROJECT       : PROG 8125 - Assignment #4
 *  PROGRAMMER    : Artem Bordiuh
 *  FIRST VERSION : 2017-04-04
 *  DESCRIPTION   : Debit Machine. Getting amount to charge from debit cash 
					software in the store and send the user data to the bank
					through the same debit cash software in the store since 
					STM32 don't have Internet connection.
					Store cash software - SCS
 *	IMPROVMENTS   :	Provide in future encryption for user data sent to bank 
					and personal private key for STM32 debit machine.
 ******************************************************************************/

#include "DebitMachine.h"
#include "HD44780.h"
#include <inttypes.h>
#include <stdio.h>

/* Private variables ---------------------------------------------------------*/
// Pin numbers with buttons
static const int16_t BtnPin_OK = 0;
static const int16_t BtnPin_Cancel = 1;
static const int16_t BtnPin_Add1 = 3;
static const int16_t BtnPin_Add2 = 4;

// Global variable for holding amount of money received from store cash software
char amount[8] = { 0 };

//Store type of account to charge
//Chequing = 31 = '1'; Saving = 32 = '2'; None = 0
char accountToCarge = 0;

//Imitating debit card magnet Track1 and Track 2 numbers sent to bank
char[] cardTrack1 = "Track 1";
char[] cardTrack2 = "Track 2";

// States declaration for debit state machine
enum debitMachineState {
	welcome = 0, amountReceived, chqSav, pinCode, prooveTransaction, cancel
};

// Setting up current (initial) state for debit machine
static int8_t debitMachineCurrentState = welcome;

// Declaring button names
enum debitMachineButtons {
	btn_none = 0, btn_ok, btn_cancel, btn_add1, btn_add2
};

/**
 * @brief Initialize the debit machine including lcd, sound driver and buttons
 * @param None
 * @retval None
 */
void initDebitMachine() {
	initSoundDriver();
	HD44780_Init();
	deBounceInit(BtnPin_OK, 'A', 1);
	deBounceInit(BtnPin_Cancel, 'A', 1);
	deBounceInit(BtnPin_Add1, 'A', 1);
	deBounceInit(BtnPin_Add2, 'A', 1);
}

/**
 * @brief Loops until any of the declared buttons will be pressed by user. 
 * Used debounce routine
 * @param None
 * @retval debitMachineButtons: Number of the button pressed
 */
uint8_t waitAndGetBtnPressed() {
	uint8_t btnPressed = btn_none;
	while (btnPressed == btn_none) {
		if (deBounceReadPin(BtnPin_OK, 'A', 50) == 0)
			btnPressed = btn_ok;
		else if (deBounceReadPin(BtnPin_Cancel, 'A', 50) == 0)
			btnPressed = btn_cancel;
		else if (deBounceReadPin(BtnPin_Add1, 'A', 50) == 0)
			btnPressed = btn_add1;
		else if (deBounceReadPin(BtnPin_Add2, 'A', 50) == 0)
			btnPressed = btn_add2;
	}
	return btnPressed;
}

/**
 * @brief Will show "Ok" and "CANCEL" at the left and right bottom sides of LCD
 * @param None
 * @retval None
 */
void showOkCancelTextOnLcd() {
	HD44780_GotoXY(0, 1);
	HD44780_PutStr("OK");
	HD44780_GotoXY(10, 1);
	HD44780_PutStr("CANCEL");
}

/**
 * @brief Welcome state:
 * Shows welcome text and wait for request from store cash software
 * @param None
 * @retval None
 */
void welcomeState() {
	HD44780_ClrScr();
	HD44780_PutStr("Welcome!");
	char debitRequest[32] = { '\0' };
	int8_t result = scanf("%s", debitRequest);
	if (result <= 0)	//then somehow non-float chars were entered
			{						//and nothing was assigned to %f
		fpurge(stdin); 	//clear the last erroneous char(s) from the input stream
	} else {
		if (strstr(debitRequest, "RQ:") != NULL) {
			strncpy(amount, debitRequest + 3, 6);
			amount[8] = '\0';
			debitMachineCurrentState++;
		} else {						//and nothing was assigned to %f
			fpurge(stdin); //clear the last erroneous char(s) from the input stream
		}
	}
		HAL_Delay(1000);
}

/**
 * @brief Amount state:
 * Shows amount of money to charge sent by store cash software (SCS)
 * @param None
 * @retval None
 */
void amountState() {

	HD44780_ClrScr();
	char stringBuffer[15];
	strcpy(stringBuffer, "Amount: $");
	strcat(stringBuffer, amount);
	HD44780_PutStr(stringBuffer);

	showOkCancelTextOnLcd();
	if (waitAndGetBtnPressed() == btn_ok)
		debitMachineCurrentState++;
	else
		debitMachineCurrentState = cancel;
}

/**
 * @brief Chequing or Saving account state:
 * Gives the user choice of chosing which type of account to charge
 * @param None
 * @retval None
 */
void chqSavState() {

	HD44780_ClrScr();
	HD44780_PutStr("Choose account:");
	HD44780_GotoXY(0, 1);
	HD44780_PutStr("Chq         Sav");

	if (waitAndGetBtnPressed() == btn_ok) {
		HD44780_ClrScr();
		HD44780_PutStr("Continue chq?");
		accountToCarge = 31;
	} else {
		HD44780_ClrScr();
		HD44780_PutStr("Continue sav?");
		accountToCarge = 32;
	}

	showOkCancelTextOnLcd();
	if (waitAndGetBtnPressed() == btn_ok)
		debitMachineCurrentState++;
	else
		debitMachineCurrentState = cancel;
}

/**
 * @brief Shows editable digit on the screen for password editing surround by []
 * @param const char*: Pointer to editable digit
 * @param uint8_t: Position
 * @retval None
 */
void pwdDigitChoseAndDisplay(const char* digit, uint8_t pos) {
	HD44780_ClrScr();
	HD44780_PutStr("Enter pwd:");
	HD44780_GotoXY(10, 0);
	for (int i = 0; i < 4; i++) {
		if (i == pos) {
			HD44780_PutChar('[');
			HD44780_PutChar(*digit);
			HD44780_PutChar(']');
		}
		HD44780_PutChar('*');
	}
	showOkCancelTextOnLcd();
}

/**
 * @brief Function for PIN code edditing by using buttons
 * @param char*: Pointer to char array for returning the final Pin code
 * @retval None
 */
void pinCodeEdit(char* pwdOut) {
	uint8_t btnPressed = btn_none;
	char password[4] = { '0', '0', '0', '0' };
	uint8_t cursorPosition = 0;
	while ((btnPressed != btn_ok) && (btnPressed != btn_cancel)) {

		if (btnPressed == btn_add2) {
			if (cursorPosition < 3)
				cursorPosition++;
			else
				cursorPosition = 0;
		}

		if (btnPressed == btn_add1) {
			if (cursorPosition <= 3)
				if (password[cursorPosition] < 57) // '0'
					password[cursorPosition]++;
				else
					password[cursorPosition] = 48; // '9'
			else
				cursorPosition = 0;
		}

		switch (cursorPosition) {
		case 0:
			pwdDigitChoseAndDisplay(&password[cursorPosition], cursorPosition);
			break;
		case 1:
			pwdDigitChoseAndDisplay(&password[cursorPosition], cursorPosition);
			break;
		case 2:
			pwdDigitChoseAndDisplay(&password[cursorPosition], cursorPosition);
			break;
		case 3:
			pwdDigitChoseAndDisplay(&password[cursorPosition], cursorPosition);
			break;
		default:
			break;
		}
		btnPressed = waitAndGetBtnPressed();
	}
	if (btnPressed == btn_ok) {
		debitMachineCurrentState++;
		strcpy(pwdOut, password);
	} else
		debitMachineCurrentState = cancel;

}

/**
 * @brief Pin code state:
 * Shows screen for entering pin code and send all the USER DATA(Track1 Track2) to BANK 
 * server through the SCS (store software cash) (since no internet on stm32)
 * @param None
 * @retval None
 */
void pinCodeState() {
	HD44780_ClrScr();
	HD44780_PutStr("Enter pwd:");
	char pwdToSend[5] = { 0 };
	pinCodeEdit(pwdToSend);
	pwdToSend[5] = '\0';
	//User data should be encrypted in future
	char *data;
	strcpy(data, cardTrack1);
	strcpy(data, ',');
	strcat(data, cardTrack2);
	strcpy(data, ',');
	strcpy(data, pwdToSend);
	strcpy(data, ',');
	strcpy(data, amount);
	strcpy(data, ',');
	strcpy(data, accountToCarge);
	strcpy(data, '\0');
	printf("Info:%s\r\n", data);
}

/**
 * @brief Prooving state
 * Waits for bank proove the transaction
 * @param None
 * @retval None
 */
void prooveTransactionState() {
	HD44780_ClrScr();
	HD44780_PutStr("Approving...");
	char debitRequest[10] = { 0 };
	int8_t result = scanf("%s", debitRequest);
	if (result == 0) {					//and nothing was assigned to %f
		fpurge(stdin); 	//clear the last erroneous char(s) from the input stream
	} else if (strstr(debitRequest, "Ok") != NULL) {
		playNote(400,500);
		playNote(800,500);
		HD44780_ClrScr();
		HD44780_PutStr("Approved");
		HAL_Delay(1000);
		debitMachineCurrentState = welcome;
	} else
		debitMachineCurrentState = cancel;
}

/**
 * @brief Cancel state:
 * Each state will turn to this one in case of failed transaction.
 * This state will turn to the welcome state
 * Prints "Transaction canceled!"
 * @param None
 * @retval None
 */
void cancelState() {
	printf("Cncld");
	playNote(800,500);
	playNote(400,500);
	HD44780_ClrScr();
	HD44780_PutStr("Transaction");
	HD44780_GotoXY(0, 1);
	HD44780_PutStr("canceled!");
	HAL_Delay(1000);
	welcomeState();
}

/**
 * @brief State machine switch for declared and defined states above
 * Called by infinit loop in the main function of the project
 * **Initialisation of the debit machine should be called first!
 * @param None
 * @retval None
 */
void debitMachineHandleStates() {
	switch (debitMachineCurrentState) {
	case welcome:
		welcomeState();
		break;
	case amountReceived:
		amountState();
		break;
	case chqSav:
		chqSavState();
		break;
	case pinCode:
		pinCodeState();
		break;
	case prooveTransaction:
		prooveTransactionState();
		break;
	case cancel:
		cancelState();
		break;
	default:
		break;
	}
}
