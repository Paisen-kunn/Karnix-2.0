// Validation Middleware
const { body, query, param, validationResult } = require('express-validator');

// Handle validation errors
const handleValidationErrors = (req, res, next) => {
  const errors = validationResult(req);
  
  if (!errors.isEmpty()) {
    return res.status(400).json({
      success: false,
      message: 'Validation errors',
      errors: errors.array().map(err => ({
        field: err.param,
        message: err.msg,
        value: err.value
      }))
    });
  }
  
  next();
};

// Validate login
const validateLogin = [
  body('username')
    .trim()
    .notEmpty()
    .withMessage('Username is required')
    .isLength({ min: 3 })
    .withMessage('Username must be at least 3 characters'),
  body('password')
    .notEmpty()
    .withMessage('Password is required'),
  handleValidationErrors
];

// Validate registration
const validateRegister = [
  body('username')
    .trim()
    .notEmpty()
    .withMessage('Username is required')
    .isLength({ min: 3 })
    .withMessage('Username must be at least 3 characters')
    .matches(/^[a-zA-Z0-9_-]+$/)
    .withMessage('Username can only contain letters, numbers, underscores, and hyphens'),
  body('email')
    .trim()
    .notEmpty()
    .withMessage('Email is required')
    .isEmail()
    .withMessage('Must be a valid email address')
    .normalizeEmail(),
  body('password')
    .notEmpty()
    .withMessage('Password is required')
    .isLength({ min: 6 })
    .withMessage('Password must be at least 6 characters')
    .matches(/^(?=.*[a-z])(?=.*[A-Z])(?=.*\d)/)
    .withMessage('Password must contain at least one uppercase letter, one lowercase letter, and one number'),
  body('firstName')
    .optional()
    .trim()
    .isLength({ max: 50 })
    .withMessage('First name must not exceed 50 characters'),
  body('lastName')
    .optional()
    .trim()
    .isLength({ max: 50 })
    .withMessage('Last name must not exceed 50 characters'),
  handleValidationErrors
];

// Validate change password
const validateChangePassword = [
  body('currentPassword')
    .notEmpty()
    .withMessage('Current password is required'),
  body('newPassword')
    .notEmpty()
    .withMessage('New password is required')
    .isLength({ min: 6 })
    .withMessage('New password must be at least 6 characters')
    .matches(/^(?=.*[a-z])(?=.*[A-Z])(?=.*\d)/)
    .withMessage('New password must contain at least one uppercase letter, one lowercase letter, and one number'),
  handleValidationErrors
];

// Validate patient creation
const validatePatient = [
  body('patient_id')
    .trim()
    .notEmpty()
    .withMessage('Patient ID is required')
    .matches(/^P\d{3}$/)
    .withMessage('Patient ID must be in format P### (e.g., P001)'),
  body('age')
    .isInt({ min: 0, max: 150 })
    .withMessage('Age must be between 0 and 150'),
  body('sex')
    .trim()
    .notEmpty()
    .withMessage('Sex is required')
    .isIn(['M', 'F', 'm', 'f'])
    .withMessage('Sex must be M or F'),
  body('date')
    .notEmpty()
    .withMessage('Date is required')
    .isISO8601()
    .withMessage('Date must be in ISO 8601 format'),
  body('risk_score')
    .isFloat({ min: 0, max: 100 })
    .withMessage('Risk score must be between 0 and 100'),
  body('abnormal_prob')
    .isFloat({ min: 0, max: 1 })
    .withMessage('Abnormal probability must be between 0 and 1'),
  body('classification')
    .trim()
    .notEmpty()
    .withMessage('Classification is required'),
  body('image_path')
    .optional()
    .trim(),
  handleValidationErrors
];

// Validate patient update
const validatePatientUpdate = [
  body('age')
    .optional()
    .isInt({ min: 0, max: 150 })
    .withMessage('Age must be between 0 and 150'),
  body('sex')
    .optional()
    .trim()
    .isIn(['M', 'F', 'm', 'f'])
    .withMessage('Sex must be M or F'),
  body('date')
    .optional()
    .isISO8601()
    .withMessage('Date must be in ISO 8601 format'),
  body('risk_score')
    .optional()
    .isFloat({ min: 0, max: 100 })
    .withMessage('Risk score must be between 0 and 100'),
  body('abnormal_prob')
    .optional()
    .isFloat({ min: 0, max: 1 })
    .withMessage('Abnormal probability must be between 0 and 1'),
  body('classification')
    .optional()
    .trim()
    .notEmpty()
    .withMessage('Classification cannot be empty'),
  body('image_path')
    .optional()
    .trim(),
  handleValidationErrors
];

// Validate patient ID parameter
const validatePatientId = [
  param('id')
    .trim()
    .notEmpty()
    .withMessage('Patient ID is required')
    .matches(/^P\d{3}$/)
    .withMessage('Patient ID must be in format P### (e.g., P001)'),
  handleValidationErrors
];

// Validate date range query
const validateDateRange = [
  query('startDate')
    .notEmpty()
    .withMessage('Start date is required')
    .isISO8601()
    .withMessage('Start date must be in ISO 8601 format'),
  query('endDate')
    .notEmpty()
    .withMessage('End date is required')
    .isISO8601()
    .withMessage('End date must be in ISO 8601 format')
    .custom((value, { req }) => {
      if (new Date(value) < new Date(req.query.startDate)) {
        throw new Error('End date must be after start date');
      }
      return true;
    }),
  handleValidationErrors
];

// Validate pagination query
const validatePagination = [
  query('page')
    .optional()
    .isInt({ min: 1 })
    .withMessage('Page must be a positive integer'),
  query('limit')
    .optional()
    .isInt({ min: 1, max: 100 })
    .withMessage('Limit must be between 1 and 100'),
  handleValidationErrors
];

// Validate risk score filter
const validateRiskScoreFilter = [
  query('minRiskScore')
    .optional()
    .isFloat({ min: 0, max: 100 })
    .withMessage('Minimum risk score must be between 0 and 100'),
  query('maxRiskScore')
    .optional()
    .isFloat({ min: 0, max: 100 })
    .withMessage('Maximum risk score must be between 0 and 100')
    .custom((value, { req }) => {
      if (req.query.minRiskScore && parseFloat(value) < parseFloat(req.query.minRiskScore)) {
        throw new Error('Maximum risk score must be greater than minimum risk score');
      }
      return true;
    }),
  handleValidationErrors
];

// Validate search query
const validateSearch = [
  query('q')
    .trim()
    .notEmpty()
    .withMessage('Search query is required')
    .isLength({ min: 1, max: 100 })
    .withMessage('Search query must be between 1 and 100 characters'),
  handleValidationErrors
];

module.exports = {
  validateLogin,
  validateRegister,
  validateChangePassword,
  validatePatient,
  validatePatientUpdate,
  validatePatientId,
  validateDateRange,
  validatePagination,
  validateRiskScoreFilter,
  validateSearch,
  handleValidationErrors
};
