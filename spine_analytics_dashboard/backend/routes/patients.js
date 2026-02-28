// Patient Routes
const express = require('express');
const router = express.Router();
const patientController = require('../controllers/patientController');
const {
  validatePatient,
  validatePatientUpdate,
  validatePatientId,
  validateDateRange,
  validatePagination,
  validateSearch
} = require('../middleware/validate');
const { verifyToken, requireAuth } = require('../middleware/auth');
const { readLimiter, createLimiter, searchLimiter } = require('../middleware/rateLimiter');

// All routes require authentication
router.use(verifyToken);
router.use(requireAuth);

// Get all patients with filtering and pagination
router.get(
  '/',
  readLimiter,
  validatePagination,
  patientController.getAllPatients
);

// Search patients
router.get(
  '/search',
  searchLimiter,
  validateSearch,
  patientController.searchPatients
);

// Get abnormal patients
router.get(
  '/abnormal',
  readLimiter,
  patientController.getAbnormalPatients
);

// Get patients by date range
router.get(
  '/date-range',
  readLimiter,
  validateDateRange,
  patientController.getPatientsByDateRange
);

// Get patient by ID
router.get(
  '/:id',
  readLimiter,
  validatePatientId,
  patientController.getPatientById
);

// Get patient summary
router.get(
  '/:id/summary',
  readLimiter,
  validatePatientId,
  patientController.getPatientSummary
);

// Create new patient
router.post(
  '/',
  createLimiter,
  validatePatient,
  patientController.createPatient
);

// Update patient
router.put(
  '/:id',
  createLimiter,
  validatePatientId,
  validatePatientUpdate,
  patientController.updatePatient
);

// Delete patient
router.delete(
  '/:id',
  createLimiter,
  validatePatientId,
  patientController.deletePatient
);

module.exports = router;
