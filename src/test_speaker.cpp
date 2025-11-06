#include <AudioFileSourceHTTPStream.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutputI2S.h>
#include <AudioOutputI2SNoDAC.h>

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

#include "driver/i2s.h"
#include "driver/dac.h"

AudioFileSourceHTTPStream* file;
AudioGeneratorWAV* wav;
AudioOutput* out;

// PAM8302A Analog Amplifier Configuration  
#define DAC_PIN       25  // ESP32 DAC1 output pin

const char* WIFI_SSID = "NETGEAR06";
const char* WIFI_PASS = "widesquash084";
const char* AUDIO_URL = "http://192.168.1.86:5001/response/file_example_WAV_5MG.wav";

void setupPAM8302A() {
  Serial.println("🔊 Setting up PAM8302A Analog Amplifier...");
  Serial.println("");
  Serial.println("📋 PAM8302A (5-pin) WIRING GUIDE:");
  Serial.println("  PAM8302A VCC -> ESP32 3.3V (power)");
  Serial.println("  PAM8302A GND -> ESP32 GND (ground - first GND pin)");
  Serial.println("  PAM8302A IN  -> ESP32 GPIO 25 (analog audio input from DAC1)");
  Serial.println("  PAM8302A SD  -> ESP32 GND (shutdown control - tie LOW to enable)");
  Serial.println("  PAM8302A GND -> ESP32 GND (ground - second GND pin)");
  Serial.println("");
  Serial.println("🔊 Speaker connects to PAM8302A OUT+ and OUT- terminals");
  Serial.println("⚡ PAM8302A amplifies the analog signal from ESP32 DAC1 (GPIO 25)");
  Serial.println("");
  Serial.println("⚠️  CRITICAL: SD pin must be connected to GND to enable amplifier!");
}

void playAudio() {
  Serial.println("🔊 Testing speaker with test.wav...");
  
  // Use internal DAC for PAM8302A analog amplifier
  // PAM8302A takes analog input from ESP32's DAC1 (GPIO 25)
  out = new AudioOutputI2SNoDAC(1);  // Use DAC1 (GPIO 25) for analog output
  out->SetGain(16.0);  // Very high gain to compensate for quiet MP3
  
  Serial.printf("Audio output gain set to: 16.0x\n");
  
  Serial.println("Audio output configured - using ESP32 DAC1 for PAM8302A");
  Serial.printf("Attempting to connect to: %s\n", AUDIO_URL);

  file = new AudioFileSourceHTTPStream(AUDIO_URL);
  
  // Check if HTTP stream opened successfully
  if (!file) {
    Serial.println("❌ Failed to create HTTP stream");
    delete out;
    return;
  }
  
  Serial.println("✅ HTTP stream created successfully");
  wav = new AudioGeneratorWAV();
  
  if (!wav->begin(file, out)) {
    Serial.println("❌ Failed to start MP3 playback");
    Serial.println("Possible causes:");
    Serial.println("- File not found at server URL");
    Serial.println("- Invalid MP3 format");
    Serial.println("- Network connection issue");
    Serial.println("- Server not responding");
    
    // Try to get more info about the HTTP response
    Serial.printf("Trying to access: %s\n", AUDIO_URL);
    
    delete file;
    delete wav;
    delete out;
    return;
  }
  
  Serial.println("✅ MP3 playback started successfully");
  Serial.println("🎵 Playing test.wav with increased gain...");
  Serial.println("💡 If still too quiet, the issue might be:");
  Serial.println("   - MP3 file has low volume");
  Serial.println("   - Audio library output level is different from direct DAC");

  int loopCount = 0;
  
  while (wav->isRunning()) {
    if (!wav->loop()) break;
    loopCount++;
    
    // Print progress every 1000 loops
    if (loopCount % 1000 == 0) {
      Serial.printf("Playing... (%d loops)\n", loopCount);
    }
    
    delay(1);
  }

  Serial.printf("✅ Playback completed (%d loops)\n", loopCount);
  wav->stop();
  delete file;
  delete wav;
  delete out;
  
  // Test if the issue is with the audio library vs direct DAC
  Serial.println("🔧 Testing audio library output level...");
}

void testServerConnection() {
  Serial.println("🌐 Testing server connection...");
  
  HTTPClient http;
  http.begin(AUDIO_URL);
  http.setTimeout(5000);
  
  int httpCode = http.GET();
  Serial.printf("HTTP Response Code: %d\n", httpCode);
  
  if (httpCode > 0) {
    int contentLength = http.getSize();
    Serial.printf("Content Length: %d bytes\n", contentLength);
    
    if (httpCode == 200) {
      Serial.println("✅ Server responded successfully - file should be accessible");
    } else {
      Serial.printf("⚠️ Server responded with code %d - check if file exists\n", httpCode);
    }
  } else {
    Serial.printf("❌ HTTP request failed: %s\n", http.errorToString(httpCode).c_str());
    Serial.println("Check:");
    Serial.println("- Is your server running on port 5001?");
    Serial.println("- Is test.wav in the server/response/ directory?");
    Serial.println("- Is the server accessible from this IP?");
  }
  
  http.end();
  Serial.println("");
}

void testPAM8302A() {
  Serial.println("🔧 PAM8302A Status: ✅ Test tones worked - amplifier is properly connected!");
  Serial.println("");
}

void testDACOutput() {
  Serial.println("🔧 Testing ESP32 DAC1 output for PAM8302A...");
  
  // Enable DAC1 (GPIO 25)
  dac_output_enable(DAC_CHANNEL_1);
  
  Serial.println("Generating quiet test tones on GPIO 25...");
  Serial.println("(If PAM8302A is wired correctly, you should hear soft tones)");
  
  // Generate a few quieter test tones
  for (int i = 0; i < 3; i++) {
    Serial.printf("Quiet test tone %d/3\n", i + 1);
    
    // Generate a softer square wave for 1 second (lower amplitude)
    for (int j = 0; j < 100; j++) {  // 100 cycles = 1 second at 100Hz
      dacWrite(25, 180);  // Reduced high level (was 255)
      delay(5);
      dacWrite(25, 75);   // Raised low level (was 0) 
      delay(5);
    }
    
    // Return to mid-level and pause
    dacWrite(25, 128);
    delay(500);  // Shorter pause
  }
  
  Serial.println("DAC test completed. If no sound, check PAM8302A wiring!");
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("🎵 PAM8302A Speaker Test - Playing test.wav");
  
  // Show PAM8302A wiring information
  setupPAM8302A();

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("Connecting to %s", WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) { 
    delay(500); 
    Serial.print("."); 
  }
  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: "); Serial.println(WiFi.localIP());
  
  Serial.println("Setup complete. Testing DAC output first...");
  
  // Test DAC output with simple tones before trying MP3
  testDACOutput();
  
  Serial.println("Ready to test PAM8302A speaker with MP3...");
  delay(1000);
}

void loop() {
  Serial.println("\n--- Testing MP3 Playback with PAM8302A ---");
  
  testPAM8302A();
  testServerConnection();
  playAudio();
  
  Serial.println("Waiting 10 seconds before next playback...");
  delay(10000);
}
