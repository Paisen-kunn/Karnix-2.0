const express = require('express');
const router = express.Router();
const vitalsController = require('../controllers/vitalsController');
const { verifyToken, requireAuth } = require('../middleware/auth');

// All routes require authentication
router.use(verifyToken);
router.use(requireAuth);

// Get latest vitals for all patients
router.get('/latest', vitalsController.getLatestVitals);

// Get real-time metrics
router.get('/realtime', vitalsController.getRealtimeMetrics);

// Get heart rate trends
router.get('/trends/heart-rate', vitalsController.getHeartRateTrends);

// Get SpO2 trends
router.get('/trends/spo2', vitalsController.getSpO2Trends);

// Get vitals for specific patient
router.get('/:patientId', vitalsController.getPatientVitals);

// Get vitals statistics for specific patient
router.get('/:patientId/stats', vitalsController.getVitalsStats);

module.exports = router;
