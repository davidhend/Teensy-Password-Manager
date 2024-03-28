void write_to_sd()
{
  /* open the file test.txt on the sd card for writing  */
  myFile = SD.open("test.txt", FILE_WRITE); 
 
  /* if file available write all passwords to file  */
  if (myFile) {
    
    unsigned char temp_pwd_storage[16];
    
    Serial.println("Writing PW Data to SD");
    for (int i = 0; i <= NumPassEntries-1; i++){
      for (int j = 0; j <= 15; j++){ 
        temp_pwd_storage[j] = pwd[i][j];
      }
      myFile.write(temp_pwd_storage, 16);
      myFile.write('\n');
    }
    myFile.close();
    
  }
}


void read_all_password_data()
{
  Serial.println("Reading Password Data.");
  
  for(int i = 0; i <= NumPassEntries-1; i++)
  {
    /* read first line of sd card */
    sd_data = readFile(i+1);
    
    /* if a password exists on the line we are reading */
    if(sd_data.length() > 5)
    {
           /* itterate over the sixteen bits that make up the password */
           for(int k = 0; k <= 15; k++)
           {
             /* store the 16 bits in pw1 char array */
             pwd[i][k] = sd_data[k];
           }
           /* reset data buffer for next read */
           sd_data = "";  
    }
  }
}

String readFile(int line)
{
  myFile = SD.open("test.txt",FILE_READ);
  if(myFile)
  {
    String received = "";
    int cur = 0;
    char ch;
    while (myFile.available())
    {
      ch = myFile.read();
      if(ch == '\n' && received.length() >= 16) 
      {
        cur++;
        if(cur == line)
        {
          myFile.close();
          return String(received);
        }
        received = "";
      }
      else
      {
        received += ch;
      }
    }
  }
  return "";
}


