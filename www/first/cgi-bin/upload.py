#!/usr/bin/env python3
import cgi, cgitb; cgitb.enable()
import os, sys, json

# ==== CONFIGURE THESE TO MATCH YOUR LAYOUT ====
HERE         = os.path.dirname(os.path.abspath(__file__))            
PROJECT_ROOT = os.path.abspath(os.path.join(HERE, '..', '..', '..'))
DEFAULT_DIR  = os.path.join(PROJECT_ROOT, 'www', 'first', 'uploads')
JSON_FILE    = os.path.join(PROJECT_ROOT, 'www', 'first', 'resources', 'uploads.json')
FIELD_NAME   = 'uploadFile'
# ==============================================

# Determine upload target directory
UPLOAD_DIR = os.environ.get('UPLOAD_DIR', DEFAULT_DIR)
if not os.path.isabs(UPLOAD_DIR):
    UPLOAD_DIR = os.path.abspath(os.path.join(PROJECT_ROOT, UPLOAD_DIR))

os.makedirs(UPLOAD_DIR, exist_ok=True)

def send_json(status_code, payload):
    body = json.dumps(payload)
    print(f"Status: {status_code}\r\n"
          "Content-Type: application/json\r\n"
          f"Content-Length: {len(body)}\r\n"
          "\r\n"
          f"{body}")
    sys.exit(0)

# Parse the form
form = cgi.FieldStorage()
if FIELD_NAME not in form:
    send_json("400 Bad Request", {"error": f"Field '{FIELD_NAME}' not found"})
file_item = form[FIELD_NAME]
if not file_item.filename:
    send_json("400 Bad Request", {"error": "No file selected"})

# Save the uploaded file
filename = os.path.basename(file_item.filename)
dest = os.path.join(UPLOAD_DIR, filename)
try:
    with open(dest, 'wb') as out:
        while True:
            chunk = file_item.file.read(8192)
            if not chunk:
                break
            out.write(chunk)
except Exception as e:
    send_json("500 Internal Server Error", {"error": f"Cannot save upload: {e}"})

# Ensure JSON file exists (do NOT recreate resources folder if missing)
if not os.path.isfile(JSON_FILE):
    os.makedirs(os.path.dirname(JSON_FILE), exist_ok=True)
    with open(JSON_FILE, 'w') as jf:
        json.dump([], jf)

# Rebuild JSON index from UPLOAD_DIR (not DEFAULT_DIR!)
try:
    # Define files to exclude
    excluded_files = ["index.html", "index.cgi", "index.cpp"]
    # Define extensions to exclude
    excluded_extensions = [".cgi", ".cpp", ".py"]
    
    entries = sorted(
        fn for fn in os.listdir(UPLOAD_DIR)
        if os.path.isfile(os.path.join(UPLOAD_DIR, fn)) and 
           fn not in excluded_files and
           not any(fn.endswith(ext) for ext in excluded_extensions)
    )
    with open(JSON_FILE, 'w') as jf:
        json.dump(entries, jf, indent=2)
except Exception as e:
    send_json("500 Internal Server Error", {"error": f"Cannot update JSON index: {e}"})
# Success
send_json("200 OK", {"status": "ok", "filename": filename})
