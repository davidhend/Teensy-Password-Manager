void load_menu_options()
{
  Serial.println("Reading Menu Options.");
  
  boolean unlabeled_menu_options_found = false;
  
  /* iterate over the menu file */
  for(int i = 0; i <= NumPassEntries-1; i++){
    /* menuItem temporarily holds line data from SD */
    String menuItem = "";
    /* read a menu item from the file */
    menuItem = readMenuEntry(i+1); 
    
    /* if unlabeled menu options are found, re-label them as UNUSED */
    if(menuItem == "" || menuItem == " "){
      menuItem = "UNUSED";
      /* since we had to relabel some entries, set flag to true, that way the menu names are saved to SD */
      unlabeled_menu_options_found = true;
    }
    
    /* copy the menu item into the profile options for use */
    profile_options[i] = menuItem;
    
    Serial.print(i);
    Serial.print(",");
  }  
  Serial.println();
  
  /* since we had to relabel some entries, we need to save the menu names to SD */
  if(unlabeled_menu_options_found == true){
    /* delete the menu text file, so we can replace it */
    SD.remove("menu.txt");
    /* small ammount of delay to finish delete operation*/
    delay(5);
    /* re-write the menu file to the SD card */
    writeMenu(); 
  }
  
}



void writeMenu()
{
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  myFile = SD.open("menu.txt", FILE_WRITE);
  
  // if the file opened okay, write to it:
  if (myFile) {
    /* iterate over menu entries */
    for(int i = 0; i <= NumPassEntries-1; i++){
      /* find the size of the munu entry */
      int option_size = sizeof(profile_options[i]);
      /* create a character buffer equal to the size of the menu entry */
      char buffer[option_size];
      /* clear buffer array before filling it*/
      for (int i = 0; i < option_size; i++){
        buffer[i] = ' '; 
      }
      /* convert the string menu entries into character arrays to be written to SD*/
      profile_options[i].toCharArray(buffer, option_size);
      /* write entry to file*/
      myFile.write(buffer, option_size);
      /* write seperator character */
      myFile.write(',');
    }
    // close the file:
    myFile.close();

  } else {
    // if the file didn't open, do nothing.
  }
  
}


String readMenuEntry(int line)
{
  /* open the text file for reading */
  myFile = SD.open("menu.txt",FILE_READ);
  /* if the file is ready for reading */
  if(myFile)
  {
    String received = "";
    int cur = 0;
    char ch;
    while (myFile.available())
    {
      ch = myFile.read();
      if(ch == ',') 
      {
        cur++;
        if(cur == line)
        {
          myFile.close();
          return received;
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

