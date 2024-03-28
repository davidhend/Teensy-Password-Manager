void encrypt(int profile_number)
{
  if(profile_number >= 0 && profile_number <= NumPassEntries-1)
  { 
    /* encrypt password */
    nrounds = aesSetupEncrypt(rk, encryptionKey, KEYBITS);
    aesEncrypt(rk, nrounds, plaintext, ciphertext);

    /* clear non-encrypted data from memory */
    for(int i = 0; i <= 16; i++){
       plaintext[i] = ' ';
    }  
 
    for(int i = 0; i <= 15; i++){
       pwd[profile_number][i] = ciphertext[i]; 
    }
  }
}

void decrypt(int profile_number)
{  
  Serial.print("Decrypt Profile # ");
  Serial.println(profile_selected);
  if(profile_number >= 0 && profile_number <= NumPassEntries-1)
  {
    for(int i = 0; i <= 15; i++){
       ciphertext[i] = pwd[profile_number][i]; 
    }
  
    /* decrypt password */
    nrounds = aesSetupDecrypt(rk, encryptionKey, KEYBITS);
    aesDecrypt(rk, nrounds, ciphertext, plaintext); 
  }  
}

