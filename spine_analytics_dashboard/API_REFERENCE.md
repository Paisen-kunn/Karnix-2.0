# API Reference - Spine Analytics Dashboard

Base URL: `http://localhost:5000/api`

## Authentication Endpoints

### POST /auth/register
Register a new user account.

**Request:**
```json
{
  "username": "string (min: 3 chars, required)",
  "email": "string (valid email, required)",
  "password": "string (min: 6 chars, must contain uppercase, lowercase, number, required)",
  "firstName": "string (optional)",
  "lastName": "string (optional)"
}
```

**Response:** `201 Created`
```json
{
  "success": true,
  "message": "User registered successfully",
  "token": "JWT_TOKEN",
  "user": {
    "id": "uuid",
    "username": "string",
    "email": "string",
    "role": "user",
    "firstName": "string",
    "lastName": "string"
  }
}
```

---

### POST /auth/login
Login and receive JWT token.

**Rate Limit:** 5 requests per 15 minutes

**Request:**
```json
{
  "username": "string (required)",
  "password": "string (required)"
}
```

**Response:** `200 OK`
```json
{
  "success": true,
  "message": "Login successful",
  "token": "JWT_TOKEN",
  "user": {
    "id": "string",
    "username": "string",
    "email": "string",
    "role": "admin|user",
    "createdAt": "ISO8601",
    "lastLogin": "ISO8601"
  }
}
```

**Errors:**
- `401`: Invalid credentials
- `403`: Account is deactivated
- `429`: Too many login attempts

---

### GET /auth/me
Get current authenticated user's information.

**Headers:** `Authorization: Bearer TOKEN`

**Response:** `200 OK`
```json
{
  "success": true,
  "user": {
    "id": "string",
    "username": "string",
    "email": "string",
    "role": "admin|user"
  }
}
```

---

### POST /auth/logout
Logout current user.

**Headers:** `Authorization: Bearer TOKEN`

**Response:** `200 OK`
```json
{
  "success": true,
  "message": "Logged out successfully"
}
```

---

### POST /auth/change-password
Change user's password.

**Headers:** `Authorization: Bearer TOKEN`

**Request:**
```json
{
  "currentPassword": "string (required)",
  "newPassword": "string (min: 6 chars, required)"
}
```

**Response:** `200 OK`
```json
{
  "success": true,
  "message": "Password changed successfully"
}
```

---

## Patient Management Endpoints

### GET /patients
Get all patients with optional filtering and pagination.

**Headers:** `Authorization: Bearer TOKEN`

**Rate Limit:** 50 requests per minute

**Query Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| page | integer | Page number (default: 1) |
| limit | integer | Items per page (default: 50, max: 100) |
| riskCategory | string | HIGH, MODERATE, or LOW |
| minRiskScore | number | Minimum risk score (0-100) |
| maxRiskScore | number | Maximum risk score (0-100) |
| sex | string | M or F |
| minAge | integer | Minimum age |
| maxAge | integer | Maximum age |
| search | string | Search by patient ID or classification |

**Response:** `200 OK`
```json
{
  "success": true,
  "data": [
    {
      "id": "uuid",
      "patient_id": "P001",
      "age": 45,
      "sex": "M",
      "date": "ISO8601",
      "risk_score": 75.5,
      "abnormal_prob": 0.85,
      "classification": "Abnormal",
      "risk_category": "HIGH",
      "image_path": "string",
      "createdAt": "ISO8601"
    }
  ],
  "pagination": {
    "total": 146,
    "page": 1,
    "limit": 50,
    "totalPages": 3
  }
}
```

---

### GET /patients/:id
Get a specific patient by ID.

**Headers:** `Authorization: Bearer TOKEN`

