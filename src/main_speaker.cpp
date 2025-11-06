// #include <AudioFileSourceHTTPStream.h>
// #include <AudioGeneratorMP3.h>
// #include <AudioOutputI2S.h>
// #include <AudioOutputI2SNoDAC.h>

// #include <Arduino.h>
// #include <WiFi.h>
// #include <HTTPClient.h>
// #include <base64.h>
// #include <ArduinoJson.h>

// #include "driver/i2s.h"

// AudioFileSourceHTTPStream* file;
// AudioGeneratorMP3* mp3;
// AudioOutput* out;  // Changed to base class to support both I2S and DAC

// // I2S Pin definitions - Matching your hardware documentation
// #define I2S_WS   14  // GPIO 14 - I2S word-select (WS/LRCLK)
// #define I2S_SCK  26  // GPIO 26 - I2S bit clock (SCK/BCLK) - MICROPHONE ONLY
// #define I2S_SD   35  // GPIO 35 - I2S data (SD/DOUT) - Input only pin, perfect for mic data

// const char* WIFI_SSID = "NETGEAR06";
// const char* WIFI_PASS = "widesquash084";
// const char* SERVER_URL = "http://192.168.1.86:5001/voice";

// // Using built-in ESP32 audio capabilities
// void setupI2SMic() {
//   Serial.println("Setting up I2S microphone...");
  
//   // I2S configuration for microphone
//   i2s_config_t i2s_config = {
//     .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
//     .sample_rate = 16000,
//     .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
//     .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
//     .communication_format = I2S_COMM_FORMAT_STAND_I2S,
//     .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
//     .dma_buf_count = 4,
//     .dma_buf_len = 1024,
//     .use_apll = false,
//     .tx_desc_auto_clear = false,
//     .fixed_mclk = 0
//   };

//   // Try different pin configuration - more standard ESP32 I2S pins
//   i2s_pin_config_t pin_config = {
//     .bck_io_num = I2S_SCK, 
//     .ws_io_num = I2S_WS, 
//     .data_out_num = I2S_PIN_NO_CHANGE, 
//     .data_in_num = I2S_SD 
//   };

//   // Install and start I2S driver
//   esp_err_t result = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
//   if (result != ESP_OK) {
//     Serial.printf("Failed to install I2S driver: %d\n", result);
//     return;
//   }

//   result = i2s_set_pin(I2S_NUM_0, &pin_config);
//   if (result != ESP_OK) {
//     Serial.printf("Failed to set I2S pins: %d\n", result);
//     return;
//   }

//   Serial.println("I2S microphone setup complete");
// }

// // Test function to check if I2S is working at all
// void testI2S() {
//   Serial.println("Testing I2S functionality...");
  
//   uint8_t testBuffer[1024];
//   size_t bytesRead = 0;
  
//   esp_err_t result = i2s_read(I2S_NUM_0, testBuffer, sizeof(testBuffer), &bytesRead, 2000);
//   Serial.printf("Test I2S read: %s, bytes read: %d\n", esp_err_to_name(result), (int)bytesRead);
  
//   if (bytesRead > 0) {
//     Serial.printf("First 20 bytes: ");
//     for (int i = 0; i < 20 && i < bytesRead; i++) {
//       Serial.printf("%02X ", testBuffer[i]);
//     }
//     Serial.println();
    
//     // Check for non-zero data
//     int nonZero = 0;
//     for (int i = 0; i < bytesRead; i++) {
//       if (testBuffer[i] != 0) nonZero++;
//     }
//     Serial.printf("Non-zero bytes: %d/%d\n", nonZero, (int)bytesRead);
    
//     if (nonZero == 0) {
//       Serial.println("WARNING: All bytes are zero! Check microphone connections:");
//       Serial.println("- Verify microphone is connected");  
//       Serial.println("- Check I2S pin connections");
//       Serial.println("- Try different pin configuration");
//     }
//   }
// }

// String recordAudioBase64(float seconds = 1.0) {  // Accept floating point seconds
//   int totalSamples = (int)(16000 * seconds);
//   int bytesToRead = totalSamples * 2; // 16-bit = 2 bytes per sample
  
//   uint8_t* audioBuffer = (uint8_t*)malloc(bytesToRead);
//   if (!audioBuffer) {
//     Serial.println("Memory allocation failed!");
//     return "";
//   }

//   // Clear the buffer first
//   memset(audioBuffer, 0, bytesToRead);
  
//   size_t bytesRead = 0;
//   size_t totalBytesRead = 0;
//   Serial.println("Recording...");
  
//   // Clear any existing data in I2S buffer
//   i2s_zero_dma_buffer(I2S_NUM_0);
//   delay(100); // Give I2S time to start
  
//   // Read in smaller chunks to avoid timeout issues
//   int chunkSize = 1024;
//   int chunks = bytesToRead / chunkSize;
  
