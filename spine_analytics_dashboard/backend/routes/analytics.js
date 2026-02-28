// Analytics Routes
const express = require('express');
const router = express.Router();
const analyticsController = require('../controllers/analyticsController');
const { verifyToken, requireAuth } = require('../middleware/auth');
const { analyticsLimiter, exportLimiter } = require('../middleware/rateLimiter');

// All routes require authentication
router.use(verifyToken);
router.use(requireAuth);

// Dashboard statistics
router.get(
  '/dashboard',
  analyticsLimiter,
  analyticsController.getDashboardStats
);

// Risk distribution
router.get(
  '/risk-distribution',
  analyticsLimiter,
  analyticsController.getRiskDistribution
);

// Trends over time
router.get(
  '/trends',
  analyticsLimiter,
  analyticsController.getTrends
);

// Abnormality rates by demographics
router.get(
  '/abnormality-rates',
  analyticsLimiter,
  analyticsController.getAbnormalityRates
);

// Classification breakdown
router.get(
  '/classification-breakdown',
  analyticsLimiter,
  analyticsController.getClassificationBreakdown
);

// High-risk patients
router.get(
  '/high-risk-patients',
  analyticsLimiter,
  analyticsController.getHighRiskPatients
);

// Export data
router.get(
  '/export',
  exportLimiter,
  analyticsController.exportData
);

module.exports = router;
