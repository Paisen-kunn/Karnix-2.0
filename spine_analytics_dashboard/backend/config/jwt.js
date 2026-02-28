// JWT Configuration
require('dotenv').config();

module.exports = {
  secret: process.env.JWT_SECRET || 'spine-analytics-super-secret-jwt-key-2026',
  expiresIn: '24h',
  refreshExpiresIn: '7d',
  
  // Token generation options
  options: {
    issuer: 'spine-analytics-system',
    audience: 'spine-analytics-users',
    algorithm: 'HS256'
  },
  
  // Security settings
  bcryptRounds: 10,
  maxLoginAttempts: 5,
  lockoutDuration: 15 * 60 * 1000 // 15 minutes
};
