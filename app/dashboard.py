import sqlite3
from flask import Flask, render_template_string, jsonify

app = Flask(__name__)
DB_PATH = "/app/data/plant_data.db"

HTML_TEMPLATE = """
<!DOCTYPE html>
<html>
<head>
    <title>Plant Care Dashboard</title>
    <meta http-equiv="refresh" content="30">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }
        h1 { color: #2e7d32; }
        .cards { display: flex; flex-wrap: wrap; gap: 15px; margin-bottom: 20px; }
        .card { background: white; padding: 20px; border-radius: 8px; 
                box-shadow: 0 2px 4px rgba(0,0,0,0.1); min-width: 180px; }
        .value { font-size: 2em; font-weight: bold; color: #1976d2; }
        .label { color: #666; margin-bottom: 5px; }
        .warning { color: #d32f2f; }
        table { border-collapse: collapse; width: 100%; background: white; }
        th, td { border: 1px solid #ddd; padding: 10px; text-align: left; }
        th { background: #2e7d32; color: white; }
        tr:nth-child(even) { background: #f9f9f9; }
    </style>
</head>
<body>
    <h1>Plant Care Monitoring Dashboard</h1>
    
    {% if latest %}
    <div class="cards">
        <div class="card">
            <div class="label">Temperature</div>
            <div class="value">{{ "%.1f"|format(latest.temperature) }} C</div>
        </div>
        <div class="card">
            <div class="label">Air Humidity</div>
            <div class="value">{{ "%.1f"|format(latest.humidity) }} %</div>
        </div>
        <div class="card">
            <div class="label">Soil Moisture</div>
            <div class="value {% if latest.moisture < 20 %}warning{% endif %}">
                {{ "%.1f"|format(latest.moisture) }} %
            </div>
            {% if latest.moisture < 20 %}
            <div class="warning">Needs water!</div>
            {% endif %}
        </div>
        <div class="card">
            <div class="label">Light Level</div>
            <div class="value">{{ "%.1f"|format(latest.light) }} %</div>
        </div>
    </div>
    <p>Last update: {{ latest.timestamp }}</p>
    {% else %}
    <p>No data yet. Waiting for ESP32 to send readings...</p>
    {% endif %}
    
    <h2>Recent Readings (Last 20)</h2>
    <table>
        <tr>
            <th>Timestamp</th>
            <th>Temp (C)</th>
            <th>Humidity (%)</th>
            <th>Moisture (%)</th>
            <th>Light (%)</th>
        </tr>
        {% for r in readings %}
        <tr>
            <td>{{ r.timestamp }}</td>
            <td>{{ "%.1f"|format(r.temperature) if r.temperature else "N/A" }}</td>
            <td>{{ "%.1f"|format(r.humidity) if r.humidity else "N/A" }}</td>
            <td>{{ "%.1f"|format(r.moisture) if r.moisture else "N/A" }}</td>
            <td>{{ "%.1f"|format(r.light) if r.light else "N/A" }}</td>
        </tr>
        {% endfor %}
    </table>
</body>
</html>
"""

def get_readings(limit=20):
    try:
        conn = sqlite3.connect(DB_PATH)
        conn.row_factory = sqlite3.Row
        cursor = conn.cursor()
        cursor.execute("SELECT * FROM sensor_readings ORDER BY id DESC LIMIT ?", (limit,))
        rows = cursor.fetchall()
        conn.close()
        return rows
    except Exception as e:
        print(f"Database error: {e}")
        return []

@app.route("/")
def index():
    readings = get_readings()
    latest = readings[0] if readings else None
    return render_template_string(HTML_TEMPLATE, readings=readings, latest=latest)

@app.route("/api/readings")
def api_readings():
    readings = get_readings(100)
    return jsonify([dict(r) for r in readings])

if __name__ == "__main__":
    print("Starting dashboard on port 5000...")
    app.run(host="0.0.0.0", port=5000, debug=False)