//   for (int i = 0; i < chunks && totalBytesRead < bytesToRead; i++) {
//     size_t chunkBytesRead = 0;
//     int remainingBytes = bytesToRead - totalBytesRead;
//     int readSize = (chunkSize < remainingBytes) ? chunkSize : remainingBytes;
//     esp_err_t result = i2s_read(I2S_NUM_0, audioBuffer + totalBytesRead, 
//                                readSize, 
//                                &chunkBytesRead, 1000);
    
//     if (result == ESP_OK) {
//       totalBytesRead += chunkBytesRead;
//     } else {
//       Serial.printf("I2S read error in chunk %d: %s\n", i, esp_err_to_name(result));
//       break;
//     }
    
//     // Small delay between reads
//     delay(10);
//   }
  
//   // More thorough audio data validation
//   if (totalBytesRead == 0) {
//     Serial.println("No audio data recorded!");
//     free(audioBuffer);
//     return "";
//   }

//   String audioB64 = base64::encode(audioBuffer, totalBytesRead);
  
//   free(audioBuffer);
//   return audioB64;
// }

// String sendToGemini(String audioBase64) {
//   Serial.println("Sending request to server...");
  
//   HTTPClient http;
//   http.begin(SERVER_URL);
//   http.addHeader("Content-Type", "application/json");
//   http.setTimeout(30000);
  
//   if (audioBase64.length() == 0) {
//     Serial.println("No audio data available!");
//     return "";
//   }

//   // Build JSON payload
//   String payload = "{\"audio\":\"";
//   payload += audioBase64;
//   payload += "\"}";
  
//   int httpCode = http.POST(payload);
//   String response = http.getString();
//   http.end();

//   if (httpCode != 200) {
//     Serial.printf("HTTP error %d\n", httpCode);
//     return "";
//   }
  
//   return response;
// }

// void playFromURL(String url) {
//   Serial.println("Playing audio response...");
//   Serial.printf("Audio URL: %s\n", url.c_str());
  
//   // Stop I2S microphone before audio playback
//   i2s_driver_uninstall(I2S_NUM_0);
//   delay(100);
  
//   // Use internal DAC for audio output
//   // This will use GPIO 25 (DAC1) and GPIO 26 (DAC2)
//   // We'll temporarily lose GPIO 26 for microphone during playback
//   out = new AudioOutputI2SNoDAC(1);  
//   out->SetGain(4.0);
  
//   Serial.println("Audio output configured - using ESP32 internal DAC");
//   Serial.println("🔊 SPEAKER WIRING (based on your documentation):");
//   Serial.println("   Speaker + -> GPIO 25 -> Electrolytic Cap (+) -> Speaker +");
//   Serial.println("   Speaker - -> GND (common ground)");
//   Serial.println("   Note: GPIO 26 temporarily unavailable during audio playback");

//   file = new AudioFileSourceHTTPStream(url.c_str());
//   mp3 = new AudioGeneratorMP3();
  
//   if (!mp3->begin(file, out)) {
//     Serial.println("Failed to start MP3 playback");
//     Serial.println("Possible causes:");
//     Serial.println("- Invalid MP3 file/URL");
//     Serial.println("- Network connection issues");
//     Serial.println("- Audio output initialization failed");
//     delete file;
//     delete mp3;
//     delete out;
//     setupI2SMic(); // Restart microphone
//     return;
//   }
  
//   Serial.println("MP3 playback started successfully");

//   int loopCount = 0;
  
//   while (mp3->isRunning()) {
//     if (!mp3->loop()) break;
//     loopCount++;
//     delay(1);
//   }

//   Serial.printf("Playback completed (%d loops)\n", loopCount);
//   mp3->stop();
//   delete file;
//   delete mp3;
//   delete out;
  
//   // Restart I2S microphone after audio playback
//   delay(100);
//   setupI2SMic();
// }

// void setup() {
//   Serial.begin(115200);
//   delay(2000); // Longer delay for stability
//   Serial.println("Starting Gemini Voice Assistant...");

//   WiFi.begin(WIFI_SSID, WIFI_PASS);
//   Serial.printf("Connecting to %s", WIFI_SSID);
//   while (WiFi.status() != WL_CONNECTED) { 
//     delay(500); 
//     Serial.print("."); 
//   }
//   Serial.println("\nWiFi Connected!");
//   Serial.print("IP Address: "); Serial.println(WiFi.localIP());

//   setupI2SMic();
  
//   // Test I2S functionality
//   delay(1000);
//   testI2S();
  
//   Serial.println("Setup complete. Starting main loop...");
// }

// void loop() {
//   Serial.println("\n--- New cycle ---");
  
//   String audioData = recordAudioBase64(0.25);
  
//   if (audioData.length() > 0) {
//     String response = sendToGemini(audioData);
//     if (response.length() > 0) {
//       playFromURL(response);
//     }
//   }

//   Serial.println("Waiting before next cycle...");
//   delay(5000);
// }
