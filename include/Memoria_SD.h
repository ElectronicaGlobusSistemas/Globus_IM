#include "time.h"
#include <ESP32Time.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

void writeFile(fs::FS &fs, const char path, const char message)
{
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print("\n"))
  {
    if (file.print(message))
    {
      Serial.println("Message appended");
    }
  }
  else
  {
    Serial.println("Write failed");
  }
  file.close();
}


void appendFile(fs::FS &fs, const char path, const char message)
{
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file)
  {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print("\n"))
  {
    if (file.print(message))
    {
      Serial.println("Message appended");
    }
  }
  else
  {
    Serial.println("Append failed");
  }
  file.close();
}

void readFile(fs::FS &fs, const char *path)
{
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file)
  {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available())
  {
    Serial.write(file.read());
  }
  file.close();
}
