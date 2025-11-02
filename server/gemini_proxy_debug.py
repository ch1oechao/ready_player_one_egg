from flask import Flask, request, send_file, jsonify
import requests, base64, os, uuid, io, struct, datetime, traceback

app = Flask(__name__)

# Load API keys
GOOGLE_API_KEY = os.getenv("GOOGLE_API_KEY")

# Ensure log folder
os.makedirs("logs", exist_ok=True)
LOGFILE = "logs/gemini.log"

def log(msg):
    """Append timestamped message to logs/gemini.log"""
    ts = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    with open(LOGFILE, "a", encoding="utf-8") as f:
        f.write(f"[{ts}] {msg}\n")
    print(msg)

@app.route("/voice", methods=["POST"])
def voice():
    try:
        # Debug: Log raw request details
        log(f"Content-Type: {request.content_type}")
        log(f"Content-Length: {request.content_length}")
        log(f"Raw data (first 100 chars): {request.get_data(as_text=True)[:100]}")
        
        # Check if request has JSON content type
        if not request.is_json:
            return jsonify({"error": "Content-Type must be application/json"}), 400
        
        # Try to get JSON data with better error handling
        try:
            json_data = request.get_json(force=True)
            if json_data is None:
                log("Received empty or null JSON data")
                return jsonify({"error": "Invalid JSON or empty request body"}), 400
        except Exception as json_error:
            log(f"JSON parsing error: {json_error}")
            log(f"Raw request data (full): {request.get_data(as_text=True)}")
            return jsonify({
                "error": f"JSON parsing failed: {str(json_error)}", 
                "raw_data": request.get_data(as_text=True)[:200],
                "suggestion": "Check ESP32 payload construction - data appears truncated"
            }), 400
        
        audio_b64 = json_data.get("audio", "")
        if not audio_b64:
            return jsonify({"error": "missing audio field in JSON"}), 400

        pcm_data = base64.b64decode(audio_b64)
        log(f"Received audio length={len(pcm_data)} bytes")

        # --- Build WAV header ---
        buf = io.BytesIO()
        num_channels = 1
        sample_rate = 16000
        bits_per_sample = 16
        byte_rate = sample_rate * num_channels * bits_per_sample // 8
        block_align = num_channels * bits_per_sample // 8
        subchunk2_size = len(pcm_data)
        chunk_size = 36 + subchunk2_size

        buf.write(b'RIFF')
        buf.write(struct.pack('<I', chunk_size))
        buf.write(b'WAVEfmt ')
        buf.write(struct.pack('<IHHIIHH',
            16, 1, num_channels, sample_rate, byte_rate, block_align, bits_per_sample))
        buf.write(b'data')
        buf.write(struct.pack('<I', subchunk2_size))
        buf.write(pcm_data)
        wav_bytes = buf.getvalue()
        wav_b64 = base64.b64encode(wav_bytes).decode('ascii')

        # --- Gemini Request ---
        prompt = "You are a friendly voice assistant. Listen to the audio and answer briefly."
        gemini_req = {
            "contents": [{
                "role": "user",
                "parts": [
                    {"text": prompt},
                    {"inline_data": {"mime_type": "audio/wav", "data": wav_b64}}
                ]
            }]
        }

        g = requests.post(
            f"https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key={GOOGLE_API_KEY}",
            json=gemini_req,
            timeout=60
        )

        log(f"Gemini status={g.status_code}")
        log(f"Gemini response: {g.text[:400]}...")  # first 400 chars for brevity

        if g.status_code != 200:
            return jsonify({"error": "Gemini call failed", "details": g.text}), g.status_code

        text = g.json()["candidates"][0]["content"]["parts"][0]["text"]

        # --- TTS Request ---
        tts_req = {
            "input": {"text": text},
            "voice": {"languageCode": "en-US", "name": "en-US-Neural2-F"},
            "audioConfig": {"audioEncoding": "MP3"}
        }

        tts = requests.post(
            f"https://texttospeech.googleapis.com/v1/text:synthesize?key={GOOGLE_API_KEY}",
            json=tts_req,
            timeout=30
        )
        log(f"TTS status={tts.status_code}")
        log(f"TTS response: {tts.text[:200]}...")

        if tts.status_code != 200:
            return jsonify({"error": "TTS call failed", "details": tts.text}), tts.status_code

        fname = f"reply_{uuid.uuid4().hex}.mp3"
        with open(f"response/{fname}", "wb") as f:
            f.write(base64.b64decode(tts.json()["audioContent"]))

        log(f"Saved reply as {fname}")
        return f"http://{request.host}/response/{fname}"

    except Exception as e:
        tb = traceback.format_exc()
        log(f"Exception: {str(e)}\n{tb}")
        return jsonify({"error": str(e), "trace": tb}), 500


@app.route("/test", methods=["GET", "POST"])
def test():
    """Test endpoint to verify server is working"""
    if request.method == "GET":
        return jsonify({"status": "server is running", "message": "Use POST with JSON data to test"})
    else:
        try:
            data = request.get_json(force=True)
            return jsonify({"received": data, "status": "success"})
        except Exception as e:
            return jsonify({"error": str(e), "raw_data": request.get_data(as_text=True)[:200]}), 400

@app.route("/<path:fname>")
def serve_file(fname):
    try:
        return send_file(fname, mimetype="audio/mpeg")
    except Exception as e:
        log(f"Serve file error: {e}")
        return jsonify({"error": str(e)}), 404


if __name__ == "__main__":
    log("Starting Gemini Proxy Debug Server")
    app.run(host="0.0.0.0", port=5001)
