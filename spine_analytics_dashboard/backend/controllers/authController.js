// Authentication Controller
const jwt = require('jsonwebtoken');
const bcrypt = require('bcryptjs');
const { db, pool } = require('../config/postgres');
const jwtConfig = require('../config/jwt');
const User = require('../models/User');

// Helper function to generate JWT token
const generateToken = (user) => {
  const payload = {
    id: user.id,
    username: user.username,
    role: user.role
  };

  return jwt.sign(payload, jwtConfig.secret, {
    expiresIn: jwtConfig.expiresIn,
    issuer: jwtConfig.options.issuer,
    audience: jwtConfig.options.audience
  });
};

// Helper function to verify token
const verifyTokenHelper = (token) => {
  try {
    return jwt.verify(token, jwtConfig.secret, {
      issuer: jwtConfig.options.issuer,
      audience: jwtConfig.options.audience
    });
  } catch (error) {
    throw new Error('Invalid token');
  }
};

class AuthController {
  // Register new user
  async register(req, res) {
    try {
      const { username, email, password, firstName, lastName } = req.body;

      // Validate input
      const validation = User.validate({ username, email, password });
      if (!validation.valid) {
        return res.status(400).json({
          success: false,
          errors: validation.errors
        });
      }

      // Check if user already exists
      const existingUser = await db.getUserByUsername(username);
      if (existingUser) {
        return res.status(409).json({
          success: false,
          message: 'User already exists'
        });
      }

      // Hash password
      const hashedPassword = await User.hashPassword(password);

      // Create new user
      const userData = {
        username,
        email,
        password: hashedPassword,
        role: 'user'
      };

      const newUser = await db.createUser(userData);

      // Generate token
      const token = generateToken(newUser);

      res.status(201).json({
        success: true,
        message: 'User registered successfully',
        token,
        user: {
          id: newUser.id,
          username: newUser.username,
          email: newUser.email,
          role: newUser.role
        }
      });
    } catch (error) {
      console.error('Registration error:', error);
      res.status(500).json({
        success: false,
        message: 'Server error during registration'
      });
    }
  }

  // Login user
  async login(req, res) {
    try {
      const { username, password } = req.body;

      // Validate input
      if (!username || !password) {
        return res.status(400).json({
          success: false,
          message: 'Username and password are required'
        });
      }

      // Find user
      const user = await db.getUserByUsername(username);
      if (!user) {
        return res.status(401).json({
          success: false,
          message: 'Invalid credentials'
        });
      }

      // Check if user is active
      if (!user.is_active) {
        return res.status(403).json({
          success: false,
          message: 'Account is deactivated'
        });
      }

      // Verify password
      const isPasswordValid = await User.comparePassword(password, user.password);
      if (!isPasswordValid) {
        return res.status(401).json({
          success: false,
          message: 'Invalid credentials'
        });
      }

      // Generate token
      const token = generateToken(user);

      res.status(200).json({
        success: true,
        message: 'Login successful',
        token,
        user: {
          id: user.id,
          username: user.username,
          email: user.email,
          role: user.role
        }
      });
    } catch (error) {
      console.error('Login error:', error);
      res.status(500).json({
        success: false,
        message: 'Server error during login'
      });
    }
  }

  // Get current user
  async getCurrentUser(req, res) {
    try {
      const user = req.user;
      if (!user) {
        return res.status(404).json({
          success: false,
          message: 'User not found'
        });
      }

      res.json({
        success: true,
        user: new User(user).getSafeUser()
      });
    } catch (error) {
      console.error('Get current user error:', error);
      res.status(500).json({
        success: false,
        message: 'Server error'
      });
    }
  }

  // Logout user
  async logout(req, res) {
    try {
      // In a real application, you would invalidate the token
      // For this implementation, client-side will remove the token

      res.json({
        success: true,
        message: 'Logged out successfully'
      });
    } catch (error) {
      console.error('Logout error:', error);
      res.status(500).json({
        success: false,
        message: 'Server error during logout'
      });
    }
  }

  // Patient login – look up by patient_id (no system user account needed)
  async patientLogin(req, res) {
    try {
      const { patient_id } = req.body;

      if (!patient_id || !patient_id.trim()) {
        return res.status(400).json({
          success: false,
          message: 'Patient ID is required'
        });
      }

      const pid = patient_id.trim().toUpperCase();

      // Look up the patient record directly
      let patient = null;
      try {
        const result = await pool.query(
          'SELECT * FROM patients WHERE UPPER(patient_id) = $1 LIMIT 1',
          [pid]
        );
        if (result.rows.length > 0) patient = result.rows[0];
      } catch (dbErr) {
        // DB unavailable – try fallback in-memory records (demo mode)
      }

      // Demo fallback: accept PT-001 through PT-020 even without DB
      if (!patient) {
        const match = pid.match(/^PT-?(\d+)$/i);
        if (match) {
          const n = parseInt(match[1], 10);
          if (n >= 1 && n <= 20) {
            patient = {
              patient_id: pid,
              age: 30 + n,
              sex: n % 2 === 0 ? 'F' : 'M',
              risk_category: n <= 6 ? 'HIGH' : n <= 10 ? 'MODERATE' : 'LOW',
              risk_score: 100 - n * 4,
              dl_classification: n <= 6 ? 'Abnormal' : 'Normal',
              scan_date: new Date().toISOString().split('T')[0]
            };
          }
        }
      }

      if (!patient) {
        return res.status(401).json({
          success: false,
          message: 'Patient ID not found. Please check your ID and try again.'
        });
      }

      // Issue a JWT with role = 'patient' and their patient_id
      const payload = {
        id: patient.patient_id,
        username: patient.patient_id,
        role: 'patient',
        patient_id: patient.patient_id
      };
      const token = jwt.sign(payload, jwtConfig.secret, {
        expiresIn: jwtConfig.expiresIn,
        issuer: jwtConfig.options.issuer,
        audience: jwtConfig.options.audience
      });

      return res.status(200).json({
        success: true,
        message: 'Patient login successful',
        token,
        user: {
          id: patient.patient_id,
          username: patient.patient_id,
          role: 'patient',
          patient_id: patient.patient_id,
          age: patient.age,
          sex: patient.sex,
          risk_category: patient.risk_category,
          risk_score: patient.risk_score,
          classification: patient.dl_classification,
          scan_date: patient.scan_date
        }
      });
    } catch (error) {
      console.error('Patient login error:', error);
      res.status(500).json({ success: false, message: 'Server error during patient login' });
    }
  }

  // Generate JWT token (kept for compatibility)
  generateToken(user) {
    return generateToken(user);
  }

  // Verify token (kept for compatibility)
  verifyToken(token) {
    return verifyTokenHelper(token);
  }

  // Change password
  async changePassword(req, res) {
    try {
      const { currentPassword, newPassword } = req.body;
      const user = req.user;

      // Validate input
      if (!currentPassword || !newPassword) {
        return res.status(400).json({
          success: false,
          message: 'Current and new passwords are required'
        });
      }

      // Verify current password
      const isValid = await User.comparePassword(currentPassword, user.password);
      if (!isValid) {
        return res.status(401).json({
          success: false,
          message: 'Current password is incorrect'
        });
      }

      // Hash new password
      user.password = await User.hashPassword(newPassword);
      user.updatedAt = new Date().toISOString();

      res.json({
        success: true,
        message: 'Password changed successfully'
      });
    } catch (error) {
      console.error('Change password error:', error);
      res.status(500).json({
        success: false,
        message: 'Server error'
      });
    }
  }
}

module.exports = new AuthController();
