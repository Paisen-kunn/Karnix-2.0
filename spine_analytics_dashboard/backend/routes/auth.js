// Authentication Routes
const express = require('express');
const router = express.Router();
const authController = require('../controllers/authController');
const { validateLogin, validateRegister, validateChangePassword } = require('../middleware/validate');
const { authLimiter } = require('../middleware/rateLimiter');
const { verifyToken, requireAuth } = require('../middleware/auth');

// Public routes
router.post('/register', authLimiter, validateRegister, authController.register);
router.post('/login', authLimiter, validateLogin, authController.login);
router.post('/patient-login', authLimiter, authController.patientLogin);

// Protected routes
router.get('/me', verifyToken, requireAuth, authController.getCurrentUser);
router.post('/logout', verifyToken, requireAuth, authController.logout);
router.post('/change-password', verifyToken, requireAuth, validateChangePassword, authController.changePassword);

module.exports = router;