**URL Parameters:**
- `id`: Patient ID (format: P###)

**Response:** `200 OK`
```json
{
  "success": true,
  "data": {
    "id": "uuid",
    "patient_id": "P001",
    "age": 45,
    "sex": "M",
    "date": "ISO8601",
    "risk_score": 75.5,
    "abnormal_prob": 0.85,
    "classification": "Abnormal",
    "risk_category": "HIGH",
    "image_path": "string"
  }
}
```

**Errors:**
- `404`: Patient not found

---

### GET /patients/:id/summary
Get patient summary with risk analysis.

**Headers:** `Authorization: Bearer TOKEN`

**Response:** `200 OK`
```json
{
  "success": true,
  "data": {
    "patient_id": "P001",
    "risk_category": "HIGH",
    "risk_score": 75.5,
    "abnormal_prob": 0.85,
    "is_abnormal": true,
    "age": 45,
    "sex": "M"
  }
}
```

---

### GET /patients/search
Search patients by query string.

**Headers:** `Authorization: Bearer TOKEN`

**Rate Limit:** 30 requests per minute

**Query Parameters:**
- `q` (required): Search query (1-100 characters)

**Response:** `200 OK`
```json
{
  "success": true,
  "data": [...],
  "count": 10,
  "query": "abnormal"
}
```

---

### GET /patients/abnormal
Get all patients with abnormal classification.

**Headers:** `Authorization: Bearer TOKEN`

**Response:** `200 OK`
```json
{
  "success": true,
  "data": [...],
  "count": 73
}
```

---

### GET /patients/date-range
Get patients within a date range.

**Headers:** `Authorization: Bearer TOKEN`

**Query Parameters:**
- `startDate` (required): ISO 8601 date
- `endDate` (required): ISO 8601 date

**Response:** `200 OK`
```json
{
  "success": true,
  "data": [...],
  "count": 50,
  "dateRange": {
    "startDate": "2025-01-01",
    "endDate": "2025-12-31"
  }
}
```

---

### POST /patients
Create a new patient record.

**Headers:** `Authorization: Bearer TOKEN`

**Rate Limit:** 10 requests per minute

**Request:**
```json
{
  "patient_id": "string (format: P###, required)",
  "age": "integer (0-150, required)",
  "sex": "string (M or F, required)",
  "date": "string (ISO 8601, required)",
  "risk_score": "number (0-100, required)",
  "abnormal_prob": "number (0-1, required)",
  "classification": "string (required)",
  "image_path": "string (optional)"
}
```

**Response:** `201 Created`
```json
{
  "success": true,
  "message": "Patient record created successfully",
  "data": {...}
}
```

**Errors:**
- `400`: Validation errors
- `409`: Patient with this ID already exists

---

### PUT /patients/:id
Update an existing patient record.

**Headers:** `Authorization: Bearer TOKEN`

**Rate Limit:** 10 requests per minute

**Request:** (all fields optional)
```json
{
  "age": 46,
  "risk_score": 80.0,
  "classification": "High Risk"
}
```

**Response:** `200 OK`
```json
{
  "success": true,
  "message": "Patient record updated successfully",
  "data": {...}
}
```

**Errors:**
- `404`: Patient not found
- `400`: Validation errors

---

### DELETE /patients/:id
Delete a patient record.

**Headers:** `Authorization: Bearer TOKEN`

**Rate Limit:** 10 requests per minute

**Response:** `200 OK`
```json
{
  "success": true,
  "message": "Patient record deleted successfully"
}
```

**Errors:**
- `404`: Patient not found

---

## Analytics Endpoints

### GET /analytics/dashboard
Get comprehensive dashboard statistics.

**Headers:** `Authorization: Bearer TOKEN`

**Rate Limit:** 20 requests per minute

**Response:** `200 OK`
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
    },
    "ageStatistics": {
      "average": 45.5,
      "minimum": 18,
      "maximum": 85
    },
    "riskScoreStatistics": {
      "average": 50.25,
      "minimum": 5.50,
      "maximum": 98.75
    },
    "abnormalityStatistics": {
      "averageProbability": 0.65
    },
    "classificationDistribution": {
      "Abnormal": 73,
      "Normal": 73
    }
  }
}
```

---

### GET /analytics/risk-distribution
Get detailed risk distribution breakdown.

**Headers:** `Authorization: Bearer TOKEN`

**Response:** `200 OK`
```json
{
  "success": true,
  "data": {
    "summary": {
      "high": 30,
      "moderate": 80,
      "low": 36
    },
    "detailed": {
      "critical": 10,
      "high": 20,
      "moderateHigh": 40,
      "moderate": 40,
      "low": 26,
      "minimal": 10
    },
    "percentages": {
      "high": "20.55%",
      "moderate": "54.79%",
      "low": "24.66%"
    }
  }
}
```

---

### GET /analytics/trends
Get trends over time.

**Headers:** `Authorization: Bearer TOKEN`

**Query Parameters:**
- `period` (optional): Number of days (default: 30)

**Response:** `200 OK`
```json
{
  "success": true,
  "data": [
    {
      "date": "2025-06-01",
      "scanCount": 5,
      "abnormalCount": 2,
      "abnormalRate": "40.00",
      "avgRiskScore": "52.30",
      "avgAbnormalProb": "0.65"
    }
  ],
  "period": "30 days",
  "summary": {
    "totalDays": 30,
    "totalScans": 150,
    "totalAbnormal": 75
  }
}
```

---

### GET /analytics/abnormality-rates
Get abnormality rates by demographics.

**Headers:** `Authorization: Bearer TOKEN`

**Response:** `200 OK`
```json
{
  "success": true,
  "data": {
    "byGender": {
      "male": {
        "total": 73,
        "abnormal": 36,
        "rate": "49.32%"
      },
      "female": {
        "total": 73,
        "abnormal": 37,
        "rate": "50.68%"
      }
    },
    "byAgeGroup": {
      "0-20": {"total": 10, "abnormal": 3, "rate": "30.00%"},
      "21-40": {"total": 45, "abnormal": 20, "rate": "44.44%"},
      "41-60": {"total": 60, "abnormal": 35, "rate": "58.33%"},
      "61-80": {"total": 25, "abnormal": 13, "rate": "52.00%"},
      "81+": {"total": 6, "abnormal": 2, "rate": "33.33%"}
    },
    "overall": {
      "total": 146,
      "abnormal": 73,
      "rate": "50.00%"
    }
  }
}
```

---

### GET /analytics/classification-breakdown
Get classification distribution with statistics.

**Headers:** `Authorization: Bearer TOKEN`

**Response:** `200 OK`
```json
{
  "success": true,
  "data": {
    "Abnormal": {
      "count": 73,
      "patients": ["P001", "P003", ...],
      "avgRiskScore": "75.25",
      "percentage": "50.00%"
    },
    "Normal": {
      "count": 73,
      "patients": ["P002", "P004", ...],
      "avgRiskScore": "25.30",
      "percentage": "50.00%"
    }
  }
}
```

---

### GET /analytics/high-risk-patients
Get top 20 highest-risk patients.

**Headers:** `Authorization: Bearer TOKEN`

**Response:** `200 OK`
```json
{
  "success": true,
  "data": [
    {
      "patient_id": "P001",
      "risk_score": 98.75,
      "risk_category": "HIGH",
      "abnormal_prob": 0.99
    }
  ],
  "count": 20
}
```

---

### GET /analytics/export
Export all patient data.

**Headers:** `Authorization: Bearer TOKEN`

**Rate Limit:** 10 requests per hour

**Query Parameters:**
- `format` (optional): json or csv (default: json)
- `includeImages` (optional): true or false (default: false)

**Response (JSON):** `200 OK`
```json
{
  "success": true,
  "data": [...],
  "exportDate": "ISO8601",
  "totalRecords": 146
}
```

**Response (CSV):** `200 OK`
```csv
patient_id,age,sex,date,risk_score,abnormal_prob,classification
P001,45,M,2025-06-15T10:30:00Z,75.5,0.85,Abnormal
...
```

---

## Health Check

### GET /health
Check server health status.

**No authentication required**

**Response:** `200 OK`
```json
{
  "status": "healthy",
  "timestamp": "ISO8601",
  "uptime": 1234.567,
  "environment": "development",
  "database": {
    "patients": 146,
    "users": 1
  }
}
```

---

## Error Responses

All endpoints return error responses in this format:

```json
{
  "success": false,
  "message": "Error description"
}
```

**Common Status Codes:**
- `400 Bad Request`: Invalid input or validation errors
- `401 Unauthorized`: Missing or invalid token
- `403 Forbidden`: Insufficient permissions
- `404 Not Found`: Resource not found
- `409 Conflict`: Resource already exists
- `429 Too Many Requests`: Rate limit exceeded
- `500 Internal Server Error`: Server error

**Validation Errors:**
```json
{
  "success": false,
  "message": "Validation errors",
  "errors": [
    {
      "field": "email",
      "message": "Must be a valid email address",
      "value": "invalid-email"
    }
  ]
}
```

---

## Rate Limits

| Endpoint Type | Limit |
|---------------|-------|
| Auth (login/register) | 5 requests / 15 minutes |
| General API | 100 requests / 15 minutes |
| Create/Update | 10 requests / minute |
| Search | 30 requests / minute |
| Analytics | 20 requests / minute |
| Export | 10 requests / hour |
| Read | 50 requests / minute |

**Rate Limit Headers:**
```
RateLimit-Limit: 100
RateLimit-Remaining: 95
RateLimit-Reset: 1234567890
```

---

## Authentication

All protected endpoints require a JWT token in the Authorization header:

```
Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...
```

**Token Expiration:** 24 hours

**Token Payload:**
```json
{
  "id": "user_id",
  "username": "username",
  "role": "admin|user",
  "iat": 1234567890,
  "exp": 1234654290,
  "iss": "spine-analytics-system",
  "aud": "spine-analytics-users"
}
```

---

**Last Updated:** February 27, 2026
