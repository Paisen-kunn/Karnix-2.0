# 🏥 Spine Analytics Dashboard

A comprehensive full-stack web application for medical spine imaging analytics with advanced visualizations, powerful API, and enterprise-grade security features.

## 🌟 Features

### Backend Features
- **JWT Authentication** - Secure token-based authentication system
- **Role-Based Access Control** - Admin and user roles with different permissions
- **Rate Limiting** - Protection against abuse with tiered rate limits
- **Input Validation** - Comprehensive validation using express-validator
- **Security Hardening** - Helmet.js, CORS, bcrypt password hashing
- **RESTful API** - Clean MVC architecture following best practices
- **In-Memory Database** - Fast data access with Map-based storage
- **Error Handling** - Global error handler with detailed logging
- **Compression** - Response compression for improved performance
- **Request Logging** - Morgan HTTP request logger for debugging

### API Capabilities
- **Patient Management** - CRUD operations, search, filtering, pagination
- **Analytics & Insights** - Dashboard stats, risk distribution, trends over time
- **Advanced Querying** - Filter by risk score, age, gender, date range
- **Data Export** - Export data in JSON or CSV format
- **Health Monitoring** - Health check endpoint for uptime monitoring

## 📋 Prerequisites

- Node.js (v14 or higher)
- npm or yarn
- macOS, Linux, or Windows

## 🚀 Quick Start

### Installation

1. **Navigate to the project directory**
```bash
cd /Users/sudharsans/Orion/spine_analytics_dashboard
```

2. **Install backend dependencies**
```bash
cd backend
npm install
```

3. **Create environment file**
```bash
cp .env.example .env
```

4. **Edit .env file** (optional - defaults work for development)
```bash
# Recommended: Generate a secure JWT secret
JWT_SECRET=your-super-secret-jwt-key-change-this-in-production
```

5. **Start the backend server**
```bash
npm start
```

The server will start on `http://localhost:5000`

### Default Admin Credentials
```
Username: admin
Password: admin123
```

**⚠️ Change these credentials in production!**

## 📁 Project Structure

```
spine_analytics_dashboard/
├── backend/
│   ├── config/
│   │   ├── database.js      # In-memory database implementation
│   │   └── jwt.js            # JWT configuration
│   ├── controllers/
│   │   ├── authController.js      # Authentication logic
│   │   ├── patientController.js   # Patient CRUD operations
│   │   └── analyticsController.js # Analytics & reporting
│   ├── middleware/
│   │   ├── auth.js           # JWT verification middleware
│   │   ├── validate.js       # Request validation rules
│   │   ├── errorHandler.js   # Global error handling
│   │   └── rateLimiter.js    # Rate limiting configuration
│   ├── models/
│   │   ├── User.js           # User model with bcrypt
│   │   └── Patient.js        # Patient model with risk categorization
│   ├── routes/
│   │   ├── auth.js           # Authentication routes
│   │   ├── patients.js       # Patient routes
│   │   └── analytics.js      # Analytics routes
│   ├── server.js             # Express server entry point
│   ├── package.json
│   └── .env.example
└── frontend/                 # React frontend (to be implemented)
```

## 🔐 API Documentation

### Base URL
```
http://localhost:5000/api
```

### Authentication

All protected endpoints require a JWT token in the Authorization header:
```http
Authorization: Bearer <your-jwt-token>
```

#### Register New User
```http
POST /api/auth/register
Content-Type: application/json

{
  "username": "johndoe",
  "email": "john@example.com",
  "password": "SecurePass123",
  "firstName": "John",
  "lastName": "Doe"
}
```

#### Login
```http
POST /api/auth/login
Content-Type: application/json

{
  "username": "admin",
  "password": "admin123"
}
```

**Response:**
```json
{
  "success": true,
  "message": "Login successful",
  "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
  "user": {
    "id": "admin",
    "username": "admin",
    "email": "admin@spine-analytics.com",
    "role": "admin"
  }
}
```

#### Get Current User
```http
GET /api/auth/me
Authorization: Bearer <token>
```

#### Logout
```http
POST /api/auth/logout
Authorization: Bearer <token>
```

#### Change Password
```http
POST /api/auth/change-password
Authorization: Bearer <token>
Content-Type: application/json

{
  "currentPassword": "oldpassword",
  "newPassword": "NewSecurePass123"
}
```

### Patient Management

#### Get All Patients
```http
GET /api/patients?page=1&limit=50
Authorization: Bearer <token>
```

**Query Parameters:**
- `page` (optional): Page number (default: 1)
- `limit` (optional): Items per page (default: 50, max: 100)
- `riskCategory` (optional): HIGH, MODERATE, or LOW
- `minRiskScore` (optional): Minimum risk score (0-100)
- `maxRiskScore` (optional): Maximum risk score (0-100)
- `sex` (optional): M or F
- `minAge` (optional): Minimum age
- `maxAge` (optional): Maximum age
- `search` (optional): Search by patient ID or classification

**Example:**
```bash
curl -H "Authorization: Bearer <token>" \
  "http://localhost:5000/api/patients?riskCategory=HIGH&page=1&limit=20"
```

#### Get Patient by ID
```http
GET /api/patients/:id
Authorization: Bearer <token>
```

#### Search Patients
```http
GET /api/patients/search?q=abnormal
Authorization: Bearer <token>
```

#### Get Abnormal Patients
```http
GET /api/patients/abnormal
Authorization: Bearer <token>
```

