#include <AudioFileSourceHTTPStream.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>
#include <AudioOutputI2SNoDAC.h>

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <base64.h>
#include <ArduinoJson.h>

#include "driver/i2s.h"
#include "driver/dac.h"

AudioFileSourceHTTPStream* file;
AudioGeneratorMP3* mp3;
AudioOutput* out;  // Changed to base class to support both I2S and DAC

// I2S Pin definitions - Current configuration
// If getting all zeros, try one of the alternative sets below
#define I2S_WS   13
#define I2S_SCK  14 
#define I2S_SD   36

const char* WIFI_SSID = "NETGEAR06";
const char* WIFI_PASS = "widesquash084";
const char* SERVER_URL = "http://192.168.1.83:5001/voice";

// Using built-in ESP32 audio capabilities
void setupI2SMic() {
  Serial.println("Setting up I2S microphone...");
  
  // I2S configuration for microphone
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 1024,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  // Try different pin configuration - more standard ESP32 I2S pins
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK, 
    .ws_io_num = I2S_WS, 
    .data_out_num = I2S_PIN_NO_CHANGE, 
    .data_in_num = I2S_SD 
  };

  // Install and start I2S driver
  esp_err_t result = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  if (result != ESP_OK) {
    Serial.printf("Failed to install I2S driver: %d\n", result);
    return;
  }

  result = i2s_set_pin(I2S_NUM_0, &pin_config);
  if (result != ESP_OK) {
    Serial.printf("Failed to set I2S pins: %d\n", result);
    return;
  }

  Serial.println("I2S microphone setup complete");
}

void setupPAM8302A() {
  Serial.println("Setting up PAM8302A analog amplifier...");
  Serial.println("PAM8302A is an ANALOG amplifier, not I2S digital!");
  Serial.println("");
  Serial.println("🔌 CORRECT WIRING for 5-pin PAM8302A:");
  Serial.println("  PAM8302A IN  -> ESP32 GPIO 25 (DAC1 analog output)");
  Serial.println("  PAM8302A VCC -> ESP32 3.3V (power)");
  Serial.println("  PAM8302A GND -> ESP32 GND (ground)");  
  Serial.println("  PAM8302A SD  -> ESP32 GND (shutdown control - tie low to enable)");
  Serial.println("  PAM8302A GND -> ESP32 GND (second ground pin)");
  Serial.println("");
  Serial.println("🔊 Speaker connects to PAM8302A output terminals");
  Serial.println("⚡ ESP32 DAC1 (GPIO 25) will output 0-3.3V analog audio signal");
  
  // Initialize DAC for audio output
  dac_output_enable(DAC_CHANNEL_1);  // GPIO 25
  Serial.println("DAC1 (GPIO 25) enabled for analog audio output");
}

// Test function to check if I2S is working at all
void testI2S() {
  Serial.println("Testing I2S functionality...");
  
  uint8_t testBuffer[1024];
  size_t bytesRead = 0;
  
  esp_err_t result = i2s_read(I2S_NUM_0, testBuffer, sizeof(testBuffer), &bytesRead, 2000);
  Serial.printf("Test I2S read: %s, bytes read: %d\n", esp_err_to_name(result), (int)bytesRead);
  
  if (bytesRead > 0) {
    Serial.printf("First 20 bytes: ");
    for (int i = 0; i < 20 && i < bytesRead; i++) {
      Serial.printf("%02X ", testBuffer[i]);
    }
    Serial.println();
    
    // Check for non-zero data
    int nonZero = 0;
    for (int i = 0; i < bytesRead; i++) {
      if (testBuffer[i] != 0) nonZero++;
    }
    Serial.printf("Non-zero bytes: %d/%d\n", nonZero, (int)bytesRead);
    
    if (nonZero == 0) {
      Serial.println("WARNING: All bytes are zero! Check microphone connections:");
      Serial.println("- Verify microphone is connected");  
      Serial.println("- Check I2S pin connections");
      Serial.println("- Try different pin configuration");
    }
  }
}

