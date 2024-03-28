/* keeps track of secondary authentication status */
boolean key_needed = true;
/* keeps track of pin attempts */
int key_attempts = 0;
/* used when comparing andriod pin number */
int counter = 0;
/* clear the string that holds the data from bluetooth */
String bt_data = "";

boolean entries_needed = true;

void get_authorization()
{
 /* while we still need a pin number */
 while(key_needed){
   /* read the bluetooth adapter for incoming data */
   if(Serial1.available() > 0){
     /* read the data as a char */
     char c = Serial1.read();
     
     if (c == '!'){
       parse_key(); 
     }
     
     /* append the char we read to the string that holds the pin number */
     bt_data += c;
   }
 }
 
}

void get_password_entries()
{
  while(entries_needed){
   /* read the bluetooth adapter for incoming data */
   if(Serial1.available() > 0){
     /* read the data as a char */
     char c = Serial1.read();
     
     if (c == '!'){
       parse_entries(); 
     }
     
     /* append the char we read to the string that holds the pin number */
     bt_data += c;
   }
 }
  
}


void parse_key()
{
  /* reset the sms character counter */
  int byte_counter = 0;
  
  //Convert bt_data string to char array
  int bt_string_size = bt_data.length() + 1;
  
  char bt_buffer[bt_string_size];
  
  /* convert bt_data string to char array for splitting */
  bt_data.toCharArray(bt_buffer, bt_string_size);

  /* itterate over bt_buffer and extract all the values that will make up the encrption key */
  char *x = bt_buffer;
  char *st;
  while ((st = strtok_r(x, ",", &x)) != NULL ) /* delimiter is the comma */
  { 
    int val = atoi(st); 
    encryptionKey[byte_counter] = val;
    Serial.print(encryptionKey[byte_counter]);
    Serial.print(",");
    byte_counter++;
  }  
  Serial.println();
  
  bt_data = ""; 
  key_needed = false;
  
}


void parse_entries()
{
  
  String sub_string = bt_data.substring(1,3);
   
  int val = sub_string.toInt();
  
  Serial.print("Number Of Password Entries: ");
  Serial.println(val);
  
  NumPassEntries = val;
  
  bt_data = ""; 
  entries_needed = false;
  
}
