import axios from 'axios';

const API_BASE_URL = '/api';

// Create axios instance
const api = axios.create({
  baseURL: API_BASE_URL,
  headers: {
    'Content-Type': 'application/json'
  }
});

// Add token to requests
api.interceptors.request.use(
  (config) => {
    const token = localStorage.getItem('token');
    if (token) {
      config.headers.Authorization = `Bearer ${token}`;
    }
    return config;
  },
  (error) => Promise.reject(error)
);

// Handle response errors
api.interceptors.response.use(
  (response) => response,
  (error) => {
    if (error.response?.status === 401) {
      localStorage.removeItem('token');
      localStorage.removeItem('user');
      window.location.href = '/login';
    }
    return Promise.reject(error);
  }
);

// Auth API
export const authAPI = {
  login: (credentials) => api.post('/auth/login', credentials),
  patientLogin: (patient_id) => api.post('/auth/patient-login', { patient_id }),
  register: (userData) => api.post('/auth/register', userData),
  getCurrentUser: () => api.get('/auth/me'),
  logout: () => api.post('/auth/logout'),
  changePassword: (data) => api.post('/auth/change-password', data)
};

// Patient API
export const patientAPI = {
  getAll: (params) => api.get('/patients', { params }),
  getById: (id) => api.get(`/patients/${id}`),
  getMyRecord: (patient_id) => api.get(`/patients/${encodeURIComponent(patient_id)}`),
  search: (query) => api.get('/patients/search', { params: { q: query } }),
  getAbnormal: () => api.get('/patients/abnormal'),
  getByDateRange: (startDate, endDate) => 
    api.get('/patients/date-range', { params: { startDate, endDate } }),
  create: (data) => api.post('/patients', data),
  update: (id, data) => api.put(`/patients/${id}`, data),
  delete: (id) => api.delete(`/patients/${id}`)
};

// Analytics API
export const analyticsAPI = {
  getDashboard: () => api.get('/analytics/dashboard'),
  getRiskDistribution: () => api.get('/analytics/risk-distribution'),
  getTrends: (period) => api.get('/analytics/trends', { params: { period } }),
  getAbnormalityRates: () => api.get('/analytics/abnormality-rates'),
  getClassificationBreakdown: () => api.get('/analytics/classification-breakdown'),
  getHighRiskPatients: () => api.get('/analytics/high-risk-patients'),
  exportData: (format) => api.get('/analytics/export', { 
    params: { format },
    responseType: format === 'csv' ? 'blob' : 'json'
  })
};

// Health Check
export const healthCheck = () => api.get('/health');

export default api;