#### Get Patients by Date Range
```http
GET /api/patients/date-range?startDate=2025-01-01&endDate=2025-12-31
Authorization: Bearer <token>
```

#### Create Patient
```http
POST /api/patients
Authorization: Bearer <token>
Content-Type: application/json

{
  "patient_id": "P150",
  "age": 45,
  "sex": "M",
  "date": "2025-06-15T10:30:00Z",
  "risk_score": 75.5,
  "abnormal_prob": 0.85,
  "classification": "Abnormal",
  "image_path": "data/images/P150.jpg"
}
```

#### Update Patient
```http
PUT /api/patients/:id
Authorization: Bearer <token>
Content-Type: application/json

{
  "risk_score": 80.0,
  "classification": "High Risk"
}
```

#### Delete Patient
```http
DELETE /api/patients/:id
Authorization: Bearer <token>
```

### Analytics

#### Dashboard Statistics
```http
GET /api/analytics/dashboard
Authorization: Bearer <token>
```

**Response:**
```json
{
  "success": true,
  "data": {
    "overview": {
      "totalPatients": 146,
      "abnormalCases": 73,
      "abnormalRate": "50.00%",
      "recentScans": 25
    },
    "riskDistribution": {
      "high": 30,
      "moderate": 80,
      "low": 36
    },
    "demographics": {
      "male": 73,
      "female": 73,
      "malePercentage": "50.00%",
      "femalePercentage": "50.00%"
    }
  }
}
```

#### Risk Distribution
```http
GET /api/analytics/risk-distribution
Authorization: Bearer <token>
```

#### Trends Over Time
```http
GET /api/analytics/trends?period=30
Authorization: Bearer <token>
```

#### Abnormality Rates
```http
GET /api/analytics/abnormality-rates
Authorization: Bearer <token>
```

#### High-Risk Patients
```http
GET /api/analytics/high-risk-patients
Authorization: Bearer <token>
```

#### Export Data
```http
GET /api/analytics/export?format=json&includeImages=false
Authorization: Bearer <token>
```

**CSV Export:**
```bash
curl -H "Authorization: Bearer <token>" \
  "http://localhost:5000/api/analytics/export?format=csv" \
  -o spine_data_export.csv
```

### Health Check

```http
GET /api/health
```

## 🔒 Security Features

### Authentication & Authorization
- **JWT Tokens**: 24-hour access tokens with HS256 algorithm
- **Password Hashing**: bcrypt with 10 salt rounds
- **Role-Based Access**: Admin and user roles

### Rate Limiting
- Auth endpoints: 5 requests per 15 minutes
- General API: 100 requests per 15 minutes
- Create/Update: 10 requests per minute
- Search: 30 requests per minute

### Security Headers
- Helmet.js with 15+ security headers
- CORS with configurable origins
- Content Security Policy

## 📊 Data Model

### User
```javascript
{
  id: string,              // UUID
  username: string,        // 3+ characters
  email: string,           // Valid email
  password: string,        // bcrypt hashed
  role: string,            // 'admin' or 'user'
  isActive: boolean
}
```

### Patient
```javascript
{
  patient_id: string,      // Format: P###
  age: number,             // 0-150
  sex: string,             // 'M' or 'F'
  date: string,            // ISO 8601
  risk_score: number,      // 0-100
  abnormal_prob: number,   // 0-1
  classification: string
}
```

### Risk Categories
- **HIGH**: risk_score >= 70
- **MODERATE**: 30 <= risk_score < 70
- **LOW**: risk_score < 30

## 🧪 Testing the API

### Using cURL

**Login and save token:**
```bash
TOKEN=$(curl -s -X POST http://localhost:5000/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}' \
  | grep -o '"token":"[^"]*"' | cut -d'"' -f4)
```

**Get dashboard stats:**
```bash
curl -H "Authorization: Bearer $TOKEN" \
  http://localhost:5000/api/analytics/dashboard
```

**Get all patients:**
```bash
curl -H "Authorization: Bearer $TOKEN" \
  http://localhost:5000/api/patients
```

**Filter high-risk patients:**
```bash
curl -H "Authorization: Bearer $TOKEN" \
  "http://localhost:5000/api/patients?riskCategory=HIGH"
```

## 🛠️ Development

### Running in Development Mode
```bash
npm run dev
```

### Environment Variables
```bash
PORT=5000
NODE_ENV=development
CORS_ORIGIN=http://localhost:3000
JWT_SECRET=your-super-secret-jwt-key
JWT_EXPIRES_IN=24h
```

## 🔧 Technology Stack

### Backend
- Node.js + Express.js
- JWT Authentication
- bcrypt Password Hashing
- express-validator
- helmet Security
- express-rate-limit
- compression
- morgan Logger

### Frontend (Coming Soon)
- React
- Chart.js/Recharts
- Axios
- React Router

## 🚧 Roadmap

### Phase 1: Backend API ✅
- [x] User authentication
- [x] Patient CRUD
- [x] Analytics endpoints
- [x] Security middleware
- [x] Rate limiting

### Phase 2: Frontend (Planned)
- [ ] React application
- [ ] Authentication UI
- [ ] Dashboard with charts
- [ ] Patient management
- [ ] Responsive design

### Phase 3: Enhancements
- [ ] Real-time updates
- [ ] Email notifications
- [ ] PDF reports
- [ ] Multi-language support

## 📝 License

ISC License

---

**Built with ❤️ for medical professionals and data analysts**
