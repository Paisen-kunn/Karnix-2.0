# Quick Start Guide - Spine Analytics Dashboard

## 🚀 Get Started in 3 Minutes

### Step 1: Start the Backend Server

```bash
cd spine_analytics_dashboard/backend
node server.js
```

**Expected Output:**
```
✅ Loaded 2 patient records into database

==================================================
🚀 Spine Analytics Dashboard Server
==================================================
📡 Server running on port 5000
📊 Environment: development
🔒 Security features enabled
🌐 API available at: http://localhost:5000/api

API Endpoints:
  🔐 Auth:      http://localhost:5000/api/auth
  👤 Patients:  http://localhost:5000/api/patients
  📈 Analytics: http://localhost:5000/api/analytics
  ❤️  Health:    http://localhost:5000/api/health
==================================================
```

### Step 2: Test the API

**Option A: Use the Test Script**
```bash
cd spine_analytics_dashboard/backend
./test_api.sh
```

**Option B: Manual Testing with cURL**

1. **Login:**
```bash
curl -X POST http://localhost:5000/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}'
```

2. **Save the token:**
```bash
TOKEN=$(curl -s -X POST http://localhost:5000/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}' \
  | grep -o '"token":"[^"]*"' | cut -d'"' -f4)
```

3. **Get dashboard stats:**
```bash
curl -H "Authorization: Bearer $TOKEN" \
  http://localhost:5000/api/analytics/dashboard | jq '.'
```

4. **Get all patients:**
```bash
curl -H "Authorization: Bearer $TOKEN" \
  http://localhost:5000/api/patients | jq '.'
```

### Step 3: Explore the API

**Common Endpoints:**

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/health` | GET | Health check (no auth required) |
| `/api/auth/login` | POST | Login and get JWT token |
| `/api/patients` | GET | Get all patients |
| `/api/patients/abnormal` | GET | Get abnormal patients only |
| `/api/analytics/dashboard` | GET | Get dashboard statistics |
| `/api/analytics/risk-distribution` | GET | Get risk distribution |
| `/api/analytics/trends` | GET | Get trends over time |

## 🔐 Authentication

**Default Credentials:**
- Username: `admin`
- Password: `admin123`

**Using the Token:**

All protected endpoints require the JWT token in the Authorization header:
```bash
curl -H "Authorization: Bearer YOUR_TOKEN_HERE" \
  http://localhost:5000/api/patients
```

## 📊 Quick Examples

### Get High-Risk Patients
```bash
curl -H "Authorization: Bearer $TOKEN" \
  "http://localhost:5000/api/patients?riskCategory=HIGH"
```

### Search Patients
```bash
curl -H "Authorization: Bearer $TOKEN" \
  "http://localhost:5000/api/patients/search?q=abnormal"
```

### Export Data as CSV
```bash
curl -H "Authorization: Bearer $TOKEN" \
  "http://localhost:5000/api/analytics/export?format=csv" \
  -o patients.csv
```

### Get Trends (Last 30 Days)
```bash
curl -H "Authorization: Bearer $TOKEN" \
  "http://localhost:5000/api/analytics/trends?period=30"
```

### Get Abnormality Rates by Demographics
```bash
curl -H "Authorization: Bearer $TOKEN" \
  "http://localhost:5000/api/analytics/abnormality-rates"
```

## 🛑 Stop the Server

Press `Ctrl + C` in the terminal where the server is running.

Or kill the process:
```bash
lsof -ti:5000 | xargs kill
```

## 📖 Full Documentation

For complete API documentation, see [README.md](../README.md)

## 🐛 Troubleshooting

**Port already in use?**
```bash
lsof -ti:5000 | xargs kill -9
```

**Can't login?**
- Check that you're using `admin` / `admin123`
- Make sure the Content-Type header is set to `application/json`
- Verify the server is running on port 5000

**Token expired?**
- Tokens expire after 24 hours
- Simply login again to get a new token

## 💡 Tips

1. **Use jq for pretty JSON output:**
   ```bash
   brew install jq
   curl ... | jq '.'
   ```

2. **Save your token as environment variable:**
   ```bash
   export API_TOKEN="your-token-here"
   curl -H "Authorization: Bearer $API_TOKEN" ...
   ```

3. **Test with Postman:**
   - Import the API endpoints from the README
   - Set up Bearer Token authorization
   - Use collection variables for the token

## 🎯 Next Steps

1. ✅ API is working - check!
2. 🔄 Create React frontend for data visualization
3. 📈 Add more analytics features
4. 🚀 Deploy to production

---

**Happy Testing! 🎉**
