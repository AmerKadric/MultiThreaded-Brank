#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>


//these are the threads that i am going to be using
pthread_t SouthThread;
pthread_t NorthThread;
pthread_t WestThread;
pthread_t EastThread;

pthread_mutex_t account_creation_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_var = PTHREAD_COND_INITIALIZER;

struct TransactionTracker{//use this struct to put inside of my Data struct so I can store all of my tansactions
char TranHistory[100];
};


char CurrentSession[100][50];//this is for the current session transactions
int CurrentSessionCounter = 0;//use this to keep track of current session transactions
struct Data //struct that i am going to be storing the data into
{
int AccountID;
char branch[50];
char name[50];
int amount;
struct TransactionTracker TransactionHistory[100];
int NumOfTransactions;
};
struct Transaction {
    int AccountID;
    int Amount;
    char WitOrDep[50];
    char Branch[20];
    char NameOnAccount[50];
    int CurrentSeshOrNot; //1 means its the current session and 0 means were just reading the csv file
};
struct Data DataSet[100];//this is where im storing the data

int num_rows = 0;//use this to keep track of how many slots in the struct we have

void *CreateAccount (void *thread_data){//int AccountID, char branch[], char NameOnAccount[]
	struct Transaction *data = (struct Transaction *)thread_data;//this is where i convert the pointer of thread_data into a pointer of the correct data type
	
	//error handling
   	if (data == NULL) {
		fprintf(stderr, "Error: Invalid thread data in CreateAccount.\n");
		return NULL;	
   	 }
   	 
	pthread_mutex_lock(&mutex);//use this to only let 1 thread at a time
   
	//assign the values here in the struct array
	DataSet[num_rows].AccountID = data->AccountID;
	strncpy(DataSet[num_rows].branch, data->Branch, sizeof(DataSet[num_rows].branch));
	strncpy(DataSet[num_rows].name, data->NameOnAccount, sizeof(DataSet[num_rows].name));
	num_rows++;
	
	pthread_cond_signal(&cond_var); // Signal after creating an account
	pthread_mutex_unlock(&mutex);//use this to only let 1 thread at a time
   	
	return NULL;
}
//"FileItems.csv","w"
void WriteCSV(const char *filename, int NumPlacement, int ID,char Branch[],char Name[],char operation[], int amount  ){
	FILE *fp;
	fp = fopen(filename,"a");
	if (fp == NULL) {
      	  perror("Error opening the file");
       	 return;
    	}
    	fprintf(fp, "%d,%d,%s,%s,%s,%d\n", NumPlacement, ID, Branch, Name, operation, amount);
	fclose(fp); // Close the file when done writing
}
//int AccountID, int amount, char DepOrWit[]
void *DepositOrWithdrawFunction(void *thread_data) {
    struct Transaction *data = (struct Transaction *)thread_data;

    if (data == NULL) {
        fprintf(stderr, "Error: Invalid thread data in DepositOrWithdrawFunction.\n");
        return NULL;
    }
    int i = 0;//for placement in csv file
    pthread_mutex_lock(&mutex);
    while (1) {
        int found = 0;
        for (int i = 0; i < num_rows; i++) {
            if (DataSet[i].AccountID == data->AccountID) {
                found = 1;
                if (strcmp(data->WitOrDep, "deposit") == 0) {
                    	DataSet[i].amount = DataSet[i].amount + data->Amount;
                    	char concatenatedString[100];// Concatenate the AccountID, WitOrDep, and Amount into one string
			snprintf(concatenatedString, sizeof(concatenatedString), "Account: %d %s %d", data->AccountID, data->WitOrDep, data->Amount);
			strcpy(DataSet[i].TransactionHistory[DataSet[i].NumOfTransactions].TranHistory, concatenatedString);
			DataSet[i].NumOfTransactions++;
			
			if (data->CurrentSeshOrNot == 0){//this is checking the flag to see if were in the current session or not
				continue;
			}
			else{
			strcpy(CurrentSession[CurrentSessionCounter], concatenatedString);
			CurrentSessionCounter++;
			WriteCSV("FileItems.csv", i, DataSet[i].AccountID,DataSet[i].branch,DataSet[i].name,"deposit",DataSet[i].amount);
			}
			
                } else {
                	if (data->Amount > DataSet[i].amount){//this is checking the flag to see if were in the current session or not
                		printf("Insufficiant funds in your account.\n");
                	}
                	else{              	
                    		DataSet[i].amount = DataSet[i].amount - data->Amount;
                    		char concatenatedString[100];// Concatenate the AccountID, WitOrDep, and Amount into one string
				snprintf(concatenatedString, sizeof(concatenatedString), "Account: %d %s %d", data->AccountID, data->WitOrDep, data->Amount);
                		strcpy(DataSet[i].TransactionHistory[DataSet[i].NumOfTransactions].TranHistory, concatenatedString);
                		DataSet[i].NumOfTransactions++;
                		
                		if (data->CurrentSeshOrNot == 0){
				continue;
				}
				else{
				strcpy(CurrentSession[CurrentSessionCounter], concatenatedString);
				CurrentSessionCounter++;
				WriteCSV("FileItems.csv", i, DataSet[i].AccountID,DataSet[i].branch,DataSet[i].name,"withdraw",DataSet[i].amount);
				}
                    }
                }		
                break;
            }
        }
        if (found) {
            break;
        } else {
            pthread_cond_wait(&cond_var, &mutex);
        }
    } 
    i++;
    pthread_mutex_unlock(&mutex);
    return NULL;
}
void AllHistory(){
	if (CurrentSessionCounter > 0) {
		printf("Current transactions in the session: \n\n");       				 
		for (int x =0 ; x< CurrentSessionCounter; x++) { 
			printf("%d: %s\n", x+1,CurrentSession[x]);    
		}
	}
	else{
		printf("No current transcations history."); 
	}		
}
void *checkBalance (void *thread_data){
	struct Transaction *data = (struct Transaction *)thread_data;//this is where i convert the pointer of thread_data into a pointer of the correct data type

	for(int i = 0; i<num_rows;i++){	//this is where i iterate through the struct array to find the account
		if( DataSet[i].AccountID == data->AccountID){//i am going to check to see if there is a account that it is in the struct array
			printf("The amount that you have in your account is : %d\n",DataSet[i].amount);//print out the amount
			break;
				}	
	}	
	return NULL; 
}
void *history (void *thread_data){
	struct Transaction *data = (struct Transaction *)thread_data;//this is where i convert the pointer of thread_data into a pointer of the correct data type

	for(int i = 0; i<num_rows;i++){	//this is where i iterate through the struct array to find the account
		if( DataSet[i].AccountID == data->AccountID){//i am going to check to see if there is a account that it is in the struct array
			if (DataSet[i].NumOfTransactions > 0) {//print out the last 5 history here
        			printf("The last 5 history transactions in your account : \n");
        			for (int x = DataSet[i].NumOfTransactions - 5; x < DataSet[i].NumOfTransactions; x++) {
        				 if (strcmp(DataSet[i].TransactionHistory[x].TranHistory, "") == 0){
        				 	continue;
        				 }
           				 else{
           					 printf("%s\n", DataSet[i].TransactionHistory[x].TranHistory);
           				 }
       					 }
   					 } 
   			if(DataSet[i].NumOfTransactions == 0){
       				printf("No transaction history for this account.\n");
   				 }
			break;
		
				}		
	}
	return NULL; 
}
void menu(){//this is the menu for the bank app
	printf("Enter '1' to create an account.\n");
	printf("Enter '2' to check your balance.\n");
	printf("Enter '3' to make a transaction. \n");
	printf("Enter '4' to see your last 5 transactions in your account\n");
	printf("Enter '5' to see all previous transaction history in current session\n");
	printf("Enter '99' to exit out of the program\n");
}
int inputChecker(){
	//asking the user for the account id 
		int IDnumber;
		int foundAccount;
		do {
		    foundAccount = 0;  // Reset the flag

		    // Asking for what the user wants to input
		    printf("Please enter your account ID: \n");
		    scanf("%d", &IDnumber);
		    
		    //loop to check to see if the id entered equals to one of the accounts already in the system	
		    for (int i = 0; i < num_rows; i++) {
			if (IDnumber == DataSet[i].AccountID) {
			    foundAccount = 1;
			    break;  // Found the account, so no need to continue the loop
			}
		    }

		    if (!foundAccount) {
			printf("No account found with the entered ID. Please try again.\n\n");
		    }
		} while (!foundAccount);
		
	return IDnumber;
}
char* PickBranch(int IDNum) {
    char* check_branch = malloc(sizeof(char) * 15);
    if (check_branch != NULL) {
        int num = IDNum;
        char num_str[15]; // Assuming a maximum of 20 digits for the integer
        sprintf(num_str, "%d", num); // Convert integer to string

        if (num_str[0] == '1') {
            strncpy(check_branch, "east", 14);
        } else if (num_str[0] == '2') {
            strncpy(check_branch, "west", 14);
        } else if (num_str[0] == '3') {
            strncpy(check_branch, "north", 14);
        } else if (num_str[0] == '4') {
            strncpy(check_branch, "south", 14);
        } else {
            printf("No branch selected. There is an error.");
            free(check_branch); // Free memory in case of an error
            return NULL; // Return NULL to indicate an error
        }

        check_branch[14] = '\0'; // Null-terminate the string
    } else {
        printf("Memory allocation failed.\n");
    }
    return check_branch;
}
void createAndJoinThread(char branch[], void *(*threadFunction)(void *), struct Transaction *tran) {
    int result;
    if (strcmp(branch, "north") == 0) {
        result = pthread_create(&NorthThread, NULL, threadFunction, tran);
    } else if (strcmp(branch, "south") == 0) {
        result = pthread_create(&SouthThread, NULL, threadFunction, tran);
    } else if (strcmp(branch, "east") == 0) {
        result = pthread_create(&EastThread, NULL, threadFunction, tran);
    } else if (strcmp(branch, "west") == 0) {
        result = pthread_create(&WestThread, NULL, threadFunction, tran);
    } else {// Error handling: No matching thread for the given branch
        return;
    }
    if (result != 0) {
        perror("Thread creation failed");
        return;
    }
    if (strcmp(branch, "north") == 0) {
        pthread_join(NorthThread, NULL);
    } else if (strcmp(branch, "south") == 0) {
        pthread_join(SouthThread, NULL);
    } else if (strcmp(branch, "east") == 0) {
        pthread_join(EastThread, NULL);
    } else if (strcmp(branch, "west") == 0) {
        pthread_join(WestThread, NULL);
    }
}
void *calculateInterest(void *arg) {
    while (1) {
        sleep(120);// Sleep for 2 minutes (120 seconds)
        for (int i = 0; i < num_rows; i++) {// Calculate interest for each account
            double interest = DataSet[i].amount * 0.01; // Calculate interest as 1.00% of the current balance
            DataSet[i].amount += interest; // Update the balance with the calculated interest
        }
    }
    return NULL;
}
void processCSV(const char *filename) {
//opeing and reading the file and seeing if it exists
FILE *fp;
fp = fopen(filename,"r");
if (fp == NULL) {
        perror("Error opening the file");
        return;
    }
   //read the csv file using fgets that reads it one line at a time 
   char line[1000];
   	int q = 0;  
   while (fgets(line, sizeof(line), fp) != NULL) {
    if (num_rows >= 25) {
        printf("Reached the maximum number of rows (25).\n");
        break;
    }
    char *token;// token is my delimiter. It is used to read each column in the CSV by breaking it up by every comma
    token = strtok(line, ",");
    if (token == NULL) {
        // Handle the case when a line doesn't have the expected number of fields.
        continue;
    }
    token = strtok(NULL, ",");// Parse the branch
    if (token == NULL) {
        continue;
    }
    int IDNUMBER = atoi(token);// Parse the AccountID
    token = strtok(NULL, ",");// Parse the branch
    if (token == NULL) {
        continue;
    }
    char BRANCH[50];
    strncpy(BRANCH, token, sizeof(BRANCH));
    token = strtok(NULL, ",");// Parse the name
    if (token == NULL) {
        continue;
    }
    char NAME[50];
    strncpy(NAME, token, sizeof(NAME));
    NAME[strcspn(NAME, "\n")] = '\0';  // Remove newline character
    token = strtok(NULL, ",");// Parse the recentTransaction
    if (token == NULL) {
        continue;
    }
    char TRANSACTION[50];
    strncpy(TRANSACTION, token, sizeof(TRANSACTION));
    TRANSACTION[strcspn(TRANSACTION, "\n")] = '\0';  // Remove newline character
    // Parse the amount
    token = strtok(NULL, ",");
    if (token == NULL) {
        continue;
    }
    int AMOUNT = atoi(token);

   	// Check if the account already exists
	int accountIndex = -1;
	for (int i = 0; i < num_rows; i++) {
	    if (IDNUMBER == DataSet[i].AccountID) {
		accountIndex = i;
		break;
	    }
	}	
	struct Transaction help;// Making your struct that you're going to pass through

	if (accountIndex == -1) { // Account doesn't exist, create a new one
	 //  printf("Used this new; %s\n\n",NAME);
	    help.AccountID = IDNUMBER;
	    strcpy(help.Branch, BRANCH);
	    strcpy(help.NameOnAccount, NAME);
	    help.Amount = AMOUNT;
	    strcpy(help.WitOrDep, TRANSACTION);
	    help.CurrentSeshOrNot = 0;

	    createAndJoinThread(BRANCH, CreateAccount, &help);
	    createAndJoinThread(BRANCH, DepositOrWithdrawFunction, &help);

	} 
	else {// Account already exists, update it
	    help.AccountID = IDNUMBER;
	    help.Amount = AMOUNT;
	    strcpy(help.WitOrDep, TRANSACTION);
	    help.CurrentSeshOrNot = 0;
	    
	    createAndJoinThread(BRANCH, DepositOrWithdrawFunction, &help);

	}
	}
	fclose(fp);	
	
		
	 
}
int main(){

	const char *csvFileName = "initial_transaction.csv";
    	processCSV(csvFileName);
    	const char *csvFileName2 = "FileItems.csv";
    	processCSV(csvFileName2);
    	pthread_t interestThread;//use this thread to calculate the interest every two minutes
    if (pthread_create(&interestThread, NULL, calculateInterest, NULL) != 0) {
        perror("Interest calculation thread creation failed");
    }
    	printf("All accounts from previous sessions: ");
    	for (int i = 0; i < num_rows; i++) {
	    printf("Account ID: %d\n", DataSet[i].AccountID);
	    printf("Branch: %s\n", DataSet[i].branch);
	    printf("Name: %s\n", DataSet[i].name);
	    printf("Amount: %d\n", DataSet[i].amount);
	    printf("\n");
	}
	
    printf("Welcome to you online bank!\n");
    menu();
    int option;
    scanf("%d", &option);
    while(option !=99){//this is the while loop i am using to get the user input in the terminal
    	switch (option) {
        case 1:{//this is the case for when the user wants to add an account
        	 // checking to see if user entered a id that already exists
		 int IDnumber;
   		 int duplicateFound;
    		do {
       		 duplicateFound = 0;  // Reset the duplicate flag
		// Asking for what the user wants to input
		printf("Enter desired account ID (starting with 1 - 2 - 3 - 4): \n");
		scanf("%d", &IDnumber);
      		  for (int i = 0; i < num_rows; i++) {
           		 if (IDnumber == DataSet[i].AccountID) {
             		   duplicateFound = 1;
             		   printf("This user already exists. Try again.\n");
              			  break;
          			  }
          	int num = IDnumber;
    		char num_str[20];// Assuming a maximum of 20 digits for the integer
   		sprintf(num_str, "%d", num);// Convert integer to string
          		 if (num_str[0] != '1' && num_str[0] != '2' && num_str[0] != '3' && num_str[0] != '4') {
             		   duplicateFound = 1;
             		   printf("The account does not start with (1 - 2 - 3 - 4). Try again.\n");
              			  break;
          			  }
          	}
    			} while (duplicateFound);//keep looping until the duplicate found at the end of the code equals 1

		printf("Please enter what your name is: \n");
		char PersonName[20];
		scanf("%s", PersonName);
		char branchResult[15];//Declare a char array to store the result
		strncpy(branchResult, PickBranch(IDnumber), sizeof(branchResult));//Call your function and store the result in branchResult
		branchResult[sizeof(branchResult) - 1] = '\0';	//Ensure the string is null-terminated
		
		// Making your struct that you're going to pass through
		struct Transaction help;
		help.AccountID = IDnumber;
		strcpy(help.Branch, branchResult);
		strcpy(help.NameOnAccount, PersonName);
		
		createAndJoinThread(branchResult, CreateAccount, &help);
			
            	printf("The account has been added!");	    
            }
            break;     
        case 2:{//this is the case for when the user wants to check account balance    
        	
		int IDnumber = inputChecker();//get the id number using the inputChecker function
		char branchResult[15];//Declare a char array to store the result
		strncpy(branchResult, PickBranch(IDnumber), sizeof(branchResult));//Call your function and store the result in branchResult
		branchResult[sizeof(branchResult) - 1] = '\0';//Ensure the string is null-terminated
		struct Transaction help;
		help.AccountID = IDnumber;
		
		createAndJoinThread(branchResult, checkBalance, &help);
	
            }
            break;
        case 3:{//this is the case for depositing money into your account
        
		int IDnumber = inputChecker();//get the id number using the inputChecker function
		char DepOrWit[50];//ask the user if he wants to deposit or withdraw
		int Found;//flag
		int c;// Clear input buffer before asking for branch input
		while ((c = getchar()) != '\n' && c != EOF);

		do {//this do while loop is going to keep looping until we get a correct input for the withdaw or deposit 
			Found = 0;// Reset the flag
			printf("Enter 'Deposit' or 'Withdraw' for your specific needs: \n");// Asking for what the user wants to input
			if (fgets(DepOrWit, sizeof(DepOrWit), stdin) == NULL) {// Capture input separately for validation
			    printf("Error reading input.\n");
			    return 1;
			}
			DepOrWit[strcspn(DepOrWit, "\n")] = '\0';
			for (int i = 0; i < strlen(DepOrWit); i++) {// Convert the input to lowercase for case-insensitive comparison
			    DepOrWit[i] = tolower(DepOrWit[i]);
			}
			if (strcmp(DepOrWit, "withdraw") == 0 || strcmp(DepOrWit, "deposit") == 0) {// Check if the input is valid
			    Found = 1;
			} else {
			    printf("That is not an option. Try again. \n\n");
			}
		} while (!Found);
		
		int AmountToUse;//where im depositing the amount
		printf("Enter the amount of money you want to %s: \n", DepOrWit);//ask the user for the deposit amount
		scanf("%d", &AmountToUse);
		struct Transaction help;//making my struct that i am going to pass through
		help.AccountID = IDnumber;
		help.Amount = AmountToUse;
		strcpy(help.WitOrDep, DepOrWit);		
		char branchResult[15];//Declare a char array to store the result

		strncpy(branchResult, PickBranch(IDnumber), sizeof(branchResult));//Call your function and store the result in branchResult
		branchResult[sizeof(branchResult) - 1] = '\0';//Ensure the string is null-terminated
		help.CurrentSeshOrNot = 1;
		createAndJoinThread(branchResult, DepositOrWithdrawFunction, &help); 
		printf("Account has been updated!");         		
            }
            break;
        case 4:{
		int IDnumber = inputChecker();//get the id number using the inputChecker function
		struct Transaction help;//making my struct that i am going to pass through
		help.AccountID = IDnumber;
		char branchResult[15];//Declare a char array to store the result
		strncpy(branchResult, PickBranch(IDnumber), sizeof(branchResult));//Call your function and store the result in branchResult
		branchResult[sizeof(branchResult) - 1] = '\0';//Ensure the string is null-terminated

		createAndJoinThread(branchResult, history, &help);          		
            }	
            break;
        case 5:{
		AllHistory();
		}
            break;        
        default:
            printf("Invalid choice. Please choose a number from 1 to 5.\n");
    }
	//use sleep so everything prints out properly
	sleep(1);
	printf("\n------------------------------------------------------------------------\n");
	printf("------------------------------------------------------------------------\n");
	menu();
	option;
	scanf("%d", &option);
    }
	printf("Thank You For Visiting Our Store! \n");
	pthread_mutex_destroy(&mutex);
	pthread_mutex_destroy(&account_creation_mutex); 
	pthread_cond_destroy (&cond_var);
	return 0;
}
