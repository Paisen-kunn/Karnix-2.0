#!/bin/bash

# Spine Analytics Dashboard - API Test Script
# This script tests all major API endpoints

BASE_URL="http://localhost:5000/api"
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "========================================="
echo "🏥 Spine Analytics Dashboard API Tests"
echo "========================================="
echo ""

# Test 1: Health Check
echo -e "${YELLOW}Test 1: Health Check${NC}"
curl -s "$BASE_URL/health" | jq '.' || echo -e "${RED}Failed${NC}"
echo ""
echo ""

# Test 2: Login
echo -e "${YELLOW}Test 2: Login (admin/admin123)${NC}"
LOGIN_RESPONSE=$(curl -s -X POST "$BASE_URL/auth/login" \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}')

echo "$LOGIN_RESPONSE" | jq '.' || echo -e "${RED}Failed${NC}"

# Extract token
TOKEN=$(echo "$LOGIN_RESPONSE" | jq -r '.token')

if [ "$TOKEN" = "null" ] || [ -z "$TOKEN" ]; then
  echo -e "${RED}❌ Login failed - no token received${NC}"
  exit 1
else
  echo -e "${GREEN}✅ Login successful${NC}"
fi
echo ""
echo ""

# Test 3: Get Current User
echo -e "${YELLOW}Test 3: Get Current User${NC}"
curl -s "$BASE_URL/auth/me" \
  -H "Authorization: Bearer $TOKEN" | jq '.' || echo -e "${RED}Failed${NC}"
echo ""
echo ""

# Test 4: Get All Patients
echo -e "${YELLOW}Test 4: Get All Patients${NC}"
curl -s "$BASE_URL/patients" \
  -H "Authorization: Bearer $TOKEN" | jq '.data | length' || echo -e "${RED}Failed${NC}"
echo ""
echo ""

# Test 5: Dashboard Analytics
echo -e "${YELLOW}Test 5: Dashboard Analytics${NC}"
curl -s "$BASE_URL/analytics/dashboard" \
  -H "Authorization: Bearer $TOKEN" | jq '.data.overview' || echo -e "${RED}Failed${NC}"
echo ""
echo ""

# Test 6: Risk Distribution
echo -e "${YELLOW}Test 6: Risk Distribution${NC}"
curl -s "$BASE_URL/analytics/risk-distribution" \
  -H "Authorization: Bearer $TOKEN" | jq '.data.summary' || echo -e "${RED}Failed${NC}"
echo ""
echo ""

# Test 7: Get Abnormal Patients
echo -e "${YELLOW}Test 7: Get Abnormal Patients${NC}"
ABNORMAL_COUNT=$(curl -s "$BASE_URL/patients/abnormal" \
  -H "Authorization: Bearer $TOKEN" | jq '.count')
echo "Abnormal patients count: $ABNORMAL_COUNT"
echo ""
echo ""

# Test 8: High Risk Patients
echo -e "${YELLOW}Test 8: High Risk Patients${NC}"
curl -s "$BASE_URL/analytics/high-risk-patients" \
  -H "Authorization: Bearer $TOKEN" | jq '.count' || echo -e "${RED}Failed${NC}"
echo ""
echo ""

# Test 9: Filter Patients by Risk Category
echo -e "${YELLOW}Test 9: Filter Patients (HIGH risk)${NC}"
curl -s "$BASE_URL/patients?riskCategory=HIGH" \
  -H "Authorization: Bearer $TOKEN" | jq '.pagination.total' || echo -e "${RED}Failed${NC}"
echo ""
echo ""

# Test 10: Search Patients
echo -e "${YELLOW}Test 10: Search Patients (query: 'normal')${NC}"
curl -s "$BASE_URL/patients/search?q=normal" \
  -H "Authorization: Bearer $TOKEN" | jq '.count' || echo -e "${RED}Failed${NC}"
echo ""
echo ""

# Test 11: Trends
echo -e "${YELLOW}Test 11: Trends (30 days)${NC}"
curl -s "$BASE_URL/analytics/trends?period=30" \
  -H "Authorization: Bearer $TOKEN" | jq '.summary' || echo -e "${RED}Failed${NC}"
echo ""
echo ""

# Test 12: Abnormality Rates
echo -e "${YELLOW}Test 12: Abnormality Rates${NC}"
curl -s "$BASE_URL/analytics/abnormality-rates" \
  -H "Authorization: Bearer $TOKEN" | jq '.data.overall' || echo -e "${RED}Failed${NC}"
echo ""
echo ""

# Summary
echo "========================================="
echo -e "${GREEN}✅ All API tests completed!${NC}"
echo "========================================="
echo ""
echo "Token saved for further testing:"
echo "$TOKEN"
echo ""
echo "You can use this token for manual testing:"
echo "  export TOKEN='$TOKEN'"
echo "  curl -H \"Authorization: Bearer \$TOKEN\" $BASE_URL/analytics/dashboard"
echo ""