String recordAudioBase64(float seconds = 1.0) {  // Accept floating point seconds
  int totalSamples = (int)(16000 * seconds);
  int bytesToRead = totalSamples * 2; // 16-bit = 2 bytes per sample
  
  uint8_t* audioBuffer = (uint8_t*)malloc(bytesToRead);
  if (!audioBuffer) {
    Serial.println("Memory allocation failed!");
    return "";
  }

  // Clear the buffer first
  memset(audioBuffer, 0, bytesToRead);
  
  size_t bytesRead = 0;
  size_t totalBytesRead = 0;
  Serial.println("Recording...");
  
  // Clear any existing data in I2S buffer
  i2s_zero_dma_buffer(I2S_NUM_0);
  delay(100); // Give I2S time to start
  
  // Read in smaller chunks to avoid timeout issues
  int chunkSize = 1024;
  int chunks = bytesToRead / chunkSize;
  
  for (int i = 0; i < chunks && totalBytesRead < bytesToRead; i++) {
    size_t chunkBytesRead = 0;
    int remainingBytes = bytesToRead - totalBytesRead;
    int readSize = (chunkSize < remainingBytes) ? chunkSize : remainingBytes;
    esp_err_t result = i2s_read(I2S_NUM_0, audioBuffer + totalBytesRead, 
                               readSize, 
                               &chunkBytesRead, 1000);
    
    if (result == ESP_OK) {
      totalBytesRead += chunkBytesRead;
    } else {
      Serial.printf("I2S read error in chunk %d: %s\n", i, esp_err_to_name(result));
      break;
    }
    
    // Small delay between reads
    delay(10);
  }
  
  // More thorough audio data validation
  if (totalBytesRead == 0) {
    Serial.println("No audio data recorded!");
    free(audioBuffer);
    return "";
  }

  String audioB64 = base64::encode(audioBuffer, totalBytesRead);
  
  free(audioBuffer);
  return audioB64;
}

String sendToGemini(String audioBase64) {
  Serial.println("Sending request to server...");
  
  HTTPClient http;
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(30000);
  
  if (audioBase64.length() == 0) {
    Serial.println("No audio data available!");
    return "";
  }

  // Build JSON payload
  String payload = "{\"audio\":\"";
  payload += audioBase64;
  payload += "\"}";
  
  int httpCode = http.POST(payload);
  String response = http.getString();
  http.end();

  if (httpCode != 200) {
    Serial.printf("HTTP error %d\n", httpCode);
    return "";
  }
  
  return response;
}

void playFromURL(String url) {
  Serial.println("Playing audio response...");
  
  // Use Internal DAC output for PAM8302A
  out = new AudioOutputI2SNoDAC(1);  
  out->SetGain(4.0);

  file = new AudioFileSourceHTTPStream(url.c_str());
  mp3 = new AudioGeneratorMP3();
  
  if (!mp3->begin(file, out)) {
    Serial.println("Failed to start MP3 playback");
    delete file;
    delete mp3;
    delete out;
    return;
  }

  int loopCount = 0;
  
  while (mp3->isRunning()) {
    if (!mp3->loop()) break;
    loopCount++;
    delay(1);
  }

  Serial.printf("Playback completed (%d loops)\n", loopCount);
  mp3->stop();
  delete file;
  delete mp3;
  delete out;

  // Test PAM8302A analog amplifier connection
  Serial.println("🔧 HARDWARE TEST: PAM8302A Troubleshooting");
  Serial.println("❌ No test tones heard - checking hardware step by step...");
  Serial.println("");
  
  // Step 1: Test ESP32 DAC output directly
  Serial.println("STEP 1: Testing ESP32 DAC output (should see voltage changes on GPIO 25)");
  Serial.println("Use multimeter on GPIO 25 to verify DAC is working:");
  
  dacWrite(25, 0);    // 0V
  Serial.println("  Setting GPIO 25 to 0V... (measure now)");
  delay(2000);
  
  dacWrite(25, 128);  // ~1.65V  
  Serial.println("  Setting GPIO 25 to 1.65V... (measure now)");
  delay(2000);
  
  dacWrite(25, 255);  // ~3.3V
  Serial.println("  Setting GPIO 25 to 3.3V... (measure now)");
  delay(2000);
  
  dacWrite(25, 128);  // Back to mid-level
  Serial.println("  Back to 1.65V");
  
  Serial.println("");
  Serial.println("STEP 2: Testing with louder, slower test tones");
  Serial.println("(If PAM8302A is wired correctly, these should be VERY loud)");
  
  for (int i = 0; i < 5; i++) {
    Serial.printf("🔊 LOUD Test tone %d/5 (2 seconds each)\n", i + 1);
    
    // Much slower, louder tones - 2Hz square wave for 2 seconds
    for (int j = 0; j < 4; j++) {  // 4 cycles = 2 seconds at 2Hz
      dacWrite(25, 255);  // Maximum level
      delay(250);  // 250ms high
      dacWrite(25, 0);    // Minimum level  
      delay(250);  // 250ms low
    }
    dacWrite(25, 128);  // Return to mid-level
    delay(500);
  }
  
  Serial.println("");
  Serial.println("🔧 PAM8302A (5-pin) CRITICAL WIRING CHECK:");
  Serial.println("Pin by pin verification needed:");
  Serial.println("");
  Serial.println("1. PAM8302A 'IN'  -> ESP32 GPIO 25 (audio signal)");
  Serial.println("2. PAM8302A 'VCC' -> ESP32 3.3V (NOT 5V!)");  
  Serial.println("3. PAM8302A 'GND' -> ESP32 GND");
  Serial.println("4. PAM8302A 'SD'  -> ESP32 GND ⚠️  CRITICAL: Must be GND to enable!");
  Serial.println("5. If there's a 5th pin (GND), also connect to ESP32 GND");
  Serial.println("");
  Serial.println("🔊 Speaker connections:");
  Serial.println("   Speaker + -> PAM8302A OUT+ (or A+)");  
  Serial.println("   Speaker - -> PAM8302A OUT- (or A-)");
  Serial.println("");
  Serial.println("❌ MOST COMMON PROBLEMS:");
  Serial.println("• SD pin not connected to GND (amplifier stays disabled)");
  Serial.println("• VCC connected to 5V instead of 3.3V (may damage PAM8302A)");
  Serial.println("• Loose connections");
  Serial.println("• Speaker not connected to PAM8302A output");
  Serial.println("• Dead/blown speaker");
  
  Serial.println("");
  Serial.println("🔧 IMMEDIATE TROUBLESHOOTING STEPS:");
  Serial.println("1. Verify ESP32 DAC: Did voltage change on GPIO 25? (use multimeter)");
  Serial.println("2. Check PAM8302A SD pin: MUST be connected to GND to enable amplifier");
  Serial.println("3. Verify power: PAM8302A VCC should be 3.3V, NOT 5V");
  Serial.println("4. Test speaker: Connect speaker directly to GPIO 25 & GND");
  Serial.println("   (should hear faint audio without amplifier)");
  Serial.println("5. Check all connections are secure and not loose");
  Serial.println("");
  Serial.println("� NEXT STEPS:");
  Serial.println("• If no voltage changes on GPIO 25: ESP32 DAC problem");
  Serial.println("• If voltage changes but no sound: PAM8302A wiring issue");
  Serial.println("• Focus on SD pin - it's the #1 cause of 'no sound' problems");}

void setup() {
  Serial.begin(115200);
  delay(2000); // Longer delay for stability
  Serial.println("Starting Gemini Voice Assistant...");

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("Connecting to %s", WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) { 
    delay(500); 
    Serial.print("."); 
  }
  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: "); Serial.println(WiFi.localIP());

  setupI2SMic();
  setupPAM8302A();
  
  // Test I2S functionality
  delay(1000);
  testI2S();
  
  Serial.println("Setup complete. Starting main loop...");
}

void loop() {
  Serial.println("\n--- New cycle ---");
  
  String audioData = recordAudioBase64(0.25);
  
  if (audioData.length() > 0) {
    String response = sendToGemini(audioData);
    if (response.length() > 0) {
      playFromURL(response);
    }
  }

  Serial.println("Waiting before next cycle...");
  delay(5000);
}
